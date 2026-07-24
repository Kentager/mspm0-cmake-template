#include "uart.h"
#include "FreeRTOS.h"
#include "queue.h"
#include <ti/driverlib/dl_uart_main.h>
#include "ti_msp_dl_config.h"

/* 消息队列句柄 */
static QueueHandle_t xRxQueue = NULL;
static QueueHandle_t xTxQueue = NULL;

/* 初始化串口和消息队列 */
void UART_Init(void)
{
    /* 创建接收和发送队列 */
    if (xRxQueue == NULL) {
        xRxQueue = xQueueCreate(UART_RX_QUEUE_LEN, sizeof(uint8_t));
    }
    if (xTxQueue == NULL) {
        xTxQueue = xQueueCreate(UART_TX_QUEUE_LEN, sizeof(uint8_t));
    }

    /* 使能 UART 接收中断 */
    DL_UART_Main_enableInterrupt(UART_1_INST, DL_UART_MAIN_INTERRUPT_RX);
    NVIC_EnableIRQ(UART_1_INST_INT_IRQN);
}

/* 发送单个字节（阻塞） */
void UART_SendByte(uint8_t data)
{
    /* 等待发送缓冲区空 */
    while (!DL_UART_Main_isTXFIFOEmpty(UART_1_INST)) {}
    DL_UART_Main_transmitData(UART_1_INST, data);
}

/* 发送字符串 */
void UART_SendString(const char *str)
{
    while (*str) {
        UART_SendByte(*str++);
    }
}

/* 发送数据块 */
void UART_SendData(const uint8_t *data, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        UART_SendByte(data[i]);
    }
}


/* 从接收队列读取一个字节（阻塞） */
uint8_t UART_ReceiveByte(void)
{
    uint8_t data;
    xQueueReceive(xRxQueue, &data, portMAX_DELAY);
    return data;
}

/* 获取接收队列中的数据数量 */
uint16_t UART_GetRxCount(void)
{
    return (uint16_t)uxQueueMessagesWaiting(xRxQueue);
}

/* UART 中断处理函数 */
void UART_1_INST_IRQHandler(void)
{
    uint32_t status = DL_UART_getEnabledInterruptStatus(UART_1_INST, DL_UART_MAIN_INTERRUPT_RX);

    if (status & DL_UART_MAIN_INTERRUPT_RX) {
        /* 读取接收到的数据 */
        uint8_t data = (uint8_t)DL_UART_Main_receiveData(UART_1_INST);

        /* 发送到接收队列（非阻塞） */
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        if (xRxQueue != NULL) {
            xQueueSendFromISR(xRxQueue, &data, &xHigherPriorityTaskWoken);
        }

        /* 如果需要，进行上下文切换 */
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }

    /* 清除中断标志 */
    DL_UART_Main_clearInterruptStatus(UART_1_INST, status);
}
