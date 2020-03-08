/******************************************************************************
  File Name     : humituremanager.cpp
  Description   : SZ-WS系列(上海拓福电气)温湿度变送器通讯管理模块
  Author        : LiangLiCan
  Created       : 2020/03/08
  History       :
1.Date          :
  Author        :
  Modification  :
******************************************************************************/
#include <sys/prctl.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "humituremanager.h"
#include "commfunction.h"

#define FRAME_EXTRA_LEN   5  /* 不包含数据的帧长度,即DestAddr + MSG_TYPE + DataLen + CRC */

CHumitureManager::CHumitureManager(const WD_C8 *pTtyDevPath, const SERIAL_S *pstSerialPara, WD_U8 SensorAdd) :
    m_fRtTemper(0), m_u32RtHumidity(0), m_enBaudRate(BR_9600)

{
    assert(pTtyDevPath != NULL && pstSerialPara != NULL);

    m_u8SensorAdd = SensorAdd;

    /* 初始化串口并连接 */
    m_pCUart = new CUartOperator();
    m_pCUart->init(pTtyDevPath, pstSerialPara);
    
    CreateNormalThread(SendMsgThread, this, NULL);
    CreateNormalThread(ReceMsgThread, this, NULL);
    CreateNormalThread(cycleGetDeviceParam, this, NULL);
}


/*******************************************
* Function Name : AddSendFrameToList
* Parameter     : enAddr是寄存器地址, bWrites是否是写寄存器
* Description   : 构建一帧完整的485数据，包括地址、功能码、CRC的赋值,并加入到发送链表中去
* Return Value  : On success, 0 is returned.  
                  錯誤返回非0.
* Author        : LiangLiCan
* Created       : 2019/11/26
********************************************/
WD_S32 CHumitureManager::AddSendFrameToList(REGISTER_ADD_E enAddr, WD_U16 u16ReadNum/* = 0 */, bool bWrite/* = false */,
	                                    WD_U16 u16SetData/* = 0 */)
{
    CObjectLock ObjLock(&m_MuteSendLock);
    
    WD_U8 *pNode = NULL;
    WD_U8 aFrameHead[8] = {0};
    WD_S32 ret = 0;
    WD_U16 u16Temp = enAddr;
    aFrameHead[SFI_DEST_ADDR] = m_u8SensorAdd; // 总线上的设备地址
    aFrameHead[SFI_MSG_TYPE]  = bWrite ? MT_WRITE : MT_READ;
    ShortToChar(u16Temp, &aFrameHead[SFI_REG_ADDR], true);
    
    if(bWrite){
        ShortToChar(u16SetData, &aFrameHead[SFI_REG_PARM], true);
    }
    else {
        ShortToChar(u16ReadNum, &aFrameHead[SFI_REG_PARM], true);
    }
    // 填充CRC
    ShortToChar(createCrcCode(aFrameHead, 6), &aFrameHead[SFI_CRC]);    
    pNode = (WD_U8 *)malloc(sizeof(aFrameHead)); /* 发送线程会free掉它 */
    memcpy(pNode, aFrameHead, sizeof(aFrameHead));
    
    /* 添加入发送的列表 */
    m_SendBufLock.Lock();
    if (m_pSendBufList.empty()){
        m_SendBufLock.Signal();
    }
    m_pSendBufList.push_back(pNode);
    m_SendBufLock.UnLock();
    return ret;
}

WD_VOID CHumitureManager::handleMsg(WD_U8 *pMsgData, WD_U32 )
{
    if(pMsgData[RFI_DEST_ADDR] != 0x1){// 判断有效性
        return ;
    }
    
    if(pMsgData[RFI_MSG_TYPE] == MT_READ)
    {
        /* Baud Rate */
        m_enBaudRate = (BAUD_RATE_E)pMsgData[RFI_DATA + 1];
        
        WD_U8 u8Backup = pMsgData[RFI_DATA + 2];
        /* 温度 */
         /* bit 15为温度正负值,0为正, 1为负 */ 
        pMsgData[RFI_DATA + 2] &= 0x7F;
        m_fRtTemper = (u8Backup & 0x80 ? -CharToShort(&pMsgData[RFI_DATA + 2], true)
                                        : CharToShort(&pMsgData[RFI_DATA + 2], true)) / 10;
        /* 湿度 */
        m_u32RtHumidity = CharToShort(&pMsgData[RFI_DATA + 4], true);
    }
    else if(pMsgData[RFI_MSG_TYPE] == MT_READ_ERR)
    {
        m_fRtTemper = 0;
        m_u32RtHumidity = 0;
        //DBG_HUMI_PRINT(LEVEL_ERROR, "Get Invalid read msg\n");
    }
    //printf("\033[0;31m [%s][%d]m_enBaudRate=%d Temper=%02f°C Humidity=%d\033[0;39m \n", __func__, __LINE__,m_enBaudRate, m_fRtTemper, m_u32RtHumidity);
}

