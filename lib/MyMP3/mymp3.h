#ifndef __MYMP3_H
#define __MYMP3_H

#include "stm32f1xx_hal.h"
#include <stdint.h>
#include <stdbool.h>


// ====== Cấu hình UART và Logging ======
extern UART_HandleTypeDef huart2;   // UART kết nối DFPlayer

// ====== API ======
void DF_Config(void);                           // Khởi tạo DFPlayer
void dfp_set_volume(uint8_t vol);              // 0..30
bool dfp_play_folder_file(uint8_t folder, uint8_t file);  // /<folder>/<file>.mp3
bool dfp_stop(void);
bool dfp_pause(void);
bool dfp_resume(void);

// ====== Cập nhật trạng thái và kiểm tra ======
void dfp_poll(void);               // gọi thường xuyên để xử lý phản hồi UART
bool dfp_is_playing(void);         // kiểm tra có đang phát không
void dfp_set_expect_ms(uint32_t ms); // thiết lập thời lượng dự kiến (dự phòng timeout)

#endif