/*
 * rtos implementition
 * Copyright (C) 2023 Schneider-Electric.  All Rights Reserved.
 *
 */
#include <string.h>
#include <stdbool.h>
#include "FreeRTOS.h"	// FreeRTOS
#include "semphr.h"		// FreeRTOS semaphore
#include "usart.h"


#define COM_SEND_TIMEOUT   pdMS_TO_TICKS(10) // 发送等待延时为10ms
#define COM_SEMPHR_TIMEOUT pdMS_TO_TICKS(10) // 信号量等待延时为10ms
#define ULONG_MAX 0xffffffffUL

extern SemaphoreHandle_t xSemphr_Printf;
extern SemaphoreHandle_t xMutex_Uart1;
extern QueueHandle_t xQueue_Printf;
extern QueueHandle_t xQueue_PB;

ALIGN_32BYTES(static char PrintBuff[PRINT_BSIZE]) = {0}; // 使用DMA，内存地址需要32字节对齐

static bool isPrintTaskReady = false;


void debug_printf(const char *str)
{
    int len = 0;
    if ((str == NULL) || ((len = strlen(str)) == 0)) {
        return;
    }
 
    HAL_UART_Transmit(&huart1, (const uint8_t *)str, len, COM_SEND_TIMEOUT);
}

// 专门用于调试信息打印的任务
// 由信号量触发调用
void RTOS_DebugPrintTask(void)
{
    isPrintTaskReady = true;
    uint8_t strBuff[64] = {0};
    uint32_t buffAddr = 0;

    while(1) {
        if (0U == GetUart1TxStatus()) {
            
            // 其他任务往buff内写打印内容, 并通过信号量通知
            if (pdTRUE == xSemaphoreTake(xSemphr_Printf, 0)) { //发送缓存有要发送的内容
                //HAL_UART_Transmit(&huart1, (const uint8_t *)PrintBuff, strlen(PrintBuff), COM_SEND_TIMEOUT);
                (void)Uart_bSend_NonBlocking(&huart1, (uint8_t *)PrintBuff, strlen(PrintBuff));
            }

            // 其他任务通过消息队列发送要打印的内容
            else if (pdTRUE ==  xQueueReceive(xQueue_Printf, strBuff, 0)) {
                (void)Uart_bSend_NonBlocking(&huart1, strBuff, strlen((char const *)strBuff));
            }

            // 其他任务通过消息队列发送要打印的内容,消息值是缓存地址
            else if (pdTRUE ==  xQueueReceive(xQueue_PB, &buffAddr, 0)) {
                (void)Uart_bSend_NonBlocking(&huart1, (uint8_t *)buffAddr, strlen((char const *)buffAddr));
            }
            // 其他任务通过任务通知发送要打印的内容,通知值是缓存地址, 接收完退出前清除通知值
            else if (pdTRUE ==  xTaskNotifyWait(0, ULONG_MAX, &buffAddr, 0)) {
                (void)Uart_bSend_NonBlocking(&huart1, (uint8_t *)buffAddr, strlen((char const *)buffAddr));
            }

            vTaskDelay(50);
        } else {
            vTaskDelay(500);
        }        
    }
}

// 把需要打印的字符串拷贝到打印缓存
void printf2buff(char const *str)
{
    if (isPrintTaskReady == false) {
        return;
    }
    // 对字符串进行长度合法检查

    // 成功释放信号量后进行拷贝
    if (pdTRUE == xSemaphoreGive(xSemphr_Printf)) {
        //strncpy(PrintBuff, str, strlen(str));
        strcpy(PrintBuff, str);
        xSemaphoreGive(xSemphr_Printf);
    }
}

// 把需要打印的字符串拷贝到打印缓存
void nprintf2buff(char const *str, uint16_t len)
{
    if (isPrintTaskReady == false) {
        return;
    }
    // 对字符串进行长度合法检查

    // 成功释放信号量后进行拷贝
    if (pdTRUE == xSemaphoreGive(xSemphr_Printf)) {
        //strncpy(PrintBuff, str, strlen(str));
        strncpy(PrintBuff, str, len);
        PrintBuff[len] = '\0';
        xSemaphoreGive(xSemphr_Printf);
    }
}

// 把需要打印的字符串拷贝到打印缓存
void printf2bufffromISR(char const *str)
{
    BaseType_t xHigherPriorityTaskWoken;
    if (isPrintTaskReady == false) {
        return;
    }

    // 对字符串进行长度合法检查

    // 成功释放信号量后进行拷贝
    xHigherPriorityTaskWoken = pdFALSE;
    if (pdTRUE == xSemaphoreGiveFromISR(xSemphr_Printf, &xHigherPriorityTaskWoken)) {
        //strncpy(PrintBuff, str, strlen(str));
        strcpy(PrintBuff, str);
        xSemaphoreGiveFromISR(xSemphr_Printf, &xHigherPriorityTaskWoken);
    }

    // 
    if(xHigherPriorityTaskWoken != pdFALSE) {

    }
}


// 初始化阶段的打印信息，可以所有信息一次打印出来
void printf2buffatinit(char const *str)
{
    // 对字符串进行长度合法检查
    if ((strlen(PrintBuff) + strlen(str) + 1) > PRINT_BSIZE) {
        return;
    }

    strcat(PrintBuff, str);
    if (isPrintTaskReady == false) {
        return;
    }

    xSemaphoreGive(xSemphr_Printf); // 强制发送信号量
}

/*
***********************************************************************************
日志颜色格式说明
1.颜色日志格式
        格式：\033[显示方式;字体色;背景色m
        如缺省默认：\033[0m //表示结束打印
        \033 八进制转义 表示ESC
2.显示方式
		0（默认值）
		1（高亮）
		4（下划线）
		5（闪烁）
		7（反显）
		22（非粗体）
		24（非下划线）
		25（非闪烁）
		27（非反显）
3.字体色
		30（黑色）
		31（红色）
		32（绿色）
		33（黄色）
		34（蓝色）
		35（洋红）
		36（青色）
		37（白色）
4.背景色
		40（黑色）
		41（红色）
		42（绿色）
		43（黄色）
		44（蓝色）
		45（洋红）
		46（青色）
		47（白色）
***********************************************************************************
*/
