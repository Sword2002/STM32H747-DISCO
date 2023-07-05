/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    quadspi.c
  * @brief   This file provides code for the configuration
  *          of the QUADSPI instances.
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
#include "quadspi.h"
//#include "stm32h747i_discovery_qspi.h"
#include "stm32h747i_discovery_mt25ql_qspi.h"
//#include "mt25tl01g/mt25tl01g.h"
#include "mt25ql512abb/mt25ql512abb.h"
#include "shell_port.h"
#include <string.h>
#include "stdio.h"


#define QSPI_BST_SHELLTEST_EN 0        // 使能Shell进行FLASH的BSP层接口测试

extern QSPI_HandleTypeDef hqspi;

/* QUADSPI init function */
void MX_QUADSPI_Init(void)
{
  BSP_QSPI_Init_t init ;
  init.InterfaceMode = BSP_IT_MODE;
  init.TransferRate = BSP_TF_RATE;
  init.DualFlashMode = BSP_DF_MODE;
  /* Initialize the QSPI */
  BSP_QSPI_Init(0,&init);
}

// 自定义HAL层的MsInit
void HAL_QSPI_MspInit(QSPI_HandleTypeDef* qspiHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};
  if(qspiHandle->Instance==QUADSPI)
  {
  /* USER CODE BEGIN QUADSPI_MspInit 0 */

  /* USER CODE END QUADSPI_MspInit 0 */

  /** Initializes the peripherals clock
  */
    PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_QSPI;
    PeriphClkInitStruct.QspiClockSelection = RCC_QSPICLKSOURCE_D1HCLK;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
    {
      Error_Handler();
    }

    /* QUADSPI clock enable */
    __HAL_RCC_QSPI_CLK_ENABLE();
    __HAL_RCC_QSPI_FORCE_RESET();
    __HAL_RCC_QSPI_RELEASE_RESET();

    __HAL_RCC_GPIOG_CLK_ENABLE();
    __HAL_RCC_GPIOF_CLK_ENABLE();
    __HAL_RCC_GPIOH_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOD_CLK_ENABLE();
    /**QUADSPI GPIO Configuration
    PG9     ------> QUADSPI_BK2_IO2
    PG14     ------> QUADSPI_BK2_IO3
    PG6     ------> QUADSPI_BK1_NCS
    PF6     ------> QUADSPI_BK1_IO3
    PF7     ------> QUADSPI_BK1_IO2
    PF9     ------> QUADSPI_BK1_IO1
    PH2     ------> QUADSPI_BK2_IO0
    PH3     ------> QUADSPI_BK2_IO1
    PB2     ------> QUADSPI_CLK
    PD11     ------> QUADSPI_BK1_IO0
    */
    GPIO_InitStruct.Pin = QSPI_BK2_D2_PIN|QSPI_BK2_D3_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = QSPI_BK1_CS_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOG, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = QSPI_BK1_D2_PIN|QSPI_BK1_D3_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = QSPI_BK1_D1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF10_QUADSPI;
    HAL_GPIO_Init(GPIOF, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = QSPI_BK2_D0_PIN|QSPI_BK2_D1_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOH, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = QSPI_CLK_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    GPIO_InitStruct.Pin = QSPI_BK1_D0_PIN;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;
    GPIO_InitStruct.Alternate = GPIO_AF9_QUADSPI;
    HAL_GPIO_Init(GPIOD, &GPIO_InitStruct);

    /* QUADSPI interrupt Init */
    HAL_NVIC_SetPriority(QUADSPI_IRQn, 15, 0);
    HAL_NVIC_EnableIRQ(QUADSPI_IRQn);
  /* USER CODE BEGIN QUADSPI_MspInit 1 */

  /* USER CODE END QUADSPI_MspInit 1 */
  }
}

