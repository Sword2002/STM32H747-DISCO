/**
  ******************************************************************************
  * @file    stm32h747i_discovery_qspi.h
  * @author  MCD Application Team
  * @brief   This file contains the common defines and functions prototypes for
  *          the stm32h747i_discovery_qspi.c driver.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2018 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef STM32H747I_DISCO_QSPI_H
#define STM32H747I_DISCO_QSPI_H

#ifdef __cplusplus
 extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32h747i_discovery_conf.h"
#include "stm32h747i_discovery_errno.h"
#include "../Components/mt25ql512abb/mt25ql512abb.h"

/** @addtogroup BSP
  * @{
  */

/** @addtogroup STM32H747I_DISCO
  * @{
  */

/** @addtogroup STM32H747I_DISCO_QSPI
  * @{
  */
/* Exported types ------------------------------------------------------------*/
/** @defgroup STM32H747I_DISCO_QSPI_Exported_Types Exported Types
  * @{
  */
#define BSP_QSPI_Info_t                 MT25QL512ABB_Info_t
#define BSP_QSPI_Interface_t            MT25QL512ABB_Interface_t
#define BSP_QSPI_Transfer_t             MT25QL512ABB_Transfer_t
#define BSP_QSPI_DualFlash_t            MT25QL512ABB_DualFlash_t
#define BSP_QSPI_ODS_t                  MT25QL512ABB_ODS_t
#define BSP_QSPI_AddSize_t              MT25QL512ABB_AddressSize_t

typedef enum
{
  BSP_QSPI_ERASE_8K   =  MT25QL512ABB_ERASE_4K ,       /*!< 8K size Sector erase = 2 x 4K as Dual flash mode is used for this board   */
  BSP_QSPI_ERASE_64K  =  MT25QL512ABB_ERASE_32K ,      /*!< 64K size Sector erase = 2 x 32K as Dual flash mode is used for this board */
  BSP_QSPI_ERASE_128K =  MT25QL512ABB_ERASE_64K ,      /*!< 128K size Sector erase = 2 x 64K as Dual mode is used for this board      */
  BSP_QSPI_ERASE_CHIP =  MT25QL512ABB_ERASE_BULK       /*!< Whole chip erase */

} BSP_QSPI_Erase_t;

typedef enum
{
  QSPI_ACCESS_NONE = 0,          /*!<  Instance not initialized,             */
  QSPI_ACCESS_INDIRECT,          /*!<  Instance use indirect mode access     */
  QSPI_ACCESS_MMP                /*!<  Instance use Memory Mapped Mode read  */
} BSP_QSPI_Access_t;

typedef struct
{
  BSP_QSPI_Access_t    IsInitialized;   /*!<  Instance access Flash method     */
  BSP_QSPI_Interface_t InterfaceMode;   /*!<  Flash Interface mode of Instance */
  BSP_QSPI_Transfer_t  TransferRate;    /*!<  Flash Transfer mode of Instance  */
  uint32_t             DualFlashMode;   /*!<  Flash dual mode                  */
  uint32_t             IsMspCallbacksValid;
} BSP_QSPI_Ctx_t;

typedef struct
{
  BSP_QSPI_Interface_t        InterfaceMode;   /*!<  Current Flash Interface mode */
  BSP_QSPI_Transfer_t         TransferRate;    /*!<  Current Flash Transfer mode  */
  BSP_QSPI_DualFlash_t        DualFlashMode;   /*!<  Dual Flash mode              */
} BSP_QSPI_Init_t;

typedef struct
{
  uint32_t FlashSize;
  uint32_t ClockPrescaler;
  uint32_t SampleShifting;
  uint32_t DualFlashMode;
}MX_QSPI_Init_t;
#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
typedef struct
{
 void(*pMspInitCb)(pQSPI_CallbackTypeDef);
 void(*pMspDeInitCb)(pQSPI_CallbackTypeDef);
}BSP_QSPI_Cb_t;
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */

/**
  * @}
  */

/* Exported constants --------------------------------------------------------*/
/** @defgroup STM32H747I_DISCO_QSPI_Exported_Constants Exported Constants
  * @{
  */
/* QSPI instances number */
#define QSPI_INSTANCES_NUMBER         1U

