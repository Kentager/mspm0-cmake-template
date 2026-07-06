#include "FreeRTOS.h"
#include "task.h"
#include "motor.h"
#include "uart.h"
#include "madgwick.h"
#include "encoder.h"
#include "ti_msp_dl_config.h"
#include "motor_app.h"
#include "mpu9250.h"
#include <math.h>
#include <stdio.h>
#include "key.h"
#include "oled_app.h"

static MPU9250_Data_t sensor_data;

#define KEY_DEBOUNCE_MS  20U

static TickType_t g_key_last_tick[4] = {0};

static float pitch = 0.0f;
static float roll = 0.0f;
static float yaw = 0.0f;

static uint8_t Key_IsAccepted(key_e key, TickType_t now)
{
    TickType_t debounce_ticks;

    if ((key < KEY_0) || (key > KEY_3)) {
        return 0U;
    }

    debounce_ticks = pdMS_TO_TICKS(KEY_DEBOUNCE_MS);
    if ((now - g_key_last_tick[key]) < debounce_ticks) {
        return 0U;
    }

    g_key_last_tick[key] = now;
    return 1U;
}

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


/* 编码器+按键中断 */
void GROUP1_IRQHandler(void)
{
    uint32_t pending_a = DL_GPIO_getEnabledInterruptStatus(GPIOA,
                                                           ENC_LEFT_ENA_PIN |
                                                           ENC_LEFT_ENB_PIN |
                                                           KEY_KEY_0_PIN |
                                                           KEY_KEY_1_PIN);
    uint32_t pending_b = DL_GPIO_getEnabledInterruptStatus(GPIOB,
                                                           ENC_RIGHT_ENA_PIN |
                                                           ENC_RIGHT_ENB_PIN |
                                                           KEY_KEY_2_PIN |
                                                           KEY_KEY_3_PIN);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    key_e key_msg;
    TickType_t now;

    if (pending_a & ENC_LEFT_ENA_PIN) {
        uint8_t a = DL_GPIO_readPins(GPIOA, ENC_LEFT_ENA_PIN) ? 1 : 0;
        uint8_t b = DL_GPIO_readPins(GPIOA, ENC_LEFT_ENB_PIN) ? 1 : 0;
        Encoder_Update(&encoder_left, a, b);
        DL_GPIO_clearInterruptStatus(GPIOA, ENC_LEFT_ENA_PIN);
    }
    if (pending_a & ENC_LEFT_ENB_PIN) {
        DL_GPIO_clearInterruptStatus(GPIOA, ENC_LEFT_ENB_PIN);
    }

    if (pending_a & KEY_KEY_0_PIN) {
        DL_GPIO_clearInterruptStatus(GPIOA, KEY_KEY_0_PIN);
        now = xTaskGetTickCountFromISR();
        if (Key_IsAccepted(KEY_0, now) != 0U) {
            key = KEY_0;
            key_msg = KEY_0;
            if (xKeyQueue != NULL) {
                xQueueSendFromISR(xKeyQueue, &key_msg, &xHigherPriorityTaskWoken);
            }
        }
    }
    if (pending_a & KEY_KEY_1_PIN) {
        DL_GPIO_clearInterruptStatus(GPIOA, KEY_KEY_1_PIN);
        now = xTaskGetTickCountFromISR();
        if (Key_IsAccepted(KEY_1, now) != 0U) {
            key = KEY_1;
            key_msg = KEY_1;
            if (xKeyQueue != NULL) {
                xQueueSendFromISR(xKeyQueue, &key_msg, &xHigherPriorityTaskWoken);
            }
        }
    }
    if (pending_b & KEY_KEY_2_PIN) {
        DL_GPIO_clearInterruptStatus(GPIOB, KEY_KEY_2_PIN);
        now = xTaskGetTickCountFromISR();
        if (Key_IsAccepted(KEY_2, now) != 0U) {
            key = KEY_2;
            key_msg = KEY_2;
            if (xKeyQueue != NULL) {
                xQueueSendFromISR(xKeyQueue, &key_msg, &xHigherPriorityTaskWoken);
            }
        }
    }
    if (pending_b & KEY_KEY_3_PIN) {
        DL_GPIO_clearInterruptStatus(GPIOB, KEY_KEY_3_PIN);
        now = xTaskGetTickCountFromISR();
        if (Key_IsAccepted(KEY_3, now) != 0U) {
            key = KEY_3;
            key_msg = KEY_3;
            if (xKeyQueue != NULL) {
                xQueueSendFromISR(xKeyQueue, &key_msg, &xHigherPriorityTaskWoken);
            }
        }
    }

    if (pending_b & ENC_RIGHT_ENA_PIN) {
        uint8_t a = DL_GPIO_readPins(GPIOB, ENC_RIGHT_ENA_PIN) ? 1 : 0;
        uint8_t b = DL_GPIO_readPins(GPIOB, ENC_RIGHT_ENB_PIN) ? 0 : 1;
        Encoder_Update(&encoder_right, a, b);
        DL_GPIO_clearInterruptStatus(GPIOB, ENC_RIGHT_ENA_PIN);
    }
    if (pending_b & ENC_RIGHT_ENB_PIN) {
        DL_GPIO_clearInterruptStatus(GPIOB, ENC_RIGHT_ENB_PIN);
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

static void vMainTask(void *pvParameters) {
    (void)pvParameters;
    uint8_t flag = 0;
    vTaskDelay(pdMS_TO_TICKS(3000));
    for (;;) {
        if (OLED_AppIsAutoRunEnabled() != 0U) {
            Motor_App_SetSpeed(0.2f, 0.2f);
            Motor_App_SetTargetYaw(flag % 4 == 0 ? 0.0f :
                                   flag % 4 == 1 ? -90.0f :
                                   flag % 4 == 2 ? -180.0f : 180.0f);
            flag++;
        }
        vTaskDelay(pdMS_TO_TICKS(3000));
    }
}

static void vBlinkTask(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        DL_GPIO_togglePins(LED_PORT, LED_PIN_2_PIN);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void vMotorTask(void *pvParameters) {
    (void)pvParameters;
    Motor_App_Init();
    for (;;) {
        Motor_App_YawUpdate(yaw);
        Motor_App_Update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void vUARTTask(void *pvParameters) {
    (void)pvParameters;
    char buf[48];
    UART_Init();
    for (;;) {
        int pitch_i = (int)(pitch * 100.0f);
        int roll_i = (int)(roll * 100.0f);
        int yaw_i = (int)(yaw * 100.0f);
        int pitch_abs = pitch_i < 0 ? -pitch_i : pitch_i;
        int roll_abs = roll_i < 0 ? -roll_i : roll_i;
        int yaw_abs = yaw_i < 0 ? -yaw_i : yaw_i;
        int len = snprintf(buf, sizeof(buf),
                           "%s%d.%02d,%s%d.%02d,%s%d.%02d\r\n",
                           pitch_i < 0 ? "-" : "", pitch_abs / 100, pitch_abs % 100,
                           roll_i < 0 ? "-" : "", roll_abs / 100, roll_abs % 100,
                           yaw_i < 0 ? "-" : "", yaw_abs / 100, yaw_abs % 100);
        if (len > 0) {
            UART_SendData((uint8_t *)buf, (uint16_t)len);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


static void vMPU9250Task(void *pvParameters) {
    (void)pvParameters;
    
    vTaskDelay(pdMS_TO_TICKS(1000));
    
    if (!MPU9250_Init()) {
        UART_SendData((uint8_t *)"MPU9250 INIT FAIL\r\n", 19);
        for (;;);
    }
    UART_SendData((uint8_t *)"MPU9250 INIT OK\r\n", 17);
    
    // ----- 陀螺仪零偏校准 -----
    MPU9250_CalibrateGyro();
    
    // ----- 初始化角度 -----
    static uint32_t last_time = 0;
    
    UART_SendData((uint8_t *)"MPU9250 ready\r\n", 15);
    
    // ----- 主循环：直接用陀螺仪积分 -----
    for (;;) {
        MPU9250_ReadAll(&sensor_data);
        
        // 计算时间差 dt（秒）
        uint32_t now = xTaskGetTickCount();
        float dt = (now - last_time) / 1000.0f;
        last_time = now;
        
        // 限制 dt 防止跳变（如果任务被阻塞太久）
        if (dt > 0.05f) dt = 0.05f;
        if (dt < 0.001f) dt = 0.001f;
        
        // 陀螺仪数据（度/秒）  
        float gx = sensor_data.gyro.x;
        float gy = sensor_data.gyro.y;
        float gz = sensor_data.gyro.z;
        
        // 直接积分（角度 += 角速度 × 时间）
        pitch += gx * dt;
        roll  += gy * dt;
        yaw   += gz * dt * 119 / 120;
        
        // 可选：限制角度范围（-180 ~ 180）
        if (pitch > 180) pitch -= 360;
        if (pitch < -180) pitch += 360;
        if (roll > 180) roll -= 360;
        if (roll < -180) roll += 360;
        if (yaw > 180) yaw -= 360;
        if (yaw < -180) yaw += 360;
        
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}


static void vOLEDTask(void *pvParameters) {
    (void)pvParameters;
    key_e key_msg;

    OLED_AppInit();
    vTaskDelay(pdMS_TO_TICKS(1000));

    for (;;) {
        OLED_AppSetAttitude(pitch, roll, yaw);
        OLED_AppRefresh();
        if (xQueueReceive(xKeyQueue, &key_msg, pdMS_TO_TICKS(100)) == pdPASS) {
            OLED_AppHandleKey(key_msg);
        }
    }
}


int main(void)
{
    SYSCFG_DL_init();
    xKeyQueue = xQueueCreate(KEY_QUEUE_LEN, sizeof(key_e));
    // xTaskCreate(vI2CScanTask_OLED, "I2CScan", 256, NULL, 3, NULL);  /* 调试用，已注释 */
    xTaskCreate(vMainTask, "Main", 128, NULL, 1, NULL);
    xTaskCreate(vBlinkTask, "Blink", 128, NULL, 1, NULL);
    xTaskCreate(vUARTTask, "UART", 256, NULL, 1, NULL);
    xTaskCreate(vOLEDTask, "OLED", 256, NULL, 3, NULL);
    xTaskCreate(vMotorTask, "Motor", 128, NULL, 2, NULL);
    xTaskCreate(vMPU9250Task, "MPU9250", 384, NULL, 3, NULL);
    vTaskStartScheduler();
    for (;;) {}
}

// /* 简单 I2C 地址扫描 — 用于排查 MPU9250 通信问题 */
// static void vI2CScanTask(void *pvParameters) {
//     (void)pvParameters;
//     vTaskDelay(2000);  /* 等待系统稳定 */

//     char buf[32];
//     UART_SendData((uint8_t *)"=== I2C Scan on I2C_1 ===\r\n", 28);

//     /* 只扫描 MPU9250 可能的地址 0x68 和 0x69 */
//     const uint8_t test_addrs[] = {0x68, 0x69};
//     for (uint8_t i = 0; i < 2; i++) {
//         uint8_t addr = test_addrs[i];
//         /* 尝试发送一个字节到该地址（写 WHO_AM_I 寄存器地址） */
//         uint8_t reg = 0x75;  /* WHO_AM_I */
//         uint8_t tx_buf[1] = {reg};

//         /* 等待 I2C 空闲 */
//         uint32_t timeout = 10000;
//         while (!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
//             if (--timeout == 0) break;
//         }

//         DL_I2C_fillControllerTXFIFO(I2C_1_INST, tx_buf, 1);
//         DL_I2C_startControllerTransfer(I2C_1_INST, addr,
//                                        DL_I2C_CONTROLLER_DIRECTION_TX, 1);

//         timeout = 10000;
//         while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
//             if (--timeout == 0) break;
//         }

//         uint32_t status = DL_I2C_getControllerStatus(I2C_1_INST);
//         if (status & DL_I2C_CONTROLLER_STATUS_ERROR) {
//             int len = snprintf(buf, sizeof(buf), "Addr 0x%02X: NACK\r\n", addr);
//             UART_SendData((uint8_t *)buf, (uint16_t)len);
//         } else {
//             int len = snprintf(buf, sizeof(buf), "Addr 0x%02X: ACK!\r\n", addr);
//             UART_SendData((uint8_t *)buf, (uint16_t)len);
//         }

//         /* 重置控制器传输状态 */
//         DL_I2C_resetControllerTransfer(I2C_1_INST);
//         vTaskDelay(pdMS_TO_TICKS(50));
//     }

//     UART_SendData((uint8_t *)"=== Scan Done ===\r\n", 19);

//     for (;;) {
//         vTaskDelay(pdMS_TO_TICKS(60000));  /* 只运行一次 */
//     }
// }

// /* 简单 I2C 地址扫描 — 用于排查 OLED 通信问题 */
// static void vI2CScanTask_OLED(void *pvParameters) {
//     (void)pvParameters;
//     vTaskDelay(2000);  /* 等待系统稳定 */
    
//     char buf[32];
//     UART_SendData((uint8_t *)"=== I2C Scan on I2C_0 (OLED) ===\r\n", 32);
    
//     /* 扫描 OLED 可能的地址 0x3C 和 0x3D */
//     const uint8_t test_addrs[] = {0x3C, 0x3D};
//     for (uint8_t i = 0; i < 2; i++) {
//         uint8_t addr = test_addrs[i];
//         /* 尝试发送一个字节到该地址（写命令0x00） */
//         uint8_t reg = 0x00;  /* OLED 命令 */
//         uint8_t tx_buf[1] = {reg};
        
//         /* 等待 I2C 空闲 */
//         uint32_t timeout = 10000;
//         while (!(DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
//             if (--timeout == 0) break;
//         }
        
//         DL_I2C_fillControllerTXFIFO(I2C_0_INST, tx_buf, 1);
//         DL_I2C_startControllerTransfer(I2C_0_INST, addr,
//                                        DL_I2C_CONTROLLER_DIRECTION_TX, 1);
        
//         timeout = 10000;
//         while (DL_I2C_getControllerStatus(I2C_0_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
//             if (--timeout == 0) break;
//         }
        
//         uint32_t status = DL_I2C_getControllerStatus(I2C_0_INST);
//         if (status & DL_I2C_CONTROLLER_STATUS_ERROR) {
//             int len = snprintf(buf, sizeof(buf), "Addr 0x%02X: NACK\r\n", addr);
//             UART_SendData((uint8_t *)buf, (uint16_t)len);
//         } else {
//             int len = snprintf(buf, sizeof(buf), "Addr 0x%02X: ACK!\r\n", addr);
//             UART_SendData((uint8_t *)buf, (uint16_t)len);
//         }
        
//         /* 重置控制器传输状态 */
//         DL_I2C_resetControllerTransfer(I2C_0_INST);
//         vTaskDelay(pdMS_TO_TICKS(50));
//     }
    
//     UART_SendData((uint8_t *)"=== Scan Done ===\r\n", 19);
    
//     for (;;) {
//         vTaskDelay(pdMS_TO_TICKS(60000));  /* 只运行一次 */
//     }
// }
