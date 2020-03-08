#include "stdio.h"
#include "humituremanager.h"
#include "commfunction.h"


int main()
{
    SERIAL_S stSerialParam;
    stSerialParam.u8BaudRate = BR_9600;
    stSerialParam.u8DataBit  = 0; // 8bit
    stSerialParam.u8StopBit  = 0; // 1bit
    stSerialParam.u8Check    = 0; // None
    CHumitureManager *m_pCHumiture = new CHumitureManager("/dev/ttyS2", &stSerialParam, 1); // /dev/ttyS2

    while(1)
    {
        printf("\033[0;31m [%s][%d] humidity=%d, temperature=%f\033[0;39m \n", __func__, __LINE__,
                m_pCHumiture->humidity(), m_pCHumiture->temperature());
        
        mSleep(500);
    }
    return 0;
}

