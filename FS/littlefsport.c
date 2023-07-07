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


#define READ_PROG_BYTEMIN       128   // 读/写的最小字节数, 所有的读写都是该数的整数倍
#define CACHE_SIZE              256   // 必须是：READ_PROG_BYTEMIN的整数倍，BLOCK大小的 1/X
#define LOOKAHEADE_SIZE         128   // 8的整数倍，每个bit表示一个Block
#define BLOCK_CYCLES            500  // 100-1000

#define PORT_FS_NAME_MAX        96       // 最长文件名
#define PORT_FS_FILE_MAX        4194304  // 单文件最大容量,4M
#define PORT_FS_ATTR_MAX        128      // 文件属性最大字节数
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

//uint8_t FS_Read_Buff[CACHE_SIZE] __attribute__ ((aligned (8)));//__ALIGN_END;
//uint8_t FS_Prog_Buff[CACHE_SIZE] __attribute__ ((aligned (8)));//__ALIGN_END;
//uint8_t FS_Look_Buff[LOOKAHEADE_SIZE] __attribute__ ((aligned (8)));//__ALIGN_END;

lfs_t lfs_ext_flash;
lfs_file_t file_ext_flash;
uint8_t FileSystemStatus = 0U;    // 0:normal, !0:abnormal

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
    .block_count = 2048, //BSP_FS_BLOCK_COUNT,       // 外部FLASH的块数量
    .cache_size = CACHE_SIZE,
    .lookahead_size = LOOKAHEADE_SIZE,       // 8的整数倍，每个bit表示一个Block
    .block_cycles = BLOCK_CYCLES,            // 100-1000

#if 0 // 下面是Option配置项    
    .read_buffer = FS_Read_Buff,
    .prog_buffer = FS_Prog_Buff,
    .lookahead_buffer = FS_Look_Buff,
#endif
    .name_max = PORT_FS_NAME_MAX,
    .file_max = PORT_FS_FILE_MAX,
    .attr_max = PORT_FS_ATTR_MAX,
    .metadata_max = BSP_FS_BLOCK_SIZE
};



void FileSystemIint(void)
{
    // 初始化文件服务器
    // memset(FileLocSer, 0, sizeof(FileLocSer));

    // mount the filesystem
    int err = lfs_mount(&lfs_ext_flash, &lfs_cfg_ext_flash);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err) {
        if (lfs_format(&lfs_ext_flash, &lfs_cfg_ext_flash)) {
            FileSystemStatus = 0x05U;
        } else if (lfs_mount(&lfs_ext_flash, &lfs_cfg_ext_flash)) {
            FileSystemStatus = 0x0AU;
        }
    }
}

// 移植时重新关联外部FLASH的 Read/Write/Block Erase接口
// 如果Size不是偶数,Dual flash会返回Size+1个,可能导致内存越界
static int BSP_FS_Read(const struct lfs_config *c, lfs_block_t block, lfs_off_t off, void *buffer, lfs_size_t size)
{
    uint32_t ReadAddr = block * c->block_size + off;
 
    // Dual flash模式地址应为偶数,通过读写最小字节数自动限制 
    if ((ReadAddr + size) > QSPI_FLASH_ADDMAX) {
        return (-1);
    }

#if 1
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

    // Dual flash模式地址应为偶数,通过读写最小字节数自动限制
    if ((WriteAddr + size) > QSPI_FLASH_ADDMAX) {
        return (-1);
    }

#if 1
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
#if 1
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
                FileLocSer[i].fileId = 0U;
                FileLocSer[i].State = FSS_IDLE;
                break;
            }
        }
    }

    return (lfs_file_close(&lfs_ext_flash, hFile));
}


#if 1

// 使用shell对littlefs的接口进行测试
lfs_file_t *lfs_test_fhandle = NULL;

#define FS_TEST_TARGET_BSP 0

#if (FS_TEST_TARGET_BSP == 0)
lfs_file_t lfs_test_file;
#endif

void LFS_TEST_fCreate(char *path)
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
    int32_t ret;
    lfs_file_t hfile;
    if (!path) {
        ret = lfs_file_open(&lfs_ext_flash, &hfile, "test.txt", (LFS_O_RDWR | LFS_O_CREAT));
    } else {
        ret = lfs_file_open(&lfs_ext_flash, &hfile, path, (LFS_O_RDWR | LFS_O_CREAT));
    }
    
    char buff[64];
    sprintf(buff, "File opened and ret = %d\n\r", ret);
    user_shellprintf(buff);

    ret = lfs_file_close(&lfs_ext_flash, &hfile);
    sprintf(buff, "File closed and ret = %d\n\r", ret);
    user_shellprintf(buff);
