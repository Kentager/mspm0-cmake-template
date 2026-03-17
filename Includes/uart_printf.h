#ifndef __UART_PRINTF_H
#define __UART_PRINTF_H

#include "ti_msp_dl_config.h"
#include "ti/driverlib/dl_uart.h"
#include <stdio.h>
#include <stdarg.h>

int uart_printf(const char *fmt, ...);

#endif