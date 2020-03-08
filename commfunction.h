/******************************************************************************
  File Name     : commfunction.h
  Description   : 通用函数接口
  Author        : LiangLiCan
  Created       : 2019/6/6
  History       :
1.Date          :
  Author        :
  Modification  :
******************************************************************************/
#ifndef _COMM_FUNCTION_H_
#define _COMM_FUNCTION_H_

#include "datatype.h"
#include <pthread.h>

#define NORMAL_THREAD_STACK_SIZE (1536 * 1024)  // 1.5M
typedef	void *(*ThreadEntryPtrType)(void *);

WD_S32 CreateNormalThread(ThreadEntryPtrType entry, void *para, pthread_t *pid, WD_S32 statckSize = 0);
WD_U64 GetOsNanosecTime();
WD_VOID mSleep(WD_U32   MilliSecond);

WD_U16 CharToShort(WD_U8 *b, bool isBigEndian = false );
WD_VOID ShortToChar(WD_U16 Short, WD_U8 *byte, bool isBigEndian = false);

WD_U32 CharToInt(WD_U8* b, bool isBigEndian = false );
WD_VOID IntToChar(WD_U32 Int, WD_U8 *byte, bool isBigEndian = false );

WD_FLOAT toPrecision(WD_U8 precision);
WD_FLOAT toFloat(WD_U8 *b);

bool getNetLinkState(const WD_C8 *pu8IfName);

bool checkDirExist(const WD_C8 *pu8Dir, bool bMake = false);

#endif // _COMM_FUNCTION_H_


