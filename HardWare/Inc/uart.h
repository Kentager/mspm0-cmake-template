#ifndef UART_H
#define UART_H

#include <stdint.h>

/* 配置参数 */
#define UART_RX_QUEUE_LEN   64   /* 接收队列长度 */
#define UART_TX_QUEUE_LEN   64   /* 发送队列长度 */

/* 函数声明 */
void UART_Init(void);
void UART_SendByte(uint8_t data);
void UART_SendString(const char *str);
void UART_SendData(const uint8_t *data, uint16_t len);
uint8_t UART_ReceiveByte(void);
uint16_t UART_GetRxCount(void);

/* 中断处理函数（在 main.c 中调用） */
void UART_IRQHandler(void);

#endif /* UART_H */
