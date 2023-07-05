/*
 * STM32H747I-DISCO study
 * Copyright (C) 2023 Schneider-Electric.
 * All Rights Reserved.
 *
 */
//#include "stm32h747i_discovery_qspi.h"
#include "stm32h747i_discovery_mt25ql_qspi.h"
#include "ff.h"
#include "ffconf.h"

#include "shell_port.h"
#include <string.h>
#include "stdio.h"

#define FATFS_SHELL_TEST 1


FATFS fatfs_ex_flash;
uint8_t FatfsStatus = 0; // 0:Normal, !0: faile


void FileSystemIint(void)
{
    BYTE work[FF_MAX_SS];
    //    移植文件系统到物理设备上，这样才能使用文件系统的各个函数接口
    //    fs： 文件系统句柄
    //    path： 盘符，序号要对应
    //    opt：  0:Do not mount (delayed mount), 1:Mount immediately
    int err = f_mount(&fatfs_ex_flash, "0:", 1);

    // reformat if we can't mount the filesystem
    // this should only happen on the first boot
    if (err == FR_NO_FILESYSTEM) {
        // *path    设备盘符
        // *opt     选项，NULL默认值
        const MKFS_PARM opt = {FM_FAT, 0, 0, 0, 16}; // 指定 fmt 和 n_root 其余使用默认值
        // *work    格式化时的缓存，越大格式化越快,必须是Sector Size的整数倍
        // len      sizeof(work)，Sector Size的整数倍                
        if(FR_OK != f_mkfs("0:", &opt, work, FF_MAX_SS))
        {
            FatfsStatus = 1;
        }
    } else if (err) {
        FatfsStatus = err;
    }
}




/* -------------------------------------------------------------------------------------------------------------
FRESULT f_open (FIL* fp, const TCHAR* path, BYTE mode);				// Open or create a file 
FRESULT f_close (FIL* fp);											// Close an open file object 
FRESULT f_read (FIL* fp, void* buff, UINT btr, UINT* br);			// Read data from the file 
FRESULT f_write (FIL* fp, const void* buff, UINT btw, UINT* bw);	// Write data to the file 
FRESULT f_lseek (FIL* fp, FSIZE_t ofs);								// Move file pointer of the file object 
FRESULT f_truncate (FIL* fp);										// Truncate the file 
FRESULT f_sync (FIL* fp);											// Flush cached data of the writing file 
FRESULT f_opendir (DIR* dp, const TCHAR* path);						// Open a directory 
FRESULT f_closedir (DIR* dp);										// Close an open directory 
FRESULT f_readdir (DIR* dp, FILINFO* fno);							// Read a directory item 
FRESULT f_findfirst (DIR* dp, FILINFO* fno, const TCHAR* path, const TCHAR* pattern);	// Find first file 
FRESULT f_findnext (DIR* dp, FILINFO* fno);							// Find next file 
FRESULT f_mkdir (const TCHAR* path);								// Create a sub directory 
FRESULT f_unlink (const TCHAR* path);								// Delete an existing file or directory 
FRESULT f_rename (const TCHAR* path_old, const TCHAR* path_new);	// Rename/Move a file or directory 
FRESULT f_stat (const TCHAR* path, FILINFO* fno);					// Get file status 
FRESULT f_chmod (const TCHAR* path, BYTE attr, BYTE mask);			// Change attribute of a file/dir 
FRESULT f_utime (const TCHAR* path, const FILINFO* fno);			// Change timestamp of a file/dir 
FRESULT f_chdir (const TCHAR* path);								// Change current directory 
FRESULT f_chdrive (const TCHAR* path);								// Change current drive 
FRESULT f_getcwd (TCHAR* buff, UINT len);							// Get current directory 
FRESULT f_getfree (const TCHAR* path, DWORD* nclst, FATFS** fatfs);	// Get number of free clusters on the drive 
FRESULT f_getlabel (const TCHAR* path, TCHAR* label, DWORD* vsn);	// Get volume label 
FRESULT f_setlabel (const TCHAR* label);							// Set volume label 
FRESULT f_forward (FIL* fp, UINT(*func)(const BYTE*,UINT), UINT btf, UINT* bf);	// Forward data to the stream 
FRESULT f_expand (FIL* fp, FSIZE_t fsz, BYTE opt);					// Allocate a contiguous block to the file 
FRESULT f_mount (FATFS* fs, const TCHAR* path, BYTE opt);			// Mount/Unmount a logical drive 
FRESULT f_mkfs (const TCHAR* path, const MKFS_PARM* opt, void* work, UINT len);	// Create a FAT volume 
FRESULT f_fdisk (BYTE pdrv, const LBA_t ptbl[], void* work);		// Divide a physical drive into some partitions 
FRESULT f_setcp (WORD cp);											// Set current code page 
int f_putc (TCHAR c, FIL* fp);										// Put a character to the file 
int f_puts (const TCHAR* str, FIL* cp);								// Put a string to the file 
int f_printf (FIL* fp, const TCHAR* str, ...);						// Put a formatted string to the file 
TCHAR* f_gets (TCHAR* buff, int len, FIL* fp);						// Get a string from the file 

// Some API fucntions are implemented as macro 

#define f_eof(fp) ((int)((fp)->fptr == (fp)->obj.objsize))
#define f_error(fp) ((fp)->err)
#define f_tell(fp) ((fp)->fptr)
#define f_size(fp) ((fp)->obj.objsize)
#define f_rewind(fp) f_lseek((fp), 0)
#define f_rewinddir(dp) f_readdir((dp), 0)
#define f_rmdir(path) f_unlink(path)
#define f_unmount(path) f_mount(0, path, 0)
-------------------------------------------------------------------------------------------------------*/

