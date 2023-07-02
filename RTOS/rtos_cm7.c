/*
 * STM32H747I-DISCO study
 * Copyright (C) 2023 Schneider-Electric.
 * All Rights Reserved.
 *
 */
#include <string.h>
#include "FreeRTOS.h"	      // FreeRTOS	  
#include "task.h"           // FreeRTOS task
#include "semphr.h"		      // FreeRTOS semaphore
#include "event_groups.h"		// FreeRTOS event

#include "stm32h747i_discovery.h"
#include "usart.h"
#include "tim.h"
#include "task10ms.h"
#include "debug_printf.h"
#include "shell_port.h"


static void AppTaskCreate(void);           /* 用于创建任务 */ 
static void LED_Task(void* pvParameters);  /* LED_Task任务实现 */
void xOneSecondTimer(void);                // 软件定时器1

/**************************** 任务句柄 ********************************/
/* 
 * 任务句柄是一个指针，用于指向一个任务，当任务创建好之后，它就具有了一个任务句柄
 * 以后我们要想操作这个任务都需要通过这个任务句柄，如果是自身的任务操作自己，那么
 * 这个句柄可以为NULL。
 */
TaskHandle_t AppTaskCreateHandle;  // 创建任务句柄
TaskHandle_t LEDsTaskHandle;	      // LED任务句柄
TaskHandle_t PrintfTaskHandle;	    // 信息打印任务句柄
TaskHandle_t TenmsTaskHandle;	    // 10ms任务句柄

SemaphoreHandle_t xSemphr_Printf = NULL;  // 用于锁定打印COM口的二值信号量
SemaphoreHandle_t xMutex_Uart1 = NULL;

QueueHandle_t xQueue_Printf = NULL;       // 消息队列，消息拷贝到队列里
QueueHandle_t xQueue_PB = NULL;           // 消息队列，用于发送消息缓存

EventGroupHandle_t xEvent_eFlag = NULL;   // 事件组

static TimerHandle_t hSTimer1 = NULL;     // 软件定时器

