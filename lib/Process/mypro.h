#ifndef __MYPRO_H
#define __MYPRO_H

#include "stm32f1xx_hal.h"

void handle_json_room_status(const char *payload /* NUL-terminated */, const char *topic);

#endif