/*-----------------------------------------------------------------------*/
/* Low level disk I/O module SKELETON for FatFs     (C)ChaN, 2019        */
/*-----------------------------------------------------------------------*/
/* If a working storage control module is available, it should be        */
/* attached to the FatFs via a glue function rather than modifying it.   */
/* This is an example of glue functions to attach various exsisting      */
/* storage control modules to the FatFs module with a defined API.       */
/*-----------------------------------------------------------------------*/

#include "ff.h"			/* Obtains integer types */
#include "diskio.h"		/* Declarations of disk functions */
#include "stm32h747i_discovery_mt25ql_qspi.h"

/* Definitions of physical drive number for each drive */
#define DEV_FLASH	0	

#define EX_FLASH_SECTOR_SIEZ	8192   // 外部FLASH 每个Sub-sector的容量是2*4096
#define EX_FLASH_SECTOR_COUNT	16384  // =(64M+64M)/(4096*2)


#define FATFS_SECTOR_SIEZ		512    // FF_MAX_SS允许最大是4096,FF_MIM_SS允许最大是512
#define FATFS_SECTOR_COUNT		32768  // 16384 * (8192 / 512),这里裁剪为16M做测试
#define FATFS_SECTORN_PERBLOCK  16     // (8192 / 512)

//#define FATFS_SECTOR_SIEZ		4096   // FF_MAX_SS允许最大是4096,FF_MIM_SS允许最大是512
//#define FATFS_SECTOR_COUNT	32768  // 16384 * (8192 / 4096)
//#define FATFS_SECTORN_PERBLOCK  2      // (8192 / 4096)



/*-----------------------------------------------------------------------*/
/* Get Drive Status                                                      */
/*-----------------------------------------------------------------------*/

