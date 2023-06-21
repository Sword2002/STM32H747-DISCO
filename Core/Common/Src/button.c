/*
 * STM32H747I-DISCO study
 * Copyright (C) 2023 Schneider-Electric.
 * All Rights Reserved.
 *
 */
#include "stm32h747i_discovery.h"
#include "debug_printf.h"

#include "FreeRTOS.h"	        // FreeRTOS	  
#include "event_groups.h"		// FreeRTOS event

extern EventGroupHandle_t xEvent_eFlag;



void ButtonAndJoykeyInit(void)
{
    // Wakeup button
    BSP_PB_Init(BUTTON_WAKEUP, BUTTON_MODE_EXTI);

    // Joy keys
    BSP_JOY_Init(JOY1, JOY_MODE_EXTI, JOY_SEL);
    BSP_JOY_Init(JOY1, JOY_MODE_EXTI, JOY_DOWN);
    BSP_JOY_Init(JOY1, JOY_MODE_EXTI, JOY_LEFT);
    BSP_JOY_Init(JOY1, JOY_MODE_EXTI, JOY_RIGHT);
    BSP_JOY_Init(JOY1, JOY_MODE_EXTI, JOY_UP);
}

// Wakeup button pressed ISR callback
//void BSP_PB_Callback(Button_TypeDef Button)
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    BaseType_t xHigherPriorityTaskWoken, xResult = pdFAIL;
    if ((GPIO_Pin & BUTTON_WAKEUP_PIN) == BUTTON_WAKEUP_PIN) {
        //printf2bufffromISR("WAKEUP Pressed\r\n");

        // 设置事件标志位, 让其他事件发送打印信息
        xHigherPriorityTaskWoken = pdFALSE;
        xResult = xEventGroupSetBitsFromISR(xEvent_eFlag, WAKEUP_EVENT, &xHigherPriorityTaskWoken);
    }
    else if ((GPIO_Pin & JOY1_SEL_PIN) == JOY1_SEL_PIN) {
        //printf2bufffromISR("JOY_SEL Key Pressed\r\n");

        // 设置事件标志位, 让其他事件发送打印信息
        xHigherPriorityTaskWoken = pdFALSE;
        xResult = xEventGroupSetBitsFromISR(xEvent_eFlag, JOYSEL_EVENT, &xHigherPriorityTaskWoken);
    }
    else if ((GPIO_Pin & JOY1_DOWN_PIN) == JOY1_DOWN_PIN) {
        printf2bufffromISR("JOY_DOWN Key Pressed\r\n");
    }
    else if ((GPIO_Pin & JOY1_LEFT_PIN) == JOY1_LEFT_PIN) {
        printf2bufffromISR("JOY_LEFT Key Pressed\r\n");
    }
    else if ((GPIO_Pin & JOY1_RIGHT_PIN) == JOY1_RIGHT_PIN) {
        printf2bufffromISR("JOY_RIGHT Key Pressed\r\n");
    }
    else if ((GPIO_Pin & JOY1_UP_PIN) == JOY1_UP_PIN) {
        printf2bufffromISR("JOY_UP Key Pressed\r\n");
    }
    else {
        printf2bufffromISR("Key Press has something wrong\r\n");
    }

    if (xResult == pdPASS) {
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}



