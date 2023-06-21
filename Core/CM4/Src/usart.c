/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.c
  * @brief   This file provides code for the configuration
  *          of the USART instances.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2023 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include <stdbool.h>
#include "usart.h"
#include "stm32h7xx_ll_usart.h"
#include "stm32h7xx_ll_dma.h"

#include "FreeRTOS.h"	         // FreeRTOS	  
#include "task.h"                // FreeRTOS task


/* USER CODE BEGIN 0 */
#define INTER_FRM_SILENCE         (uint32_t)(11 * 3.5)      // 3.5 chars of 11 bits of INTER_FRM_SILENCE between frames
#define INIT_FINISH_STR ("UART8 init finished.")
#define STR_LEN (sizeof(INIT_FINISH_STR))
#define COM8_RX_BLEN (256) 
#define UART8_PERIOD   pdMS_TO_TICKS(5) // 发送等待延时为5ms

/* USER CODE END 0 */

UART_HandleTypeDef huart8;
DMA_HandleTypeDef hdma_uart8_tx;
DMA_HandleTypeDef hdma_uart8_rx;


uint8_t Rxdata = 0;
static uint8_t u8Uart8TxProgress = 0;  // UART1正在发送
static uint8_t u8Uart8RxFinished = 0;  // UART1帧接收完成，待处理
int16_t s16Uart8ReadIdx = 0;           // 从接收缓存读取的索引
static uint8_t u8Uart8LisenStatus = 0; // UART1是否已处于Lisen状态


//uint8_t Com8RxBuff[256] = {0};
ALIGN_32BYTES(uint8_t Com8RxBuff[COM8_RX_BLEN]) = {0}; // DMA访问, 内存地址需要32字节对齐，长度需要是32字节的整数倍


uint32_t u32RxDataCount = 0;   // 每次帧接收的数据字节数量
uint32_t u32DmaRxDataLen = 0;  // 每次启动DMA接收设定的接收数量

