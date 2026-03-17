
#include <stdlib.h>
#include <string.h>

#include "ti_msp_dl_config.h"
#include "uart_printf.h"

#include "FreeRTOS.h"
#include "task.h"

void printLogTask()
{
    while (true)
    {
        uart_printf("=== printLogTask thread ===\r\n");
        uart_printf("Free heap memory left: %d bytes\r\n", xPortGetFreeHeapSize());
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void blinkTask()
{
    while (true)
    {
        // LED 5Hz频率闪烁
        DL_GPIO_togglePins(PORTA_PORT, PORTA_LED_USER_PIN);
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

int main()
{
    SYSCFG_DL_init();
    TaskHandle_t printLogTask_handle;
    TaskHandle_t blinkTask_handle;
    xTaskCreate(printLogTask, "printLogTask", 0x80, NULL, configMAX_PRIORITIES - 1, &printLogTask_handle);
    xTaskCreate(blinkTask, "blinkTask", 0x80, NULL, configMAX_PRIORITIES - 1, &blinkTask_handle);
    vTaskStartScheduler();
}
