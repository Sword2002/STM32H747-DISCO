/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "eth.h"
#include "hdmi_cec.h"
#include "quadspi.h"
#include "rtc.h"
#include "sai.h"
#include "sdmmc.h"
#include "spdifrx.h"
#include "spi.h"
#include "tim.h"
#include "usb_otg.h"
#include "gpio.h"
#include "fmc.h"
#include "stm32h747i_discovery.h"



#ifndef HSEM_ID_0
#define HSEM_ID_0 (0U) /* HW semaphore 0*/
#endif

void osTaskInit(void);
long Cm4RunCnt = 0;
/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /*HW semaphore Clock enable*/
  /* Activate HSEM notification for Cortex-M4*/
  __HAL_RCC_HSEM_CLK_ENABLE();
  HAL_HSEM_ActivateNotification(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

  /*
  Domain D2 goes to STOP mode (Cortex-M4 in deep-sleep) waiting for Cortex-M7 to
  perform system initialization (system clock config, external memory configuration.. )
  */
  HAL_PWREx_ClearPendingEvent();
  HAL_PWREx_EnterSTOPMode(PWR_MAINREGULATOR_ON, PWR_STOPENTRY_WFE, PWR_D2_DOMAIN);
  /* Clear HSEM flag */
  __HAL_HSEM_CLEAR_FLAG(__HAL_HSEM_SEMID_TO_MASK(HSEM_ID_0));

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* Add Cortex-M4 user application code here */ 
  BSP_LED_Init(LED3);
  BSP_LED_On(LED3);
  HAL_Delay(2000); 
  BSP_LED_Off(LED3);
  
  /* CM4 takes HW semaphore 0 to inform CM7 that he finished his job */
  /* Do not forget to release the HW semaphore 0 once needed */
  HAL_HSEM_FastTake(HSEM_ID_0);
  HAL_HSEM_Release(HSEM_ID_0, 0); 

  /* Initialize all configured peripherals */
  // MX_RTC_Init();
  MX_UART8_Init();

  osTaskInit();  // FreeRTOS
  // 程序如果运行到这里,RTOS出现了错误
  while (1)
  {
    Cm4RunCnt++;
    if (Cm4RunCnt < 500000) {
      BSP_LED_Off(LED3);
    } else {
      BSP_LED_On(LED3);
    }

    Cm4RunCnt %= 1000000;
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  long delayCnt = 0;
  __disable_irq();
  while (1)
  {
    delayCnt++;
    if (delayCnt < 200000) {
      BSP_LED_On(LED3);
    } else {
      BSP_LED_Off(LED3);
    }
    
    delayCnt %= 1000000;
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
