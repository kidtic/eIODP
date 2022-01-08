#ifndef _EIODP_H_
#define _EIODP_H_

#include "ring.h"
#include "eiodp_config.h"

//操作系统
#define IODP_OS_NULL 10
#define IODP_OS_LINUX 1
#define IODP_OS_FREERTOS 2

//定义接受包缓存大小
#define IODP_RECV_MAX_LEN 1024
//定义返回接受包缓存大小
#define IODP_RETURN_BUFFER 512
//定义iodp配置空间大小
#define IODP_CONFIGMEM_SIZE 512

#define IODP_LOG(str,a,b,c,d,e) printf(str,a,b,c,d,e)
#define IODP_LOGMSG(str) printf(str)

//type mask
#define IODP_TYPEBIT_SR_MASK 0x80  //判断包为发送还是返回 typebit&IODP_TYPEBIT_SR_MASK==0 为返回包

//
typedef struct
{
    unsigned int iodevHandle;   //IO设备的句柄fd
    RING*  recv_ringbuf;

    RING*  retbuf_readaddr;     //readaddr 类型包的返回包缓存区
    RING*  retbuf_func;         //function 类型包的返回包缓存区

    unsigned int configmemSize;
    char configmem[IODP_CONFIGMEM_SIZE];

#if (IODP_OS==IODP_OS_LINUX)
    sem_t readaddr_retsem;
    sem_t func_retsem;
    pthread_t ptRecvPushTask;
    pthread_t ptRecvProcessTask;
    pthread_mutex_t mutex_recv;
    sem_t sem_recvSync;
#elif (IODP_OS==IODP_OS_FREERTOS)
#endif


}eIODP_TYPE;

/************************************************************
    @brief:
        初始化框架，准备缓存取、信号量、创建接受服务线程
    @param:
        fd：依赖的io设备句柄，eiodp协议需要作用于标准io设备。
    @return:
        创建的eIODP_TYPE指针，可以通过这个指针来操作iodp
*************************************************************/
eIODP_TYPE* eiodp_init(unsigned int fd);

/************************************************************
    @brief:
        写地址操作，将数据写到对方的配置空间上
        此操作主要是在对方的iodp配置空间的addr偏移地址上写指定长度的数据。
    @param:
        eiodp_fd:eiodp句柄
        addr：在对方的配置空间addr地址往后写数据
        len：数据长度，在[addr,addr+len]上覆盖数据
        sdbuf：数据头指针
*************************************************************/
void eiodpWriteAddr(eIODP_TYPE* eiodp_fd,unsigned short addr,unsigned short len,unsigned char* sdbuf);
/************************************************************
    @brief:
        读地址操作，读取对方的配置空间数据
        此操作主要是将对方的addr地址往后len个数据都读取出来
    @param:
        eiodp_fd:eiodp句柄
        addr：读取地址
        len：数据长度
        recvbuf：将数据存入该数组
    @return:
        -6 - time out
        -1 - recvlen长度不对
        -4 - 返回包中地址对不上
        -5 - 返回包包长度与实际不符
        -3 - 有返回包，但是返回了错误代码
        -2 - 返回type不对
         0 - 成功
*************************************************************/
int eiodpReadAddr(eIODP_TYPE* eiodp_fd,unsigned short addr,unsigned short len,unsigned char* recvbuf);


int checkpktcrc(unsigned char* data,unsigned int size);
int updatepktcrc(unsigned char* data,unsigned int size);
#endif
