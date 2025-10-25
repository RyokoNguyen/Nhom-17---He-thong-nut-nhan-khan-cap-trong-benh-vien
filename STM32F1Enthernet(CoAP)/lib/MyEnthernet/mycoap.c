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

/* FreeRTOS */
#include "FreeRTOS.h"
#include "task.h"

/* ---------- cấu hình ---------- */
static const uint8_t COAP_SERVER_IP[4] = {192,168,10,1};    // IP Node-RED
// 2 socket riêng: 1 cho POST, 1 cho GET
static coap_client_t g_coap_post;
static coap_client_t g_coap_get;

/* volatile uint8_t coap_get_en  = 0;
volatile uint8_t coap_post_en = 0; */

/* ---------- helpers ---------- */

// encoder option rất gọn (delta < 13, len < 13)
static uint8_t* coap_write_option_short(uint8_t *p, uint16_t *last_no,
                                        uint16_t opt_no, const uint8_t *val, uint16_t len)
{
    uint16_t delta = (uint16_t)(opt_no - *last_no);
    *last_no = opt_no;
    *p++ = (uint8_t)((delta << 4) | (len & 0x0F));
    if (len && val) { memcpy(p, val, len); p += len; }
    return p;
}

static const uint8_t* coap_find_payload(const uint8_t *pkt, int pkt_len, int *len_out){
    if (len_out) *len_out = 0;
    if (!pkt || pkt_len < 5) return NULL;
    for (int i = 4; i < pkt_len; ++i) {
        if (pkt[i] == 0xFF) {
            int n = pkt_len - (i + 1);
            if (len_out) *len_out = (n > 0) ? n : 0;
            return (n > 0) ? &pkt[i+1] : NULL;
        }
    }
    return NULL;
}

static void coap_debug_print_rx(const uint8_t *rb, int rl) {
    if (rl < 4) { printf("[CoAP] RX too short (%d)\r\n", rl); return; }
    uint8_t ver  = (rb[0] >> 6) & 3;
    uint8_t type = (rb[0] >> 4) & 3;
    uint8_t tkl  =  rb[0] & 0x0F;
    uint8_t code =  rb[1];
    uint16_t mid = (rb[2]<<8)|rb[3];
    printf("[CoAP] RX: ver=%u type=%u tkl=%u code=%u.%02u MID=0x%04X len=%d\r\n",
           ver, type, tkl, (code>>5), (code&0x1F), mid, rl);
}

/* ---------- mini client ---------- */

void coap_client_begin(coap_client_t *c, uint8_t sock, const uint8_t ip[4], uint16_t port){
    memset(c, 0, sizeof(*c));
    c->sock = sock;
    memcpy(c->remote_ip, ip, 4);
    c->remote_port = port;
    c->next_mid = 1;
    c->rx_timeout_ms = 1000; // 1s
}

int coap_client_open_socket(coap_client_t *c){
    uint8_t sr = getSn_SR(c->sock);
    if (sr == SOCK_CLOSED) {
        if (socket(c->sock, Sn_MR_UDP, 0, 0) != c->sock) return -1;
    }
    return 0;
}

/* ---------- POST /{room} với payload JSON ---------- */
int coap_post_room(coap_client_t *c, const char *room, const char *json){
    if (!room || !json) return -1;
    if (coap_client_open_socket(c) != 0) return -2;

    uint8_t pkt[300], *p = pkt;

    // Header: CON + POST
    *p++ = (uint8_t)((COAP_VER << 6) | (COAP_TYPE_NON << 4) | COAP_TKL_ZERO);
    *p++ = COAP_CODE_POST;
    uint16_t mid = c->next_mid++;
    *p++ = (mid >> 8) & 0xFF;
    *p++ = (mid     ) & 0xFF;

    // Options: Uri-Path=room, Content-Format=application/json(50)
    uint16_t last_opt = 0;
    uint16_t room_len = (uint16_t)strlen(room);
    if (room_len >= 13) return -3;

    p = coap_write_option_short(p, &last_opt, COAP_OPT_URI_PATH,
                                (const uint8_t*)room, room_len);
    uint8_t cfv[2] = { 0x00, COAP_CF_APP_JSON }; // 50
    p = coap_write_option_short(p, &last_opt, COAP_OPT_CONTENT_FORMAT, cfv, 2);

    // Payload
    *p++ = 0xFF;
    uint16_t jlen = (uint16_t)strlen(json);
    if (jlen > (sizeof(pkt) - (size_t)(p - pkt))) return -4;
    memcpy(p, json, jlen); p += jlen;

    int send_len = (int)(p - pkt);
    int r = sendto(c->sock, pkt, send_len, (uint8_t*)c->remote_ip, c->remote_port);
    if (r != send_len) return -5;

 
// Chờ ACK/RESP và log
    uint32_t t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < 200 /*ms*/) {
        uint16_t rsr = getSn_RX_RSR(c->sock);
        if (rsr) {
            uint8_t rip[4]; uint16_t rp; uint8_t rb[128];
            int rl = recvfrom(c->sock, rb, (rsr > sizeof(rb)) ? sizeof(rb) : rsr, rip, &rp);
            (void)rl; break;
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // tránh “treo”
    }

    return 0;
}

