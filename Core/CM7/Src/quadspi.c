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
#include "stm32h747i_discovery_qspi.h"
#include "mt25ql512abb/mt25ql512abb.h"
#include "shell_port.h"
#include <string.h>
#include "stdio.h"

/* QUADSPI init function */
void MX_QUADSPI_Init(void)
{
  BSP_QSPI_Init_t Init;
  Init.InterfaceMode = BSP_QSPI_QPI_MODE;
  Init.TransferRate = BSP_QSPI_STR_TRANSFER;
  Init.DualFlashMode = BSP_QSPI_DUALFLASH_ENABLE;

  if (BSP_QSPI_Init(0, &Init) != BSP_ERROR_NONE)
  {
    Error_Handler();
  }
}



/* "stm32h747i_discovery_qspi.h" 提供的可用接口
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

void EXTFLASH_Test_Write(void)
{
  int32_t ret = BSP_ERROR_NONE;
  uint8_t data[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88};
  ret = BSP_QSPI_Write(0, data, QSPI_BASE_ADDRESS, 8);
  
  char buff[64];
  sprintf(buff, "QSPI Write test result = %d.\n\r", ret);
  user_shellprintf(buff);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 QSPIW, EXTFLASH_Test_Write, external flash write test);


// 通过QSPI读取FLASH地址数据
void EXTFLASH_Test_Read(void)
{
  int32_t ret = BSP_ERROR_NONE;
  uint8_t data[8];
  ret = BSP_QSPI_Read(0, data, QSPI_BASE_ADDRESS, 8);
  
  char buff[64];
  sprintf(buff, "QSPI Read test result = %d, d0=%d.\n\r", ret, data[0]);
  user_shellprintf(buff);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 QSPIR, EXTFLASH_Test_Read, external flash write test);


// 获取FLASH的配置信息
void EXTFLASH_Test_ReadInd(void)
{
  BSP_QSPI_Info_t pInfo;
  MT25QL512ABB_GetFlashInfo(&pInfo);
  
  char buff[128];
  uint32_t size = (uint32_t)POSITION_VAL((uint32_t)pInfo.FlashSize) - 1U;
  user_shellprintf("Flash information:\n\r");
  sprintf(buff, "Size = %d, ESS = %d, ESN = %d, PPS = %d, PPN = %d \n\r", \
          size, pInfo.EraseSectorSize, pInfo.EraseSectorsNumber, \
          pInfo.ProgPageSize, pInfo.ProgPagesNumber);
  user_shellprintf(buff);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 eflashi, EXTFLASH_Test_ReadInd, external flash information);


// 通过QSPI读取FLASH地址数据
void EXTFLASH_Test_ReadStatus(void)
{
  int32_t ret = 0;
 ret = BSP_QSPI_GetStatus(0);
  
  char buff[64];
  sprintf(buff, "Read flash status result = %d \n\r", ret);
  user_shellprintf(buff);
}
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 eflashs, EXTFLASH_Test_ReadStatus, external flash status);

