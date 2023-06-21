/****************************************************************************************
 *         File : HdwITPriorities.h
 *  Description :
 *
 *
 *
 ****************************************************************************************/

#ifndef __HDWITPRIORITIES_H_
#define __HDWITPRIORITIES_H_

/****************************************************************************************
 * 				IT Management Declaration
 ****************************************************************************************/
// Define Available Priority levels (HDW_IT_PRIORITY_0 is always the Highest priority)
#define HDW_IT_DISABLED         // Not supported
#define HDW_IT_PRIORITY_0       0x00U
#define HDW_IT_PRIORITY_1       0x01U
#define HDW_IT_PRIORITY_2       0x02U
#define HDW_IT_PRIORITY_3       0x03U
#define HDW_IT_PRIORITY_4       0x04U
#define HDW_IT_PRIORITY_5       0x05U
#define HDW_IT_PRIORITY_6       0x06U
#define HDW_IT_PRIORITY_7       0x07U

#define HDW_IT_SUBPRIORITY_0    (0x00U << 4)
#define HDW_IT_SUBPRIORITY_1    (0x01U << 4)

static inline unsigned char HDW_IT_GETPRIORITY(unsigned short _GlobalPriority) { return (_GlobalPriority & 0x0FU); }
static inline unsigned char HDW_IT_GETSUBPRIORITY(unsigned short _GlobalPriority)  { return((_GlobalPriority & 0xF0U) >> 4); }    // Sub-Priority Highest 4 bits

// Priority Grouping
#define HDW_IT_PRIORITY_GROUPING                (NVIC_PRIORITYGROUP_3)                  // 3bits Priority + 1bit Sub-Priority


/****************************************************************************************
 * 				IT Priorities Declaration
 ****************************************************************************************/
// ISL
#define HDW_IT_PRIORITY_ISL_RXDMA               (HDW_IT_PRIORITY_2 + HDW_IT_SUBPRIORITY_0)      // EndRx + relaunch new Rx transfer
#define HDW_IT_PRIORITY_ISL_TXDMA               (HDW_IT_PRIORITY_2 + HDW_IT_SUBPRIORITY_0)      // EndTx (managed by HAL)
#define HDW_IT_PRIORITY_ISL_USART               (HDW_IT_PRIORITY_2 + HDW_IT_SUBPRIORITY_0)      // Rx Timeout + relaunch new Rx transfer
#define HDW_IT_PRIORITY_ISL_TIMEOUT             (HDW_IT_PRIORITY_2 + HDW_IT_SUBPRIORITY_0)      // Frame Timeout manage by Hardware

// Modbus RJ45
#define HDW_IT_PRIORITY_MBU_RXDMA               (HDW_IT_PRIORITY_5 + HDW_IT_SUBPRIORITY_0)      // DMA used
#define HDW_IT_PRIORITY_MBU_TXDMA               (HDW_IT_PRIORITY_5 + HDW_IT_SUBPRIORITY_0)      // DMA used
#define HDW_IT_PRIORITY_MBU_USART               (HDW_IT_PRIORITY_5 + HDW_IT_SUBPRIORITY_0)      // Rx Timeout

// Shell
#define HDW_IT_PRIORITY_SHELL_RXDMA               (HDW_IT_PRIORITY_5 + HDW_IT_SUBPRIORITY_0)     // DMA used
#define HDW_IT_PRIORITY_SHELL_TXDMA               (HDW_IT_PRIORITY_5 + HDW_IT_SUBPRIORITY_0)     // DMA used
#define HDW_IT_PRIORITY_SHELL_USART               (HDW_IT_PRIORITY_5 + HDW_IT_SUBPRIORITY_0)     // Rx, Tx, Rx Timeout

// Tasks
#define HDW_IT_PRIORITY_TIM2                    (HDW_IT_PRIORITY_1 + HDW_IT_SUBPRIORITY_0)
#define HDW_IT_PRIORITY_TSK_FAST                (HDW_IT_PRIORITY_1)
#define HDW_IT_PRIORITY_TSK_SLOW                (HDW_IT_PRIORITY_4)
#define HDW_IT_PRIORITY_TSK_APP                 (HDW_IT_PRIORITY_6)

#define HDW_IT_PRIORITY_WWDG                    (HDW_IT_PRIORITY_0)

#define HDW_IT_PRIORITY_SYSTICK                 (HDW_IT_PRIORITY_0)

// CAN Ioc
#define HDW_IT_PRIORITY_CAN_IOC                 (HDW_IT_PRIORITY_2)
#define HDW_IT_PRIORITY_CAN_SCE_IOC             (HDW_IT_PRIORITY_0)

// SPI for Ethernet card Embedded and Option
#define HWD_IT_PRIORITY_ETH_RXDMA               (HDW_IT_PRIORITY_2)
#define HWD_IT_PRIORITY_ETH_TXDMA               (HDW_IT_PRIORITY_2)
#define HWD_IT_PRIORITY_ETH_OPT_RXDMA           (HDW_IT_PRIORITY_2)
#define HWD_IT_PRIORITY_ETH_OPT_TXDMA           (HDW_IT_PRIORITY_2)

// ADC
#define HDW_IT_PRIORITY_ADC                     (HDW_IT_PRIORITY_5 + HDW_IT_SUBPRIORITY_0)


// Button and KEY
#define HDW_IT_PRIORITY_EXTI_KEY                (HDW_IT_PRIORITY_6 + HDW_IT_SUBPRIORITY_0)



#endif //__HDWITPRIORITIES_H_
