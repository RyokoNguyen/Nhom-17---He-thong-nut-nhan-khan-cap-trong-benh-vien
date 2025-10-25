#include "main.h"


int main(void){
  HAL_Init();
  SystemClock_Config(); //Thach anh (Xung nhip chip) (72MHz)
  GPIO_Output_Config(); //Setup Output
  GPIO_Input_Config();  //Setup Input
  LOG_Config();
  HAL_Delay(3000);

  USART2_Config(9600);
  int err = w5500_init_spi_and_chip();

  if(err != 0){
    log_i("Enthenet fail: %d", err);
    while(1){;}
  }
  w5500_set_static_ip();

  CoAP_Config();

  if(xTaskCreate(Task_CoAP_GET_Room, "Task_CoAP_Room", 384, NULL, 1, NULL) != pdPASS){
    Error_Handler();
  }

  if(xTaskCreate(Task_ButtonSendWaiting, "BTN_POST", 256, NULL, 1, NULL) != pdPASS){
    Error_Handler();
  }


  vTaskStartScheduler();
  Error_Handler();
}
