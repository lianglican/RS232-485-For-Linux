/********************************************************
  File name: datatype.h
  Author:LiangLiCan
  Date: 2019/06/1
  Description: 定义通用数据类型
********************************************************/
#ifndef _COMMDEF_H_
#define _COMMDEF_H_

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif // end of __cplusplus

#define WD_SUCCESS			0
#define WD_FAILURE			(-1)

//#ifndef	NULL
//#define	NULL 		0
//#endif

typedef unsigned char           WD_U8;
typedef unsigned short          WD_U16;
typedef unsigned int            WD_U32;
typedef unsigned long           WD_ULONG;
typedef unsigned long long      WD_U64;

typedef  char                    WD_C8;
typedef  unsigned char           WD_S8;
typedef  short                   WD_S16;
typedef  int                     WD_S32;
typedef  long                    WD_SLONG;
typedef  long long               WD_S64;

typedef float                   WD_FLOAT;
typedef double                  WD_DOUBLE;
typedef void                    WD_VOID;

//typedef bool                    WD_BOOL;

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif // __cplusplus

#endif // _COMMDEF_H_
