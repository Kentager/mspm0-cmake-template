#ifndef __MPU9250_H
#define __MPU9250_H

#include "ti_msp_dl_config.h"
#include <stdint.h>
#include <stdbool.h>

/* ---- MPU9250 I2C 设备地址（AD0 = LOW 时为 0x68，AD0 = HIGH 时为 0x69） ---- */
#define MPU9250_ADDR            0x68

/* ---- 使用 SysConfig 中配置的 I2C_1 ---- */
#define MPU9250_I2C_INST        I2C_1_INST

/* ---- MPU9250 寄存器地址表 ---- */
#define MPU9250_REG_SELF_TEST_X_GYRO    0x00
#define MPU9250_REG_SELF_TEST_Y_GYRO    0x01
#define MPU9250_REG_SELF_TEST_Z_GYRO    0x02
#define MPU9250_REG_SELF_TEST_X_ACCEL   0x0D
#define MPU9250_REG_SELF_TEST_Y_ACCEL   0x0E
#define MPU9250_REG_SELF_TEST_Z_ACCEL   0x0F
#define MPU9250_REG_SMPLRT_DIV          0x19
#define MPU9250_REG_CONFIG              0x1A
#define MPU9250_REG_GYRO_CONFIG         0x1B
#define MPU9250_REG_ACCEL_CONFIG        0x1C
#define MPU9250_REG_ACCEL_CONFIG2       0x1D
#define MPU9250_REG_LP_ACCEL_ODR        0x1E
#define MPU9250_REG_WOM_THR             0x1F
#define MPU9250_REG_FIFO_EN             0x23
#define MPU9250_REG_I2C_MST_CTRL       0x24
#define MPU9250_REG_I2C_SLV0_ADDR       0x25
#define MPU9250_REG_I2C_SLV0_REG        0x26
#define MPU9250_REG_I2C_SLV0_CTRL       0x27
#define MPU9250_REG_I2C_SLV1_ADDR       0x28
#define MPU9250_REG_I2C_SLV1_REG        0x29
#define MPU9250_REG_I2C_SLV1_CTRL       0x2A
#define MPU9250_REG_I2C_SLV2_ADDR       0x2B
#define MPU9250_REG_I2C_SLV2_REG        0x2C
#define MPU9250_REG_I2C_SLV2_CTRL       0x2D
#define MPU9250_REG_I2C_SLV3_ADDR       0x2E
#define MPU9250_REG_I2C_SLV3_REG        0x2F
#define MPU9250_REG_I2C_SLV3_CTRL       0x30
#define MPU9250_REG_I2C_SLV4_ADDR       0x31
#define MPU9250_REG_I2C_SLV4_REG        0x32
#define MPU9250_REG_I2C_SLV4_DO         0x33
#define MPU9250_REG_I2C_SLV4_CTRL       0x34
#define MPU9250_REG_I2C_SLV4_DI         0x35
#define MPU9250_REG_I2C_MST_STATUS      0x36
#define MPU9250_REG_INT_PIN_CFG         0x37
#define MPU9250_REG_INT_ENABLE          0x38
#define MPU9250_REG_INT_STATUS          0x3A
#define MPU9250_REG_ACCEL_XOUT_H        0x3B
#define MPU9250_REG_ACCEL_XOUT_L        0x3C
#define MPU9250_REG_ACCEL_YOUT_H        0x3D
#define MPU9250_REG_ACCEL_YOUT_L        0x3E
#define MPU9250_REG_ACCEL_ZOUT_H        0x3F
#define MPU9250_REG_ACCEL_ZOUT_L        0x40
#define MPU9250_REG_TEMP_OUT_H          0x41
#define MPU9250_REG_TEMP_OUT_L          0x42
#define MPU9250_REG_GYRO_XOUT_H         0x43
#define MPU9250_REG_GYRO_XOUT_L         0x44
#define MPU9250_REG_GYRO_YOUT_H         0x45
#define MPU9250_REG_GYRO_YOUT_L         0x46
#define MPU9250_REG_GYRO_ZOUT_H         0x47
#define MPU9250_REG_GYRO_ZOUT_L         0x48
#define MPU9250_REG_EXT_SENS_DATA_00    0x49
#define MPU9250_REG_I2C_SLV0_DO         0x63
#define MPU9250_REG_I2C_SLV1_DO         0x64
#define MPU9250_REG_I2C_SLV2_DO         0x65
#define MPU9250_REG_I2C_SLV3_DO         0x66
#define MPU9250_REG_I2C_MST_DELAY_CTRL 0x67
#define MPU9250_REG_SIGNAL_PATH_RESET   0x68
#define MPU9250_REG_MOT_DETECT_CTRL     0x69
#define MPU9250_REG_USER_CTRL           0x6A
#define MPU9250_REG_PWR_MGMT_1          0x6B
#define MPU9250_REG_PWR_MGMT_2          0x6C
#define MPU9250_REG_FIFO_COUNTH         0x72
#define MPU9250_REG_FIFO_COUNTL         0x73
#define MPU9250_REG_FIFO_R_W            0x74
#define MPU9250_REG_WHO_AM_I            0x75