/* UART8 init function */
void MX_UART8_Init(void)
{
  huart8.Instance = UART8;
  huart8.Init.BaudRate = 750000;
  huart8.Init.WordLength = UART_WORDLENGTH_8B;
  huart8.Init.StopBits = UART_STOPBITS_1;
  huart8.Init.Parity = UART_PARITY_NONE;
  huart8.Init.Mode = UART_MODE_TX_RX;
  huart8.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart8.Init.OverSampling = UART_OVERSAMPLING_16;
  huart8.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart8.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart8.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart8, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart8, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart8) != HAL_OK)
  {
    Error_Handler();
  }
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(uartHandle->Instance==UART8)
  {
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_UART8;
    PeriphClkInitStruct.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* UART8 clock enable */
    __HAL_RCC_UART8_CLK_ENABLE();
    __HAL_RCC_GPIOJ_CLK_ENABLE();
    __HAL_RCC_DMA1_CLK_ENABLE();
    /**UART8 GPIO Configuration
    PJ9     ------> UART8_RX
    PJ8     ------> UART8_TX
    */
    GPIO_InitStruct.Pin = ARD_D0_Pin|ARD_D1_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF8_UART8;
    HAL_GPIO_Init(GPIOJ, &GPIO_InitStruct);

    /* UART8 DMA Init */
    /* UART8_TX Init */
    hdma_uart8_tx.Instance = DMA1_Stream0;
    hdma_uart8_tx.Init.Request = DMA_REQUEST_UART8_TX;
    hdma_uart8_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_uart8_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart8_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart8_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart8_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart8_tx.Init.Mode = DMA_NORMAL;
    hdma_uart8_tx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_uart8_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_uart8_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_uart8_tx);

    /* UART8_RX Init */
    hdma_uart8_rx.Instance = DMA1_Stream1;
    hdma_uart8_rx.Init.Request = DMA_REQUEST_UART8_RX;
    hdma_uart8_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_uart8_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_uart8_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_uart8_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_uart8_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_uart8_rx.Init.Mode = DMA_NORMAL;
    hdma_uart8_rx.Init.Priority = DMA_PRIORITY_LOW;
    hdma_uart8_rx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_uart8_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_uart8_rx);

    /* UART8 interrupt Init */
    HAL_NVIC_SetPriority(UART8_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(UART8_IRQn);
  /* USER CODE BEGIN UART8_MspInit 1 */

    HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
    HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
  /* USER CODE END UART8_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==UART8)
  {
  /* USER CODE BEGIN UART8_MspDeInit 0 */

  /* USER CODE END UART8_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_UART8_CLK_DISABLE();

    /**UART8 GPIO Configuration
    PJ9     ------> UART8_RX
    PJ8     ------> UART8_TX
    */
    HAL_GPIO_DeInit(GPIOJ, ARD_D0_Pin|ARD_D1_Pin);

    /* UART8 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* UART8 interrupt Deinit */
    HAL_NVIC_DisableIRQ(UART8_IRQn);
  /* USER CODE BEGIN UART8_MspDeInit 1 */
    HAL_NVIC_DisableIRQ(DMA1_Stream0_IRQn);
    HAL_NVIC_DisableIRQ(DMA1_Stream1_IRQn);
  /* USER CODE END UART8_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
// USART接收完成中断,回调函数,中断标志清除在上一层完成
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == NULL) {
      Error_Handler();
    }

    // 接收的字节数 = DMA_RX期望字节数 - DMA_RX剩余的字节数(DMAx, Channelx)
    u32RxDataCount = (u32DmaRxDataLen - LL_DMA_GetDataLength(DMA1, LL_DMA_STREAM_1));
}

// USART发送完成中断,回调函数,中断标志清除在上一层完成
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == NULL) {
    Error_Handler();
  }

  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_TCF); // 清除发送完成标志
  u8Uart8TxProgress = 0U;                       // UART1发送完成
}


// USART错误中断,回调函数,中断标志清除在上一层完成
// 回调函数主要用于处理接收超时(帧间隔),其他仅累计次数
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
  if (huart == NULL) {
    Error_Handler();
  }

  // Clear the Error flags in the ICR register
  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_OREF | UART_CLEAR_NEF | UART_CLEAR_PEF | UART_CLEAR_FEF);

  // Send request to clear the Rx Not Empty flag
  __HAL_UART_SEND_REQ(huart, UART_RXDATA_FLUSH_REQUEST);

  if ((huart->ErrorCode & HAL_UART_ERROR_RTO) == HAL_UART_ERROR_RTO) // 接收超时
  {
    LL_USART_DisableIT_RTO(huart->Instance);     // USARTx
    LL_USART_DisableRxTimeout(huart->Instance);  // USARTx

    LL_USART_ClearFlag_RTO(huart->Instance);

    // 在Abort之前，存储接收的字节数 = DMA_RX期望字节数 - DMA_RX剩余的字节数(DMAx, Channelx)
    u32RxDataCount = (u32DmaRxDataLen - LL_DMA_GetDataLength(DMA1, LL_DMA_STREAM_1));
    HAL_UART_AbortReceive(huart);


    // 通知数据处理任务进行处理
    u8Uart8LisenStatus = 0U;   // 不是监听状态
    u8Uart8RxFinished = 0xFU;  // 新数据接收完成，允许处理
    s16Uart8ReadIdx = 0;       // 新数据读取位置从0开始

    // 在处理完数据的函数内需要重新使能DMA接收
  }
  else
  {
      //u16RxErrorCounter++;   // 其他 Error 累计错误次数
  }
}


/*--------------------------------------------------------------------------------------*/
// Class Method : Receive N bytes in non-blocking mode (IRQ or DMA)
// DMA1/DMA2不能访问DTCM RAM, 所以用于接收和发送的buff需要定位到其他区域，比如D1
// DMA访问的内存地址需要32字节对齐
// DMA接收方式：设置一个缓存能接收的最大值，通过帧间隔超时来接收一次接收
bool Uart_bReceive_NonBlocking(UART_HandleTypeDef *hUARTx, uint8_t *pDstAddr, uint32_t u32DataLen)
{
  HAL_StatusTypeDef result = HAL_ERROR;

  if ((hUARTx == NULL) || (pDstAddr == NULL)) {
    Error_Handler();
  }

  u32DmaRxDataLen = u32DataLen; // DMA接收的预期字节数，如果接收的字节数达到将会触发DMA_RX中断
  u8Uart8RxFinished = 0U;       // 准备进行下一次接收

  //Enable RX (UARTx);

  // 清缓存，避免CM7读取的只是CACHE内的值,CM4不需要?
  result = HAL_UART_Receive_DMA(hUARTx, pDstAddr, u32DataLen);           // RX with DMA
  //result = HAL_UART_Receive_IT(UARTx, pDstAddr, u32DataLen); // RX with IT

  // Enable Rx Timeout (integrated to peripheral)
  LL_USART_SetRxTimeout(hUARTx->Instance, INTER_FRM_SILENCE);
  LL_USART_EnableIT_RTO(hUARTx->Instance);
  LL_USART_EnableRxTimeout(hUARTx->Instance);

  return (result == HAL_OK ? true : false);
}

/*--------------------------------------------------------------------------------------*/
// Class Method : Send N bytes in non-blocking mode (IRQ or DMA)
// DMA1/DMA2不能访问DTCM RAM, 所以用于接收和发送的buff需要定位到其他区域，比如D1
// DMA访问的内存地址需要32字节对齐
bool Uart_bSend_NonBlocking(UART_HandleTypeDef *hUARTx, uint8_t *pSrcAddr, uint32_t u32DataLen)
{
  HAL_StatusTypeDef result = HAL_ERROR;

  if ((hUARTx == NULL) || (pSrcAddr == NULL)) {
    Error_Handler();
  }

  //Enable RX (UARTx);
  u8Uart8TxProgress = 0x0AU;

  // 清缓存，避免CM7读取的只是CACHE内的值,CM4不需要?
  //SCB_CleanDCache_by_Addr((uint32_t *)pSrcAddr, 0);            // 按照2字节的方式对齐,长度以字节为单位
  result = HAL_UART_Transmit_DMA(hUARTx, pSrcAddr, u32DataLen);   // TX with DMA
  //result = HAL_UART_Transmit_IT(hUARTx, pSrcAddr, u32DataLen);  // TX with IT

  return  (result == HAL_OK ? true : false);
}


void PutUart8ToLisen(void)
{
  // 接收完成 && 数据处理完成
  if ((u8Uart8LisenStatus == 0U) && (u8Uart8RxFinished == 0U)) {
        Uart_bReceive_NonBlocking(&huart8, Com8RxBuff, (COM8_RX_BLEN / 2));
        u8Uart8LisenStatus = 0xFU;
  }
}

uint8_t GetUart8TxStatus(void)
{
  return (u8Uart8TxProgress);
}

uint8_t GetUart8RxStatus(void)
{
  return (u8Uart8RxFinished);
}



// 从UART1的接收缓存里读取一字节
// 返回值：
// 0：没有数据
// 1：获取1字节数据
uint8_t ReadOneByteFromBuff(char *data)
{
    if ((u8Uart8RxFinished != 0U) && (s16Uart8ReadIdx >= u32RxDataCount)) {
        s16Uart8ReadIdx = 0;
        u8Uart8RxFinished = 0U; // 已经被读取了
    }

    if (0U == u8Uart8RxFinished) {
        return 0U;
    }

    *data = (s16Uart8ReadIdx < COM8_RX_BLEN ? Com8RxBuff[s16Uart8ReadIdx] : 0U);
    s16Uart8ReadIdx++;

    return 1U;
}

typedef enum _UART_PROC {
    UART_INIT = 0,
    UART_TO_LISEN,
    UART_LISEN,
    UART_RX_PROCESS,
    UART_TX_PREPARE,
    UART_TX,
    UART_WAIT_COMPLETE
} UART_ProcStatus_t;

typedef struct _Uart_Proc_handle {
    UART_ProcStatus_t enSatusMathine; // 状态机
    UART_HandleTypeDef *pUartx;       // UART句柄
    uint8_t *pRxBuff;                 // 接收缓存地址
    uint8_t *pTxBuff;                 // 发送缓存地址
    uint8_t RxFinished;               // 接收一帧数据待处理
    uint8_t TxProgress;               // 正在发送中
    int16_t ReadIndex;                // 从接收缓存出栈索引
    uint32_t RxDataCnt;               // 接收帧的数据长度
    uint32_t TxDataCnt;               // 发送帧的数据长度
} UART_ProcHandle_t;

void UART8_ProcMachine(UART_ProcStatus_t *pStatus);
// FreeRTOS task
// Task period is 5ms
void UART8_Task(void)
{
    UART_ProcStatus_t enStatus = UART_TO_LISEN;
    while (1)
    {
        UART8_ProcMachine(&enStatus);
        vTaskDelay(UART8_PERIOD);
    }
}

// 当前的处理是: 把收到的内容发送回来
void UART8_ProcMachine(UART_ProcStatus_t *pStatus)
{
    UART_ProcStatus_t enStatus = *pStatus;
    switch (enStatus) {
    case UART_TO_LISEN:
        PutUart8ToLisen();
        *pStatus = UART_LISEN;
        break;

    case UART_LISEN:
        if (u8Uart8RxFinished != 0U) {
            *pStatus = UART_RX_PROCESS;
        }
        break;

    case UART_RX_PROCESS:
        // 在这里对接收数据进行处理
        u8Uart8RxFinished = 0U;
        if (u8Uart8RxFinished == 0U) {
            *pStatus = UART_TX_PREPARE;
        }
        break;

    case UART_TX_PREPARE:
        // 在这里把要回复的数据写入到发送缓存
        *pStatus = UART_TX;
        break;

    case UART_TX:
        // 调用发送处理函数
        Uart_bSend_NonBlocking(&huart8, Com8RxBuff, u32RxDataCount);
        u32RxDataCount = 0U;
        *pStatus = UART_WAIT_COMPLETE;
        break;

    case UART_WAIT_COMPLETE:
        if (u8Uart8TxProgress == 0U) {
            *pStatus = UART_TO_LISEN;
        }
        break;

    default:
        *pStatus = UART_TO_LISEN;
        break;
    }
}

/* USER CODE END 1 */
