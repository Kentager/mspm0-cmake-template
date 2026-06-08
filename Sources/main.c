#include "FreeRTOS.h"
#include "task.h"
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

static void vBlinkTask(void *pvParameters)
{
    (void)pvParameters;
    for (;;)
    {
        DL_GPIO_togglePins(LED_PORT, LED_PIN_24_PIN);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

int main(void)
{
    SYSCFG_DL_init();
    xTaskCreate(vBlinkTask, "Blink", 128, NULL, 1, NULL);
    vTaskStartScheduler();
    for (;;) {}
}
