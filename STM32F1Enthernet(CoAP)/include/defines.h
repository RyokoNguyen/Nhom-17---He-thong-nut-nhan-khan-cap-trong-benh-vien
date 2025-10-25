#ifndef DEFINES_H
#define DEFINES_H

#include "stm32f1xx_hal.h"

#include <stdio.h>
#include <string.h>

#define DELAY_TIM_FREQUENCY_US                      1000000

#define MILLI_R                                     (R * 1000)
#define MILLI_V                                     (V * 1000)
#define MILLI_PSI                                   (PSI * 1000)

#define NO                                          0
#define YES                                         1
#define ABS(a)                                      (((a) < 0) ? -(a) : (a))
#define LIMIT(x, lowhigh)                           (((x) > (lowhigh)) ? (lowhigh) : (((x) < (-lowhigh)) ? (-lowhigh) : (x)))
#define SAT(x, lowhigh)                             (((x) > (lowhigh)) ? (1.0f) : (((x) < (-lowhigh)) ? (-1.0f) : (0.0f)))
#define SAT2(x, low, high)                          (((x) > (high)) ? (1.0f) : (((x) < (low)) ? (-1.0f) : (0.0f)))
#define STEP(from, to, step)                        (((from) < (to)) ? (MIN((from) + (step), (to))) : (MAX((from) - (step), (to))))
#define DEG(a)                                      ((a)*M_PI / 180.0f)
#define RAD(a)                                      ((a)*180.0f / M_PI)
#define SIGN(a)                                     (((a) < 0) ? (-1) : (((a) > 0) ? (1) : (0)))
#define CLAMP(x, low, high)                         (((x) > (high)) ? (high) : (((x) < (low)) ? (low) : (x)))
#define IN_RANGE(x, low, high)                      ((x) >= (low)) && ((x) <= (high))
#define SCALE(value, high, max)                     MIN(MAX(((max) - (value)) / ((max) - (high)), 0.0f), 1.0f)
#define MIN(a, b)                                   (((a) < (b)) ? (a) : (b))
#define MAX(a, b)                                   (((a) > (b)) ? (a) : (b))
#define MIN3(a, b, c)                               MIN(a, MIN(b, c))
#define MAX3(a, b, c)                               MAX(a, MAX(b, c))
#define ARRAY_LEN(x)                                (uint32_t)(sizeof(x) / sizeof(*(x)))
#define MAP(x, in_min, in_max, out_min, out_max)    (((((x) - (in_min)) * ((out_max) - (out_min))) / ((in_max) - (in_min))) + (out_min))
 
#if defined(PRINTF_FLOAT_SUPPORT) && defined(SERIAL_DEBUG) && defined(__GNUC__)
   asm(".global _printf_float");     // this is the magic trick for printf to support float. Warning: It will increase code considerably! Better to avoid!
#endif

#define log_i                                       printf
#define DEBUG_BUFFER_SIZE                           64

#define APB1_TIMER_CLOCKS                           36000000
#define APB2_TIMER_CLOCKS                           72000000

#define TIM1_FREQUENCE                              1000                   //1kHz
#define TIM1_RESOLUTION                             100                    //
#define TIM1_PSC_APB                                ((APB1_TIMER_CLOCKS/TIM1_FREQUENCE)/TIM1_RESOLUTION - 1)                                    

//I/O
//MP3
#define MP3_TX_Pin                                 GPIO_PIN_2
#define MP3_TX_GPIO_Port                           GPIOA
#define MP3_RX_Pin                                 GPIO_PIN_3
#define MP3_RX_GPIO_Port                           GPIOA

//RFID
#define RFID_TX_Pin                                GPIO_PIN_10
#define RFID_TX_GPIO_Port                          GPIOB
#define RFID_RX_Pin                                GPIO_PIN_11
#define RFID_RX_GPIO_Port                          GPIOB

//LIGHT
#define LIGHT_Pin                                  GPIO_PIN_14
#define LIGHT_GPIO_Port                            GPIOB

//BTN
#define BTN_Pin                                    GPIO_PIN_15
#define BTN_GPIO_Port                              GPIOB