#endif
}

void LFS_TEST_fRead(char *path)
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
    char buff[64];
    char data[64];
    lfs_file_t hfile;
    int32_t ret;
    if (!path) {
        ret = lfs_file_open(&lfs_ext_flash, &hfile, "test.txt", LFS_O_RDONLY);
    } else {
        ret = lfs_file_open(&lfs_ext_flash, &hfile, path, LFS_O_RDONLY);
    }
    sprintf(buff, "File opened and ret = %d\n\r", ret);
    user_shellprintf(buff);
    if (ret != 0) {return;}

    // 返回读取字节数
    ret = lfs_file_read(&lfs_ext_flash, &hfile, data, sizeof(data));
    sprintf(buff, "File read ret = %d\n\r", ret);
    user_shellprintf(buff);
    user_shellprintf((char *)data);
    user_shellprintf("\n");

    lfs_file_close(&lfs_ext_flash, &hfile);
#endif
}

const char *FS_TEST_Wstr = "Hello file system.";


void LFS_TEST_fWrite(char *path, char *data)
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
    char buff[64];
    lfs_file_t hfile;
    int32_t ret;
    if (!path) {
        ret = lfs_file_open(&lfs_ext_flash, &hfile, "test.txt", LFS_O_RDWR);
    } else {
        ret = lfs_file_open(&lfs_ext_flash, &hfile, path, LFS_O_RDWR);
    }
    sprintf(buff, "File open ret = %d\n\r", ret);
    user_shellprintf(buff);
    if (ret != 0) {return;}


    ret = lfs_file_rewind(&lfs_ext_flash, &hfile);
    sprintf(buff, "File rewind ret = %d\n\r", ret);
    user_shellprintf(buff);
    if (ret != 0) {return;}
 
    // 返回写字节数
    if (!data) {data = (char *)FS_TEST_Wstr;}
    ret = lfs_file_write(&lfs_ext_flash, &hfile, data, strlen(data));
    
    sprintf(buff, "File write ret = %d\n\r", ret);
    user_shellprintf(buff);

    lfs_file_close(&lfs_ext_flash, &hfile);
#endif
}

void LFS_TEST_fAppend(char *path, char *data)
{
    char buff[64];
    lfs_file_t hfile;
    int32_t ret;
    if (!path) {
        ret = lfs_file_open(&lfs_ext_flash, &hfile, "test.txt", LFS_O_WRONLY | LFS_O_APPEND);
    } else {
        ret = lfs_file_open(&lfs_ext_flash, &hfile, path, LFS_O_WRONLY | LFS_O_APPEND);
    }
    sprintf(buff, "File open ret = %d\n\r", ret);
    user_shellprintf(buff);
    if (ret != 0) {return;}


    ret = lfs_file_rewind(&lfs_ext_flash, &hfile);
    sprintf(buff, "File rewind ret = %d\n\r", ret);
    user_shellprintf(buff);
    if (ret != 0) {return;}
 
    // 返回写字节数
    ret = lfs_file_write(&lfs_ext_flash, &hfile, data, strlen(data));
    
    sprintf(buff, "File write ret = %d\n\r", ret);
    user_shellprintf(buff);

    lfs_file_close(&lfs_ext_flash, &hfile);
}

void LFS_TEST_Makedir(char *path)
{
#if (FS_TEST_TARGET_BSP == 1)

#else
    char buff[64];
    int32_t ret;
    if (!path) {
        ret = lfs_mkdir(&lfs_ext_flash, "/System/");
    } else {
        ret = lfs_mkdir(&lfs_ext_flash, path);
    }

    sprintf(buff, "Make dir ret = %d\n\r", ret);
    user_shellprintf(buff);
#endif
}

void LFS_TEST_Remove(char *path)
{
#if (FS_TEST_TARGET_BSP == 1)

#else
    char buff[64];
    int32_t ret;
    if (!path) {
        ret = lfs_remove(&lfs_ext_flash, "test.txt");
    } else {
        ret = lfs_remove(&lfs_ext_flash, path);
    }

    sprintf(buff, "remove %s ret = %d\n\r", path, ret);
    user_shellprintf(buff);
#endif
}


