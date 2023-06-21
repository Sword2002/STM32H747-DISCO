/*
 * STM32H747I-DISCO study
 * Copyright (C) 2023 Schneider-Electric.
 * All Rights Reserved.
 *
 */
#include "FreeRTOS.h"	// FreeRTOS	  
#include "task.h"		// FreeRTOS task
#include "queue.h"
#include "stm32h747i_discovery.h"
#include "shell_port.h"
#include "usart.h"
#include "stdio.h"

#define TENMS_DELAY   pdMS_TO_TICKS(10) // 发送等待延时为10ms
extern QueueHandle_t xQueue_PB;
extern TaskHandle_t PrintfTaskHandle;
extern volatile uint32_t u32CpuRunTime;

// OS task, Periodic 10ms
void RTOS_Task10ms(void)
{
    unsigned int taskCnt = 0;
    char strBuff[32] = {0};
    uint32_t buffAddr = (uint32_t)strBuff;

    TickType_t pxPreviousWakeTime = xTaskGetTickCount();
    

    while (1)
    {
        if ((++taskCnt % 100) == 0) {  // 10*100 = 1000ms
            BSP_LED_Toggle(LED2);

            if (0 != ulTaskNotifyTake(pdFALSE, 0)) {                                   // 收到任务通知，转给打印任务
                sprintf(strBuff, "\r\n10ms 任务收到了LED任务的通知.\r\n");
                xTaskNotify(PrintfTaskHandle, buffAddr, eSetValueWithoutOverwrite);    // 使用任务通知替代消息队列
            } else {
                //sprintf(strBuff, "10ms Task Counter = %d\r\n", taskCnt);
                sprintf(strBuff, "TIM2 Counter = %ld\r\n", u32CpuRunTime);
                //xQueueSend(xQueue_PB, &buffAddr, 0);            // 消息的内容是buff的地址,即消息的引用
            }
        }
 
        //vTaskDelay(TENMS_DELAY);                              // 任务阻塞10ms
        vTaskDelayUntil(&pxPreviousWakeTime, TENMS_DELAY);      // 固定周期10ms
    }
}

