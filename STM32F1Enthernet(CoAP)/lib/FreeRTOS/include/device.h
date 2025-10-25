#ifndef DEVICE_H
#define DEVICE_H

#pragma GCC diagnostic ignored "-Wsign-conversion"
#include <misc.h>
#include <stm32f4xx.h>
#include <stm32f4xx_hal_gpio.h>
#include <stm32f4xx_hal_rcc.h>
#include <stm32f4xx_hal_i2c.h>
#pragma GCC diagnostic warning "-Wsign-conversion"

//#define USE_SLEEP


#define BITBAND_GPIO(bitband_addr, pin)	(*(volatile uint32_t *)(PERIPH_BB_BASE + ((intptr_t)bitband_addr - PERIPH_BASE)*32 + 4 * pin))

#define SYS_FREQ	72000000
#define Periph_enable_flag  0x01
#define I2C_CLOCK   50000

//typedef enum{FALSE, TRUE}FLAG;

#endif /* DEVICE_H */
static unsigned int delay_ms_count = 0;

//void SysTick_Handler(void)
//{
//	if (delay_ms_count != 0)
//		delay_ms_count--;
//}

void delay_ms(__IO unsigned int msec){
	delay_ms_count = msec;
	SysTick_Config(SystemCoreClock / 1000);

	while (delay_ms_count != 0);

	SysTick->CTRL &= ~SysTick_CTRL_ENABLE_Msk;
}

void delay_s(__IO unsigned int sec){
	delay_ms(sec * 1000);
}