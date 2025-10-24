#include "var.h"
#include "config.h"
#include "defines.h"
#include "myprotocol.h"

#include "w5500.h"
#include "socket.h"
#include "wizchip_conf.h"

static inline void WIZ_CS_L(void){
    HAL_GPIO_WritePin(ETH_CS_GPIO_Port, ETH_CS_Pin, GPIO_PIN_RESET);
}

static inline void WIZ_CS_H(void){
    HAL_GPIO_WritePin(ETH_CS_GPIO_Port, ETH_CS_Pin, GPIO_PIN_SET);
}

//Read 1 byte
static uint8_t spi_rb(void){
    uint8_t tx = 0xFF, rx = 0xFF;
    HAL_SPI_TransmitReceive(&hspi1, &tx, &rx, 1, 10);
    return rx;
}

//Write 1 byte
static void spi_wb(uint8_t b){
    HAL_SPI_Transmit(&hspi1, &b, 1, 10);
}

//Read len byte
static void spi_rb_burst(uint8_t *buf, uint16_t len){
    for(uint16_t i = 0; i < len; i++){
        buf[i] = spi_rb();
    }
}

//Write len byte
static void spi_wb_burst(uint8_t *buf, uint16_t len){
    HAL_SPI_Transmit(&hspi1, buf, len, 20);
}

//Reset W5500
static void w5500_hw_reset(void){
    HAL_GPIO_WritePin(ETH_RST_GPIO_Port, ETH_RST_Pin, GPIO_PIN_RESET);
    HAL_Delay(15);
    HAL_GPIO_WritePin(ETH_RST_GPIO_Port, ETH_RST_Pin, GPIO_PIN_SET);
    HAL_Delay(50);
}

static wiz_NetInfo netInfo = {
    .mac = NET_MAC,
    .ip  = NET_IP,
    .sn  = NET_SN,
    .gw  = NET_GW,
    .dns = NET_DNS,
    .dhcp= NETINFO_STATIC
};

/* =================== Small helpers =================== */
static void print_phy(uint8_t phy){
    // PHYCFGR bit meaning (datasheet): LINK[0], SPD[1], DPX[2], OPMODE[6:3], OPMDC[5:3]
    const char *spd = (phy & (1<<1)) ? "100M" : "10M";
    const char *dpx = (phy & (1<<2)) ? "Full" : "Half";
    const char *lnk = (phy & (1<<0)) ? "UP" : "DOWN";
    log_i("PHYCFGR=0x%02X (LINK:%s SPD:%s DUP:%s)\r\n", phy, lnk, spd, dpx);
}

static int wait_link_up(uint32_t timeout_ms){
    uint32_t t0 = HAL_GetTick();
    for(;;){
        uint8_t phy = getPHYCFGR();
        if (phy & 0x01) {          // PHYLNK = 1
            print_phy(phy);
            return 0;
        }
        if (HAL_GetTick() - t0 > timeout_ms) {
            print_phy(getPHYCFGR());
            return -1;
        }
        HAL_Delay(50);
    }
}


/* =================== Public API =================== */
/* 1) Đăng ký SPI callbacks + init W5500 */
int w5500_init_spi_and_chip(void){
    // SPI init của bạn
    SPI1_Config();

    // đảm bảo CS HIGH trước khi nói chuyện
    HAL_GPIO_WritePin(ETH_CS_GPIO_Port, ETH_CS_Pin, GPIO_PIN_SET);

    // callback cho ioLibrary
    reg_wizchip_cs_cbfunc(WIZ_CS_L, WIZ_CS_H);
    reg_wizchip_spi_cbfunc(spi_rb, spi_wb);
    reg_wizchip_spiburst_cbfunc(spi_rb_burst, spi_wb_burst);

    // reset phần cứng
    w5500_hw_reset();

    // phân bổ RAM: 2KB cho mỗi socket (8 socket)
    uint8_t txrx[2][8] = {
        {2,2,2,2,2,2,2,2},  // TX
        {2,2,2,2,2,2,2,2}   // RX
    };
    if (wizchip_init(txrx[0], txrx[1]) != 0){
        log_i("wizchip_init fail\r\n");
        return -1;
    }

    // Tăng timeout & retry mặc định cho TCP
    setRTR(2000);   // 2000 ms mỗi retry
    setRCR(3);      // retry 3 lần

    // Đặt NetInfo
    wizchip_setnetinfo(&netInfo);

    // kiểm tra version register
    uint8_t ver = getVERSIONR();
    log_i("W5500 VERSIONR=0x%02X (expect 0x04)\r\n", ver);
    if (ver != 0x04){
        return -2;  // 0x04 là W5500
    }

    // Chờ link lên tối đa 5s
    if (wait_link_up(5000) != 0){
        log_i("PHY LINK DOWN (timeout)\r\n");
        return -3;
    }

    // In ra NetInfo thực tế
    w5500_get_netinfo();

    return 0;
}

int w5500_set_static_ip(void){
    wizchip_setnetinfo(&netInfo);
    return 0;
}

void w5500_get_netinfo(void){
    wiz_NetInfo ni;
    wizchip_getnetinfo(&ni);
    log_i("IP %d.%d.%d.%d GW %d.%d.%d.%d SN %d.%d.%d.%d \r\n", 
        ni.ip[0], ni.ip[1], ni.ip[2], ni.ip[3],
        ni.gw[0], ni.gw[1], ni.gw[2], ni.gw[3],
        ni.sn[0], ni.sn[1], ni.sn[2], ni.sn[3]);
}

/* Đóng socket an toàn */
void w5500_close_socket(uint8_t s){
    // thử graceful
    disconnect(s);
    HAL_Delay(10);
    close_wiz(s);
}