int32_t scan_files(char* path)
{
    int32_t res;
    lfs_dir_t dir;
    uint32_t i;
    struct lfs_info info;


    res = lfs_dir_open(&lfs_ext_flash, &dir, path);
    if (res == 0) {
        lfs_dir_tell(&lfs_ext_flash, &dir);
        for (;;) {
            res = lfs_dir_read(&lfs_ext_flash, &dir, &info);
            
            if ((res < 0) || (info.type > LFS_TYPE_DIR) || (info.name[0] == 0)) {break;}
            
            if (info.type == LFS_TYPE_DIR) {
                if (0 == strcmp(info.name, ".") || 0 == strcmp(info.name, "..")) {continue;}
                
                i = strlen(path);
                sprintf(&path[i], "/%s", info.name);
                user_shellprintf(path);
                user_shellprintf("\n\r");
                res = scan_files(path);                    /* Enter the directory */
                if (res != 0) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
                i = strlen(path);
                sprintf(&path[i], "/%s", info.name);
                user_shellprintf(path);
                user_shellprintf("\n\r");
                path[i] = 0;
            }
        }
        lfs_dir_close(&lfs_ext_flash, &dir);
    }

    return res;
}


void LFS_TEST_Listdir(char *path)
{
    char buff[128];
    if (!path) {
        strcpy(buff, ".");
    } else {
        strcpy(buff, path);
    }
    
    scan_files(buff);
}


// Traverse through all blocks in use by the filesystem
//
// The provided callback will be called with each block address that is
// currently in use by the filesystem. This can be used to determine which
// blocks are in use or how much of the storage is available.
//
// Returns a negative error code on failure.
// int lfs_fs_traverse(lfs_t *lfs, int (*cb)(void*, lfs_block_t), void *data);
int ReadBlockStatus(void *ags, lfs_block_t block)
{
    (void)ags;
    uint8_t buff[16];
    int err = BSP_FS_Read(lfs_ext_flash.cfg, block, 0, buff, sizeof(buff));

    if (0 > err) {
        return err;
    }

    int i;
    for (i=0; i<16; i++) {
        if (buff[i] != 0xFF) {
            user_shellprintf("0 ");
            break;
        }
    }

    if (i == 16) {
        user_shellprintf("1 ");
    }
    return err;
}

void LFS_TEST_Traverse(char *path)
{
    int32_t data = 0;
    int err = lfs_fs_traverse(&lfs_ext_flash, ReadBlockStatus, &data);

    char buff[64];
    sprintf(buff, "Trave res: %d data: %d\n\r", err, data);
    user_shellprintf(buff);
}

void LFS_TEST_Size(char *path)
{
    int res = lfs_fs_size(&lfs_ext_flash);

    char buff[64];
    sprintf(buff, "Size: %d\n\r", res);
    user_shellprintf(buff);
}


int FS_TEST_Example(void)
{
        #if 1
            int err;
            lfs_file_t hfile;
            uint32_t boot_count = 0;
            char buff[64];

            err = lfs_file_open(&lfs_ext_flash, &hfile, "boot_count.bin", LFS_O_RDWR | LFS_O_CREAT);
            if (err < 0) {
                user_shellprintf("boot_count open failed\n");
                return err;
            }

            err = lfs_file_read(&lfs_ext_flash, &hfile, &boot_count, sizeof(boot_count));
            if (err < 0) {
                return lfs_file_close(&lfs_ext_flash, &hfile);
            }

            sprintf(buff, "boot_count: %d\n", boot_count);
            user_shellprintf(buff);

            // update boot count
            boot_count += 1;
            err = lfs_file_rewind(&lfs_ext_flash, &hfile);
            if (err < 0) {
                user_shellprintf("boot_count rewind failed\n");
                return lfs_file_close(&lfs_ext_flash, &hfile);
            }

            err = lfs_file_write(&lfs_ext_flash, &hfile, &boot_count, sizeof(boot_count));
            if (err < 0) {
                user_shellprintf("boot_count write failed\n");
            }

            // remember the storage is not updated until the file is closed successfully
            err = lfs_file_close(&lfs_ext_flash, &hfile);
            return err;
        #endif
}


SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fscreate, LFS_TEST_fCreate, File system create a file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fsread, LFS_TEST_fRead, File system read a file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fswrite, LFS_TEST_fWrite, File system write a file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fsappend, LFS_TEST_fAppend, File system write a file(append));
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fsmkdir, LFS_TEST_Makedir, File system make dir);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fsrmdir, LFS_TEST_Remove, File system remove dir or file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fslsdir, LFS_TEST_Listdir, File system list dir or file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fssize, LFS_TEST_Size, File system size);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fstrave, LFS_TEST_Traverse, File system block traverse);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fstest, FS_TEST_Example, File system test example);

#endif