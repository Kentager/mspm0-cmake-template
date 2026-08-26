#include "FreeRTOS.h"
#include "projdefs.h"
#include "task.h"
#include "motor.h"
#include "uart.h"
#include "madgwick.h"
#include "encoder.h"
#include "ti_msp_dl_config.h"
#include "super_sensor.h"
#include "motor_app.h"
#include "mpu9250.h"
#include <math.h>
#include <stdio.h>
#include "key.h"
#include "oled_app.h"

static MPU9250_Data_t sensor_data;

#define KEY_COOLDOWN_MS  500U

static TickType_t g_key_last_tick = 0;
static uint8_t g_key_has_last_tick = 0U;

static float pitch = 0.0f;
static float roll = 0.0f;
static float yaw = 0.0f;

static uint16_t sensor_values[8] = {0};

#define BLUETOOTH_NAME        "YHUTB"
#define BLUETOOTH_PASSWORD    "2025"
#define BLUETOOTH_AT_DELAY_MS 200U

static uint8_t Key_IsAccepted(key_e key, TickType_t now)
{
    TickType_t cooldown_ticks;

    if ((key < KEY_0) || (key > KEY_3)) {
        return 0U;
    }

    cooldown_ticks = pdMS_TO_TICKS(KEY_COOLDOWN_MS);
    if ((g_key_has_last_tick != 0U) && ((now - g_key_last_tick) < cooldown_ticks)) {
        return 0U;
    }

    g_key_last_tick = now;
    g_key_has_last_tick = 1U;
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
    Job_e received_job;
    static Job_e msg = Job_None;
    static uint8_t Job_1_flag = 0;
    vTaskDelay(pdMS_TO_TICKS(3000));
    for (;;) {
        if (xQueueReceive(xJobQueue, &received_job, 0) == pdPASS) {
            if (received_job == Job_Stop) {
                msg = Job_None;
                Job_1_flag = 0U;    
                Motor_App_Brake();
            } else {
                msg = received_job;
            }
        }
        switch (msg) {
        case Job_0:
          // Motor_App_SetSpeed(0.3084f, 0.2f);    //0.3084--0.2  0.1542--0.1  0.0771--0.05  0.3855--0.25 0.4626--0.3
            Motor_App_Drive(1.30, 1.30, 0.4);
            while(!Motor_App_IsReached());
            Motor_App_SetSpeed((0.68f * 1.025), 0.4f);
            Motor_App_SetMode(SENSOR_MODE);
            while(!(yaw<=-175||yaw>=175));
            // Motor_App_SetSpeed(0.0f, 0.0f);
            Motor_App_SetSpeed(0.4f, 0.4f);
            Motor_App_SetMode(ANGLE_MODE);
            Motor_App_SetTargetYaw(-177);
            Motor_App_Drive(1.35, 1.35, 0.4);
            while(!Motor_App_IsReached());
            Motor_App_SetSpeed((0.68f * 1.025), 0.4f);
            Motor_App_SetMode(SENSOR_MODE);
            while(!(yaw>=-5&&yaw<=5));
            Motor_App_SetMode(ANGLE_MODE);
            Motor_App_SetTargetYaw(0);
            Motor_App_SetSpeed(0.0f, 0.0f);
            OLED_AppPauseTimer();
            msg = Job_None;
            break;
        case Job_1:
          // Motor_App_Drive(1.7, 1.7, 0.3);
            for(int i = 0;i < 20;i++){
              Motor_App_SetSpeed(0.4f * i / 20, 0.4f * i / 20);
              vTaskDelay(pdMS_TO_TICKS(160));
            }
            Motor_App_Drive(0.8, 0.8, 0.4);
            
            while(!Motor_App_IsReached());

            for(int i = 0;i < 20;i++){
              Motor_App_SetSpeed(0.4f * (19 - i) / 20, 0.4f * (19 - i) / 20);
              if(i == 5)            OLED_AppPauseTimer();
              vTaskDelay(pdMS_TO_TICKS(160));
            }
            OLED_AppPauseTimer();
            msg = Job_None;
            break;
        case Job_2:
            for(int i = 0;i < 20;i++){
                Motor_App_SetSpeed(0.3f * i / 20, 0.3f * i / 20);
                vTaskDelay(pdMS_TO_TICKS(150));
            }

            Motor_App_SetMode(SENSOR_MODE);
            
            Motor_App_SetSpeed(0.3f, 0.3f);
            Motor_App_Drive(1.29, 1.29, 0.3f);
            while(!Motor_App_IsReached());
            Motor_App_SetSpeed((0.4f * 1.025), 0.3f);

            while (!(yaw <= -179 || yaw >= 179))
              ;
            Motor_App_SetMode(ANGLE_MODE);
            Motor_App_SetTargetYaw(-180);
            Motor_App_Drive(1.10, 1.10, 0.34f);
            while(!Motor_App_IsReached());
            Motor_App_SetMode(SENSOR_MODE);
            Motor_App_SetSpeed((0.4f * 1.025 *( 0.32 / 0.3 )), 0.32f);
            while(!(yaw>=-1&&yaw<=1));
            Motor_App_SetMode(ANGLE_MODE);
            Motor_App_SetTargetYaw(0);
            Motor_App_Drive(0.2, 0.2, 0.32f);
            OLED_AppPauseTimer();
            while(!Motor_App_IsReached());
            Motor_App_SetMode(SPEED_MODE);
            for(int i = 0;i < 20;i++){
                Motor_App_SetSpeed(0.32f * (19 - i) / 20, 0.32f * (19 - i) / 20);
                vTaskDelay(pdMS_TO_TICKS(150));
            }
            msg = Job_None;
            break;
        case Job_3:
            for(int i = 0;i < 20;i++){
                Motor_App_SetSpeed(0.3f * i / 20, 0.3f * i / 20);
                vTaskDelay(pdMS_TO_TICKS(150));
            }

            Motor_App_SetMode(SENSOR_MODE);
            
            Motor_App_SetSpeed(0.3f, 0.3f);
            Motor_App_Drive(1.29, 1.29, 0.3f);
            while(!Motor_App_IsReached());
            Motor_App_SetSpeed((0.4f * 1.025), 0.3f);

            while (!(yaw <= -179 || yaw >= 179))
              ;
            Motor_App_SetMode(ANGLE_MODE);
            Motor_App_SetTargetYaw(-180);
            Motor_App_Drive(1.10, 1.10, 0.25f);
            while(!Motor_App_IsReached());
            Motor_App_SetMode(SENSOR_MODE);
            Motor_App_SetSpeed((0.4f * 1.025 *( 0.3 / 0.25 )), 0.25f);
            while(!(yaw>=-1&&yaw<=1));
            Motor_App_SetMode(ANGLE_MODE);
            Motor_App_SetTargetYaw(0);
            Motor_App_Drive(0.2, 0.2, 0.32f);
            OLED_AppPauseTimer();
            while(!Motor_App_IsReached());
            Motor_App_SetMode(SPEED_MODE);
            for(int i = 0;i < 20;i++){
                Motor_App_SetSpeed(0.32f * (19 - i) / 20, 0.32f * (19 - i) / 20);
                vTaskDelay(pdMS_TO_TICKS(150));
            }
            msg = Job_None;
            break;
        case Job_None:
        case Job_Stop:
        default:
            break;
        };
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

static void vBlinkTask(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
      DL_GPIO_togglePins(LED_PORT, LED_PIN_PIN);
        vTaskDelay(pdMS_TO_TICKS(1000));
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
    char buf[24];
    TickType_t last_wake_time;

    UART_Init();
    last_wake_time = xTaskGetTickCount();
    for (;;) {
        int yaw_i = (int)(yaw * 100.0f);
        int yaw_abs = yaw_i < 0 ? -yaw_i : yaw_i;
        int len = snprintf(buf, sizeof(buf), "YAW:%s%d.%02d\r\n",
                           yaw_i < 0 ? "-" : "", yaw_abs / 100, yaw_abs % 100);
        if (len > 0) {
            UART_SendData((uint8_t *)buf, (uint16_t)len);
        }
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(20));
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
        OLED_AppSetSensorValues(sensor_values);
        OLED_AppRefresh();
        if (xQueueReceive(xKeyQueue, &key_msg, pdMS_TO_TICKS(100)) == pdPASS) {
            OLED_AppHandleKey(key_msg);
        }
    }
}


static void vSensorTask(void *pvParameters) {
    (void)pvParameters;
    Super_Sensor_Init(&superSensor);
    for (;;) {
        Super_Sensor_Update(&superSensor);
        for (uint8_t i = 0U; i < SUPER_SENSOR_COUNT; i++) {
            sensor_values[i] = superSensor.sensorState[i];
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void)
{
    SYSCFG_DL_init();
    xKeyQueue = xQueueCreate(KEY_QUEUE_LEN, sizeof(key_e));
    xJobQueue = xQueueCreate(JOB_QUEUE_LEN, sizeof(Job_e));
    // xTaskCreate(vI2CScanTask_OLED, "I2CScan", 256, NULL, 3, NULL);  /* 调试用，已注释 */
    xTaskCreate(vMainTask, "Main", 128, NULL, 1, NULL);
    xTaskCreate(vBlinkTask, "Blink", 128, NULL, 1, NULL);
    xTaskCreate(vUARTTask, "UART", 256, NULL, 1, NULL);
    xTaskCreate(vMotorTask, "Motor", 128, NULL, 3, NULL);
    xTaskCreate(vSensorTask, "Sensor", 128, NULL, 3, NULL);
    xTaskCreate(vOLEDTask, "OLED", 256, NULL, 2, NULL);
    xTaskCreate(vMPU9250Task, "MPU9250", 384, NULL, 4, NULL);
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
