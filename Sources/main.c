#include "FreeRTOS.h"
#include "task.h"
#include "motor.h"
#include "ti_msp_dl_config.h"



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

static void vMainTask(void *pvParameters) {
    (void)pvParameters;
    uint8_t flag = 0;
    
    for (;;) {
        if(flag == 0){
          Motor_SetTarget(&motor_left, 2000);
          Motor_SetTarget(&motor_right, 0);
          flag = 1;
        } else {
          Motor_SetTarget(&motor_left, 0);
          Motor_SetTarget(&motor_right, -2000);
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
    for (;;) {
        Motor_Update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

static void Motor_UpdateTask(void *pvParameters) {
    (void)pvParameters;
    for (;;) {
        // Motor_Update();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

int main(void)
{
  SYSCFG_DL_init();
    
    xTaskCreate(vMainTask, "Main", 128, NULL, 1, NULL);
    xTaskCreate(vBlinkTask, "Blink", 128, NULL, 2, NULL);
    xTaskCreate(vMotorTask, "Motor", 128, NULL, 3, NULL);
    xTaskCreate(Motor_UpdateTask, "Motor Update", 128, NULL, 3, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