//ENTHERNET
#define ETH_RST_Pin                                GPIO_PIN_12
#define ETH_RST_GPIO_Port                          GPIOA
#define ETH_CS_Pin                                 GPIO_PIN_15
#define ETH_CS_GPIO_Port                           GPIOA
#define ETH_SCK_Pin                                GPIO_PIN_3
#define ETH_SCK_GPIO_Port                          GPIOB
#define ETH_MISO_Pin                               GPIO_PIN_4
#define ETH_MISO_GPIO_Port                         GPIOB
#define ETH_MOSI_Pin                               GPIO_PIN_5
#define ETH_MOSI_GPIO_Port                         GPIOB
#define ETH_INT_Pin                                GPIO_PIN_6
#define ETH_INT_GPIO_Port                          GPIOB

#define LIGHT_ON                                   HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, 1)
#define LIGHT_OFF                                  HAL_GPIO_WritePin(LIGHT_GPIO_Port, LIGHT_Pin, 0)

//ROOM
#define ROOM                                       "room2"                                //Thay tên phòng cho đúng


// MQTT (PC làm broker)
#define MQTT_SOCK                                  1                                      
#define MQTT_BROKER_IP                             {192, 168, 10, 1}
#define MQTT_BROKER_PORT                           1883
#define MQTT_CLIENT_ID                             "stm32f1-w5500-room2"                  //Thay ID
#define MQTT_KEEPALIVE                             60

// Net static (W5500)
#define NET_MAC                                    {0x00, 0x08, 0xDC, 0x11, 0x22, 0x33}   //Thay MAC chỗ này để ko trùng với bo khác
#define NET_IP                                     {192, 168,  10, 4}                     //Thay IP chỗ này để ko trùng với bo khác
#define NET_SN                                     {255, 255, 255, 0}
#define NET_GW                                     {192, 168, 10, 1}
#define NET_DNS                                    {8, 8, 8, 8}

/* ===== CoAP constants (RFC7252, rút gọn) ===== */
#define COAP_SERVER_PORT                           5683
#define COAP_VER                                   1
#define COAP_TYPE_CON                              0     // Confirmable
#define COAP_TKL_ZERO                              0     // không dùng Token cho gọn
#define COAP_TYPE_NON                              1


#define COAP_CODE_GET                              0x01
#define COAP_CODE_POST                             0x02

#define COAP_OPT_URI_PATH                          11
#define COAP_OPT_CONTENT_FORMAT                    12
#define COAP_OPT_URI_QUERY                         15
/* IANA Content-Format for application/json = 50 */
#define COAP_CF_APP_JSON                           50

//
typedef int8_t s8;
typedef __IO int8_t vs8;
typedef __I int8_t vsc8;      /*!< Read Only */
typedef const int8_t sc8;     /*!< Read Only */

typedef uint8_t u8;
typedef __IO uint8_t vu8;
typedef __I uint8_t vuc8;     /*!< Read Only */
typedef const uint8_t uc8;    /*!< Read Only */

typedef int16_t s16;
typedef __IO int16_t vs16;
typedef __I int16_t vsc16;    /*!< Read Only */
typedef const int16_t sc16;   /*!< Read Only */

typedef uint16_t u16;
typedef __IO uint16_t vu16;
typedef __I uint16_t vuc16;   /*!< Read Only */
typedef const uint16_t uc16;  /*!< Read Only */

typedef int32_t s32;
typedef __IO int32_t vs32;
typedef __I int32_t vsc32;    /*!< Read Only */
typedef const int32_t sc32;   /*!< Read Only */

typedef uint32_t  u32;
typedef __IO uint32_t vu32;
typedef __I uint32_t vuc32;   /*!< Read Only */
typedef const uint32_t uc32;  /*!< Read Only */

SPI_HandleTypeDef hspi1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;
UART_HandleTypeDef huart3;

typedef struct {
   uint8_t     remote_ip[4];   // ví dụ 192.168.10.1 (Node-RED)
   uint16_t    remote_port;    // thường 5683
   uint8_t     sock;           // số socket W5500 dùng cho CoAP
   uint16_t    next_mid;       // message id tăng dần
   uint16_t    rx_timeout_ms;  // timeout nhận phản hồi
} coap_client_t;

#endif