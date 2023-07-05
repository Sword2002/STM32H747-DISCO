/*
 * STM32H747I-DISCO study
 * Copyright (C) 2023 Schneider-Electric.
 * All Rights Reserved.
 *
 */
//#include "stm32h747i_discovery_qspi.h"
#include "stm32h747i_discovery_mt25ql_qspi.h"
#include "lfs.h"

#include "shell_port.h"
#include <string.h>
#include "stdio.h"


#define READ_PROG_BYTEMIN       16   // 读/写的最小字节数, 所有的读写都是该数的整数倍
#define CACHE_SIZE              16   // 必须是：READ_PROG_BYTEMIN的整数倍，BLOCK大小的 1/X
#define LOOKAHEADE_SIZE         16   // 8的整数倍，每个bit表示一个Block
#define BLOCK_CYCLES            500  // 100-1000

#define PORT_FS_NAME_MAX        96       // 最长文件名
#define PORT_FS_FILE_MAX        4194304  // 单文件最大容量,4M
#define PORT_FS_ATTR_MAX        256      // 文件属性最大字节数
#define FILE_BUFF_SIZE          256      // 文件操作buff尺寸
#define FILE_OPEN_MAX           10       // 同时允许打开的最多文件数量



static int BSP_FS_Read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size);
static int BSP_FS_Prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size);
static int BSP_FS_Erase(const struct lfs_config *c, lfs_block_t block);
static int BSP_FS_Sync(const struct lfs_config *c);
#ifdef LFS_THREADSAFE
static int BSP_FS_Lock(const struct lfs_config *c);
static int BSP_FS_UnLock(const struct lfs_config *c);
#endif

#ifndef LFS_READONLY
#define FS_O_NUM    8
#else
#define FS_O_NUM    1
#endif

uint8_t FS_Read_Buff[CACHE_SIZE] __attribute__ ((aligned (8)));//__ALIGN_END;
uint8_t FS_Prog_Buff[CACHE_SIZE] __attribute__ ((aligned (8)));//__ALIGN_END;
uint8_t FS_Look_Buff[LOOKAHEADE_SIZE] __attribute__ ((aligned (8)));//__ALIGN_END;

lfs_t lfs_ext_flash;
lfs_file_t file_ext_flash;

// 文件服务器状态
typedef enum FileServerStates {
    FSS_IDLE = 0,
    FSS_USED = 1,
    //FSS_OPENED = 1,
    //FSS_READING = 2,
    //FSS_WRITING = 3,
} fileServ_state_t;

// 文件服务器数据结构
typedef struct FileLocalServer {
    fileServ_state_t State;
    uint8_t FileBuff[FILE_BUFF_SIZE];
    uint16_t fileId;
    lfs_file_t hFile;
    // 增加开始时的tick值,用于超时管理
} fileServ_Struct_t;

// 文件服务器，最多支持10个文件,这里会消耗RAM 10*FILE_BUFF_SIZE
fileServ_Struct_t FileLocSer[FILE_OPEN_MAX];


// configuration of the filesystem is provided by this struct
const struct lfs_config lfs_cfg_ext_flash = {
    //.context
    .read  = BSP_FS_Read,
    .prog  = BSP_FS_Prog,
    .erase = BSP_FS_Erase,
    .sync  = BSP_FS_Sync,  // 用于RAM模式
#ifdef LFS_THREADSAFE
    .lock   = BSP_FS_Lock,
    .unlock = BSP_FS_UnLock,
#endif
    // block device configuration
    .read_size = READ_PROG_BYTEMIN,          // 读取的最小字节数
    .prog_size = READ_PROG_BYTEMIN,          // 写的最小字节数
    .block_size = BSP_FS_BLOCK_SIZE,         // 外部FLASH的块大小
    .block_count = BSP_FS_BLOCK_COUNT,       // 外部FLASH的块数量
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEADE_SIZE,       // 8的整数倍，每个bit表示一个Block
    .block_cycles = BLOCK_CYCLES,            // 100-1000

#if 1
    // 下面是Option配置项
    .read_buffer = FS_Read_Buff,
    .prog_buffer = FS_Prog_Buff,
    .lookahead_buffer = FS_Look_Buff,
    .name_max = PORT_FS_NAME_MAX,
    .file_max = PORT_FS_FILE_MAX,
    .attr_max = PORT_FS_ATTR_MAX,
    .metadata_max = BSP_FS_BLOCK_SIZE
#endif
};



void FileSystemIint(void)
{
    // 初始化文件服务器
    memset(FileLocSer, 0, sizeof(FileLocSer));

    // mount the filesystem
    int err = lfs_mount(&lfs_ext_flash, &lfs_cfg_ext_flash);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        lfs_format(&lfs_ext_flash, &lfs_cfg_ext_flash);
        lfs_mount(&lfs_ext_flash, &lfs_cfg_ext_flash);
    }
}

