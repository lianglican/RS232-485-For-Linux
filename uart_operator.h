/******************************************************************************
  File Name     : uart_operator.h
  Description   : 串口相关操作
  Author        : LiangLiCan
  Created       : 2019/06/26
  History       :
1.Date          :
  Author        :
  Modification  :
******************************************************************************/
#ifndef _UART_OPERATION_H_
#define _UART_OPERATION_H_

#include "datatype.h"
#include "lock.h"

/* 串口波特率定义 */
typedef enum _BAUD_RATE_E
{
    BR_1200,
    BR_2400, // 1
    BR_4800,
    BR_9600,
    BR_14400,
    BR_19200, // 5
    BR_38400,
    BR_57600,
    BR_115200,
    BR_230400,
    BR_380400, // 10
    BR_460800,
    BR_921600,
    BR_BUTT,
}BAUD_RATE_E;

typedef struct _SERIAL_S
{
    WD_U8  u8BaudRate;        /* 波特率，0-1200，1-2400，2-4800，3-9600 参照 BAUD_RATE_E */
    WD_U8  u8DataBit;         /* 数据位，0-8，1-7，2-6，3-5 */
    WD_U8  u8StopBit;         /* 停止位，0-1，1-1.5, 2-2 */
    WD_U8  u8Check;           /* 校验，0-None，1-Odd，2-Even,3-Space */
    WD_U8  reserve[16];
}SERIAL_S;

class CUartOperator
{
public:

    CUartOperator();
    ~CUartOperator();
    
    WD_S32 init(const WD_C8 *pTtyDevPath, const SERIAL_S *pstSerialPara);
    WD_VOID DeInit();
    
    inline bool isInit(){return m_bIsInit;}
    WD_S32 writeData(WD_U8 *pBuf, WD_S32 writeSize);
    WD_S32 readData(WD_U8 *pBuf, WD_S32 BuffSize);
    bool dataAvailable(WD_S32 TimeOutMsec);
    
private:
    WD_S32 SetBaudRate(WD_S32    DevFd, BAUD_RATE_E enBaudRate);
    WD_S32 SetDataBit(WD_S32    DevFd, WD_U8 enDataBit);
    WD_S32 SetStopBit(WD_S32    DevFd, WD_U8 enStopBit);
    WD_S32 SetCheck(WD_S32    DevFd, WD_U8 enCheck);
    
private:
    
    bool m_bIsInit;
    WD_S32 m_s32DevFd;
    CMuteLock m_MuteRWLock; 
};

#endif // _UART_OPERATION_H_

