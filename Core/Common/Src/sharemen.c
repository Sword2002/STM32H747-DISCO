/*
 * STM32H747I-DISCO study
 * Copyright (C) 2023 Schneider-Electric.
 * All Rights Reserved.
 *
 */
#include <stdint.h>

// SRAM4:   0x38000000 <----> 0x3800FFFF 64kByte
// SRAM3:   0x30040000 <----> 0x30047FFF 32kByte
// SRAM2:   0x30020000 <----> 0x3003FFFF 128kByte
// SRAM1:   0x30000000 <----> 0x3001FFFF 128kByte
// AXI RAM: 0x24000000 <----> 0x2407FFFF 512kByte
#define SHARE_MEM_START     (0x38000000)
#define M4_U32DATA_NUM      (64)
#define M7_U32DATA_NUM      (64)
#define SDATA_STATUS_EMPTY   (0) // status: 0 = empty
#define SDATA_STATUS_HASDATA (1) // status: 1 = has data
#define SDATA_STATUS_LOCK    (2) // status: 2 = locked



typedef struct _MEM_Shared_Data_ {
    uint8_t  u8Sts4to7;                         // status: 0 = empty, 1 = has data, 2 = locked (CM4-CM7)
    uint8_t  u8Sts7to4;                         // status: 0 = empty, 1 = has data, 2 = locked (CM7-CM4)
    uint32_t au32M4toM7[M4_U32DATA_NUM];        // 256 bytes from CM4 to CM7
    uint32_t au32M7toM4[M7_U32DATA_NUM];        // 256 bytes from CM7 to CM4
} MEM_Shared_Data_t;
 


#if defined ( __ICCARM__ ) /*!< IAR Compiler */

#pragma location=SHARE_MEM_START
__root volatile MEM_Shared_Data_t M7M4ShareDate;
// volatile MEM_Shared_Data_t M7M4_ShareDate @ SHARE_MEM_START; // 这种方法也可以写

#elif defined ( __CC_ARM )  /* MDK ARM Compiler */

__attribute__((at(SHARE_MEM_START))) volatile MEM_Shared_Data_t M7M4ShareDate;

#elif defined ( __GNUC__ ) /* GNU Compiler */

volatile MEM_Shared_Data_t M7M4ShareDate __attribute__((section(".sharemem")));

#endif

volatile MEM_Shared_Data_t *const pShareData = &M7M4ShareDate;


//----------------- used in Core M4 ---------------------
#if defined (CORE_CM4)
//-------------------------------------------------------


uint32_t *ReadM7Data(void);           // get data from M7 to M4
void WriteM4Data(uint32_t *dataBuff); // put data from M4 to M7

// get data from M7 to M4 buffer
uint32_t *ReadM7Data(void)
{
	static uint32_t buffer[M7_U32DATA_NUM];                     // buffer to receive data
	if (pShareData->u8Sts7to4 == SDATA_STATUS_HASDATA)                 // if M7 to M4 buffer has data
	{
		pShareData->u8Sts7to4 = SDATA_STATUS_LOCK;                  // lock the M4 to M4 buffer
		for(int n = 0; n < M7_U32DATA_NUM; n++)
		{
			buffer[n] = pShareData->au32M7toM4[n];  // transfer data
			pShareData->au32M7toM4[n] = 0;          // clear M7 to M4 buffer
		}
		pShareData->u8Sts7to4 = SDATA_STATUS_EMPTY;                  // M4 to M4 buffer is empty
	}
	return buffer;                                  // return the buffer (pointer)
}

// send data from M4 to M7
void WriteM4Data(uint32_t *dataBuff)
{
	if (pShareData->u8Sts4to7 == SDATA_STATUS_EMPTY)                   // if M4 to M4 buffer is empty
	{
		pShareData->u8Sts4to7 = SDATA_STATUS_LOCK;                    // lock the M4 to M7 buffer
		for(int n = 0; n < M4_U32DATA_NUM; n++)
		{
			pShareData->au32M4toM7[n] = dataBuff[n];  // transfer data
			dataBuff[n] = 0;                          // clear M4 to M7 buffer
		}
		pShareData->u8Sts4to7 = SDATA_STATUS_HASDATA;                    // M4 to M7 buffer has data
	}
}

//----------------- used in Core M7 ---------------------
#else
//-------------------------------------------------------

uint32_t *ReadM4Data(void);           // get data from M4 to M7
void WriteM7Data(uint32_t *dataBuff); // put data from M7 to M4

// get data from M4 to M7 buffer
uint32_t *getM4Data(void)
{
	static uint32_t buffer[M4_U32DATA_NUM];            // buffer to receive data
	if (pShareData->u8Sts4to7 == SDATA_STATUS_HASDATA) // if M4 to M7 buffer has data
	{
		pShareData->u8Sts4to7 = SDATA_STATUS_LOCK;     // lock the M4 to M7 buffer
		for(int n = 0; n < M4_U32DATA_NUM; n++)
		{
			buffer[n] = pShareData->au32M4toM7[n];     // transfer data
			pShareData->au32M4toM7[n] = 0;             // clear M4 to M7 buffer
		}
		pShareData->u8Sts4to7 = SDATA_STATUS_EMPTY;    // M4 to M7 buffer is empty
	}
	return buffer;                                     // return the buffer (pointer)
}

// send data from M7 to M4
void WriteM7Data(uint32_t *dataBuff)
{
	if (pShareData->u8Sts7to4 == SDATA_STATUS_EMPTY)  // if M7 to M4 buffer is empty
	{
		pShareData->u8Sts7to4 = SDATA_STATUS_LOCK;    // lock the M7 to M4 buffer
		for(int n = 0; n < M7_U32DATA_NUM; n++)
		{
			pShareData->au32M7toM4[n] = dataBuff[n];  // transfer data
			dataBuff[n] = 0;                          // clear M7 to M4 buffer
		}
		pShareData->u8Sts7to4 = SDATA_STATUS_HASDATA; // M7 to M4 buffer has data
	}
}

//-------------------------------------------------------
#endif
//-------------------------------------------------------