// 移植时重新关联外部FLASH的 Read/Write/Block Erase接口
// 如果Size不是偶数,Dual flash会返回Size+1个,可能导致内存越界
static int BSP_FS_Read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t ReadAddr = block * c->block_size + off;
    //if (ReadAddr % 2 != 0) { // Dual flash模式地址应为偶数,通过读写最小字节数自动限制 
    //    ReadAddr++;
    //}

    if (ReadAddr > QSPI_FLASH_ADDMAX) {
        return (-1);
    }

#if 0
    return (BSP_QSPI_Read(0, (uint8_t *)buffer, ReadAddr, size));
#else
    // 作为调试信息打印输出读取结果
    BSP_QSPI_Read(0, (uint8_t *)buffer, ReadAddr, size);
    char buff[64];
    sprintf(buff, "Read addr: %d size: %d\n\r", ReadAddr, size);
    user_shellprintf(buff);
    for (int i = 0; i < size; i++) {
        sprintf(buff, "0x%02X ", *((uint8_t *)buffer + i));
        user_shellprintf(buff);
    }
    user_shellprintf("\n\r");
    
    return 0;
#endif
}

// 如果Size不是偶数,Dual flash会写Size+1个,最后一个字节会是未知数
static int BSP_FS_Prog(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, const void *buffer, lfs_size_t size)
{
    uint32_t WriteAddr = block * c->block_size + off;
    //if (WriteAddr % 2 != 0) { // Dual flash模式地址应为偶数,通过读写最小字节数自动限制
    //    WriteAddr++;
    //}

    if (WriteAddr > QSPI_FLASH_ADDMAX) {
        return (-1);
    }

#if 0
    return (BSP_QSPI_Write(0, (uint8_t *)buffer, WriteAddr, size));
#else
    // 作为调试信息打印输出读取结果
    char buff[64];
    sprintf(buff, "Write addr: %d size: %d\n\r", WriteAddr, size);
    user_shellprintf(buff);
    for (int i = 0; i < size; i++) {
        sprintf(buff, "0x%02X ", *((uint8_t *)buffer + i));
        user_shellprintf(buff);
    }
    user_shellprintf("\n\r");

    return 0;
#endif
}

static int BSP_FS_Erase(const struct lfs_config *c, lfs_block_t block)
{
    uint32_t BlockAddress = block * c->block_size;
    if (BlockAddress > QSPI_FLASH_ADDMAX) {
        return (-1);
    }
#if 0
    return (BSP_QSPI_EraseBlock(0, BlockAddress, BSP_QSPI_ERASE_8K));
#else
    // 作为调试信息打印输出读取结果
    char buff[64];
    sprintf(buff, "Erase block: %d\n\r", block);
    user_shellprintf(buff);
    return 0;
#endif
}

static int BSP_FS_Sync(const struct lfs_config *c)
{
    (void)c;
    return 0;
}

#ifdef LFS_THREADSAFE // 使能线程安全
static int BSP_FS_Lock(const struct lfs_config *c)
{
    (void)c;
    return 0;
}

static int BSP_FS_UnLock(const struct lfs_config *c)
{
    (void)c;
    return 0;
}
#endif

struct fsOpenMode2Flag {
    char mode[4];
    int32_t flag;
};


const struct fsOpenMode2Flag FsOpenModeTab[FS_O_NUM] = {
    {"r",   (int32_t)LFS_O_RDONLY},   // Open a file as read only
#ifndef LFS_READONLY
    {"w",   (int32_t)LFS_O_WRONLY},   // Open a file as write only
    {"rw",  (int32_t)LFS_O_RDWR},     // Open a file as read and write
    {"w+",  (int32_t)LFS_O_CREAT},    // Create a file if it does not exist
    {"f",   (int32_t)LFS_O_EXCL},     // Fail if a file already exists
    {"t",   (int32_t)LFS_O_TRUNC},    // Truncate the existing file to zero size
    {"a",   (int32_t)LFS_O_APPEND},   // Move to end of file on every write
    {"wa+", (int32_t)(LFS_O_CREAT|LFS_O_APPEND)},   // 
#endif
};