UBaseType_t uxHighWaterMark[5] = {0};     // 任务堆栈剩余值数组


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
static void LED_Task(void* parameter)
{	
    EventBits_t uxBits;
    char strBuff[512] = {0};
    char taskInfo[256] = {0};
    uint32_t buffAddr = (uint32_t)strBuff;

    //printf2buff("LED_Task Start Running Now...\r\n");
    //xQueueSend(xQueue_Printf, "LED_Task Start Running Now...\r\n", 0);
    (void)xTaskNotifyGive(TenmsTaskHandle); // 用任务通知,通知10ms任务

    while (1)
    {
        BSP_LED_Toggle(LED1);

        // 接收按键事件
        uxBits = xEventGroupClearBits(xEvent_eFlag, (WAKEUP_EVENT|JOYSEL_EVENT));
        if ((uxBits & WAKEUP_EVENT) == WAKEUP_EVENT) {
            xQueueSend(xQueue_Printf, "WAKEUP Pressed\r\n", 0);
        } else if ((uxBits & JOYSEL_EVENT) == JOYSEL_EVENT) {
            //xQueueSend(xQueue_Printf, "JOY_SEL Key Pressed\r\n", 0);

            memset(taskInfo,0,sizeof(taskInfo));
            vTaskList((char *)&taskInfo); //获取任务运行时间信息
            strcpy(strBuff, "\r\n---------------------------------------------\r\n");
            strcat(strBuff, "任务名\t\t任务状态 优先级\t剩余栈\t任务序号\r\n");
            strcat(strBuff, taskInfo);
            strcat(strBuff, "---------------------------------------------\r\n");
            memset(taskInfo,0,sizeof(taskInfo));
            vTaskGetRunTimeStats((char*)taskInfo);
            strcat(strBuff, "任务名\t\t运行计数\t使用率\r\n");
            strcat(strBuff, taskInfo);
            strcat(strBuff, "---------------------------------------------\r\n");
            xQueueSend(xQueue_PB, &buffAddr, 0);
        } else {
            // 读取任务堆栈剩余最小值
            uxHighWaterMark[0] = uxTaskGetStackHighWaterMark( LEDsTaskHandle );
            uxHighWaterMark[1] = uxTaskGetStackHighWaterMark( PrintfTaskHandle );
            uxHighWaterMark[2] = uxTaskGetStackHighWaterMark( TenmsTaskHandle );
       }       
        vTaskDelay(500);        // 延时500个tick
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
                          (uint16_t 	    )256,	                  //任务堆栈大小
                          (void* 		      )NULL,				          //传递参数
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

    // 创建打印任务互斥信号量
    //xMutex_Uart1 = xSemaphoreCreateMutex();

    // 1.写缓存的任务Give信号量(通知); 2.打印专用任务Take信号量(有打印任务)
    xSemphr_Printf = xSemaphoreCreateBinary();

    // 消息队列,打印的内容通过消息队列传递给打印任务
    xQueue_Printf = xQueueCreate(4, sizeof(char)*64); // 消息队列长度是4，每个消息可以发送64个字符

    // 消息队列,打印的内容通过消息队列传递给打印任务
    xQueue_PB = xQueueCreate(2, sizeof(void *));      // 消息队列长度是2，消息发送的是缓存地址

    // 事件组, bit0:Wakeup key, bit1:Joy SET
    xEvent_eFlag = xEventGroupCreate();

    result = xTaskCreate( (TaskFunction_t )RTOS_DebugPrintTask,
                          (const char*    )"PrintfTask",
                          (uint16_t       )128,
                          (void*          )NULL,
                          (UBaseType_t    )2,
                          (TaskHandle_t*  )&PrintfTaskHandle);

    if((pdPASS == result) && (NULL != xSemphr_Printf)) {
      printf2buffatinit("PrintfTask Create Success\r\n");
    }
 
    result = pdFAIL;
    result = xTaskCreate( (TaskFunction_t )LED_Task,          //任务函数
                          (const char*    )"LED_Task",        //任务名称
                          (uint16_t       )512,               //任务堆栈大小,单位是Word
                          (void*          )NULL,              //传递参数
                          (UBaseType_t    )3,                 //任务优先级,数字越小优先级越低,0:留给IdleTask
                          (TaskHandle_t*  )&LEDsTaskHandle);	//任务句柄,如何后续不使用可以设置为NULL
    
    if(pdPASS == result) { // 创建成功
      printf2buffatinit("LED_Task Create Success\r\n");
    }

    // 需要创建的其他任务...
    result = pdFAIL;
    result = xTaskCreate( (TaskFunction_t )RTOS_Task10ms,
                          (const char*    )"RTOS_Task10ms",
                          (uint16_t       )128,
                          (void*          )NULL,
                          (UBaseType_t    )6,
                          (TaskHandle_t*  )&TenmsTaskHandle);
    
    if(pdPASS == result) { // 创建成功
      printf2buffatinit("RTOS_Task10ms Create Success\r\n");
    }

    // shell任务创建
    Shell_Task_Create();

    // 创建软件定时器
    hSTimer1 = xTimerCreate( (const char*	)"OneSecondTimer",            // 定时器名称
                             (TickType_t		)pdMS_TO_TICKS(1000),       // 定时器周期, 1000ms
                             (UBaseType_t	)pdTRUE,                      // 自动重载
                             (void*				)1,                           // 定时器ID
                             (TimerCallbackFunction_t)xOneSecondTimer); // 回调函数
    if (hSTimer1 != NULL) {
        xTimerStart(hSTimer1,1000);	// 等待1000tick后开启定时器
    }

    HAL_TIM_Base_Start(&htim2);

    printf2buffatinit("AppTaskCreate Delete Success\r\n");
    vTaskDelete(AppTaskCreateHandle); //最后删除AppTaskCreate任务
    taskEXIT_CRITICAL();              //退出临界区
}


// 软件定时器回调函数
void xOneSecondTimer(void)
{
    BSP_LED_Toggle(LED4);
}



/*********************************** 下面是使用静态创建任务的例子 *****************************************/
#if 0

static StaticTask_t AppTaskCreateTCB;
static StaticTask_t LEDsTaskTCB;
static StaticTask_t IdleTaskTCB;
static StaticTask_t TimerTaskTCB;

static StackType_t AppTaskCreateStack[128];                        //
static StackType_t LEDsTaskStack[128];                             //
static StackType_t IdleTaskStack[configMINIMAL_STACK_SIZE];        //
static StackType_t TimerTaskStack[configTIMER_TASK_STACK_DEPTH];   //
 

/*****************************************************************
  * @brief  任务创建函数
  * @param  无
  * @retval 无
  * @note   实现RTOS的任务创建，创建完成后删除本任务
  ****************************************************************/
void osTaskInit(void)
{	
	AppTaskCreateHandle = xTaskCreateStatic((TaskFunction_t)AppTaskCreate,		                    //任务函数
															(const char* 	)"AppTaskCreate",		//任务名称
															(uint32_t 		)128,	                //任务堆栈大小
															(void* 		  	)NULL,				    //传递给任务函数的参数
															(UBaseType_t 	)3, 	                //任务优先级
															(StackType_t*   )AppTaskCreateStack,	//任务堆栈
															(StaticTask_t*  )&AppTaskCreateTCB);	//任务控制块   
															
	if(NULL != AppTaskCreateHandle) {
        vTaskStartScheduler();   // 启动任务，开启调度
    }
  
  while(1); // 正常不会执行到这里
}
 
 
/***********************************************************************
  * @ 函数名  ： AppTaskCreate
  * @ 功能说明： 为了方便管理，所有的任务创建函数都放在这个函数里面
  * @ 参数    ： 无  
  * @ 返回值  ： 无
  **********************************************************************/
static void AppTaskCreate(void)
{
  taskENTER_CRITICAL();           //进入临界区
 
  /* 创建LED_Task任务 */
	LEDsTaskHandle = xTaskCreateStatic(
                                       (TaskFunction_t)LED_Task,		//任务函数
									   (const char* 	)"LED_Task",	//任务名称
                                       (uint32_t 		)128,			//任务堆栈大小
                                       (void* 		  	)NULL,		    //传递给任务函数的参数
                                       (UBaseType_t 	)4, 		    //任务优先级
                                       (StackType_t*   )LEDsTaskStack,	//任务堆栈
                                       (StaticTask_t*  )&LEDsTaskTCB);	//任务控制块   
	
	if(NULL != LEDsTaskHandle) { // 创建成功

    }
	
  vTaskDelete(AppTaskCreateHandle); //删除AppTaskCreate任务
  
  taskEXIT_CRITICAL();            //退出临界区
}


/**
	* 使用了静态分配内存，以下这两个函数是由用户实现，函数在task.c文件中有引用
	*	当且仅当 configSUPPORT_STATIC_ALLOCATION 这个宏定义为 1 的时候才有效
	*/

/**
  **********************************************************************
  * @brief  获取空闲任务的任务堆栈和任务控制块内存
	*					ppxTimerTaskTCBBuffer	:		任务控制块内存
	*					ppxTimerTaskStackBuffer	:	任务堆栈内存
	*					pulTimerTaskStackSize	:		任务堆栈大小
  * @date    2018-xx-xx
  **********************************************************************
  */ 
void vApplicationGetIdleTaskMemory(StaticTask_t **ppxIdleTaskTCBBuffer, 
								   StackType_t **ppxIdleTaskStackBuffer, 
								   uint32_t *pulIdleTaskStackSize)
{
	*ppxIdleTaskTCBBuffer = &IdleTaskTCB;/* 任务控制块内存 */
	*ppxIdleTaskStackBuffer = IdleTaskStack;/* 任务堆栈内存 */
	*pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;/* 任务堆栈大小 */
}
 
/**
  *********************************************************************
  * @brief  获取定时器任务的任务堆栈和任务控制块内存
	*					ppxTimerTaskTCBBuffer	:		任务控制块内存
	*					ppxTimerTaskStackBuffer	:	任务堆栈内存
	*					pulTimerTaskStackSize	:		任务堆栈大小
  **********************************************************************
  */ 
void vApplicationGetTimerTaskMemory(StaticTask_t **ppxTimerTaskTCBBuffer, 
									StackType_t **ppxTimerTaskStackBuffer, 
									uint32_t *pulTimerTaskStackSize)
{
	*ppxTimerTaskTCBBuffer = &TimerTaskTCB;/* 任务控制块内存 */
	*ppxTimerTaskStackBuffer = TimerTaskStack;/* 任务堆栈内存 */
	*pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;/* 任务堆栈大小 */
}
#endif // End使用静态创建任务

