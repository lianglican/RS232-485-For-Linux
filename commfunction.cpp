/******************************************************************************

  Copyright (C), 2018, xxxxxxxxx.

 ******************************************************************************
  File Name     : commfunction.c
  Version       : 
  Author        : Lianglican
  Created       : 2018/12/18
  Description   :
  History       :
  1.Date        : 2018/12/18
    Author      :
    Modification: Created file

******************************************************************************/
#include <sys/select.h>
#include <time.h>
#include <stdio.h>
#include <linux/sockios.h>
#include <linux/mii.h>
#include <sys/ioctl.h>
#include <sys/types.h>          /* See NOTES */
#include <sys/socket.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <net/if.h>
#include <linux/ethtool.h>
#include <dirent.h>
#include <sys/stat.h>
#include <time.h>
#include <sys/time.h>
#include "commfunction.h"

/*******************************************
* FUNCTION NAME : CreateNormalThread
* PARAMETER 	: 
* DESCRIPTION	: create an Normal Thread
* RETURN VALUE	：On success, 0 is returned.
				  On error, -1 is returned
********************************************/
WD_S32 CreateNormalThread(ThreadEntryPtrType entry, WD_VOID *para, pthread_t *pid, WD_S32 statckSize /* =0 */)
{
    pthread_t ThreadId;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    if (statckSize){
        pthread_attr_setstacksize(&attr, statckSize);
    }
    else{
		pthread_attr_setstacksize(&attr, NORMAL_THREAD_STACK_SIZE); 
    }
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);// 绑定
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);// 分离
    if(pthread_create(&ThreadId, &attr, entry, para) == 0)// 创建线程
    {
        pthread_attr_destroy(&attr);
        if(pid)
        {
        	*pid = ThreadId;
        }

        return WD_SUCCESS;
    }

    pthread_attr_destroy(&attr);
    return WD_FAILURE;
}

WD_VOID mSleep(WD_U32   MilliSecond)
{
    struct timeval time;

    time.tv_sec = MilliSecond / 1000;// seconds
    time.tv_usec = MilliSecond * 1000 % 1000000;// microsecond

    select(0, NULL, NULL, NULL, &time);
}

/* 获取从系统启动这一刻起开始计时,到当前的纳秒数 */
WD_U64 GetOsNanosecTime()
{
    WD_U64 time;
	struct timespec t;
	
	clock_gettime(CLOCK_MONOTONIC, &t);
	time = t.tv_sec;
	time = time * 1000000000 + t.tv_nsec;

	return time;/* nanoseconds */
}

/*******************************************
* Function Name : CharToShort
* Parameter     : isBigEndian 是否是大端模式
* Description   : 以对应的大小端模式将b地址数据转换为short性
* Return Value  : On success, 0 is returned.
                  失败返回非0
* Author        : LiangLiCan
* Created       : 2019/09/6
********************************************/
WD_U16 CharToShort(WD_U8 *b, bool isBigEndian/* = false */)
{
    WD_U16 accum = 0;
    if(isBigEndian)
    {
        accum |= b[1];
        accum |= b[0] << 8;
    }
    else
    {
        accum |= b[0];
        accum |= b[1] << 8;
    }
    return accum;
}

WD_VOID ShortToChar(WD_U16 Short, WD_U8 *byte, bool isBigEndian/* = false */)
{
    if(isBigEndian)
    {
        byte[0] = (Short >> 8) & 0xFF;
        byte[1] =  Short & 0xFF;
    }
    else 
    {
        byte[0] = Short & 0xFF;
        byte[1] = (Short >> 8) & 0xFF;
    }
}

WD_U32 CharToInt(WD_U8* b, bool isBigEndian/* = false */)
{
    WD_U32 accum = 0;
    if(isBigEndian)
    {
        accum |= b[3];
        accum |= b[2] << 8;
        accum |= b[1] << 16;
        accum |= b[0] << 24;
    }
    else
    {
        accum |= b[0];
        accum |= b[1] << 8;
        accum |= b[2] << 16;
        accum |= b[3] << 24;
    }
    return accum ;
}

