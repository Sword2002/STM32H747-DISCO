/*
 * STM32H747I-DISCO study
 * Copyright (C) 2023 Schneider-Electric.
 * All Rights Reserved.
 *
 */
#include <string.h>
#include "FreeRTOS.h"	         // FreeRTOS	  
#include "task.h"                // FreeRTOS task
#include "semphr.h"		         // FreeRTOS semaphore
#include "event_groups.h"		 // FreeRTOS event

#include "stm32h747i_discovery.h"
#include "usart.h"

#define LED_DELAY   pdMS_TO_TICKS(300) // 发送等待延时为200ms

static void AppTaskCreate(void);           /* 用于创建任务 */ 
static void LED_Task(void);                /* LED_Task任务实现 */


/**************************** 任务句柄 ********************************/
/* 
 * 任务句柄是一个指针，用于指向一个任务，当任务创建好之后，它就具有了一个任务句柄
 * 以后我们要想操作这个任务都需要通过这个任务句柄，如果是自身的任务操作自己，那么
 * 这个句柄可以为NULL。
 */
TaskHandle_t AppTaskCreateHandle;  // 创建任务句柄
TaskHandle_t LEDsTaskHandle;	      // LED任务句柄


/********************************** 内核对象句柄 *********************************/
/*
 * 信号量，消息队列，事件标志组，软件定时器这些都属于内核的对象，要想使用这些内核
 * 对象，必须先创建，创建成功之后会返回一个相应的句柄。实际上就是一个指针，后续我
 * 们就可以通过这个句柄操作这些内核对象。
 *
 * 内核对象说白了就是一种全局的数据结构，通过这些数据结构我们可以实现任务间的通信，
 * 任务间的事件同步等各种功能。至于这些功能的实现我们是通过调用这些内核对象的函数
 * 来完成的
 * 
 */
 
/******************************* 全局变量声明 ************************************/
 
/**
* @brief  Handles the tick increment
* @param  none.
* @retval none.
*/
extern void xPortSysTickHandler(void);
void osSystickHandler(void)
{

#if (INCLUDE_xTaskGetSchedulerState  == 1 )
  if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
  {
#endif  /* INCLUDE_xTaskGetSchedulerState */  
    xPortSysTickHandler();
#if (INCLUDE_xTaskGetSchedulerState  == 1 )
  }
#endif  /* INCLUDE_xTaskGetSchedulerState */  
}

/**********************************************************************
  * @ 函数名  ： LED_Task
  * @ 功能说明： LED_Task任务主体
  * @ 参数    ：   
  * @ 返回值  ： 无
  ********************************************************************/
static void LED_Task(void)
{	
    uint8_t strBuff[64] = {0};
    strcpy(strBuff, "Uart8 init success!\r\n");
    //(void)Uart_bSend_NonBlocking(&huart8, (uint8_t *)strBuff, strlen(strBuff));
    HAL_UART_Transmit(&huart8, (uint8_t *)strBuff, strlen(strBuff), 10);
    while (1)
    {
        BSP_LED_Toggle(LED3);      
        vTaskDelay(LED_DELAY);        // 
    }
}

/*****************************************************************
  * @brief  任务创建管理函数
  * @param  无
  * @retval 无
  * @note   实现RTOS的所有任务创建，创建完成后删除本任务
  ****************************************************************/
void osTaskInit(void)
{	
    BaseType_t result = pdFAIL;
    result = xTaskCreate( (TaskFunction_t )AppTaskCreate,         //任务函数
                          (const char* 	  )"AppTaskCreate",       //任务名称
                          (uint16_t 	  )128,	                  //任务堆栈大小
                          (void* 		  )NULL,				  //传递参数
                          (UBaseType_t 	  )3, 	                  //任务优先级
                          (TaskHandle_t*  )&AppTaskCreateHandle); //任务句柄   
															
	  if(pdPASS == result) {
        //printf("Task Start Success\r\n");
        vTaskStartScheduler();   // 启动任务，开启调度
    }
  
    while(1) {;} // 正常不会执行到这里
}

/***********************************************************************
  * @ 函数名  ： AppTaskCreate
  * @ 功能说明： 为了方便管理，所有的任务创建函数都放在这个函数里面
  * @ 参数    ： 无  
  * @ 返回值  ： 无
  * @ 注意    ： 任务优先级要小于(系统的配置相关，当前最大值为16)
  **********************************************************************/
static void AppTaskCreate(void)
{
    taskENTER_CRITICAL();           //进入临界区
    BaseType_t result = pdFAIL;

    result = xTaskCreate( (TaskFunction_t )LED_Task,          //任务函数
                          (const char*    )"LED_Task",        //任务名称
                          (uint16_t       )256,               //任务堆栈大小,单位是Word
                          (void*          )NULL,              //传递参数
                          (UBaseType_t    )3,                 //任务优先级,数字越小优先级越低,0:留给IdleTask
                          (TaskHandle_t*  )&LEDsTaskHandle);	//任务句柄,如何后续不使用可以设置为NULL
    
    if(pdPASS == result) { // 创建成功
      //printf2buffatinit("LED_Task Create Success\r\n");
    }

    vTaskDelete(AppTaskCreateHandle); //最后删除AppTaskCreate任务
    taskEXIT_CRITICAL();              //退出临界区
}

// 如果使能Tick hook, 就不能使用HAL层和Tick相关的API
// 比如 HAL_Delay(x)
//void vApplicationTickHook( void )
//{
    //HAL_IncTick();
//}