/* Definition for QSPI modes */
#define BSP_QSPI_SPI_MODE            (BSP_QSPI_Interface_t)MT25QL512ABB_SPI_MODE      /* 1 Cmd Line, 1 Address Line and 1 Data Line    */
#define BSP_QSPI_SPI_1I2O_MODE       (BSP_QSPI_Interface_t)MT25QL512ABB_SPI_1I2O_MODE /* 1 Cmd Line, 1 Address Line and 2 Data Lines   */
#define BSP_QSPI_SPI_2IO_MODE        (BSP_QSPI_Interface_t)MT25QL512ABB_SPI_2IO_MODE  /* 1 Cmd Line, 2 Address Lines and 2 Data Lines  */
#define BSP_QSPI_SPI_1I4O_MODE       (BSP_QSPI_Interface_t)MT25QL512ABB_SPI_1I4O_MODE /* 1 Cmd Line, 1 Address Line and 4 Data Lines   */
#define BSP_QSPI_SPI_4IO_MODE        (BSP_QSPI_Interface_t)MT25QL512ABB_SPI_4IO_MODE  /* 1 Cmd Line, 4 Address Lines and 4 Data Lines  */
#define BSP_QSPI_DPI_MODE            (BSP_QSPI_Interface_t)MT25QL512ABB_DPI_MODE      /* 2 Cmd Lines, 2 Address Lines and 2 Data Lines */
#define BSP_QSPI_QPI_MODE            (BSP_QSPI_Interface_t)MT25QL512ABB_QPI_MODE      /* 4 Cmd Lines, 4 Address Lines and 4 Data Lines */

/* Definition for QSPI transfer rates */
#define BSP_QSPI_STR_TRANSFER        (BSP_QSPI_Transfer_t)MT25QL512ABB_STR_TRANSFER /* Single Transfer Rate */
#define BSP_QSPI_DTR_TRANSFER        (BSP_QSPI_Transfer_t)MT25QL512ABB_DTR_TRANSFER /* Double Transfer Rate */

/* Definition for QSPI dual flash mode */
#define BSP_QSPI_DUALFLASH_DISABLE   (BSP_QSPI_DualFlash_t)MT25QL512ABB_DUALFLASH_DISABLE   /* Dual flash mode enabled  */
#define BSP_QSPI_DUALFLASH_ENABLE   (BSP_QSPI_DualFlash_t)MT25QL512ABB_DUALFLASH_ENABLE   /* Dual flash mode enabled  */
/* Definition for QSPI Flash ID */
#define BSP_QSPI_FLASH_ID            QSPI_FLASH_ID_1

/* QSPI block sizes for dual flash */
#define BSP_QSPI_BLOCK_8K            MT25QL512ABB_SUBSECTOR_4K
#define BSP_QSPI_BLOCK_64K           MT25QL512ABB_SUBSECTOR_32K
#define BSP_QSPI_BLOCK_128K          MT25QL512ABB_SECTOR_64K

/* QSPI address bytes */
#define BSP_QSPI_ADDSIZE_3BYTES        (BSP_QSPI_AddSize_t)MT25QL512ABB_3BYTES_SIZE /* Single Transfer Rate */
#define BSP_QSPI_ADDSIZE_4BYTES        (BSP_QSPI_AddSize_t)MT25QL512ABB_4BYTES_SIZE /* Double Transfer Rate */


/* Definition for QSPI clock resources */
#define QSPI_CLK_ENABLE()              __HAL_RCC_QSPI_CLK_ENABLE()
#define QSPI_CLK_DISABLE()             __HAL_RCC_QSPI_CLK_DISABLE()
#define QSPI_CLK_GPIO_CLK_ENABLE()     __HAL_RCC_GPIOB_CLK_ENABLE()
#define QSPI_BK1_CS_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOG_CLK_ENABLE()
#define QSPI_BK1_D0_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOD_CLK_ENABLE()
#define QSPI_BK1_D1_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_BK1_D2_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_BK1_D3_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOF_CLK_ENABLE()
#define QSPI_BK2_CS_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOG_CLK_ENABLE()
#define QSPI_BK2_D0_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOH_CLK_ENABLE()
#define QSPI_BK2_D1_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOH_CLK_ENABLE()
#define QSPI_BK2_D2_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOG_CLK_ENABLE()
#define QSPI_BK2_D3_GPIO_CLK_ENABLE()  __HAL_RCC_GPIOG_CLK_ENABLE()


#define QSPI_FORCE_RESET()         __HAL_RCC_QSPI_FORCE_RESET()
#define QSPI_RELEASE_RESET()       __HAL_RCC_QSPI_RELEASE_RESET()

