/**
 * @file shell_port.c
 * @author Letter (NevermindZZT@gmail.com)
 * @brief 
 * @version 0.1
 * @date 2019-02-22
 * 
 * @copyright (c) 2019 Letter
 * 
 */

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "shell_port.h"
#include "usart.h"
#include "debug_printf.h"
#include "stm32h7xx.h"
#include "stm32h7xx_hal.h"
//#include "serial.h"
//#include "cevent.h"

#define SHELL_RUN_IN_IDLE_TASK (1)  // shell 任务放在RTOS的IDLE任务内运行
#define SHELL_BUFF_LEN (512)        // 缓存长度
#define SHELL_RXTX_TOUT (20)        // 串口发送/接收等待超时,是系统tick数,比如tick的频率是1kHz,则刚好是ms



Shell shell;
ALIGN_32BYTES(char shellBuffer[SHELL_BUFF_LEN]) = {0};

/**
 * @brief 用户shell写
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return short 实际写入的数据长度
 */
short userShellWrite(char *data, unsigned short len)
{
    //serialTransmit(&debugSerial, (uint8_t *)data, len, 0x1FF);
	//调用STM32 HAL库 API 使用查询方式发送
	HAL_UART_Transmit(&huart1, (uint8_t*)data, len, SHELL_RXTX_TOUT);

    return len;
}


/**
 * @brief 用户shell读
 * 
 * @param data 数据
 * @param len 数据长度
 * 
 * @return short 实际读取到
 */
short userShellRead(char *data, unsigned short len)
{
    //return serialReceive(&debugSerial, (uint8_t *)data, len, 0);
    
    // shell 1个字符1个字符接收

    // OK返回0, 使用中断/常规
    //if (0 == HAL_UART_Receive_IT(&huart1, (uint8_t*)data, len)) {
    if (0 == HAL_UART_Receive(&huart1, (uint8_t*)data, len, SHELL_RXTX_TOUT)) {
        return len;
    } else {
        return 0;
    }
}

#if (SHELL_TASK_WHILE == 1)            // 使用RTOS

#if (SHELL_USING_LOCK == 1)            // 使用互斥信号量
static SemaphoreHandle_t shellMutex;

/**
 * @brief 用户shell上锁
 * 
 * @param shell shell
 * 
 * @return int 0
 */
int userShellLock(Shell *shell)
{
    xSemaphoreTakeRecursive(shellMutex, portMAX_DELAY);
    return 0;
}

/**
 * @brief 用户shell解锁
 * 
 * @param shell shell
 * 
 * @return int 0
 */
int userShellUnlock(Shell *shell)
{
    xSemaphoreGiveRecursive(shellMutex);
    return 0;
}
#endif

/**
 * @brief 用户shell初始化
 * 
 */
void User_Shell_Init(void)
{
    shell.write = userShellWrite;
    shell.read = userShellRead;
    #if (SHELL_USING_LOCK == 1)
    shellMutex = xSemaphoreCreateMutex();
    shell.lock = userShellLock;
    shell.unlock = userShellUnlock;
    #endif
    shellInit(&shell, shellBuffer, SHELL_BUFF_LEN);
    HAL_Delay(500);           // shell 初始化会通过串口发送字符
}

#if (SHELL_RUN_IN_IDLE_TASK == 1)   // shell 任务放在RTOS的IDLE任务内运行

// 这个钩子函数不可以调用会引起空闲任务阻塞的API函数，例如：
// vTaskDelay()、带有阻塞时间的队列和信号量函数
extern UBaseType_t uxHighWaterMark[5];
void vApplicationIdleHook( void )
{
    char data = 0;
    //uint32_t buff[32] = {0}; // 用于测试任务堆栈使用
    if (shell.read && shell.read(&data, 1) == 1)
    {
        shellHandler(&shell, data);
    }
    uxTaskGetStackHighWaterMark( NULL );
    //uxHighWaterMark[3] = buff[0];
}

// shell任务放在RTOS的IDLE任务内运行,不需要单独创建任务
void Shell_Task_Create(void)
{

}

#else                              // shell 任务单独创建任务运行

/**
 * @brief shell 任务
 * 
 * @param param 参数(shell对象)
 * 
 */
void User_Shell_Task(void *param)
{
    char data = 0;
    Shell *shell = (Shell *)param;
    while(1)
    {
        if (shell->read && shell->read(&data, 1) == 1)
        {
            shellHandler(shell, data);
        }
        //vTaskDelay(2);  // 任务加延时会导致很多按键不可用
    }
}


void Shell_Task_Create(void)
{
    BaseType_t result = pdFAIL;
    result = xTaskCreate( (TaskFunction_t )User_Shell_Task,
                          (const char*    )"shell",
                          (uint16_t       )256,
                          (void*          )&shell,
                          (UBaseType_t    )1,                // 最低的优先级
                          (TaskHandle_t*  )NULL);
    
    if(pdPASS == result) { // 创建成功
      //printf2buffatinit("shell task Create Success\r\n");
    }
}
#endif
//CEVENT_EXPORT(EVENT_INIT_STAGE2, userShellInit);

#else // 不使用RTOS时

// 把该任务放在10ms任务里运行
void User_Shell_Task(void)
{
    char data = 0;
    while(shell.read && (shell.read(&data, 1) == 1))
    {
        shellHandler(&shell, data);
    }
}

void User_Shell_Init(void)
{
	//注册自己实现的写函数
    shell.write = userShellWrite;
    shell.read = userShellRead;
	
	//调用shell初始化函数
    shellInit(&shell, shellBuffer, SHELL_BUFF_LEN);
}
#endif

#if 0
uint8_t recv_buf = 0;
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    /* 判断是哪个串口触发的中断 */
    if(huart ->Instance == USART1)
    {
        //调用shell处理数据的接口
		shellHandler(&shell, recv_buf);
        //使能串口中断接收
		HAL_UART_Receive_IT(&huart1, (uint8_t*)&recv_buf, 1);
    }
}
#endif


void user_shellprintf(char *str)
{
    shellPrint(&shell, "%s", str);
}