// 自定义HAL层的MsDeInit
void HAL_QSPI_MspDeInit(QSPI_HandleTypeDef* qspiHandle)
{
  if(qspiHandle->Instance==QUADSPI)
  {
  /* USER CODE BEGIN QUADSPI_MspDeInit 0 */
    __HAL_RCC_QSPI_FORCE_RESET();
    __HAL_RCC_QSPI_RELEASE_RESET();
  /* USER CODE END QUADSPI_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_QSPI_CLK_DISABLE();

    /**QUADSPI GPIO Configuration
    PG9     ------> QUADSPI_BK2_IO2
    PG14     ------> QUADSPI_BK2_IO3
    PG6     ------> QUADSPI_BK1_NCS
    PF6     ------> QUADSPI_BK1_IO3
    PF7     ------> QUADSPI_BK1_IO2
    PF9     ------> QUADSPI_BK1_IO1
    PH2     ------> QUADSPI_BK2_IO0
    PH3     ------> QUADSPI_BK2_IO1
    PB2     ------> QUADSPI_CLK
    PD11     ------> QUADSPI_BK1_IO0
    */
    HAL_GPIO_DeInit(GPIOG, QSPI_BK2_D2_PIN|QSPI_BK2_D3_PIN|QSPI_BK1_CS_PIN);

    HAL_GPIO_DeInit(GPIOF, QSPI_BK1_D3_PIN|QSPI_BK1_D2_PIN|QSPI_BK1_D1_PIN);

    HAL_GPIO_DeInit(GPIOH, QSPI_BK2_D0_PIN|QSPI_BK2_D1_PIN);

    HAL_GPIO_DeInit(GPIOB, QSPI_CLK_PIN);

    HAL_GPIO_DeInit(GPIOD, QSPI_BK1_D0_PIN);

    /* QUADSPI interrupt Deinit */
    HAL_NVIC_DisableIRQ(QUADSPI_IRQn);
  /* USER CODE BEGIN QUADSPI_MspDeInit 1 */

  /* USER CODE END QUADSPI_MspDeInit 1 */
  }
}

#if (QSPI_BST_SHELLTEST_EN == 1)
/*
* ---------------------- BSP Functions testing with shell ------------------------------------
* BSP interface:
*
int32_t BSP_QSPI_Read(uint32_t Instance, uint8_t *pData, uint32_t ReadAddr, uint32_t Size);
int32_t BSP_QSPI_Write(uint32_t Instance, uint8_t *pData, uint32_t WriteAddr, uint32_t Size);
int32_t BSP_QSPI_EraseBlock(uint32_t Instance, uint32_t BlockAddress, BSP_QSPI_Erase_t BlockSize);
int32_t BSP_QSPI_EraseChip(uint32_t Instance);
int32_t BSP_QSPI_GetStatus(uint32_t Instance);
int32_t BSP_QSPI_GetInfo(uint32_t Instance, BSP_QSPI_Info_t *pInfo);
int32_t BSP_QSPI_EnableMemoryMappedMode(uint32_t Instance);
int32_t BSP_QSPI_DisableMemoryMappedMode(uint32_t Instance);
int32_t BSP_QSPI_ReadID(uint32_t Instance, uint8_t *Id);
int32_t BSP_QSPI_ConfigFlash(uint32_t Instance, BSP_QSPI_Interface_t Mode, BSP_QSPI_Transfer_t Rate);
*
*/

// 测试写到External flash
// 1. 选择从0地址开始 或从结尾-8开始
void EXTFLASH_Test_Write(uint8_t addLocal)
{
  int32_t ret = BSP_ERROR_NONE;
  uint8_t data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};

  if (addLocal == 1U) {
    ret = BSP_QSPI_Write(0, data, (QSPI_FLASH_ADDMAX - 8), 8);
  } else if (addLocal == 7U) {
    ret = BSP_QSPI_Write(0, data, 0, 7); // 测试写奇数个
  } else {
    ret = BSP_QSPI_Write(0, data, 0, 8);
  }
  
  char buff[64];
  sprintf(buff, "Write external flash test result = %d.\n\r", ret);
  user_shellprintf(buff);
}


// 测试从External flash读取数据
// 1. 选择从0地址开始 或从结尾-8开始
void EXTFLASH_Test_Read(uint8_t addLocal)
{
  int32_t ret = BSP_ERROR_NONE;
  uint8_t data[8] = {0, 0, 0, 0, 0, 0, 0, 0};

  if (addLocal == 1U) {
    ret = BSP_QSPI_Read(0, data, (QSPI_FLASH_ADDMAX - 8), 8);
  } else if (addLocal == 7U) {
    ret = BSP_QSPI_Read(0, data, 0, 7); // 测试读奇数个
  } else {
    ret = BSP_QSPI_Read(0, data, 0, 8);
  }
  
  char buff[64];
  sprintf(buff, "Read external flash: result=%d\n\r", ret);
  user_shellprintf(buff);
  for (int8_t i = 0; i < 8; i++) {
    sprintf(buff, "Date[%d]=0x%02X.\n\r", i, data[i]);
    user_shellprintf(buff);
  }
}

