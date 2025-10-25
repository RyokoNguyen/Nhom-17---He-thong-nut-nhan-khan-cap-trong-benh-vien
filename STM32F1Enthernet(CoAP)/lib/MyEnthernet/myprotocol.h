#ifndef __MYPROTOCOL_H
#define __MYPROTOCOL_H

#include "stm32f1xx_hal.h"

void w5500_get_netinfo(void);
int w5500_set_static_ip(void);
int w5500_init_spi_and_chip(void);
void w5500_close_socket(uint8_t s);


#endif