#include "var.h"
#include "mypro.h"
#include "config.h"
#include "mycoap.h"
#include "defines.h"

#include <string.h>
#include <stdio.h>

/* ioLibrary WIZnet */
#include "socket.h"
#include "wizchip_conf.h"

#include "FreeRTOS.h"
#include "task.h"

static const uint8_t COAP_SERVER_IP[4] = {192, 168, 10, 1};

static coap_client_t g_coap_post;
static coap_client_t g_coap_get;

/* ===== Helpers ===== */

/* Ghi 1 option với delta dồn (delta = opt_no - last_no), length giản lược (<13).
 * ĐỦ dùng cho ví dụ này: các option ngắn và số option tăng dần nhỏ.
 * Trả về con trỏ sau cùng.
 */
static uint8_t* coap_write_option_short(uint8_t *p, uint16_t *last_no, uint16_t opt_no, const uint8_t *val, uint16_t len){
    uint16_t delta = (uint16_t)(opt_no - *last_no);
    *last_no = opt_no;

    // Chỉ hỗ trợ len < 13 và delta < 13
    *p++ = (uint8_t)((delta << 4) | (len & 0x0F));
    if (len && val) {
        memcpy(p, val, len);
        p += len;
    }
    return p;
}

const uint8_t* coap_find_payload(const uint8_t *pkt, int pkt_len, int *len_out){
    if (len_out) *len_out = 0;
    if (!pkt || pkt_len < 5) return NULL;

    for (int i = 4; i < pkt_len; ++i) {
        if (pkt[i] == 0xFF) {
            if (i + 1 < pkt_len) {
                if (len_out) *len_out = (pkt_len - (i + 1));
                return &pkt[i + 1];
            }
            return NULL;
        }
    }
    return NULL;
}

/* ===== API ===== */

void coap_client_begin(coap_client_t *c, uint8_t sock, const uint8_t ip[4], uint16_t port){
    memset(c, 0, sizeof(*c));
    c->sock = sock;
    memcpy(c->remote_ip, ip, 4);
    c->remote_port = port;
    c->next_mid = 1;
    c->rx_timeout_ms = 1000; // mặc định 1 giây
}

int coap_client_open_socket(coap_client_t *c){
    // Nếu socket chưa open (trạng thái SOCK_CLOSED), mở UDP ở port 0
    // (W5500 sẽ tự cấp source port ngẫu nhiên)
    uint8_t s = c->sock;
    uint8_t sr = getSn_SR(s);
    if (sr == SOCK_CLOSED) {
        if (socket(s, Sn_MR_UDP, 0, 0) != s) {
            return -1;
        }
    }
    return 0;
}

int coap_post_room(coap_client_t *c, const char *room, const char *json){
    if (!room || !json) return -1;
    if (coap_client_open_socket(c) != 0) return -2;

    uint8_t pkt[300];
    uint8_t *p = pkt;

    /* Header */
    *p++ = (uint8_t)((COAP_VER << 6) | (COAP_TYPE_CON << 4) | COAP_TKL_ZERO); // ver/type/tkl
    *p++ = COAP_CODE_POST;
    uint16_t mid = c->next_mid++;
    *p++ = (mid >> 8) & 0xFF;
    *p++ = (mid     ) & 0xFF;

    /* Options (PHẢI tăng dần theo số opt):
       1) Uri-Path: "roomX"
       2) Content-Format: app/json (50) → 2 byte BE (0x00, 0x32)
    */
    uint16_t last_opt = 0;
    const uint8_t *room_b = (const uint8_t*)room;
    uint16_t room_len = (uint16_t)strlen(room);
    if (room_len >= 13) return -3; // giản lược encoder

    p = coap_write_option_short(p, &last_opt, COAP_OPT_URI_PATH, room_b, room_len);

    uint8_t cfv[2] = { 0x00, COAP_CF_APP_JSON };
    p = coap_write_option_short(p, &last_opt, COAP_OPT_CONTENT_FORMAT, cfv, sizeof(cfv));

    /* Payload marker + JSON */
    *p++ = 0xFF;
    uint16_t jlen = (uint16_t)strlen(json);
    if (jlen > (sizeof(pkt) - (size_t)(p - pkt))) return -4;
    memcpy(p, json, jlen);
    p += jlen;

    int send_len = (int)(p - pkt);
    int r = sendto(c->sock, pkt, send_len, (uint8_t*)c->remote_ip, c->remote_port);
    if (r != send_len) return -5;

    /* (Tuỳ chọn) chờ ACK/RESP trong thời gian ngắn để debug */
    uint32_t t0 = HAL_GetTick();
    do {
        uint16_t rsr = getSn_RX_RSR(c->sock);
        if (rsr) {
            uint8_t rip[4]; uint16_t rp; 
            uint8_t rb[300];
            int rl = recvfrom(c->sock, rb, (rsr > sizeof(rb)) ? sizeof(rb) : rsr, rip, &rp);
            (void)rl; (void)rip; (void)rp;
            break;
        }
    } while ((HAL_GetTick() - t0) < c->rx_timeout_ms);

    return 0;
}