/* ---- AK8963 磁力计寄存器（MPU9250 内部） ---- */
#define AK8963_ADDR             0x0C
#define AK8963_REG_WHO_AM_I     0x00
#define AK8963_REG_INFO         0x01
#define AK8963_REG_ST1          0x02
#define AK8963_REG_HXL          0x03
#define AK8963_REG_HXH          0x04
#define AK8963_REG_HYL          0x05
#define AK8963_REG_HYH          0x06
#define AK8963_REG_HZL          0x07
#define AK8963_REG_HZH          0x08
#define AK8963_REG_ST2          0x09
#define AK8963_REG_CNTL1        0x0A
#define AK8963_REG_CNTL2        0x0B
#define AK8963_REG_ASTC         0x0C
#define AK8963_REG_ASAX         0x10
#define AK8963_REG_ASAY         0x11
#define AK8963_REG_ASAZ         0x12

/* ---- 陀螺仪量程 ---- */
#define MPU9250_GYRO_FS_250DPS  0x00
#define MPU9250_GYRO_FS_500DPS  0x08
#define MPU9250_GYRO_FS_1000DPS 0x10
#define MPU9250_GYRO_FS_2000DPS 0x18

/* ---- 加计量程 ---- */
#define MPU9250_ACCEL_FS_2G     0x00
#define MPU9250_ACCEL_FS_4G     0x08
#define MPU9250_ACCEL_FS_8G     0x10
#define MPU9250_ACCEL_FS_16G    0x18

/* ---- 数字低通滤波器 ---- */
#define MPU9250_DLPF_184HZ      0x01
#define MPU9250_DLPF_92HZ       0x02
#define MPU9250_DLPF_41HZ       0x03
#define MPU9250_DLPF_20HZ       0x04
#define MPU9250_DLPF_10HZ       0x05
#define MPU9250_DLPF_5HZ        0x06

/* ---- 数据结构 ---- */

/* 三轴原始值 */
typedef struct {
    int16_t x;
    int16_t y;
    int16_t z;
} MPU9250_AxesRaw_t;

/* 三轴浮点值 */
typedef struct {
    float x;    /* 单位：g（加计）/ dps（陀螺仪）/ uT（磁力计） */
    float y;
    float z;
} MPU9250_Axes_t;

/* MPU9250 完整数据 */
typedef struct {
    MPU9250_Axes_t  accel;      /* 单位：g   */
    MPU9250_Axes_t  gyro;       /* 单位：dps */
    MPU9250_Axes_t  mag;        /* 单位：uT  */
    float           temperature;/* 单位：℃   */
} MPU9250_Data_t;

/* ---- 公开 API ---- */

/**
 * @brief  初始化 MPU9250（加计 + 陀螺仪 + AK8963 磁力计）
 *         需在 SYSCFG_DL_init() 之后调用，使用 I2C_1
 * @return true 成功，false WHO_AM_I 校验失败
 */
bool MPU9250_Init(void);

/**
 * @brief  读取加速度、陀螺仪、温度、磁力计全部数据
 * @param  data  存储结果的指针
 */
void MPU9250_ReadAll(MPU9250_Data_t *data);

/**
 * @brief  单独读取加速度
 * @param  accel 存储结果的指针（单位：g）
 */
void MPU9250_ReadAccel(MPU9250_Axes_t *accel);

/**
 * @brief  单独读取陀螺仪
 * @param  gyro 存储结果的指针（单位：dps）
 */
void MPU9250_ReadGyro(MPU9250_Axes_t *gyro);

/**
 * @brief  读取温度传感器
 * @return 温度值（单位：℃）
 */
float MPU9250_ReadTemperature(void);

/**
 * @brief  单独读取 AK8963 磁力计
 * @param  mag 存储结果的指针（单位：uT）
 */
void MPU9250_ReadMag(MPU9250_Axes_t *mag);

/**
 * @brief  写 MPU9250 单个寄存器
 */
void MPU9250_WriteReg(uint8_t reg, uint8_t value);

/**
 * @brief  读 MPU9250 单个寄存器
 */
uint8_t MPU9250_ReadReg(uint8_t reg);

#endif /* __MPU9250_H */
