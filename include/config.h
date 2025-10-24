#ifndef __CONFIG_H
#define __CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

#include "defines.h"

void GPIO_Input_Config(void);
void GPIO_Output_Config(void);
void SystemClock_Config(void);

void USART1_Config(uint32_t _baudrate);
void USART2_Config(uint32_t _baudrate);
void USART3_Config(uint32_t _baudrate);

void SPI1_Config();

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif