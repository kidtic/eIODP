#ifndef _EIODPCONFIG_H_
#define _EIODPCONFIG_H_

#include "eiodp.h"

//#define IOREAD(fd,buf,len) udpread(fd,buf,len)
//#define IOWRITE(fd,buf,len) udpsend(fd,buf,len)
#define IODP_OS IODP_OS_NULL     //"FreeRTos" "vxWorks" 




#if (IODP_OS==IODP_OS_LINUX)
    #include <pthread.h>
    #include <semaphore.h>
    #include <unistd.h>
    #include <sys/time.h>
    #define IODP_SEM_TAKE(sem) sem_wait(sem)
    #define IODP_SEM_GIVE(sem) sem_post(sem)
#elif (IODP_OS==IODP_OS_FREERTOS)
    #include "FreeRTOS.h"
    #define IODP_SEM_TAKE(sem) xSemaphoreTake(sem,(TickType_t)xMaxBlockTime)
    #define IODP_SEM_GIVE(sem) xSemaphoreGive(sem)
#elif (IODP_OS==IODP_OS_NULL)
#endif

#endif
