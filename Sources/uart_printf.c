
#include "uart_printf.h"

int uart_printf(const char *fmt, ...)
{
    static char buf[128];
    uint32_t i,len;
    va_list args;
    va_start(args,fmt);
    len = vsnprintf((char *)buf,sizeof(buf),(char *)fmt,args);
    va_end(args);
    i=0;
    while(i<len)
    {
        DL_UART_transmitData(UART0_INST,buf[i]);
        // 等待FIFO清空再输出
        while(!DL_UART_isTXFIFOEmpty(UART0));
        i++;
    }
    return len;
}