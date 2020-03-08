/******************************************************************************
  File Name     : wldlock.h
  Description   : 封装锁相关接口
  Author        : LiangLiCan
  Created       : 2019/06/14
  History       :
1.Date          :
  Author        :
  Modification  :
******************************************************************************/

#ifndef _WLD_LOCK_H_
#define _WLD_LOCK_H_

#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include "datatype.h"

/* 互斥锁 */
class CMuteLock
{
 public:
    CMuteLock()
    {
        pthread_mutex_init(&m_Lock, NULL);
    }
    ~CMuteLock()
    {
        pthread_mutex_destroy(&m_Lock);
    }
    WD_S32 TryLock()
    {
        return pthread_mutex_trylock(&m_Lock);
    }
    WD_VOID Lock()
    {
        pthread_mutex_lock(&m_Lock);
    }

    WD_VOID UnLock()
    {
        pthread_mutex_unlock(&m_Lock);
    }
	
 protected:
	pthread_mutex_t m_Lock;
};

/* 对象锁 */
class CObjectLock
{
 public:
    CObjectLock(CMuteLock *pMuteLock):m_pMuteLock(pMuteLock)
    {
        m_pMuteLock->Lock();
    }

    ~CObjectLock()
    {
        m_pMuteLock->UnLock();
    }
 private:
 	CMuteLock *m_pMuteLock;
};

/* condition lock */
class CCondLock : public CMuteLock {
public:
    CCondLock():CMuteLock() 
    {
        pthread_condattr_t cattr;
        pthread_condattr_init(&cattr);
        pthread_condattr_setclock(&cattr, CLOCK_MONOTONIC);
        pthread_cond_init(&mCond, &cattr);
    }

    ~CCondLock()
    {
        pthread_cond_destroy(&mCond);
    }

    WD_S32 Wait(){  
        return pthread_cond_wait(&mCond, &m_Lock); /* 阻塞等待,加入队列后自动释放锁,直到收到signal信号 */
    }

    WD_S32 Signal(){
        return pthread_cond_signal(&mCond);
    }

    WD_S32 Broadcast() {
        return pthread_cond_broadcast(&mCond); /* 激活所有 */
    }

    static struct timespec &getTimespec(long long nsecs, struct timespec &tv)
    {
        clock_gettime(CLOCK_MONOTONIC, &tv);
        nsecs = tv.tv_nsec + nsecs;
        tv.tv_nsec = nsecs % 1000000000;
        tv.tv_sec += (nsecs / 1000000000);
        return tv;
    }
    // 纳秒, 即10的-9次方秒
    WD_S32 timeWait(WD_S64 NanoSecs) 
    {
        struct timespec tv;
        return pthread_cond_timedwait(&mCond, &m_Lock, &getTimespec(NanoSecs, tv));
    }

    WD_S32 timeWait(struct timespec &ts) {
        return pthread_cond_timedwait(&mCond, &m_Lock, &ts);
    }

    
	private:
		pthread_cond_t mCond;
};


#endif // _WLD_LOCK_H_
