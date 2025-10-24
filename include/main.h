#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"
#include "FreeRTOS.h"
#include "task.h"

#include "var.h"
#include "mylog.h"
#include "mymp3.h"
#include "mycoap.h"
#include "config.h"
#include "defines.h"

#include "myprotocol.h"

void Task_Main(void *pvParameters);
#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
