/******************************************************************************
  File Name     : rs485_operation.cpp
  Description   : RS485相关操作
  Author        : LiangLiCan
  Created       : 2019/06/26
  History       :
1.Date          :
  Author        :
  Modification  :
******************************************************************************/
#include <stdio.h>
#include <unistd.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "uart_operator.h"

CUartOperator::CUartOperator()
{
    m_bIsInit = false;
    m_s32DevFd = 0;
}

CUartOperator::~CUartOperator()
{

}

/*******************************************
* Function Name : init
* Parameter     : pTtyDevPath,串口所使用的tty设备绝对路径,如/dev/tty02
* Description   : 配置串口参数
* Return Value  : On success, 0 is returned.  
                  On error, -1 is returned
* Author        : LiangLiCan
* Created       : 2019/06/26
********************************************/
WD_S32 CUartOperator::init(const WD_C8 *pTtyDevPath, const SERIAL_S *pstSerialPara)
{
    if (NULL == pTtyDevPath || NULL == pstSerialPara)
    {
        //DBG_UART_PRINT(LEVEL_ERROR, "Get Null pointer, Check!!!\n");
        printf("Get Null pointer, Check!!!\n");
        return WD_FAILURE;
    }
    /* O_NOCTTY : 表示当前进程不期望与终端关联 */
    m_s32DevFd  = open(pTtyDevPath, O_RDWR | O_NOCTTY);
    if (m_s32DevFd < 0)
    {
        printf("Open dev %s fail! \n", pTtyDevPath);
        return WD_FAILURE;
    }
    printf("Open dev %s success! \n", pTtyDevPath);

    /* 先清空参数 */
    struct termios stOldParm;
	bzero(&stOldParm, sizeof(stOldParm));
	tcsetattr(m_s32DevFd, TCSANOW, &stOldParm);
	
    if (SetBaudRate(m_s32DevFd, (BAUD_RATE_E)pstSerialPara->u8BaudRate) != WD_SUCCESS)
    {
        printf("SetBaudRate(%d) fail! \n", pstSerialPara->u8BaudRate);
        close(m_s32DevFd);
        return WD_FAILURE;
    }
    
    if (SetDataBit(m_s32DevFd, pstSerialPara->u8DataBit) != WD_SUCCESS)
    {
        printf("SetDataBit fail! \n");
        close(m_s32DevFd);
        return WD_FAILURE;
    }
    
    if (SetCheck(m_s32DevFd, pstSerialPara->u8Check) != WD_SUCCESS)
    {
        printf("SetCheck fail! \n");
        close(m_s32DevFd);
        return WD_FAILURE;
    }
        
    if (SetStopBit(m_s32DevFd, pstSerialPara->u8StopBit) != WD_SUCCESS)
    {
        printf("SetStopBit fail! \n");
        close(m_s32DevFd);
        return WD_FAILURE;
    }
    
    m_bIsInit = true;
    return WD_SUCCESS;
}

WD_VOID CUartOperator::DeInit()
{
    if (m_s32DevFd > 0)
    {
        close(m_s32DevFd);
    }
    
    m_bIsInit = false;
}

/*******************************************
* Function Name : readData
* Parameter     : pBuf, 存储所读取的数据，BuffSize，传进来的buf大小
* Description   : 读取串口信息
* Return Value  : 成功返回所读取到的字节数,失败返回-1;  
* Author        : LiangLiCan
* Created       : 2019/06/26
********************************************/
WD_S32 CUartOperator::readData(WD_U8 *pBuf, WD_S32 BuffSize)
{
    if(!m_bIsInit)
    {
        //DBG_UART_PRINT(LEVEL_ERROR, "UnInit, Check!!!\n");
        printf("UnInit, Check!!!\n");
        return WD_FAILURE;
    }
    
    if (NULL == pBuf)
    {
        printf("Get Null pointer, Check!!!\n");
        return WD_FAILURE;
    }
    
    CObjectLock ObjLock(&m_MuteRWLock);
    
    WD_S32 readCount = 0;
    readCount = read(m_s32DevFd, pBuf, BuffSize);
    if (readCount < 0)
    {
        printf("read serial err, fd=%d  errno= %s Check!!!\n", m_s32DevFd, strerror(errno));
        return WD_FAILURE;
    }
    //DBG_UART_PRINT(LEVEL_ERROR, "m_s32DevFd=%d readCount=%d, BuffSize=%d \n",m_s32DevFd, readCount, BuffSize);
    return readCount;
}

WD_S32 CUartOperator::writeData(WD_U8 *pBuf, WD_S32 writeSize)
{
    if(!m_bIsInit){
        return WD_FAILURE;
    }
    
    if (NULL == pBuf)
    {
        printf("Get Null pointer, Check!!!\n");
        return WD_FAILURE;
    }
    
    CObjectLock ObjLock(&m_MuteRWLock);
    
    WD_S32 writeCount = 0;
    writeCount = write(m_s32DevFd, pBuf, writeSize);
    if (writeCount != writeSize)
    {
        printf("write err, writeCount=%d, writeSize=%d \n", writeCount, writeSize);
        return WD_FAILURE;
    }
    //DBG_UART_PRINT(LEVEL_ERROR, "m_s32DevFd=%d writeCount=%d\n",m_s32DevFd, writeCount);
    return WD_SUCCESS;
}

/*******************************************
* Function Name : SetCheck
* Parameter     : enCheck, 校验方式对应的枚举，参考SERIAL_S结构体定义
* Description   : 设置奇偶校验
* Return Value  : On success, 0 is returned.  
                  On error, -1 is returned
* Author        : LiangLiCan
* Created       : 2019/06/26
********************************************/
WD_S32 CUartOperator::SetCheck(WD_S32    DevFd, WD_U8 enCheck)
{
    if (DevFd < 0)
    {
        printf("Invalid Fd %d ,Check!!! \n", DevFd);
        return WD_FAILURE;
    }
    WD_S8 aCheckPrint[10] = {0};
    struct termios stTermios;
    tcgetattr(DevFd, &stTermios); /* 获取串口参数 */
    
	stTermios.c_cflag &= ~PARENB;    /* Clear parity enable */
	stTermios.c_iflag &= ~INPCK;	 /* Enable parity checking */
	//stTermios.c_cflag &= ~PARODD;
	stTermios.c_iflag &= ~CMSPAR;
	
    switch(enCheck)
    {
        case 0: // None
        {
            stTermios.c_cflag &= ~PARENB;  /* Clear parity enable */
            stTermios.c_iflag &= ~INPCK;   /* Enable parity checking */
            memcpy(aCheckPrint, "None", sizeof(aCheckPrint));
            break;
        }
        case 1: // Odd
        {
            stTermios.c_cflag |= (PARODD | PARENB);  /* 设置为奇效验*/ 
            stTermios.c_iflag |= INPCK;              /* Disnable parity checking */ 
            memcpy(aCheckPrint, "Odd", sizeof(aCheckPrint));
            break;
        }
        case 2: // Even
        {
            stTermios.c_cflag |= PARENB;     /* Enable parity */    
            stTermios.c_cflag &= ~PARODD;    /* 转换为偶效验*/     
            stTermios.c_iflag |= INPCK;      /* Disnable parity checking */
            memcpy(aCheckPrint, "Even", sizeof(aCheckPrint));
            break;
        }
        case 3: // Space
        {
            stTermios.c_cflag &= ~PARENB;  
            stTermios.c_cflag &= ~CSTOPB;
            memcpy(aCheckPrint, "Space", sizeof(aCheckPrint));
            break;
        }
        default:
        {
            printf("Unknow Check Param %d , Check!!! \n", enCheck);
            return WD_FAILURE;
        }       
    }
    
	tcflush(DevFd, TCIFLUSH); /* 先清空输入缓冲区 */
    if (tcsetattr(DevFd, TCSANOW, &stTermios) != 0)  /* 设置串口参数 */
    {
        printf("tc set attr error!!!\n");
        return WD_FAILURE;
    }
    
    printf("Set Check(%s)\n", aCheckPrint);
    return WD_SUCCESS;
}

/*******************************************
* Function Name : SetStopBit
* Parameter     : enStopBit, 停止位对应的枚举，参考SERIAL_S结构体定义
* Description   : 设置串口停止位
* Return Value  : On success, 0 is returned.  
                  On error, -1 is returned
* Author        : LiangLiCan
* Created       : 2019/06/26
********************************************/
WD_S32 CUartOperator::SetStopBit(WD_S32    DevFd, WD_U8 enStopBit)
{
    if (DevFd < 0)
    {
        printf("Invalid Fd %d ,Check!!! \n", DevFd);
        return WD_FAILURE;
    }
    WD_S32 StopBitPrint = 0;
    struct termios stTermios;
    tcgetattr(DevFd, &stTermios); /* 获取串口参数 */
    switch(enStopBit)
    {
        case 0: // 1
        {
            stTermios.c_cflag &= ~CSTOPB;
            StopBitPrint = 1;
            break;
        }
        case 2: // 2
        {
            stTermios.c_cflag |= CSTOPB;
            StopBitPrint = 2;
            break;
        }
        case 1: // 1.5
        default:
        {
            printf("Unknow Stop Bit %d ,Check!!! \n", enStopBit);
            return WD_FAILURE;
        }       
    }
    
	tcflush(DevFd, TCIFLUSH); /* 先清空输入缓冲区 */
    if (tcsetattr(DevFd, TCSANOW, &stTermios) != 0)  /* 设置串口参数 */
    {
        printf("tc set attr error!!!\n");
        return WD_FAILURE;
    }
    
    printf("Set Stop Bit %d \n", StopBitPrint);
    return WD_SUCCESS;
}

/*******************************************
* Function Name : SetDataBit
* Parameter     : enDataBit, 数据位对应的枚举，参考SERIAL_S结构体定义
* Description   : 设置串口数据位
* Return Value  : On success, 0 is returned.  
                  On error, -1 is returned
* Author        : LiangLiCan
* Created       : 2019/06/26
********************************************/
WD_S32 CUartOperator::SetDataBit(WD_S32    DevFd, WD_U8 enDataBit)
{
    if (DevFd < 0)
    {
        printf("Invalid Fd %d ,Check!!! \n", DevFd);
        return WD_FAILURE;
    }
    
    struct termios stTermios;
    tcgetattr(DevFd, &stTermios); /* 获取串口参数 */

    stTermios.c_cflag &= ~CSIZE;  /* 屏蔽字符大小位 */
    WD_S32 DataBitPrint = 0;
    switch(enDataBit)
    {
        case 0: // 8
        {
            stTermios.c_cflag |= CS8;
            DataBitPrint = 8;
            break;
        }
        case 1: // 7
        {
            stTermios.c_cflag |= CS7;
            DataBitPrint = 7;
            break;
        }
        case 2: // 6
        {
            stTermios.c_cflag |= CS6;
            DataBitPrint = 6;
            break;
        }
        case 3: // 5
        {
            stTermios.c_cflag |= CS5;
            DataBitPrint = 5;
            break;
        }
        default:
        {
            printf("Unknow Data Bit %d ,Check!!! \n", enDataBit);
            return WD_FAILURE;
        }       
    }
    
	tcflush(DevFd, TCIFLUSH); /* 先清空输入缓冲区 */
    if (tcsetattr(DevFd, TCSANOW, &stTermios) != 0)  /* 设置串口参数 */
    {
        printf("tc set attr error!!!\n ");
        return WD_FAILURE;
    }
    
    printf("Set Data Bit = %d\n", DataBitPrint);
    return WD_SUCCESS;   
}