/* Definition for QSPI Pins */
#define QSPI_CLK_PIN               GPIO_PIN_2
#define QSPI_CLK_GPIO_PORT         GPIOB
/* Bank 1 */
#define QSPI_BK1_CS_PIN            GPIO_PIN_6
#define QSPI_BK1_CS_GPIO_PORT      GPIOG
#define QSPI_BK1_D0_PIN            GPIO_PIN_11
#define QSPI_BK1_D0_GPIO_PORT      GPIOD
#define QSPI_BK1_D1_PIN            GPIO_PIN_9
#define QSPI_BK1_D1_GPIO_PORT      GPIOF
#define QSPI_BK1_D2_PIN            GPIO_PIN_7
#define QSPI_BK1_D2_GPIO_PORT      GPIOF
#define QSPI_BK1_D3_PIN            GPIO_PIN_6
#define QSPI_BK1_D3_GPIO_PORT      GPIOF

/* Bank 2 */
#define QSPI_BK2_CS_PIN            GPIO_PIN_6
#define QSPI_BK2_CS_GPIO_PORT      GPIOG
#define QSPI_BK2_D0_PIN            GPIO_PIN_2
#define QSPI_BK2_D0_GPIO_PORT      GPIOH
#define QSPI_BK2_D1_PIN            GPIO_PIN_3
#define QSPI_BK2_D1_GPIO_PORT      GPIOH
#define QSPI_BK2_D2_PIN            GPIO_PIN_9
#define QSPI_BK2_D2_GPIO_PORT      GPIOG
#define QSPI_BK2_D3_PIN            GPIO_PIN_14
#define QSPI_BK2_D3_GPIO_PORT      GPIOG




// QSPI和External flash的全局配置
#define BSP_DF_MODE               BSP_QSPI_DUALFLASH_ENABLE  // Dual flash mode
#define BSP_IT_MODE               BSP_QSPI_QPI_MODE          // SPI Mode
#define BSP_TF_RATE               BSP_QSPI_STR_TRANSFER      // Transfer mode
#define BSP_ADD_BYTE              BSP_QSPI_ADDSIZE_4BYTES    // Address mode


/* MT25QL512ABB Micron memory 64M Bytes, Dual flash need *2 */
#define QSPI_FLASH_SIZE_BYTE       (MT25QL512ABB_FLASH_SIZE << (BSP_DF_MODE == BSP_QSPI_DUALFLASH_ENABLE ? 1 : 0))

// Address size need 27bit
#define QSPI_FLASH_SIZE_ADDR       26  // 2^(26+1) = 128 * 1024 * 1024, 0 <-----> 0x7FF FFFF


#define QSPI_PAGE_SIZE             (MT25QL512ABB_PAGE_SIZE << (BSP_DF_MODE == BSP_QSPI_DUALFLASH_ENABLE ? 1 : 0))

/* QSPI Base Address */
#define QSPI_BASE_ADDRESS          0x90000000   // 内存映射模式使用

#define QSPI_FLASH_ADDMAX          ((uint32_t)0x7FFFFFFU) // (((uint32_t)1U << QSPI_FLASH_SIZE_ADDR) - 1U)



/**
  * @}
  */

/** @addtogroup STM32H747I_DISCO_QSPI_Exported_Variables
  * @{
  */
extern QSPI_HandleTypeDef hqspi;
extern BSP_QSPI_Ctx_t     QSPI_Ctx[];
/**
  * @}
  */

/* Exported functions --------------------------------------------------------*/
/** @addtogroup STM32H747I_DISCO_QSPI_Exported_Functions
  * @{
  */
int32_t BSP_QSPI_Init(uint32_t Instance, BSP_QSPI_Init_t *Init);
int32_t BSP_QSPI_DeInit(uint32_t Instance);
#if (USE_HAL_QSPI_REGISTER_CALLBACKS == 1)
int32_t BSP_QSPI_RegisterMspCallbacks (uint32_t Instance, BSP_QSPI_Cb_t *CallBacks);
int32_t BSP_QSPI_RegisterDefaultMspCallbacks (uint32_t Instance);
#endif /* (USE_HAL_QSPI_REGISTER_CALLBACKS == 1) */
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

/* These functions can be modified in case the current settings
   need to be changed for specific application needs */
HAL_StatusTypeDef MX_QSPI_Init(QSPI_HandleTypeDef *hQspi, MX_QSPI_Init_t *Config);

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
#ifdef __cplusplus
}
#endif

#endif /*STM32H747I_DISCO_QSPI_H */
