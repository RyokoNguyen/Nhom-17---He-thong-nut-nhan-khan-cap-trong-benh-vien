
#include "mylog.h"
#include "config.h"
#include "defines.h"

#include <stdio.h>
#include <string.h>

void LOG_Config(void){
    USART1_Config(115200);
}

/* retarget the C library printf function to the USART */
#ifdef __GNUC__
    #define PUTCHAR_PROTOTYPE int __io_putchar(int ch)
#else
    #define PUTCHAR_PROTOTYPE int fputc(int ch, FILE *f)
#endif
PUTCHAR_PROTOTYPE {
    HAL_UART_Transmit(&huart1, (uint8_t *)&ch, 1, 1000);
    return ch;
}

#ifdef __GNUC__
    int _write(int file, char *data, int len){
        int i;
        for(i = 0; i < len; i++){
            __io_putchar(*data++);
        }
        return len;
    }
#endif