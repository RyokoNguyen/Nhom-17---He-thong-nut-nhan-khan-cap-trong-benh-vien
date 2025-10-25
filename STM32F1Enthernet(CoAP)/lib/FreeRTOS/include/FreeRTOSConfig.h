#ifndef FREERTOS_CONFIG_H
#define FREERTOS_CONFIG_H

#if defined(__ICCARM__) || defined(__CC_ARM) || defined(__GNUC__)
  #include <stdint.h>
  extern uint32_t SystemCoreClock;
  void xPortSysTickHandler(void);
#endif

/* ==== Kernel core ==== */
#define configUSE_PREEMPTION                 1
#define configUSE_IDLE_HOOK                  0
#define configUSE_TICK_HOOK                  0
#define configCPU_CLOCK_HZ                   ( SystemCoreClock )
#define configTICK_RATE_HZ                   ( ( TickType_t ) 1000 )

/* 128 word = 512 bytes stack tối thiểu */
#define configMINIMAL_STACK_SIZE             ( ( uint16_t ) 128 )

/* Heap cho FreeRTOS: 9KB (chọn heap_4.c trong project) */
#define configTOTAL_HEAP_SIZE                ( ( size_t ) ( 9 * 1024 ) )

#define configMAX_PRIORITIES                 ( 5 )
#define configMAX_TASK_NAME_LEN              ( 16 )
#define configUSE_16_BIT_TICKS               0
#define configIDLE_SHOULD_YIELD              1

/* ==== Debug / Safety ==== */
#define configUSE_TRACE_FACILITY             0
#define configCHECK_FOR_STACK_OVERFLOW       0      /* dùng phương án 2: bắt tràn tốt hơn */
#define configUSE_MALLOC_FAILED_HOOK         0      //1
#define configASSERT( x ) if( ( x ) == 0 ) { taskDISABLE_INTERRUPTS(); for( ;; ); }

/* ==== Sync primitives ==== */
#define configUSE_MUTEXES                    1
#define configUSE_RECURSIVE_MUTEXES          1
#define configUSE_COUNTING_SEMAPHORES        1
#define configQUEUE_REGISTRY_SIZE            8

/* ==== Software Timers ==== */
#define configUSE_TIMERS                     0
#define configTIMER_TASK_PRIORITY            2
#define configTIMER_QUEUE_LENGTH             6
#define configTIMER_TASK_STACK_DEPTH         ( configMINIMAL_STACK_SIZE * 2 )  /* 1KB */

/* ==== API include ==== */
#define INCLUDE_vTaskPrioritySet             1
#define INCLUDE_uxTaskPriorityGet            1
#define INCLUDE_vTaskDelete                  1
#define INCLUDE_vTaskSuspend                 1
#define INCLUDE_vTaskDelay                   1
#define INCLUDE_vTaskDelayUntil              1
#define INCLUDE_xTaskGetSchedulerState       1
#define INCLUDE_xTaskGetTickCount            1
#define INCLUDE_vTaskCleanUpResources        0

/* ==== Cortex-M (STM32F1: NVIC 4 bit) ==== */
#ifdef __NVIC_PRIO_BITS
  #define configPRIO_BITS                    __NVIC_PRIO_BITS
#else
  #define configPRIO_BITS                    4   /* 16 mức ưu tiên */
#endif

/* Thư viện ưu tiên (0 = cao nhất, 15 = thấp nhất) */
#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5

/* Giá trị dịch trái dùng bởi port layer */
#define configKERNEL_INTERRUPT_PRIORITY \
  ( configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
  ( configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8 - configPRIO_BITS) )

/* Map handlers chuẩn CMSIS */
#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler

/* QUAN TRỌNG: 
   Nếu HAL vẫn dùng SysTick làm timebase thì GIỮ NGUYÊN dòng dưới ở trạng thái COMMENT.
   CubeMX thường gọi osSystickHandler() trong SysTick_Handler.
   Nếu bạn MUỐN FreeRTOS nắm SysTick, hãy UNCOMMENT dòng dưới và đảm bảo HAL timebase chuyển sang TIM khác. */
/* #define xPortSysTickHandler SysTick_Handler */

#endif /* FREERTOS_CONFIG_H */