WD_VOID* CHumitureManager::SendMsgThread(WD_VOID *arg)
{
    CHumitureManager *pCHumiture = (CHumitureManager *)arg;
    pCHumiture->SendMsgThreadBody();  
    return NULL;
}
/*******************************************
* Function Name : SendMsgThreadBody
* Parameter     : 
* Description   : 从队列中取出数据进行发送
* Return Value  : 
                  
* Author        : LiangLiCan
* Created       : 2019/11/26
********************************************/
WD_VOID CHumitureManager::SendMsgThreadBody()
{
    prctl(PR_SET_NAME, (WD_U32 *)"HumitSend");
    WD_U8 *pu8SenBufNode = NULL;
    while(1)
    {
        /* 从消息队列中取出一条发送的消息 */
        m_SendBufLock.Lock();
        if (m_pSendBufList.empty()){
            m_SendBufLock.Wait();
        }
        pu8SenBufNode = m_pSendBufList.front();
        m_pSendBufList.pop_front();
        m_SendBufLock.UnLock();
        
        m_pCUart->writeData(pu8SenBufNode, 8);
        delete pu8SenBufNode;
    }
}

WD_VOID* CHumitureManager::ReceMsgThread(WD_VOID *arg)
{
    CHumitureManager *pCHumiture = (CHumitureManager *)arg;
    pCHumiture->ReceMsgThreadBody();
    return NULL;
}

WD_VOID CHumitureManager::ReceMsgThreadBody()
{
    prctl(PR_SET_NAME, (WD_U32 *)"HumiRece");
    WD_S32 readLen   = 0;
    WD_S32 readCount = 0; /* 已读数据长度 */
    WD_S32 MaxBufLen = sizeof(WD_U8) * MAX_FRAME_LEN;
    m_pReadBuf = new WD_U8[MaxBufLen];
    
    while(1)
    {  
        if(!m_pCUart->dataAvailable(100)){
            continue;
        }
        memset(m_pReadBuf, 0, sizeof(MaxBufLen));
        readCount = 0;
        do{
            if(m_pCUart->dataAvailable(100))
            {
                readLen = m_pCUart->readData(m_pReadBuf + readCount, MaxBufLen);
                if (readLen < 0){
                    break;
                }
                readCount += readLen;
            }
        }while(readCount < m_pReadBuf[RFI_DATA_LEN] + FRAME_EXTRA_LEN);
        
        handleMsg(m_pReadBuf, readCount);
    }
    delete [] m_pReadBuf;
}

WD_VOID* CHumitureManager::cycleGetDeviceParam(WD_VOID *arg)
{
    CHumitureManager *pCHumitu = (CHumitureManager *)arg;
    pCHumitu->cycleGetDeviceParamBody();
    return NULL;
}

WD_VOID CHumitureManager::cycleGetDeviceParamBody()
{
    prctl(PR_SET_NAME, (WD_U32 *)"HumituGet");
                                         
    while(1)
    {
        if (m_pCUart->isInit()){
            AddSendFrameToList(RA_BAUD_RATE, 3);
        }
        mSleep(2000);
    }
}

WD_U16 CHumitureManager::createCrcCode(WD_U8 *data, WD_U32 dataLen)
{
    WD_U16 u16CrcCode = 0;
    WD_U8 SaveHi, SaveLo;
    WD_U8 CRCLo = 0xFF;   
    WD_U8 CRCHi = 0xFF;
    for(WD_U8 i = 0; i < dataLen; ++i)
    {
        CRCLo = CRCLo^data[i];
        for(WD_U8 Flag = 0; Flag < 8; ++Flag)
        {
            SaveHi = CRCHi;
            SaveLo = CRCLo;
            CRCHi = CRCHi >> 1;
            CRCLo = CRCLo >> 1;
            if ((SaveHi & 1) == 1)
            {
                CRCLo = CRCLo | 0x80;
            }

            if ((SaveLo & 1) == 1)
            {
                CRCHi = CRCHi^0xA0;
                CRCLo = CRCLo^0x01;
            }
            
        }
    }
    u16CrcCode |= CRCLo;
    u16CrcCode |= CRCHi << 8;
    return u16CrcCode;
}

