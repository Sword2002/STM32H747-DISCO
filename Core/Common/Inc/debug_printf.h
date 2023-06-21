/**
  ******************************************************************************
  * @file    debug_printf.h
  * @author  Drive FW team
  * @brief   Header file of debug information printf.
  ******************************************************************************
  * @attention
  *
  * Copyright (C) 2023 Schneider-Electric.
  * All rights reserved.
  *
  *
  ******************************************************************************
  */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __DEBUG_PRINTF_H__
#define __DEBUG_PRINTF_H__

#ifdef __cplusplus
extern "C" {
#endif

#define WAKEUP_EVENT   (0x0001U)        // 按键事件
#define JOYSEL_EVENT   (0x0002U)        // 按键事件
#define DEBUG_PRINT_EN (1)              // 使能/禁能调试信息打印

#if (DEBUG_PRINT_EN == 1)
void debug_printf(char const *str);
#define printf(str) debug_printf(str)
#else
#define printf(str)
#endif

void RTOS_DebugPrintTask(void);
void printf2buff(char const *str);
void printf2bufffromISR(char const *str);
void printf2buffatinit(char const *str);
void nprintf2buff(char const *str, uint16_t len);



#ifdef __cplusplus
}
#endif


#endif /* __DEBUG_PRINTF_H__ */
