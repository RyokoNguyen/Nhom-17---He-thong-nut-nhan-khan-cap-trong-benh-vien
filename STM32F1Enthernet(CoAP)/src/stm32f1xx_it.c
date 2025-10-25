
#include "main.h"
#include "stm32f1xx_it.h"

#include "FreeRTOS.h"
#include "task.h"


void NMI_Handler(void){
  while(1){

  }
}

void HardFault_Handler(void){
  while(1){

  }
}

void MemManage_Handler(void){
  while(1){

  }
}

void BusFault_Handler(void){
  while (1){

  }
}

void UsageFault_Handler(void){
  while (1){

  }
}

/* void SVC_Handler(void){

} */

void DebugMon_Handler(void){

}


/* void PendSV_Handler(void){

}
 */
void SysTick_Handler(void){
  HAL_IncTick();
  #if(INCLUDE_xTaskGetSchedulerState == 1)
    if(xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED){
  #endif
      xPortSysTickHandler();
  #if(INCLUDE_xTaskGetSchedulerState == 1)
    }
  #endif
}

//UART1
/* void DMA1_Channel5_IRQHandler(void){
  HAL_DMA_IRQHandler(&hdma_usart1_rx);
}

void USART1_IRQHandler(void){
  HAL_UART_IRQHandler(&huart1);
} */

//YOUR CODE

