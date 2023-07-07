/**
  ******************************************************************************
  * @file    fsapi.h
  * @author  Drive FW team
  * @brief   Header file of file system.
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
#ifndef __FS_API_H__
#define __FS_API_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "lfs.h"


// 文件系统初始化
void FileSystemIint(void);


extern lfs_t lfs_ext_flash;
extern const struct lfs_config lfs_cfg_ext_flash;

#ifndef LFS_READONLY
// Format a block device with the littlefs
#define fs_ext_flash_format()           lfs_format(&lfs_ext_flash, &lfs_cfg_ext_flash)
#endif

// Mounts a littlefs
#define fs_ext_flash_mount()            lfs_mount(&lfs_ext_flash, &lfs_cfg_ext_flash)

// Unmounts a littlefs
#define fs_ext_flash_unmount()          lfs_unmount(&lfs_ext_flash)

/// General operations ///

#ifndef LFS_READONLY
// Removes a file or directory
#define errFileRemove(path)             lfs_remove(&lfs_ext_flash, (const char *)path)
#endif

#ifndef LFS_READONLY
// Rename or move a file or directory
#define errRename(oldpath, newpath)     lfs_rename(&lfs_ext_flash, (const char *)oldpath, (const char *)newpath)
#endif

// Find info about a file or directory
#define errFileInfo(path, info)         lfs_stat(&lfs_ext_flash, (const char *)path, (struct lfs_info *)info)

// Get a custom attribute
#define errReadAttr(path, type, buffer, size)         lfs_getattr(&lfs_ext_flash, (const char *)path, (uint8_t)type, (void *)buffer, (lfs_size_t)size)


#ifndef LFS_READONLY
// Set custom attributes
#define errWriteAttr(path, type, buffer, size)         lfs_setattr(&lfs_ext_flash, (const char *)path, (uint8_t)type, (void *)buffer, (lfs_size_t)size)

// Removes a custom attribute
#define errRemoveAttr(path, type, buffer, size)         lfs_removeattr(&lfs_ext_flash, (const char *)path, (uint8_t)type)
#endif


/// File operations ///

#ifndef LFS_NO_MALLOC
// Open a file
lfs_file_t *hFileOpen(const char* filename, const char* mode);
#endif



// Close a file
int32_t errFileClose(lfs_file_t* hFile);

// Synchronize a file on storage
//
// Any pending writes are written out to storage.
// Returns a negative error code on failure.
int lfs_file_sync(lfs_t *lfs, lfs_file_t *file);

// Read data from file
//
// Takes a buffer and size indicating where to store the read data.
// Returns the number of bytes read, or a negative error code on failure.
lfs_ssize_t lfs_file_read(lfs_t *lfs, lfs_file_t *file,
        void *buffer, lfs_size_t size);

#ifndef LFS_READONLY
// Write data to file
//
// Takes a buffer and size indicating the data to write. The file will not
// actually be updated on the storage until either sync or close is called.
//
// Returns the number of bytes written, or a negative error code on failure.
lfs_ssize_t lfs_file_write(lfs_t *lfs, lfs_file_t *file,
        const void *buffer, lfs_size_t size);
#endif

// Change the position of the file
//
// The change in position is determined by the offset and whence flag.
// Returns the new position of the file, or a negative error code on failure.
lfs_soff_t lfs_file_seek(lfs_t *lfs, lfs_file_t *file,
        lfs_soff_t off, int whence);

#ifndef LFS_READONLY
// Truncates the size of the file to the specified size
//
// Returns a negative error code on failure.
int lfs_file_truncate(lfs_t *lfs, lfs_file_t *file, lfs_off_t size);
#endif

// Return the position of the file
//
// Equivalent to lfs_file_seek(lfs, file, 0, LFS_SEEK_CUR)
// Returns the position of the file, or a negative error code on failure.
lfs_soff_t lfs_file_tell(lfs_t *lfs, lfs_file_t *file);

// Change the position of the file to the beginning of the file
//
// Equivalent to lfs_file_seek(lfs, file, 0, LFS_SEEK_SET)
// Returns a negative error code on failure.
int lfs_file_rewind(lfs_t *lfs, lfs_file_t *file);

// Return the size of the file
//
// Similar to lfs_file_seek(lfs, file, 0, LFS_SEEK_END)
// Returns the size of the file, or a negative error code on failure.
lfs_soff_t lfs_file_size(lfs_t *lfs, lfs_file_t *file);


/// Directory operations ///

#ifndef LFS_READONLY
// Create a directory
//
// Returns a negative error code on failure.
int lfs_mkdir(lfs_t *lfs, const char *path);
#endif

// Open a directory
//
// Once open a directory can be used with read to iterate over files.
// Returns a negative error code on failure.
int lfs_dir_open(lfs_t *lfs, lfs_dir_t *dir, const char *path);

// Close a directory
//
// Releases any allocated resources.
// Returns a negative error code on failure.
int lfs_dir_close(lfs_t *lfs, lfs_dir_t *dir);

// Read an entry in the directory
//
// Fills out the info structure, based on the specified file or directory.
// Returns a positive value on success, 0 at the end of directory,
// or a negative error code on failure.
int lfs_dir_read(lfs_t *lfs, lfs_dir_t *dir, struct lfs_info *info);

// Change the position of the directory
//
// The new off must be a value previous returned from tell and specifies
// an absolute offset in the directory seek.
//
// Returns a negative error code on failure.
int lfs_dir_seek(lfs_t *lfs, lfs_dir_t *dir, lfs_off_t off);

// Return the position of the directory
//
// The returned offset is only meant to be consumed by seek and may not make
// sense, but does indicate the current position in the directory iteration.
//
// Returns the position of the directory, or a negative error code on failure.
lfs_soff_t lfs_dir_tell(lfs_t *lfs, lfs_dir_t *dir);

// Change the position of the directory to the beginning of the directory
//
// Returns a negative error code on failure.
int lfs_dir_rewind(lfs_t *lfs, lfs_dir_t *dir);


/// Filesystem-level filesystem operations

// Finds the current size of the filesystem
//
// Note: Result is best effort. If files share COW structures, the returned
// size may be larger than the filesystem actually is.
//
// Returns the number of allocated blocks, or a negative error code on failure.
lfs_ssize_t lfs_fs_size(lfs_t *lfs);

// Traverse through all blocks in use by the filesystem
//
// The provided callback will be called with each block address that is
// currently in use by the filesystem. This can be used to determine which
// blocks are in use or how much of the storage is available.
//
// Returns a negative error code on failure.
int lfs_fs_traverse(lfs_t *lfs, int (*cb)(void*, lfs_block_t), void *data);

#ifndef LFS_READONLY
#ifdef LFS_MIGRATE
// Attempts to migrate a previous version of littlefs
//
// Behaves similarly to the lfs_format function. Attempts to mount
// the previous version of littlefs and update the filesystem so it can be
// mounted with the current version of littlefs.
//
// Requires a littlefs object and config struct. This clobbers the littlefs
// object, and does not leave the filesystem mounted. The config struct must
// be zeroed for defaults and backwards compatibility.
//
// Returns a negative error code on failure.
int lfs_migrate(lfs_t *lfs, const struct lfs_config *cfg);
#endif
#endif






#ifdef __cplusplus
}
#endif


#endif /* __FS_API_H__ */