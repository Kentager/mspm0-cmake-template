#include "FreeRTOS.h"
#include "task.h"
#include "motor.h"
#include "uart.h"
#include "encoder.h"
#include "ti_msp_dl_config.h"
#include "motor_app.h"
#include "mpu9250.h"
#include <stdio.h>

static MPU9250_Data_t data;


void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    for (;;) {}
}

void vApplicationMallocFailedHook(void)
{
    for (;;) {}
}

/* 编码器中断*/
void GROUP1_IRQHandler(void)
{
    uint32_t pending = DL_GPIO_getEnabledInterruptStatus(GPIOA, ENC_LEFT_ENA_PIN);

    if (pending & ENC_LEFT_ENA_PIN) {
        uint8_t a = DL_GPIO_readPins(GPIOA, ENC_LEFT_ENA_PIN) ? 1 : 0;
        uint8_t b = DL_GPIO_readPins(GPIOA, ENC_LEFT_ENB_PIN) ? 1 : 0;
        Encoder_Update(&encoder_left, a, b);
        DL_GPIO_clearInterruptStatus(GPIOA, ENC_LEFT_ENA_PIN);
    }

    pending = DL_GPIO_getEnabledInterruptStatus(GPIOB, ENC_RIGHT_ENA_PIN);

    if (pending & ENC_RIGHT_ENA_PIN) {
        uint8_t a = DL_GPIO_readPins(GPIOB, ENC_RIGHT_ENA_PIN) ? 1 : 0;
        uint8_t b = DL_GPIO_readPins(GPIOB, ENC_RIGHT_ENB_PIN) ? 0 : 1;
        Encoder_Update(&encoder_right, a, b);
        DL_GPIO_clearInterruptStatus(GPIOB, ENC_RIGHT_ENA_PIN);
    }

}

static void vMainTask(void *pvParameters) {
    (void)pvParameters;
    uint8_t flag = 0;
    for (;;) {
        if (flag == 0) {
            Motor_App_SetSpeed(-0.2, -0.2);
            flag = 1;
        }
        else {
            Motor_App_SetSpeed(0.2, 0.2);
            flag = 0;
        }
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

static void vBlinkTask(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        DL_GPIO_togglePins(LED_PORT, LED_PIN_2_PIN);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void vMotorTask(void *pvParameters) {
    (void)pvParameters;
    Motor_App_Init();
    for (;;) {
        Motor_App_Update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void vUARTTask(void *pvParameters) {
    (void)pvParameters;
    char buf[48];
    UART_Init();
    for (;;) {
        
        // float left  = Encoder_GetVelocity(&encoder_left);
        // float right = Encoder_GetVelocity(&encoder_right);
        // /* float 拆成整数打印（nano.specs 不支持 %f） */
        // int l_i = (int)(left  * 100.0f);
        // int r_i = (int)(right * 100.0f);
        // int l_abs = l_i < 0 ? -l_i : l_i;
        // int r_abs = r_i < 0 ? -r_i : r_i;
        // int len = snprintf(buf, sizeof(buf), "L:%s%d.%02d R:%s%d.%02d\r\n",
        //                    l_i < 0 ? "-" : "", l_abs / 100, l_abs % 100,
        //                    r_i < 0 ? "-" : "", r_abs / 100, r_abs % 100);
        float x = data.mag.x;
        float y = data.mag.y;
        float z = data.mag.z;
        int x_i = (int)(x * 100.0f);
        int y_i = (int)(y * 100.0f);
        int z_i = (int)(z * 100.0f);
        int x_abs = x_i < 0 ? -x_i : x_i;
        int y_abs = y_i < 0 ? -y_i : y_i;
        int z_abs = z_i < 0 ? -z_i : z_i;
        int len = snprintf(buf, sizeof(buf),
                           "X:%s%d.%02d Y:%s%d.%02d Z:%s%d.%02d\r\n",
                           x_i < 0 ? "-" : "", x_abs / 100, x_abs % 100,
                           y_i < 0 ? "-" : "", y_abs / 100, y_abs % 100,
                           z_i < 0 ? "-" : "", z_abs / 100, z_abs % 100);
        if (len > 0) {
            UART_SendData((uint8_t *)buf, (uint16_t)len);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


static void vMPU9250Task(void *pvParameters) {
    (void)pvParameters;
    vTaskDelay(1000);
    if (!MPU9250_Init()) {
        UART_SendData((uint8_t *)"MPU9250 INIT FAIL\r\n", 19);
    }else {
        UART_SendData((uint8_t *)"MPU9250 INIT OK\r\n", 17);
    }

    vTaskDelay(pdMS_TO_TICKS(500));

    for (;;) {
      MPU9250_ReadAll(&data);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
}

/* 简单 I2C 地址扫描 — 用于排查 MPU9250 通信问题 */
static void vI2CScanTask(void *pvParameters) {
    (void)pvParameters;
    vTaskDelay(2000);  /* 等待系统稳定 */

    char buf[32];
    UART_SendData((uint8_t *)"=== I2C Scan on I2C_1 ===\r\n", 28);

    /* 只扫描 MPU9250 可能的地址 0x68 和 0x69 */
    const uint8_t test_addrs[] = {0x68, 0x69};
    for (uint8_t i = 0; i < 2; i++) {
        uint8_t addr = test_addrs[i];
        /* 尝试发送一个字节到该地址（写 WHO_AM_I 寄存器地址） */
        uint8_t reg = 0x75;  /* WHO_AM_I */
        uint8_t tx_buf[1] = {reg};

        /* 等待 I2C 空闲 */
        uint32_t timeout = 10000;
        while (!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
            if (--timeout == 0) break;
        }

        DL_I2C_fillControllerTXFIFO(I2C_1_INST, tx_buf, 1);
        DL_I2C_startControllerTransfer(I2C_1_INST, addr,
                                       DL_I2C_CONTROLLER_DIRECTION_TX, 1);

        timeout = 10000;
        while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
            if (--timeout == 0) break;
        }

        uint32_t status = DL_I2C_getControllerStatus(I2C_1_INST);
        if (status & DL_I2C_CONTROLLER_STATUS_ERROR) {
            int len = snprintf(buf, sizeof(buf), "Addr 0x%02X: NACK\r\n", addr);
            UART_SendData((uint8_t *)buf, (uint16_t)len);
        } else {
            int len = snprintf(buf, sizeof(buf), "Addr 0x%02X: ACK!\r\n", addr);
            UART_SendData((uint8_t *)buf, (uint16_t)len);
        }

        /* 重置控制器传输状态 */
        DL_I2C_resetControllerTransfer(I2C_1_INST);
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    UART_SendData((uint8_t *)"=== Scan Done ===\r\n", 19);

    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(60000));  /* 只运行一次 */
    }
}

int main(void)
{
  SYSCFG_DL_init();
    
    // xTaskCreate(vI2CScanTask, "I2CScan", 256, NULL, 3, NULL);  /* 调试用，已注释 */
    xTaskCreate(vMainTask, "Main", 128, NULL, 1, NULL);
    xTaskCreate(vBlinkTask, "Blink", 128, NULL, 1, NULL);
    xTaskCreate(vUARTTask, "UART", 512, NULL, 1, NULL);
    xTaskCreate(vMotorTask, "Motor", 128, NULL, 2, NULL);
    xTaskCreate(vMPU9250Task, "MPU9250", 256, NULL, 3, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