lfs_file_t *hFileOpen(const char* filename, const char* mode)
{
    int8_t i;
   int32_t flag = (int32_t)LFS_O_RDONLY;
    lfs_file_t *hFile = NULL;
    for (i = 0; i < FS_O_NUM; i++) {
        if (0 == strcmp(FsOpenModeTab[i].mode, mode)) {
            flag = FsOpenModeTab[i].flag;
            break;
        }
    }

    for (i = 0; i < FILE_OPEN_MAX; i++) {
        if (FileLocSer[i].State == FSS_IDLE) {
            hFile = &FileLocSer[i].hFile;
            memset(FileLocSer[i].FileBuff, 0, FILE_BUFF_SIZE);
            hFile->cache.buffer = FileLocSer[i].FileBuff;
            break;
        }
    }

    // 这时i不能被修改
    if (hFile) {
        if (lfs_file_open(&lfs_ext_flash, hFile, filename, flag)) {
            FileLocSer[i].State = FSS_USED;
            FileLocSer[i].fileId = hFile->id;
        }
    }
    
    return hFile;
}

int32_t errFileClose(lfs_file_t* hFile)
{
    int8_t i;
    if (hFile) {
        for (i = 0; i < FILE_OPEN_MAX; i++) {
            if (FileLocSer[i].fileId == hFile->id) {
                hFile->cache.buffer = NULL;
                FileLocSer[i].fileId = 0U;
                FileLocSer[i].State = FSS_IDLE;
                break;
            }
        }
    }

    return (lfs_file_close(&lfs_ext_flash, hFile));
}


#if 0

// 使用shell对littlefs的接口进行测试
lfs_file_t *lfs_test_fhandle = NULL;

#define FS_TEST_TARGET_BSP 0

#if (FS_TEST_TARGET_BSP == 0)
lfs_file_t lfs_test_file;
#endif

void LFS_TEST_Open(void)
{
#if (FS_TEST_TARGET_BSP == 1)
    lfs_test_fhandle = hFileOpen("System:/Temp/test.txt","wa+");

    if (!lfs_test_fhandle) {
        user_shellprintf("File open failed!\n\r");
    } else {
        char buff[64];
        sprintf(buff, "file id = %d\n\r", lfs_test_fhandle->id);
        user_shellprintf(buff);
    }
#else
    int32_t ret = lfs_file_open(&lfs_ext_flash, &lfs_test_file, "test_txt", (LFS_O_RDWR | LFS_O_CREAT));
    char buff[64];
    sprintf(buff, "File opened and ret = %d\n\r", ret);
    user_shellprintf(buff);
#endif
}

void LFS_TEST_Close(void)
{
#if (FS_TEST_TARGET_BSP == 1)
    if (!lfs_test_fhandle) {
        user_shellprintf("File is not valid!\n\r");
    } else {
        int32_t ret = errFileClose(lfs_test_fhandle);
        
        char buff[64];
        sprintf(buff, "File closed and ret = %d\n\r", ret);
        user_shellprintf(buff);

        if (lfs_test_fhandle) {
            user_shellprintf("handle need free by user\n\r");
            lfs_test_fhandle = NULL;
        }
    }
#else
    int32_t ret = lfs_file_close(&lfs_ext_flash, &lfs_test_file);
    char buff[64];
    sprintf(buff, "File closed and ret = %d\n\r", ret);
    user_shellprintf(buff);
#endif
}

const char *FS_TEST_Wstr = "Hello file system.";


void LFS_TEST_Write(void)
{
#if (FS_TEST_TARGET_BSP == 1)
    if (!lfs_test_fhandle) {
        user_shellprintf("File is not valid!\n\r");
        return;
    }

    lfs_file_rewind(&lfs_ext_flash, lfs_test_fhandle);
    lfs_ssize_t ret = lfs_file_write(&lfs_ext_flash, lfs_test_fhandle, FS_TEST_Wstr, strlen(FS_TEST_Wstr));
    if (ret == strlen(FS_TEST_Wstr)) {
        user_shellprintf("File write success\n\r");
    } else {
        char buff[64];
        sprintf(buff, "File write faile and ret = %d\n\r", ret);
        user_shellprintf(buff);
    }
#else
    lfs_file_rewind(&lfs_ext_flash, &lfs_test_file);
    int32_t ret = lfs_file_write(&lfs_ext_flash, &lfs_test_file, FS_TEST_Wstr, strlen(FS_TEST_Wstr));
    char buff[64];
    sprintf(buff, "File write success and ret = %d\n\r", ret);
    user_shellprintf(buff);
#endif
}

void LFS_TEST_Read(void)
{
#if (FS_TEST_TARGET_BSP == 1)

#else
    uint8_t data[17];    
    int32_t ret = lfs_file_read(&lfs_ext_flash, &lfs_test_file, data, 16);
    char buff[64];
    sprintf(buff, "File read success and ret = %d\n\r", ret);
    user_shellprintf(buff);
    data[16] = 0;
    user_shellprintf((char *)data);
    user_shellprintf("\n\r");
#endif
}


SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fsopen, LFS_TEST_Open, File system open a file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fsclose, LFS_TEST_Close, File system close a file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fswrite, LFS_TEST_Write, File system write a file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fsread, LFS_TEST_Read, File system read a file);

#endif