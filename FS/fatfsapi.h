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
#ifndef __FAT_FS_API_H__
#define __FAT_FS_API_H__

#ifdef __cplusplus
extern "C" {
#endif


#include "ff.h"


// 文件系统初始化
void FileSystemIint(void);









#ifdef __cplusplus
}
#endif


#endif /* __FAT_FS_API_H__ */