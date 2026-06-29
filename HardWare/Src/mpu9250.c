#include "mpu9250.h"
#include "uart.h"
#include <stdio.h>

/* ---- 私有变量 ---- */
static float accel_scale;   /* 加速度灵敏度，原始值 * accel_scale = g */
static float gyro_scale;    /* 陀螺仪灵敏度，原始值 * gyro_scale = dps */
static float mag_adjust[3]; /* AK8963 出厂灵敏度校准系数 */

static float gyro_bias[3] = {0, 0, 0};
static bool gyro_calibrated = false;
/* I2C 超时（循环次数） */
#define I2C_TIMEOUT     10000

/* ========================================================================== */
/*                         底层 I2C 读写辅助函数                               */
/* ========================================================================== */

/**
 * @brief  通过 I2C1 向 MPU9250 写入数据
 *         tx_buf[0] 为起始寄存器地址，后续为写入数据
 */
static bool MPU9250_I2C_Write(uint8_t devAddr, const uint8_t *tx_buf, uint8_t len)
{
    uint32_t timeout;

    /* 等待控制器空闲 */
    timeout = I2C_TIMEOUT;
    while (!(DL_I2C_getControllerStatus(MPU9250_I2C_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--timeout == 0) return false;
    }

    /* 先将数据填入 TX FIFO，再原子启动传输 */
    DL_I2C_fillControllerTXFIFO(MPU9250_I2C_INST, tx_buf, len);
    DL_I2C_startControllerTransfer(MPU9250_I2C_INST, devAddr,
                                   DL_I2C_CONTROLLER_DIRECTION_TX, len);

    /* 等待传输完成 */
    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(MPU9250_I2C_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
        if (--timeout == 0) return false;
    }

    /* 检查错误（ERROR 标志涵盖 NACK 等情况） */
    if (DL_I2C_getControllerStatus(MPU9250_I2C_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        return false;
    }

    return true;
}

/**
 * @brief  通过 I2C1 从 MPU9250 读取数据
 *         先写寄存器地址（STOP），再读取数据（新 START）
 *         rx_buf 至少需要 len 字节空间
 */
static bool MPU9250_I2C_Read(uint8_t devAddr, uint8_t reg, uint8_t *rx_buf, uint8_t len)
{
    uint32_t timeout;
    uint8_t regAddr = reg;
    uint8_t received;
    uint8_t remaining;

    /* 第一阶段：写寄存器地址 */
    timeout = I2C_TIMEOUT;
    while (!(DL_I2C_getControllerStatus(MPU9250_I2C_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--timeout == 0) return false;
    }

    DL_I2C_fillControllerTXFIFO(MPU9250_I2C_INST, &regAddr, 1);
    DL_I2C_startControllerTransfer(MPU9250_I2C_INST, devAddr,
                                   DL_I2C_CONTROLLER_DIRECTION_TX, 1);

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(MPU9250_I2C_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
        if (--timeout == 0) return false;
    }

    if (DL_I2C_getControllerStatus(MPU9250_I2C_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        return false;
    }

    /* 第二阶段：读数据 */
    DL_I2C_flushControllerRXFIFO(MPU9250_I2C_INST);

    timeout = I2C_TIMEOUT;
    while (!(DL_I2C_getControllerStatus(MPU9250_I2C_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--timeout == 0) return false;
    }

    DL_I2C_startControllerTransfer(MPU9250_I2C_INST, devAddr,
                                   DL_I2C_CONTROLLER_DIRECTION_RX, len);

    remaining = len;
    while (remaining > 0) {
        timeout = I2C_TIMEOUT;
        while ((DL_I2C_getControllerRXFIFOCounter(MPU9250_I2C_INST) == 0) && remaining > 0) {
            if (!(DL_I2C_getControllerStatus(MPU9250_I2C_INST) & DL_I2C_CONTROLLER_STATUS_BUSY)) {
                if (DL_I2C_getControllerRXFIFOCounter(MPU9250_I2C_INST) == 0) break;
            }
            if (--timeout == 0) return false;
        }

        received = DL_I2C_getControllerRXFIFOCounter(MPU9250_I2C_INST);
        if (received == 0) break;
        if (received > remaining) received = remaining;

        for (uint8_t i = 0; i < received; i++) {
            *rx_buf++ = DL_I2C_receiveControllerData(MPU9250_I2C_INST);
        }
        remaining -= received;
    }

    if (DL_I2C_getControllerStatus(MPU9250_I2C_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        return false;
    }

    return (remaining == 0);
}

/**
 * @brief  从 MPU9250 内部的 AK8963 磁力计读取数据
 */
static bool AK8963_I2C_Read(uint8_t reg, uint8_t *rx_buf, uint8_t len)
{
    return MPU9250_I2C_Read(AK8963_ADDR, reg, rx_buf, len);
}

/**
 * @brief  向 MPU9250 内部的 AK8963 磁力计写入数据
 */
static bool AK8963_I2C_Write(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    return MPU9250_I2C_Write(AK8963_ADDR, buf, 2);
}

/* ========================================================================== */
/*                           寄存器读写接口                                    */
/* ========================================================================== */

void MPU9250_WriteReg(uint8_t reg, uint8_t value)
{
    uint8_t buf[2] = {reg, value};
    MPU9250_I2C_Write(MPU9250_ADDR, buf, 2);
}

uint8_t MPU9250_ReadReg(uint8_t reg)
{
    uint8_t val = 0;
    MPU9250_I2C_Read(MPU9250_ADDR, reg, &val, 1);
    return val;
}

/* ========================================================================== */
/*                              初始化                                         */
/* ========================================================================== */

bool MPU9250_Init(void)
{
    uint8_t whoami;
    char dbg[32];

    /* I2C 控制器由 SysConfig 在 SYSCFG_DL_init() 中完成初始化 */

    /* 复位 MPU6500 */
    UART_SendData((uint8_t *)"MPU: reset\r\n", 12);
    MPU9250_WriteReg(MPU9250_REG_PWR_MGMT_1, 0x80);
    for (volatile uint32_t i = 0; i < 200000; i++);

    /* 唤醒，使用 X 轴陀螺仪 PLL 作为时钟源 */
    UART_SendData((uint8_t *)"MPU: wakeup\r\n", 13);
    MPU9250_WriteReg(MPU9250_REG_PWR_MGMT_1, 0x01);
    for (volatile uint32_t i = 0; i < 200000; i++);

    /* 校验 WHO_AM_I: 0x71=MPU9250, 0x70=MPU6500 */
    whoami = MPU9250_ReadReg(MPU9250_REG_WHO_AM_I);
    int len = snprintf(dbg, sizeof(dbg), "MPU: WHO_AM_I=0x%02X\r\n", whoami);
    UART_SendData((uint8_t *)dbg, (uint16_t)len);
    if (whoami != 0x71 && whoami != 0x70) {
        UART_SendData((uint8_t *)"MPU: WHO_AM_I FAIL\r\n", 21);
        return false;
    }
    UART_SendData((uint8_t *)"MPU: WHO_AM_I OK\r\n", 18);

    /* 采样率 = 1kHz / (1 + SMPLRT_DIV) = 200 Hz */
    MPU9250_WriteReg(MPU9250_REG_SMPLRT_DIV, 4);

    /* 数字低通滤波：陀螺仪带宽 41 Hz */
    MPU9250_WriteReg(MPU9250_REG_CONFIG, MPU9250_DLPF_41HZ);

    /* 陀螺仪量程：±250 dps */
    MPU9250_WriteReg(MPU9250_REG_GYRO_CONFIG, MPU9250_GYRO_FS_250DPS);
    gyro_scale = 1.0f / 131.0f;  /* 250/32768 */

    /* 加计量程：±8g */
    MPU9250_WriteReg(MPU9250_REG_ACCEL_CONFIG, MPU9250_ACCEL_FS_2G);
    accel_scale = 1.0f / 16384.0f; /* 2/32768 */

    /* 加计数字低通滤波：41 Hz */
    MPU9250_WriteReg(MPU9250_REG_ACCEL_CONFIG2, MPU9250_DLPF_41HZ);

    /* ---- 磁力计 (AK8963) 初始化 ---- */
    MPU9250_WriteReg(MPU9250_REG_USER_CTRL, 0x00);
    MPU9250_WriteReg(MPU9250_REG_INT_PIN_CFG, 0x02);  /* BYPASS_EN */
    for (volatile uint32_t i = 0; i < 100000; i++);

    AK8963_I2C_Read(AK8963_REG_WHO_AM_I, &whoami, 1);
    len = snprintf(dbg, sizeof(dbg), "MPU: AK8963 WHO=0x%02X\r\n", whoami);
    UART_SendData((uint8_t *)dbg, (uint16_t)len);
    if (whoami != 0x48) {
        UART_SendData((uint8_t *)"MPU: AK8963 FAIL (no mag)\r\n", 27);
        /* 无磁力计也继续，只返回 0 */
        mag_adjust[0] = mag_adjust[1] = mag_adjust[2] = 1.0f;
    } else {
        UART_SendData((uint8_t *)"MPU: AK8963 OK\r\n", 16);
        AK8963_I2C_Write(AK8963_REG_CNTL2, 0x01);  /* 复位 */
        for (volatile uint32_t i = 0; i < 100000; i++);
        AK8963_I2C_Write(AK8963_REG_CNTL1, 0x0F);  /* Fuse ROM */
        for (volatile uint32_t i = 0; i < 100000; i++);
        uint8_t raw[3];
        AK8963_I2C_Read(AK8963_REG_ASAX, raw, 3);
        mag_adjust[0] = (float)(raw[0] - 128) / 256.0f + 1.0f;
        mag_adjust[1] = (float)(raw[1] - 128) / 256.0f + 1.0f;
        mag_adjust[2] = (float)(raw[2] - 128) / 256.0f + 1.0f;
        AK8963_I2C_Write(AK8963_REG_CNTL1, 0x16);  /* 连续模式 */
    }

    UART_SendData((uint8_t *)"MPU: init OK\r\n", 14);
    return true;
}

/* ========================================================================== */
/*                            数据读取                                         */
/* ========================================================================== */

void MPU9250_ReadAccel(MPU9250_Axes_t *accel)
{
    uint8_t buf[6];
    MPU9250_I2C_Read(MPU9250_ADDR, MPU9250_REG_ACCEL_XOUT_H, buf, 6);

    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_z = (int16_t)((buf[4] << 8) | buf[5]);

    accel->x = (float)raw_x * accel_scale;
    accel->y = (float)raw_y * accel_scale;
    accel->z = (float)raw_z * accel_scale;
}

void MPU9250_ReadGyro(MPU9250_Axes_t *gyro)
{
    uint8_t buf[6];
    MPU9250_I2C_Read(MPU9250_ADDR, MPU9250_REG_GYRO_XOUT_H, buf, 6);

    int16_t raw_x = (int16_t)((buf[0] << 8) | buf[1]);
    int16_t raw_y = (int16_t)((buf[2] << 8) | buf[3]);
    int16_t raw_z = (int16_t)((buf[4] << 8) | buf[5]);

    gyro->x = (float)raw_x * gyro_scale;
    gyro->y = (float)raw_y * gyro_scale;
    gyro->z = (float)raw_z * gyro_scale;
}

float MPU9250_ReadTemperature(void)
{
    uint8_t buf[2];
    MPU9250_I2C_Read(MPU9250_ADDR, MPU9250_REG_TEMP_OUT_H, buf, 2);

    int16_t raw_temp = (int16_t)((buf[0] << 8) | buf[1]);
    return (float)raw_temp / 333.87f + 21.0f;
}

void MPU9250_ReadMag(MPU9250_Axes_t *mag)
{
    uint8_t buf[7];
    int16_t raw_x, raw_y, raw_z;

    /* 读取 ST1 状态寄存器，检查数据是否就绪 */
    AK8963_I2C_Read(AK8963_REG_ST1, buf, 1);
    if (!(buf[0] & 0x01)) {
        /* 数据未就绪 */
        mag->x = 0;
        mag->y = 0;
        mag->z = 0;
        return;
    }

    /* 读取 6 字节磁力数据 + ST2（必须读 ST2 以释放数据锁存） */
    AK8963_I2C_Read(AK8963_REG_HXL, buf, 7);

    raw_x = (int16_t)((buf[1] << 8) | buf[0]);  /* AK8963 为小端序 */
    raw_y = (int16_t)((buf[3] << 8) | buf[2]);
    raw_z = (int16_t)((buf[5] << 8) | buf[4]);

    /* 16 位模式：0.15 uT/LSB，乘以出厂校准系数 */
    mag->x = (float)raw_x * 0.15f * mag_adjust[0];
    mag->y = (float)raw_y * 0.15f * mag_adjust[1];
    mag->z = (float)raw_z * 0.15f * mag_adjust[2];
}

void MPU9250_ReadAll(MPU9250_Data_t *data)
{
    uint8_t buf[14];

    /* 连续读取加计+温度+陀螺仪共 14 字节（ACCEL_XOUT_H 到 GYRO_ZOUT_L） */
    MPU9250_I2C_Read(MPU9250_ADDR, MPU9250_REG_ACCEL_XOUT_H, buf, 14);

    int16_t raw_ax = (int16_t)((buf[0]  << 8) | buf[1]);
    int16_t raw_ay = (int16_t)((buf[2]  << 8) | buf[3]);
    int16_t raw_az = (int16_t)((buf[4]  << 8) | buf[5]);
    int16_t raw_t  = (int16_t)((buf[6]  << 8) | buf[7]);
    int16_t raw_gx = (int16_t)((buf[8]  << 8) | buf[9]);
    int16_t raw_gy = (int16_t)((buf[10] << 8) | buf[11]);
    int16_t raw_gz = (int16_t)((buf[12] << 8) | buf[13]);

    data->accel.x = (float)raw_ax * accel_scale;
    data->accel.y = (float)raw_ay * accel_scale;
    data->accel.z = (float)raw_az * accel_scale;

    if (gyro_calibrated) {
      
        data->gyro.x = (float)raw_gx * gyro_scale - gyro_bias[0];
        data->gyro.y = (float)raw_gy * gyro_scale - gyro_bias[1];
        data->gyro.z = (float)raw_gz * gyro_scale - gyro_bias[2];
    } else {
        data->gyro.x = (float)raw_gx * gyro_scale;
        data->gyro.y = (float)raw_gy * gyro_scale;
        data->gyro.z = (float)raw_gz * gyro_scale;
    }

    data->temperature = (float)raw_t / 333.87f + 21.0f;

    /* 读取磁力计 */
    MPU9250_ReadMag(&data->mag);
}

void MPU9250_CalibrateGyro(void) {
    MPU9250_Axes_t gyro;
    float sum[3] = {0, 0, 0};
    int count = 0;
    
    UART_SendData((uint8_t *)"Calibrating gyro (keep still)...\r\n", 36);
    
    // 采样500次，传感器必须静止
    for (int i = 0; i < 500; i++) {
        MPU9250_ReadGyro(&gyro);
        sum[0] += gyro.x;
        sum[1] += gyro.y;
        sum[2] += gyro.z;
        count++;
        for (volatile uint32_t j = 0; j < 5000; j++);  // 5ms延时
    }
    
    gyro_bias[0] = sum[0] / count;
    gyro_bias[1] = sum[1] / count;
    gyro_bias[2] = sum[2] / count;
    gyro_calibrated = true;
    
    char dbg[64];
    int len = snprintf(dbg, sizeof(dbg), "Gyro bias: %.3f, %.3f, %.3f °/s\r\n", 
                       gyro_bias[0], gyro_bias[1], gyro_bias[2]);
    UART_SendData((uint8_t *)dbg, (uint16_t)len);
}