/* ---------- GET /{room}/status (+Accept: json) ---------- */
int coap_get_status(coap_client_t *c, const char *room, char *out_payload, int out_len){
    if (!room) return -1;
    if (coap_client_open_socket(c) != 0) return -2;

    uint8_t pkt[256], *p = pkt;

    *p++ = (uint8_t)((COAP_VER << 6) | (COAP_TYPE_CON << 4) | COAP_TKL_ZERO);
    *p++ = COAP_CODE_GET;
    uint16_t mid = c->next_mid++;
    *p++ = (mid >> 8) & 0xFF;
    *p++ = (mid     ) & 0xFF;

    uint16_t last_opt = 0;
    uint16_t room_len = (uint16_t)strlen(room);
    if (room_len >= 13) return -3;

    p = coap_write_option_short(p, &last_opt, COAP_OPT_URI_PATH,
                                (const uint8_t*)room, room_len);
    p = coap_write_option_short(p, &last_opt, COAP_OPT_URI_PATH,
                                (const uint8_t*)"status", 6);

    // Accept: application/json (option 17, value 50) – 1 byte gọn
    uint8_t accv = COAP_CF_APP_JSON;      // 50
    p = coap_write_option_short(p, &last_opt, 17, &accv, 1);

    int send_len = (int)(p - pkt);
    int r = sendto(c->sock, pkt, send_len, (uint8_t*)c->remote_ip, c->remote_port);
    if (r != send_len) return -4;

    // chờ resp
    uint32_t t0 = HAL_GetTick();
    while ((HAL_GetTick() - t0) < c->rx_timeout_ms) {
        uint16_t rsr = getSn_RX_RSR(c->sock);
        if (!rsr) { HAL_Delay(10); continue; }

        uint8_t rip[4]; uint16_t rp; uint8_t rb[300];
        int rl = recvfrom(c->sock, rb, (rsr > sizeof(rb)) ? sizeof(rb) : rsr, rip, &rp);
        if (rl <= 0) continue;

        coap_debug_print_rx(rb, rl);

        int pay_len = 0;
        const uint8_t *pay = coap_find_payload(rb, rl, &pay_len);
        if (out_payload && out_len > 0) {
            int ncopy = (pay && pay_len > 0) ? pay_len : 0;
            if (ncopy >= out_len) ncopy = out_len - 1;
            if (ncopy > 0) memcpy(out_payload, pay, ncopy);
            out_payload[(ncopy > 0) ? ncopy : 0] = 0;
            return ncopy;
        }
        return (pay && pay_len > 0) ? pay_len : 0;
    }
    return -5; // timeout
}

/* ---------- phần “ứng dụng” ---------- */

static int coap_send_status(const char *status){
    if (!coap_post_en) return -99;
    char json[96];
    snprintf(json, sizeof(json), "{ \"id\":\"%s\", \"status\":\"%s\" }", ROOM, status);
    printf("POST /%s payload: %s\r\n", ROOM, json);
    return coap_post_room(&g_coap_post, ROOM, json);
}

/* nút nhấn: trả 1 khi nhấn, 0 khi thả (tùy wiring) */
static int btn_read_raw(void){
    return (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET) ? 1 : 0;
}

/* Khởi tạo 2 client (GET/POST) */
void CoAP_Config(void){
    // GET
    coap_client_begin(&g_coap_get, 2, COAP_SERVER_IP, COAP_SERVER_PORT);
    if (coap_client_open_socket(&g_coap_get) != 0) {
        coap_get_en = 0; printf("CoAP GET socket open fail\r\n");
    } else {
        coap_get_en = 1; printf("CoAP GET socket open ok\r\n");
    }
    // POST
    coap_client_begin(&g_coap_post, 1, COAP_SERVER_IP, COAP_SERVER_PORT);
    if (coap_client_open_socket(&g_coap_post) != 0) {
        coap_post_en = 0; printf("CoAP POST socket open fail\r\n");
    } else {
        coap_post_en = 1; printf("CoAP POST socket open ok\r\n");
    }
}

/* Task GET: hỏi /{ROOM}/status mỗi 1s và xử lý JSON */
void Task_CoAP_GET_Room(void *arg){
    (void)arg;
    char buf[128];
    for(;;){
        if (coap_get_en) {
            int n = coap_get_status(&g_coap_get, ROOM, buf, sizeof(buf));
            if (n >= 0) {
                printf("GET /%s/status payload (%dB)\r\n", ROOM, n);
                if (n > 0) handle_json_room_status(buf, ROOM);
            } else {
                printf("CoAP GET error %d\r\n", n);
            }
        } else {
            printf("CoAP GET socket not ready\r\n");
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

/* Task nút nhấn → gửi waiting */
void Task_ButtonSendWaiting(void *arg){
    (void)arg;
    const uint32_t DEBOUNCE_MS = 30;
    int stable = 0, last = 0, state = 0;

    for(;;){
        int sample = btn_read_raw();
        vTaskDelay(pdMS_TO_TICKS(10));
        if (sample == last){
            stable += 10;
            if (stable >= DEBOUNCE_MS && sample != state){
                state = sample;
                if (state == 1){
                    printf("Button pressed → send waiting\r\n");
                    int r = coap_send_status("waiting");
                    printf("CoAP POST waiting ret=%d\r\n", r);
                }
            }
        } else stable = 0;
        last = sample;
    }
}