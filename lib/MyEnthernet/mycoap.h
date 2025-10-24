#ifndef __MYCOAP_H
#define __MYCOAP_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

#include <stdint.h>
#include <stddef.h>

/* ====== Public API ======
 * Ghi chú:
 *  - Dùng UDP socket chưa bind (port 0) là đủ.
 *  - Gọi coap_client_begin() 1 lần trước khi dùng.
 *  - Hàm post/get thread-safe nếu mỗi socket chỉ dùng trong 1 task.
 */
/** Khởi tạo context. Không mở socket ở đây. */
// /void coap_client_begin(coap_client_t *c, uint8_t sock, const uint8_t ip[4], uint16_t port);
/** Mở UDP socket (nếu chưa mở). Trả 0 nếu ok. */
//int  coap_client_open_socket(coap_client_t *c);
/** Gửi POST /{room} với payload JSON (application/json). Trả 0 nếu gửi OK. */
//int  coap_post_room(coap_client_t *c, const char *room, const char *json);
/** Gửi GET /romm?/status. 
 *  - Nếu out_payload != NULL: copy payload (nếu có) vào đó và trả số byte payload.
 *  - Nếu không có payload, trả >=0 nhưng out_len=0.
 *  - Trả âm nếu lỗi (timeout, socket lỗi, ...).
 */
//int  coap_get_status(coap_client_t *c, const char *room, char *out_payload, int out_len);

/** Tiện ích: parse nhanh payload từ gói trả lời (tìm 0xFF). Trả con trỏ tới payload + set *len */
//const uint8_t* coap_find_payload(const uint8_t *pkt, int pkt_len, int *len_out);


void CoAP_Config(void);
void Task_CoAP_GET_Room(void *arg);
void Task_ButtonSendWaiting(void *arg);

#ifdef __cplusplus
}
#endif

#endif