WD_VOID IntToChar(WD_U32 Int, WD_U8 *byte, bool isBigEndian/* = false */)
{
    if (isBigEndian)
    {
        byte[0] = (Int >> 24) & 0XFF;
        byte[1] = (Int >> 16) & 0XFF;
        byte[2] = (Int >> 8) & 0XFF;
        byte[3] = Int & 0XFF;
    }
    else
    {
        byte[0] = Int & 0XFF;
        byte[1] = (Int >> 8) & 0XFF;
        byte[2] = (Int >> 16) & 0XFF;
        byte[3] = (Int >> 24) & 0XFF;
    }
}

WD_FLOAT toPrecision(WD_U8 precision)
{
    if(precision==0)
        return 1;
    WD_FLOAT data = 1;
    for(int i=0; i<precision; i++)
        data *= 10;
    return data;
}

WD_FLOAT toFloat(WD_U8 *b)
{
    WD_FLOAT   floatVariable=0;
    void *pf;
    pf = &floatVariable;
    for(WD_S8 i=0; i<4; i++)
    {
        *(((WD_U8*)pf)+i)=*(b+i);
    }
    return floatVariable;
}

/*******************************************
* Function Name : getNetLinkState
* Parameter     : pu8IfName: 网卡名称
* Description   : 获取网口物理连接是否OK
* Return Value  : On success, true is returned.  On error, false is returned
* Author        : LiangLiCan
* Created       : 
********************************************/
bool getNetLinkState(const WD_C8 *pu8IfName)
{
	WD_S32 s32SockFd = -1;
    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    
    // setup our control structures. 
	strncpy(ifr.ifr_name, pu8IfName, IFNAMSIZ-1);
	
    //open control socket. 
	if ((s32SockFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
	    //DBG_COMM_PRINT(LEVEL_ERROR, "Create socket err(%s)", strerror(errno));
		return false;
	}

#if 0 // mii
	
	if (ioctl(s32SockFd, SIOCGMIIPHY, &ifr) < 0) ///* Get address of MII PHY in use. */
	{
	    DBG_COMM_PRINT(LEVEL_ERROR, "ioctl SIOCGMIIPHY err(%s)", strerror(errno));
		close(s32SockFd);
		return false;
	}

	mii_ioctl_data *md = (mii_ioctl_data *)&ifr.ifr_data;
	md->reg_num = MII_BMSR;
	if (ioctl(s32SockFd, SIOCGMIIREG, &ifr) < 0) ///* Read MII PHY register.    */
	{
	    DBG_COMM_PRINT(LEVEL_ERROR, "ioctl SIOCGMIIREG err(%s)", strerror(errno));
		close(s32SockFd);
		return false;
	}
	close(s32SockFd);
	return md->val_out & BMSR_LSTATUS;
#else // ethtool
    struct linktest_value
    { 
        unsigned int    cmd;
        unsigned int    result; 
    }; 
    struct linktest_value edata; 
    edata.cmd = ETHTOOL_GLINK; 
    ifr.ifr_data = (caddr_t)&edata; 
    if(ioctl(s32SockFd, SIOCETHTOOL, &ifr) < 0)
    {
	    //DBG_COMM_PRINT(LEVEL_ERROR, "ioctl SIOCETHTOOL err(%s)", strerror(errno));
		close(s32SockFd);
		return false;
    }
    close(s32SockFd);
    return edata.result; 
#endif
}

/*******************************************
* Function Name : checkDirExist
* Parameter     : bMake目录不存在时,是否需要创建
* Description   : 判断目录是否存在
* RETURN VALUE	：On success, true is returned.
				  On error, false is returned
* Author        : LiangLiCan
* Created       :
********************************************/
bool checkDirExist(const WD_C8 *pu8Dir, bool bMake/* = false */)
{
	DIR* dir = opendir(pu8Dir);
	if(dir == NULL)
	{
	    if(bMake)
	    {
	        if(mkdir(pu8Dir, 0755) == WD_SUCCESS)
	        {
	            return true;
	        }
	    }
        return false;
	}
	closedir(dir);
    return true;
}

