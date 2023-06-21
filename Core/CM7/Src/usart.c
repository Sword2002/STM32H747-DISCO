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
#include "HdwITPriorities.h"
#include "debug_printf.h"

#ifndef PRINT_BSIZE
  #define PRINT_BSIZE 0
  #warning "Not define the printf buff size!"
#endif

#define INTER_FRM_SILENCE         (uint32_t)(11 * 3.5)      // 3.5 chars of 11 bits of INTER_FRM_SILENCE between frames
#define INIT_FINISH_STR ("UART1 init finished.")
#define STR_LEN (sizeof(INIT_FINISH_STR))
#define COM1_RX_BLEN (256) 

UART_HandleTypeDef huart1;
DMA_HandleTypeDef hdma_usart1_tx;
DMA_HandleTypeDef hdma_usart1_rx;


uint8_t Rxdata = 0;
static uint8_t u8Uart1TxProgress = 0;  // UART1正在发送
static uint8_t u8Uart1RxFinished = 0;  // UART1帧接收完成，待处理
int16_t s16Uart1ReadIdx = 0;           // 从接收缓存读取的索引
static uint8_t u8Uart1LisenStatus = 0; // UART1是否已处于Lisen状态


//uint8_t Com1RxBuff[256] = {0};
ALIGN_32BYTES(uint8_t Com1RxBuff[COM1_RX_BLEN]) = {0}; // DMA访问, 内存地址需要32字节对齐，长度需要是32字节的整数倍


uint32_t u32RxDataCount = 0;   // 每次帧接收的数据字节数量
uint32_t u32DmaRxDataLen = 0;  // 每次启动DMA接收设定的接收数量

/* USART1 init function */

void MX_USART1_UART_Init(void)
{
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  huart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&huart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&huart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&huart1) != HAL_OK)
  {
    Error_Handler();
  }

#if 0
  /* USER CODE BEGIN USART1_Init 2 */
  HAL_UART_Transmit(&huart1, INIT_FINISH_STR, STR_LEN, 0xFFFF);
  /* USER CODE END USART1_Init 2 */
  /* Enable the UART RX threshold interrupt */
  __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
  /* Put UART peripheral in reception process */
  HAL_UART_Receive_IT(&huart1, (uint8_t*)&Rxdata, 1);
#endif
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART1;
    PeriphClkInitStruct.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_DMA2_CLK_ENABLE();

    /* USART1 GPIO Configuration
    PA10    ------> USART1_RX
    PA9     ------> USART1_TX
    */
    GPIO_InitStruct.Pin = STLINK_TX_Pin|STLINK_RX_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF7_USART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* USART1 DMA Init */
    /* USART1_TX Init */
    hdma_usart1_tx.Instance = DMA2_Stream0;
    hdma_usart1_tx.Init.Request = DMA_REQUEST_USART1_TX;
    hdma_usart1_tx.Init.Direction = DMA_MEMORY_TO_PERIPH;
    hdma_usart1_tx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_tx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_tx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_tx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_tx.Init.Mode = DMA_NORMAL;
    hdma_usart1_tx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    hdma_usart1_tx.Init.FIFOMode = DMA_FIFOMODE_DISABLE;
    if (HAL_DMA_Init(&hdma_usart1_tx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmatx,hdma_usart1_tx);

    
    /* USART1_RX Init */
    hdma_usart1_rx.Instance = DMA2_Stream1;
    hdma_usart1_rx.Init.Request = DMA_REQUEST_USART1_RX;
    hdma_usart1_rx.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_usart1_rx.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_usart1_rx.Init.MemInc = DMA_MINC_ENABLE;
    hdma_usart1_rx.Init.PeriphDataAlignment = DMA_PDATAALIGN_BYTE;
    hdma_usart1_rx.Init.MemDataAlignment = DMA_MDATAALIGN_BYTE;
    hdma_usart1_rx.Init.Mode = DMA_NORMAL;
    hdma_usart1_rx.Init.Priority = DMA_PRIORITY_VERY_HIGH;
    if (HAL_DMA_Init(&hdma_usart1_rx) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(uartHandle,hdmarx,hdma_usart1_rx);    
    
    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn,
                         HDW_IT_GETPRIORITY(HDW_IT_PRIORITY_MBU_USART),
                         HDW_IT_GETSUBPRIORITY(HDW_IT_PRIORITY_MBU_USART));
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */

    /* DMA2_Channel3_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream0_IRQn,
                         HDW_IT_GETPRIORITY(HDW_IT_PRIORITY_MBU_TXDMA),
                         HDW_IT_GETSUBPRIORITY(HDW_IT_PRIORITY_MBU_TXDMA));
    HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    /* DMA2_Channel4_IRQn interrupt configuration */
    HAL_NVIC_SetPriority(DMA2_Stream1_IRQn,
                         HDW_IT_GETPRIORITY(HDW_IT_PRIORITY_MBU_RXDMA),
                         HDW_IT_GETSUBPRIORITY(HDW_IT_PRIORITY_MBU_RXDMA));
    HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);
    
  /* USER CODE END USART1_MspInit 1 */
  }
}