// 擦除BLOCK接口测试
// 1.Block的序号
// 2.选择Block的Size
void EXTFLASH_Test_BlockErase(uint32_t BlockId, uint8_t Size)
{
  if (BlockId >= BSP_FS_BLOCK_COUNT) {
    return;
  }

  int32_t ret = BSP_ERROR_NONE;
  if (Size == 1) {
    ret = BSP_QSPI_EraseBlock(0, (BlockId * MT25QL512ABB_SUBSECTOR_32K * 2), BSP_QSPI_ERASE_64K);
  } else if (Size == 2) {
    ret = BSP_QSPI_EraseBlock(0, (BlockId * MT25QL512ABB_SECTOR_64K * 2), BSP_QSPI_ERASE_128K);
  } else {
    ret = BSP_QSPI_EraseBlock(0, (BlockId * MT25QL512ABB_SUBSECTOR_4K * 2), BSP_QSPI_ERASE_8K);
  }

  char buff[64];
  sprintf(buff, "Erase block: result=%d\n\r", ret);
  user_shellprintf(buff);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 eferaseb, EXTFLASH_Test_BlockErase, external flash block erase);


// 擦除整片接口测试
void EXTFLASH_Test_ChipErase(void)
{
  int32_t ret = BSP_ERROR_NONE;
  ret = BSP_QSPI_EraseChip(0);
  
  char buff[64];
  sprintf(buff, "Erase chip: result=%d\n\r", ret);
  user_shellprintf(buff);
}

// 通过QSPI读取FLASH地址数据
// Dual flash mode have 6 Bytes
void EXTFLASH_Test_ReadID(void)
{
  int8_t i;
  int32_t ret = BSP_ERROR_NONE;
  uint8_t data[8]={0};
  char buff[64];

  ret = MT25QL512ABB_ReadID(&hqspi, BSP_QSPI_QPI_MODE, data, BSP_DF_MODE);
  
  sprintf(buff, "QSPI Read the flash ID:result=%d\n\r", ret);
  user_shellprintf(buff);
  for (i = 0; i < 6; i++) {
    sprintf(buff, "Date[%d]=0x%02X.\n\r", i, data[i]);
    user_shellprintf(buff);
  }
}

// 获取FLASH的配置信息
void EXTFLASH_Test_ReadInfo(void)
{
  BSP_QSPI_Info_t pInfo;
  //MT25QL512ABB_GetFlashInfo(&pInfo);
  MT25QL512ABB_GetFlashInfo(&pInfo);
  
  char buff[128];
  uint32_t size = (uint32_t)POSITION_VAL((uint32_t)pInfo.FlashSize) - 1U;
  user_shellprintf("Flash information:\n\r");
  sprintf(buff, "Size = %d, ESS = %d, ESN = %d, PPS = %d, PPN = %d \n\r", \
          size, pInfo.EraseSectorSize, pInfo.EraseSectorsNumber, \
          pInfo.ProgPageSize, pInfo.ProgPagesNumber);
  user_shellprintf(buff);
}


// 通过QSPI读取FLASH地址数据
void EXTFLASH_Test_ReadStatus(void)
{
  int8_t i;
  int32_t ret = BSP_ERROR_NONE;
  uint8_t data[8]={0,0,0,0,0,0,0,0};
  char buff[64];

  ret = MT25QL512ABB_ReadStatusRegister(&hqspi, BSP_QSPI_QPI_MODE, BSP_DF_MODE, data);
  
  sprintf(buff, "QSPI Read the flash status:result=%d\n\r", ret);
  user_shellprintf(buff);
  for (i = 0; i < 8; i++) {
    sprintf(buff, "Date[%d]=0x%02X.\n\r", i, data[i]);
    user_shellprintf(buff);
  }
}

// 测试POSITION_VAL()这个宏
void MACRO_Test_Val(uint32_t data)
{
  uint8_t ret = POSITION_VAL(data);

  char buff[64];
  sprintf(buff, "Val result = %d\n\r", ret);
  user_shellprintf(buff);
}


void EXTFLASH_Test_EnableMemoryMappedMode(void)
{
  BSP_QSPI_EnableMemoryMappedMode(0);
}

void EXTFLASH_Test_DisableMemoryMappedMode(void)
{
  BSP_QSPI_DisableMemoryMappedMode(0);
}



SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 efwrite, EXTFLASH_Test_Write, external flash write test);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 efread, EXTFLASH_Test_Read, external flash read test);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 eferasec, EXTFLASH_Test_ChipErase, external flash chip erase);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 efid, EXTFLASH_Test_ReadID, Read external flash ID);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 efinfo, EXTFLASH_Test_ReadInfo, Read external flash information);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 efstatus, EXTFLASH_Test_ReadStatus, external flash status);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 mval, MACRO_Test_Val, Macro val test);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 efmem, EXTFLASH_Test_EnableMemoryMappedMode, Enter Memory mode test);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 efdmem, EXTFLASH_Test_DisableMemoryMappedMode, Exit Memory mode test);

#endif

// -------- end of file -------