#if (FATFS_SHELL_TEST == 1)

FIL fatfs_hfile;

void FATFS_TEST_Open(char *path)
{
#if (FS_TEST_TARGET_BSP == 1)


#else
    int32_t ret;
    if (!path) {
        ret = f_open(&fatfs_hfile, "0:test.bin", FA_OPEN_ALWAYS);
    } else {
        ret = f_open(&fatfs_hfile, path, FA_OPEN_ALWAYS);
    }
    
    char buff[64];
    sprintf(buff, "File opened and ret = %d\n\r", ret);
    user_shellprintf(buff);
#endif
}

void FATFS_TEST_Close(void)
{
#if (FS_TEST_TARGET_BSP == 1)


#else
    int32_t ret = f_close(&fatfs_hfile);
    char buff[64];
    sprintf(buff, "File closed and ret = %d\n\r", ret);
    user_shellprintf(buff);
#endif
}

void FATFS_TEST_Read(char *path)
{
#if (FS_TEST_TARGET_BSP == 1)


#else
    char buff[64];
    uint8_t data[128];
    UINT nread;

    int32_t ret = f_open(&fatfs_hfile, path, FA_READ);
    sprintf(buff, "File opened and ret = %d\n\r", ret);
    user_shellprintf(buff);

    memset(data, 0, 128);
    ret = f_read(&fatfs_hfile, data, 128, &nread);
    sprintf(buff, "File read ret = %d, byte count: %d\n\r", ret, nread);
    user_shellprintf(buff);

    user_shellprintf((char *)data);
    
    user_shellprintf("\n\r");

    ret = f_close(&fatfs_hfile);
    sprintf(buff, "File closed and ret = %d\n\r", ret);
    user_shellprintf(buff);
#endif
}


void FATFS_TEST_Write(char *path, char *str)
{
#if (FS_TEST_TARGET_BSP == 1)


#else
    char buff[64];
    UINT nwrite;
    UINT dnum = strlen(str);
    int32_t ret = f_open(&fatfs_hfile, path, (FA_OPEN_APPEND | FA_WRITE));
    sprintf(buff, "File opened and ret = %d\n\r", ret);
    user_shellprintf(buff);

    ret = f_write(&fatfs_hfile, str, dnum, &nwrite);
    sprintf(buff, "File write ret = %d, byte count: %d\n\r", ret, nwrite);
    user_shellprintf(buff);

    ret = f_close(&fatfs_hfile);
    sprintf(buff, "File closed and ret = %d\n\r", ret);
    user_shellprintf(buff);
#endif
}


void FATFS_TEST_Makedir(char *path)
{
#if (FS_TEST_TARGET_BSP == 1)


#else
    int32_t ret;
    if (!path) {
        ret = f_mkdir("System/");
    } else {
        ret = f_mkdir(path);
    }

    char buff[64];
    sprintf(buff, "Makedir ret = %d\n\r", ret);
    user_shellprintf(buff);
#endif
}

// Start node to be scanned (***also used as work area***)/
FRESULT scan_files ( char* path)
{
    FRESULT res;
    DIR dir;
    UINT i;
    static FILINFO fno;


    res = f_opendir(&dir, path);                       /* Open the directory */
    if (res == FR_OK) {
        for (;;) {
            res = f_readdir(&dir, &fno);                   /* Read a directory item */
            if (res != FR_OK || fno.fname[0] == 0) break;  /* Break on error or end of dir */
            if (fno.fattrib & AM_DIR) {                    /* It is a directory */
                i = strlen(path);
                sprintf(&path[i], "/%s", fno.fname);
                user_shellprintf(path);
                user_shellprintf("\n\r");
                res = scan_files(path);                    /* Enter the directory */
                if (res != FR_OK) break;
                path[i] = 0;
            } else {                                       /* It is a file. */
                //printf("%s/%s\n", path, fno.fname);
                user_shellprintf(path);
                user_shellprintf("\n\r");
                user_shellprintf(fno.fname);
                user_shellprintf("\n\r");
            }
        }
        f_closedir(&dir);
    }

    return res;
}

void FATFS_TEST_Readdir(char *path)
{
#if (FS_TEST_TARGET_BSP == 1)


#else
    int32_t res;

    char buff[256];
    if (!path) {
        strcpy(buff, "/");
    } else {
        strcpy(buff, path);
    }
    
    res = scan_files(buff);

#endif
}

void FATFS_TEST_CtDelet(char *path)
{
    f_open(&fatfs_hfile, path, (FA_OPEN_APPEND | FA_WRITE));
    f_truncate(&fatfs_hfile);
    f_close(&fatfs_hfile);
}

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fopen, FATFS_TEST_Open, File system open a file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fclose, FATFS_TEST_Close, File system close a file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fwrite, FATFS_TEST_Write, File system write a file);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 fread, FATFS_TEST_Read, File system read a file);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 mkdir, FATFS_TEST_Makedir, make dir);
SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 rdir, FATFS_TEST_Readdir, read dir);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                 remove, f_unlink, remove dir or file);

SHELL_EXPORT_CMD(SHELL_CMD_PERMISSION(0)| SHELL_CMD_TYPE(SHELL_TYPE_CMD_FUNC), 
                ctdelet, FATFS_TEST_CtDelet, delet file context);


#endif
