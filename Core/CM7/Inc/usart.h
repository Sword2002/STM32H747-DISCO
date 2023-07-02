/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    usart.h
  * @brief   This file contains all the function prototypes for
  *          the usart.c file
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
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __USART_H__
#define __USART_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include <stdbool.h>

/* USER CODE BEGIN Includes */
#define PRINT_BSIZE (512)    // 长度需要是32字节的整数倍,使用D-CACHE需要32字节对齐
/* USER CODE END Includes */

extern UART_HandleTypeDef huart1;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart1_rx;
extern uint8_t Com1RxBuff[];


/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

void MX_USART1_UART_Init(void);

/* USER CODE BEGIN Prototypes */
bool Uart_bReceive_NonBlocking(UART_HandleTypeDef*  UARTx, uint8_t *pDstAddr, uint32_t u32DataLen);
bool Uart_bSend_NonBlocking(UART_HandleTypeDef*  UARTx, uint8_t *pSrcAddr, uint32_t u32DataLen);
uint8_t GetUart1TxStatus(void);
uint8_t GetUart1TxStatus(void);
uint8_t ReadOneByteFromBuff(char *data);
void PutUart1ToLisen(void);

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif

#endif /* __USART_H__ */