int coap_get_status(coap_client_t *c, const char *room, char *out_payload, int out_len){
    if (!room) return -1;
    if (coap_client_open_socket(c) != 0) return -2;

    uint8_t pkt[256];
    uint8_t *p = pkt;

    /* Header */
    *p++ = (uint8_t)((COAP_VER << 6) | (COAP_TYPE_CON << 4) | COAP_TKL_ZERO);
    *p++ = COAP_CODE_GET;
    uint16_t mid = c->next_mid++;
    *p++ = (mid >> 8) & 0xFF;
    *p++ = (mid     ) & 0xFF;

    /* Options: 
       1) Uri-Path = room (vd: "room1")
       2) Uri-Path = "status"
    */
    uint16_t last_opt = 0;
    const uint8_t *room_b = (const uint8_t*)room;
    uint16_t room_len = (uint16_t)strlen(room);
    if (room_len >= 13) return -3; // encoder giản lược (<13)

    p = coap_write_option_short(p, &last_opt, COAP_OPT_URI_PATH, room_b, room_len);
    p = coap_write_option_short(p, &last_opt, COAP_OPT_URI_PATH, (const uint8_t*)"status", 6);

    /* No payload */
    int send_len = (int)(p - pkt);
    int r = sendto(c->sock, pkt, send_len, (uint8_t*)c->remote_ip, c->remote_port);
    if (r != send_len) return -4;

    /* Chờ phản hồi */
    uint32_t t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < c->rx_timeout_ms) {
        uint16_t rsr = getSn_RX_RSR(c->sock);
        if (!rsr) { HAL_Delay(10); continue; }

        uint8_t rip[4]; uint16_t rp;
        uint8_t rb[300];
        int rl = recvfrom(c->sock, rb, (rsr > sizeof(rb)) ? sizeof(rb) : rsr, rip, &rp);
        if (rl <= 0) continue;

        int pay_len = 0;
        const uint8_t *pay = coap_find_payload(rb, rl, &pay_len);
        if (out_payload && out_len > 0) {
            int ncopy = (pay && pay_len > 0) ? pay_len : 0;
            if (ncopy >= out_len) ncopy = out_len - 1;
            if (ncopy > 0) memcpy(out_payload, pay, ncopy);
            if (out_len > 0) out_payload[(ncopy >= 0) ? ncopy : 0] = 0;
            return ncopy; // bytes payload copy
        }
        return (pay && pay_len > 0) ? pay_len : 0;
    }
    return -5; // timeout
}

/* Task gửi POST: waiting -> approved -> done */
/* void Task_CoAP_POST_Demo(void *arg){
    (void)arg;
    coap_client_begin(&g_coap_post, 1, COAP_SERVER_IP, COAP_SERVER_PORT);
    if (coap_client_open_socket(&g_coap_post) != 0) {
        printf("CoAP POST socket open fail\r\n");
        vTaskDelete(NULL);
    }

    // demo: room1
    vTaskDelay(pdMS_TO_TICKS(1000));
    coap_post_room(&g_coap_post, "room1", "{ \"id\":\"room1\", \"status\":\"waiting\" }");
    vTaskDelay(pdMS_TO_TICKS(1000));
    coap_post_room(&g_coap_post, "room1", "{ \"id\":\"room1\", \"status\":\"approved\" }");
    vTaskDelay(pdMS_TO_TICKS(1000));
    coap_post_room(&g_coap_post, "room1", "{ \"id\":\"room1\", \"status\":\"done\" }");

    for(;;){
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
} */

static int coap_send_status(const char *status){
    char json[96];
    // ROOM là #define trong config.h, ví dụ: #define ROOM "room1"
    snprintf(json, sizeof(json), "{ \"id\":\"%s\", \"status\":\"%s\" }", ROOM, status);
    // endpoint: /room1  (server của bạn đang bắt POST /roomX)
    return coap_post_room(&g_coap_post, ROOM, json);
}
/* ===== Quét nút ===== */
static int btn_read_raw(void){
    // trả 1 khi nhấn, 0 khi thả – tuỳ wiring của bạn
    return (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET) ? 1 : 0;
}

void CoAP_Config(void){
    coap_client_begin(&g_coap_get, 2, COAP_SERVER_IP, COAP_SERVER_PORT);
    if(coap_client_open_socket(&g_coap_get) != 0){
        printf("CoAP GET socket open fail\r\n");
        //vTaskDelete(NULL);
        coap_get_en = 0;
    }else{
        coap_get_en = 1;
        printf("CoAP GET socket open ok\r\n");
    }

    /* POST client (socket 1) */
    coap_client_begin(&g_coap_post, 1, COAP_SERVER_IP, COAP_SERVER_PORT);
    if (coap_client_open_socket(&g_coap_post) != 0) {
        coap_post_en = 0;
        printf("CoAP POST socket open fail\r\n");
    } else {
        coap_post_en = 1;
        printf("CoAP POST socket open ok\r\n");
    }
    
}

/* Task gửi GET: hỏi /room?/status mỗi 1s */
void Task_CoAP_GET_Room(void *arg){
    (void)arg;
    char buf[128];
    for(;;){
        if(coap_get_en){
            int n = coap_get_status(&g_coap_get, ROOM, buf, sizeof(buf));
            if (n >= 0) {
                printf("GET /%s/status payload (%dB)\r\n", ROOM, n);
                if(n > 0){
                    handle_json_room_status(buf, ROOM);
                }
            } else {
                printf("CoAP GET error %d\r\n", n);
            }
        }else{
            printf("CoAP GET socket open fail\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void Task_ButtonSendWaiting(void *arg){
    (void)arg;
    const uint32_t DEBOUNCE_MS = 30;
    int stable = 0, last = 0, state = 0; // state: 0 thả, 1 nhấn

    for(;;){
        int sample = btn_read_raw();
        vTaskDelay(pdMS_TO_TICKS(10)); // debounce mỗi 10ms

        if(sample == last){
            stable += 10;
            if(stable >= DEBOUNCE_MS && sample != state){
                state = sample;
                if(state == 1){
                    printf("Button pressed → send waiting\r\n");
                    int r = coap_send_status("waiting");
                    printf("CoAP POST waiting ret=%d\r\n", r);
                }
            }
        } else {
            stable = 0;
        }
        last = sample;
    }
}