void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PA10     ------> USART1_RX
    PA9     ------> USART1_TX
    */
    HAL_GPIO_DeInit(GPIOA, STLINK_TX_Pin|STLINK_RX_Pin);

  /* USER CODE BEGIN USART1_MspDeInit 1 */
    /* USART1 DMA DeInit */
    HAL_DMA_DeInit(uartHandle->hdmatx);
    HAL_DMA_DeInit(uartHandle->hdmarx);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE END USART1_MspDeInit 1 */
  }
}

// USART接收完成中断,回调函数,中断标志清除在上一层完成
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart == NULL) {
      Error_Handler();
    }

    // 接收的字节数 = DMA_RX期望字节数 - DMA_RX剩余的字节数(DMAx, Channelx)
    u32RxDataCount = (u32DmaRxDataLen - LL_DMA_GetDataLength(DMA2, LL_DMA_STREAM_1));
}

// USART发送完成中断,回调函数,中断标志清除在上一层完成
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart == NULL) {
    Error_Handler();
  }

  __HAL_UART_CLEAR_FLAG(huart, UART_CLEAR_TCF); // 清除发送完成标志
  u8Uart1TxProgress = 0U;                       // UART1发送完成
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
    u32RxDataCount = (u32DmaRxDataLen - LL_DMA_GetDataLength(DMA2, LL_DMA_STREAM_1));
    HAL_UART_AbortReceive(huart);


    // 通知数据处理任务进行处理
    u8Uart1LisenStatus = 0U;   // 不是监听状态
    u8Uart1RxFinished = 0xFU;  // 新数据接收完成，允许处理
    s16Uart1ReadIdx = 0;       // 新数据读取位置从0开始

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
  u8Uart1RxFinished = 0U;       // 准备进行下一次接收

  //Enable RX (UARTx);

  // 清缓存，避免CM7读取的只是CACHE内的值
  //SCB_CleanDCache_by_Addr((uint32_t *)pDstAddr, COM1_RX_BLEN);
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
  u8Uart1TxProgress = 0x0AU;
  SCB_CleanDCache_by_Addr((uint32_t *)pSrcAddr, PRINT_BSIZE);     // 安装32字节的方式对齐,长度以字节为单位
  result = HAL_UART_Transmit_DMA(hUARTx, pSrcAddr, u32DataLen);   // TX with DMA
  //result = HAL_UART_Transmit_IT(hUARTx, pSrcAddr, u32DataLen);  // TX with IT

  return  (result == HAL_OK ? true : false);
}


void PutUart1ToLisen(void)
{
  // 接收完成 && 数据处理完成
  if ((u8Uart1LisenStatus == 0U) && (u8Uart1RxFinished == 0U)) {
        Uart_bReceive_NonBlocking(&huart1, Com1RxBuff, (COM1_RX_BLEN / 2));
        u8Uart1LisenStatus = 0xFU;
  }
}

uint8_t GetUart1TxStatus(void)
{
  return (u8Uart1TxProgress);
}

uint8_t GetUart1RxStatus(void)
{
  return (u8Uart1RxFinished);
}



// 从UART1的接收缓存里读取一字节
// 返回值：
// 0：没有数据
// 1：获取1字节数据
uint8_t ReadOneByteFromBuff(char *data)
{
    if ((u8Uart1RxFinished != 0U) && (s16Uart1ReadIdx >= u32RxDataCount)) {
        s16Uart1ReadIdx = 0;
        u8Uart1RxFinished = 0U; // 已经被读取了
    }

    if (0U == u8Uart1RxFinished) {
        return 0U;
    }

    *data = (s16Uart1ReadIdx < COM1_RX_BLEN ? Com1RxBuff[s16Uart1ReadIdx] : 0U);
    s16Uart1ReadIdx++;

    return 1U;
}




