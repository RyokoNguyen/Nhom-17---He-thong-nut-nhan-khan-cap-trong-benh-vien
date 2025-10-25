
#include "var.h"
#include "mymp3.h"
#include "config.h"
#include "defines.h"

#include <string.h>


#define DFP_START           0x7E
#define DFP_VER             0xFF
#define DFP_LEN             0x06
#define DFP_END             0xEF

// Command codes
#define CMD_PLAY_FOLDER     0x0F
#define CMD_STOP            0x16
#define CMD_PAUSE           0x0E
#define CMD_RESUME          0x0D
#define CMD_VOLUME          0x06
#define CMD_FEEDBACK        0x41
#define CMD_RESET           0x0C

// Response types
#define RSP_PLAY_FINISH     0x3D
#define RSP_ERROR           0x40
#define RSP_FEEDBACK        0x41

static struct {
    volatile uint8_t rxbuf[64];
    volatile uint16_t head, tail;
    bool playing;
    uint32_t expect_ms;
    uint32_t start_tick;
} s;

static void uart_send(const uint8_t *d, uint16_t n) {
    HAL_UART_Transmit(&huart2, (uint8_t*)d, n, 100);
}

static void build_and_send(uint8_t cmd, uint16_t param, uint8_t needAck) {
    uint8_t f[10];
    f[0] = DFP_START;
    f[1] = DFP_VER;
    f[2] = DFP_LEN;
    f[3] = cmd;
    f[4] = needAck ? 1 : 0;
    f[5] = (param >> 8) & 0xFF;
    f[6] = (param     ) & 0xFF;

    uint16_t sum = DFP_VER + DFP_LEN + cmd + f[4] + f[5] + f[6];
    uint16_t cs = 0 - sum;
    f[7] = (cs >> 8) & 0xFF;
    f[8] = cs & 0xFF;
    f[9] = DFP_END;

    log_i("[DFP] TX cmd=0x%02X, param=0x%04X, checksum=0x%04X\r\n", cmd, param, cs);
    uart_send(f, sizeof(f));
}

void DF_Config(void) {
    s.head = s.tail = 0;
    s.playing = false;
    s.expect_ms = 0;

    log_i("[DFP] Init... enabling feedback & setting default volume\r\n");

    // Bật feedback (để DFPlayer trả về ACK)
    build_and_send(CMD_FEEDBACK, 1, 0);
    HAL_Delay(20);

    dfp_set_volume(20); // volume mặc định
    log_i("[DFP] Ready\r\n");
}

void dfp_set_volume(uint8_t vol) {
    if (vol > 30) vol = 30;
    build_and_send(CMD_VOLUME, vol, 1);
    log_i("[DFP] Set volume: %d\r\n", vol);
}

bool dfp_play_folder_file(uint8_t folder, uint8_t file) {
    if (folder == 0 || file == 0 || folder > 99 || file > 99) return false;

    uint16_t param = ((uint16_t)folder << 8) | file;
    build_and_send(CMD_PLAY_FOLDER, param, 1);

    s.playing = true;
    s.start_tick = HAL_GetTick();

    log_i("[DFP] Playing: folder %02d, file %02d\r\n", folder, file);
    return true;
}

bool dfp_stop(void) {
    build_and_send(CMD_STOP, 0, 1);
    s.playing = false;
    log_i("[DFP] Stop\r\n");
    return true;
}

bool dfp_pause(void) {
    build_and_send(CMD_PAUSE, 0, 1);
    s.playing = false;
    log_i("[DFP] Pause\r\n");
    return true;
}

bool dfp_resume(void) {
    build_and_send(CMD_RESUME, 0, 1);
    s.playing = true;
    log_i("[DFP] Resume\r\n");
    return true;
}

bool dfp_is_playing(void) {
    if (s.playing && s.expect_ms > 0) {
        if (HAL_GetTick() - s.start_tick > s.expect_ms + 300) {
            log_i("[DFP] Timeout reached, marking as stopped\r\n");
            s.playing = false;
        }
    }
    return s.playing;
}

void dfp_set_expect_ms(uint32_t ms) {
    s.expect_ms = ms;
    log_i("[DFP] Expected playback time: %lu ms\r\n", (unsigned long)ms);
}

// === UART RX BUFFER ===
void dfp_uart_rx_isr_byte(uint8_t b) {
    uint16_t nxt = (s.head + 1) % sizeof(s.rxbuf);
    if (nxt != s.tail) { s.rxbuf[s.head] = b; s.head = nxt; }
}

static int rb_available(void){ return (s.head - s.tail + sizeof(s.rxbuf)) % sizeof(s.rxbuf); }
static uint8_t rb_get(void){ uint8_t c = s.rxbuf[s.tail]; s.tail = (s.tail+1)%sizeof(s.rxbuf); return c; }

static bool try_parse_one(void) {
    if (rb_available() < 10) return false;
    while (rb_available() && s.rxbuf[s.tail] != DFP_START) rb_get();
    if (rb_available() < 10) return false;

    uint8_t f[10];
    for (int i=0; i<10; i++) f[i] = rb_get();

    if (!(f[0]==DFP_START && f[9]==DFP_END)) return true;

    uint8_t cmd = f[3];
    uint16_t par = (f[5]<<8)|f[6];
    log_i("[DFP] RX cmd=0x%02X param=0x%04X\r\n", cmd, par);

    switch (cmd) {
        case RSP_PLAY_FINISH:
            log_i("[DFP] Track finished\r\n");
            s.playing = false;
            break;
        case RSP_ERROR:
            log_i("[DFP] Error code 0x%04X\r\n", par);
            s.playing = false;
            break;
        default:
            log_i("[DFP] Ack cmd=0x%02X\r\n", cmd);
            break;
    }
    return true;
}

void dfp_poll(void) {
    while (try_parse_one()) {;}
}