/******************************************************************************
  File Name     : humituremanager.h
  Description   : SZ-WS系列(上海拓福电气)温湿度变送器通讯管理模块
  Author        : LiangLiCan
  Created       : 2020/03/08
  History       :
1.Date          :
  Author        :
  Modification  :
******************************************************************************/
#ifndef _HUMITURE_MANAGER_H_
#define _HUMITURE_MANAGER_H_

#include <iostream>
#include <list>
#include "datatype.h"
#include "lock.h"
#include "uart_operator.h"

#define MAX_FRAME_LEN  255   /* 发送和接收buf的最大长度(每一帧数据) */
/************************************************************************************************
 * 传感器->主站RS485帧结构
 *  _______________________________________________________________________________
 * | DestAddr 1Byte | MSG_TYPE 1Byte| DataLen 1Byte  | Data <= 255Byte | CRC 2Byte |
 * |________________|_______________|________________|_________________|___________|
 *
 * 主站->传感器 RS485帧结构
 *  ____________________________________________________________________________________________
 * | DestAddr 1Byte | MSG_TYPE 1Byte| Register star add 2Byte  | Register Num 2Byte | CRC 2Byte |
 * |________________|_______________|__________________________|____________________|___________|
 *
 ************************************************************************************************/

class CUartOperator;
class CHumitureManager
{
public:
     enum MSG_TYPE_E          /* 报文类型 */
     {
         MT_READ      = 0x03, /* 读数据 */
         MT_WRITE     = 0x06, /* 写数据 */
         MT_READ_ERR  = 0x83, /* 读数据异常报文 */
         MT_WRITE_ERR = 0x86, /* 写数据异常报文 */
         MT_BUTT
     };
     
     enum REGISTER_ADD_E          /* 寄存器地址 */
     {
         //RA_SENSOR      = 0x01, /* 传感器地址 */
         RA_BAUD_RATE   = 0x02,   /* 波特率地址 */
         RA_TEMPERATURE = 0x03,   /* 温度地址 */
         RA_HUMIDITY    = 0x04,   /* 湿度地址 */
         RA_BUT
     };
     
     /* 主站->传感器RS485帧数据索引 */
      enum SEND_FRAME_INDEX_E
      {
         SFI_DEST_ADDR = 0,
         SFI_MSG_TYPE  = 1,
         SFI_REG_ADDR  = 2,
         SFI_REG_PARM  = 4, /* 寄存器个数或寄存器值 */
         SFI_CRC       = 6,
         SFI_BUTT
      };
      
    /* 传感器->主站RS485帧数据索引 */
     enum REC_FRAME_INDEX_E
     {
        RFI_DEST_ADDR = 0,
        RFI_MSG_TYPE  = 1,
        RFI_DATA_LEN  = 2,
        RFI_DATA      = 3,
        RFI_BUTT
     };
     
    CHumitureManager(const WD_C8 *pTtyDevPath, const SERIAL_S *pstSerialPara, WD_U8 SensorAdd);
    ~CHumitureManager();
        
    /* parameter */
    WD_FLOAT temperature() const {return m_fRtTemper;};
    WD_U32 humidity()const {return m_u32RtHumidity;}
    BAUD_RATE_E baudRate()const{return m_enBaudRate;}
    
private:
    WD_U16 createCrcCode(WD_U8 *data, WD_U32 dataLen);
    
	static WD_VOID* ReceMsgThread(WD_VOID *arg);
	WD_VOID ReceMsgThreadBody();
	
	static WD_VOID* SendMsgThread(WD_VOID *arg);
	WD_VOID SendMsgThreadBody();
	
	static WD_VOID* cycleGetDeviceParam(WD_VOID *arg);
	WD_VOID cycleGetDeviceParamBody();
	
	WD_S32 AddSendFrameToList(REGISTER_ADD_E enAddr, WD_U16 u16ReadNum = 0, bool bWrite = false,
	                                    WD_U16 u16SetData = 0 );
	WD_VOID handleMsg(WD_U8 *pMsgData, WD_U32 MsgLen);
	
private:
    WD_U8 m_u8SensorAdd; /* 传感器地址 */

    CUartOperator *m_pCUart;
    WD_U8 *m_pReadBuf;
    
    std::list<WD_U8 *> m_pSendBufList;
    CCondLock m_SendBufLock;  /* 对m_pSendBufList操作同步锁 */
    CMuteLock m_MuteSendLock; /* 发送消息锁 */

    WD_FLOAT m_fRtTemper;   /* 实时温度, 单位°C */
    WD_U32 m_u32RtHumidity; /* 实时湿度, 单位%RH */
    BAUD_RATE_E m_enBaudRate;
};

#endif // _HUMITURE_MANAGER_H_
