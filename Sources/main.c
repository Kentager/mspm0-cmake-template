#include "FreeRTOS.h"
#include "task.h"
#include "motor.h"
#include "uart.h"
#include "encoder.h"
#include "ti_msp_dl_config.h"
#include <stdio.h>



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
        if(flag == 0){
          Motor_SetTarget(&motor_left, 500);
          Motor_SetTarget(&motor_right, 500);
          flag = 1;
        } else {
          Motor_SetTarget(&motor_left, -500);
          Motor_SetTarget(&motor_right, -500);
          flag = 0;
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
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

static void vMotorTask(void *pvParameters) {
    (void)pvParameters;
    Motor_Init();
    Encoder_Init();
    for (;;) {
        Motor_Update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void vUARTTask(void *pvParameters) {
    (void)pvParameters;
    char buf[48];
    UART_Init();
    for (;;) {
        float left  = Encoder_GetDistance_mm(&encoder_left);
        float right = Encoder_GetDistance_mm(&encoder_right);
        int len = snprintf(buf, sizeof(buf), "L:%ld R:%ld\r\n", (long)left, (long)right);
        if (len > 0) {
            UART_SendData((uint8_t *)buf, (uint16_t)len);
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main(void)
{
  SYSCFG_DL_init();
    
    xTaskCreate(vMainTask, "Main", 128, NULL, 1, NULL);
    xTaskCreate(vBlinkTask, "Blink", 128, NULL, 1, NULL);
    xTaskCreate(vUARTTask, "UART", 512, NULL, 1, NULL);
    xTaskCreate(vMotorTask, "Motor", 128, NULL, 2, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