/*******************************************
* Function Name : SetBaudRate
* Parameter     : 
* Description   : 设置串口波特率
* Return Value  : On success, 0 is returned.  
                  On error, -1 is returned
* Author        : LiangLiCan
* Created       : 2019/06/26
********************************************/
WD_S32 CUartOperator::SetBaudRate(WD_S32    DevFd, BAUD_RATE_E enBaudRate)
{
    if (DevFd < 0)
    {
        printf("Invalid Fd %d ,Check!!! \n", DevFd);
        return WD_FAILURE;
    }
    WD_S32 s32Speed = 0;    
    WD_S32 s32SpeedPrint = 0;  
    
    switch(enBaudRate)
    {
        case BR_1200:
        {
            s32Speed = B1200;
            s32SpeedPrint = 1200;
            break;
        }
        case BR_2400:
        {
            s32Speed = B2400;
            s32SpeedPrint = 2400;
            break;
        }
        case BR_4800:
        {
            s32Speed = B4800;
            s32SpeedPrint = 4800;
            break;
        }
        case BR_9600:
        {
            s32Speed = B9600;
            s32SpeedPrint = 9600;
            break;
        }
        case BR_19200:
        {
            s32Speed = B19200;
            s32SpeedPrint = 19200;
            break;
        }
        case BR_38400:
        {
            s32Speed = B38400;
            s32SpeedPrint = 38400;
            break;
        }
        case BR_57600:
        {
            s32Speed = B57600;
            s32SpeedPrint = 57600;
            break;
        }
        case BR_115200:
        {
            s32Speed = B115200;
            s32SpeedPrint = 115200;
            break;
        }
        case BR_230400:
        {
            s32Speed = B230400;
            s32SpeedPrint = 230400;
            break;
        }
        case BR_460800:
        {
            s32Speed = B460800;
            s32SpeedPrint = 460800;
            break;
        }
        case BR_921600:
        {
            s32Speed = B921600;
            s32SpeedPrint = 921600;
            break;
        }
        case BR_14400:
        case BR_380400:
        default:
        {
            printf("Unsupport Baud Rate %d \n", enBaudRate);
            return WD_FAILURE;
        }      
    }
    
    struct termios stTermios;
    tcgetattr(DevFd, &stTermios);           /* 获取串口参数 */
    
    tcflush(DevFd, TCIOFLUSH);              /* 先清空输入、输出缓冲区 */
    
    cfsetispeed(&stTermios, s32Speed);      /* 设置波特率 */
    cfsetospeed(&stTermios, s32Speed);
    stTermios.c_cflag |= (CLOCAL | CREAD);
    
    if (tcsetattr(DevFd, TCSANOW, &stTermios) != 0)  /* 设置串口参数 */
    {
        printf("tc set attr error!!!\n ");
        return WD_FAILURE;
    }
    tcflush(DevFd, TCIOFLUSH);              /* 再次清空输入、输出缓冲区 */
    
    printf("Set Baud Rate %d \n", s32SpeedPrint);
    return WD_SUCCESS;
}

/*******************************************
* Function Name : dataAvailable
* Parameter     : TimeOutMsec 超时时间,单位毫秒
* Description   : 判断串口是否有数据可读
* Return Value  : On success, true is returned.
                  On error, false is returned
* Author        : LiangLiCan
* Created       : 2019/07/01
********************************************/
bool CUartOperator::dataAvailable(WD_S32 TimeOutMsec)
{
	struct timeval timeout;
	fd_set readfds;

    if (TimeOutMsec == 0) 
    {
    	// no waiting
        timeout.tv_sec = 0;
        timeout.tv_usec = 0;
    } 
    else 
    {
        timeout.tv_sec = TimeOutMsec / 1000;
        timeout.tv_usec = (TimeOutMsec % 1000) * 1000;
    }

    FD_ZERO(&readfds);
    FD_SET(m_s32DevFd, &readfds);

    if (select(m_s32DevFd + 1, &readfds, NULL, NULL, &timeout) > 0) 
    {
        return true; // data is ready
    } 
    else
    {
        return false; // no data
    }
}