DSTATUS disk_status (
	BYTE pdrv		/* Physical drive nmuber to identify the drive */
)
{
	DSTATUS stat;
	int result;

	switch (pdrv) {
	case DEV_FLASH :  // 现在只有FLASH一个
		result = BSP_QSPI_GetStatus(0);

		// translate the reslut code here
		if (BSP_ERROR_NONE == result) {
			stat = 0;
		} else {
			stat = STA_PROTECT;
		}

		return stat;
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Inidialize a Drive                                                    */
/*-----------------------------------------------------------------------*/

DSTATUS disk_initialize (
	BYTE pdrv				/* Physical drive nmuber to identify the drive */
)
{
	//DSTATUS stat;
	//int result;

	switch (pdrv) {
	case DEV_FLASH :  // 现在只有FLASH一个
		// QSPI和FLASH的初始化放在MX_QUADSPI_Init(),这里根据初始化标志返回FLASH的status		
		return disk_status(DEV_FLASH);
	}
	return STA_NOINIT;
}



/*-----------------------------------------------------------------------*/
/* Read Sector(s)                                                        */
/*-----------------------------------------------------------------------*/

DRESULT disk_read (
	BYTE pdrv,		/* Physical drive nmuber to identify the drive */
	BYTE *buff,		/* Data buffer to store read data */
	LBA_t sector,	/* Start sector in LBA */
	UINT count		/* Number of sectors to read */
)
{
	DRESULT res = RES_ERROR;
	int result = BSP_ERROR_WRONG_PARAM;

	switch (pdrv) {
	case DEV_FLASH :
		// FATFS仅支持扇区级读写,这里会消耗非常大的heap
		if ((sector + count) <= FATFS_SECTOR_COUNT) {
			result = BSP_QSPI_Read(0, (uint8_t *)buff, (sector * FATFS_SECTOR_SIEZ), (count * FATFS_SECTOR_SIEZ));
		}
		break;
	}


	switch (result) {
	case BSP_ERROR_COMPONENT_FAILURE:
		res = RES_ERROR;
		break;

	case BSP_ERROR_NONE:
		res = RES_OK;
		break;

	case BSP_ERROR_WRONG_PARAM:
	default:
		res = RES_PARERR;
		break;
	}

	return res;
}



/*-----------------------------------------------------------------------*/
/* Write Sector(s)                                                       */
/*-----------------------------------------------------------------------*/

#if FF_FS_READONLY == 0

uint8_t SECTOR_BUFF[EX_FLASH_SECTOR_SIEZ]; // 可以把整个可擦除扇区数据读出

int32_t WriteToExtFlash(uint32_t writeAddr, uint32_t nbyte, const BYTE *data)
{
	uint32_t sector = writeAddr / EX_FLASH_SECTOR_SIEZ;  // 外部FLASH是8192字节一个扇区
	uint32_t off = writeAddr % EX_FLASH_SECTOR_SIEZ;	 // 在扇区内的地址偏移
	uint32_t remain = EX_FLASH_SECTOR_SIEZ - off;
	uint16_t i;
	uint8_t eraseFlag = 0U;
	int32_t res;

	if (nbyte <= remain) {
		remain = nbyte;
	}

	if (res = BSP_QSPI_Read(0, (uint8_t *)SECTOR_BUFF, (sector * EX_FLASH_SECTOR_SIEZ), EX_FLASH_SECTOR_SIEZ) != RES_OK) {
		return res;
	}
	
	for (i=0; i<remain; i++) {
		if (SECTOR_BUFF[off + i] != 0xFFU) {
			eraseFlag = 5U;
		}
		SECTOR_BUFF[off + i] = (uint8_t)data[i];
	}

	if (eraseFlag != 0U) {
		
		if (res = BSP_QSPI_EraseBlock(0, (sector * EX_FLASH_SECTOR_SIEZ), BSP_QSPI_ERASE_8K) != RES_OK) {
			return res;
		}

		res = BSP_QSPI_Write(0, SECTOR_BUFF, (sector * EX_FLASH_SECTOR_SIEZ), EX_FLASH_SECTOR_SIEZ);
	} else {
		res = BSP_QSPI_Write(0, (uint8_t *)data, writeAddr, remain);
	}

	return res;
}

DRESULT disk_write (
	BYTE pdrv,			/* Physical drive nmuber to identify the drive */
	const BYTE *buff,	/* Data to be written */
	LBA_t sector,		/* Start sector in LBA */
	UINT count			/* Number of sectors to write */
)
{
	DRESULT res = RES_ERROR;
	int result = BSP_ERROR_WRONG_PARAM;
	uint32_t writeAddr;

	switch (pdrv) {
	case DEV_FLASH :
		// FATFS仅支持扇区级读写,这里会消耗非常大的heap
		for (uint16_t i=0; i<count; i++) {
			// 超过了FLASH的最大容量
			if ((sector + count) <= FATFS_SECTOR_COUNT) {
				// 判断写数据对应空间是否需要擦除，如果需要擦除把整个SECTOR读取，修改后再写入
				writeAddr = (sector + i) * FATFS_SECTOR_SIEZ;
				result = WriteToExtFlash(writeAddr, FATFS_SECTOR_SIEZ, buff);

				buff += FATFS_SECTOR_SIEZ;
			} else {
				result = BSP_ERROR_WRONG_PARAM;
			}

			// 发送错误，结束操作
			if (result != BSP_ERROR_NONE) {
				break;
			}
		}
		break;
	}


	switch (result) {
	case BSP_ERROR_COMPONENT_FAILURE:
		res = RES_ERROR;
		break;

	case BSP_ERROR_NONE:
		res = RES_OK;
		break;

	case BSP_ERROR_WRONG_PARAM:
	default:
		res = RES_PARERR;
		break;
	}

	return res;
}

#endif


/*-----------------------------------------------------------------------*/
/* Miscellaneous Functions                                               */
/*-----------------------------------------------------------------------*/
/* Generic command (Used by FatFs) */
// CTRL_SYNC			0	: Complete pending write process (needed at FF_FS_READONLY == 0)
// GET_SECTOR_COUNT	    1	: Get media size (needed at FF_USE_MKFS == 1)
// GET_SECTOR_SIZE		2	: Get sector size (needed at FF_MAX_SS != FF_MIN_SS)
// GET_BLOCK_SIZE		3	: Get erase block size (needed at FF_USE_MKFS == 1)
// CTRL_TRIM			4	: Inform device that the data on the block of sectors is no longer used (needed at FF_USE_TRIM == 1)

DRESULT disk_ioctl (
	BYTE pdrv,		/* Physical drive nmuber (0..) */
	BYTE cmd,		/* Control code */
	void *buff		/* Buffer to send/receive control data */
)
{
	DRESULT res = RES_OK;
	//int result;

	switch (pdrv) {
	case DEV_FLASH :
		switch (cmd)
		{
		case CTRL_SYNC:
			*(uint32_t *)buff = 0; // 
			break;

		case GET_SECTOR_COUNT:
			*(uint32_t *)buff = FATFS_SECTOR_COUNT;
			break;

		case GET_SECTOR_SIZE:
			*(uint32_t *)buff = FATFS_SECTOR_SIEZ;		 // 取值规则：FF_MIN_SS<= <= FF_MAX_SS, 并且是2^x
			break;

		case GET_BLOCK_SIZE:
			*(uint32_t *)buff = FATFS_SECTORN_PERBLOCK;   // 取值规则：2^x
			break;

		default:
			res = RES_PARERR;
			break;
		}

		break;
	}

	return res;
}

