#ifndef _EIODP_H_
#define _EIODP_H_

#include "type.h"
#include "eiodp_config.h"
#include "stdlib.h"

//操作系统
#define IODP_OS_NULL 0
#define IODP_OS_LINUX 1
#define IODP_OS_FREERTOS 2

//定义接受包缓存大小
#define IODP_RECV_MAX_LEN 1024
//定义返回接受包缓存大小
#define IODP_RETURN_BUFFER 512
//定义iodp配置空间大小
#define IODP_CONFIGMEM_SIZE 512
//function数据包 最大返回参数数据
#define IODP_FUNCPKT_RET_LEN 256

#define IODP_LOG(str,a,b,c,d,e) printf(str,a,b,c,d,e)
#define IODP_LOGMSG(str) printf(str)

//type mask
#define IODP_TYPEBIT_SR_MASK 0x80  //判断包为发送还是返回 typebit&IODP_TYPEBIT_SR_MASK==0 为返回包

//malloc
#define MOONOS_MALLOC(size) malloc(size)
#define MOONOS_FREE(P) free(P)



//--------------error code 
#define IODP_OK 0
#define IODP_ERROR_PARAM -10
#define IODP_ERROR_HEAPOVER -11
#define IODP_ERROR_REPEATCODE -12 
#define IODP_ERROR_TIMEOUT -13
#define IODP_ERROR_SMOLL_RECVLEN -14
#define IODP_ERROR_RECVLEN -15
#define IODP_ERROR_RETCODE -16
#define IODP_ERROR_PKT -17      //返回了一个error数据包
#define IODP_ERROR_NORET -18
#define IODP_ERROR_API_HEAD -19 
#define IODP_ERROR_RADDR_HEAD -20 
#define IODP_ERROR_WADDR_HEAD -21

#define IODP_ERROR_APINODE_REPEAT -22



//eiodp服务函数链表结构
typedef struct
{
    uint16 funcode;
    int (*callbackFunc)(uint16 len, void* data,uint16* retlen,void* retdata); 
    void* pNext;
}eIODP_FUNC_NODE;

//eiodp循环缓冲buffer
typedef struct
{
    uint32 bufSize;

    uint8 *buf;
    uint32 pIn; //环形缓冲的头指针
    uint32 pOut; //环形缓冲的尾指针

}eIODP_RING;

//
typedef struct
{
    unsigned int iodevHandle;   //IO设备的句柄fd
    eIODP_RING*  recv_ringbuf;

    eIODP_RING*  retbuf_readaddr;     //readaddr 类型包的返回包缓存区
    eIODP_RING*  retbuf_func;         //function 类型包的返回包缓存区

    unsigned int configmemSize;
    char configmem[IODP_CONFIGMEM_SIZE];

    //注册的服务函数链表
    eIODP_FUNC_NODE* pFuncHead;

    //iodevHandle设备的收发函数
    int (*iodevRead)(int, char*, int);
    int (*iodevWrite)(int, char*, int);

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
        readfunc：fd设备读数据函数
        writefunc：fd设备写数据函数
    @return:
        创建的eIODP_TYPE指针，可以通过这个指针来操作iodp
*************************************************************/
eIODP_TYPE* eiodp_init(unsigned int fd, int (*readfunc)(int, char*, int),
                int (*writefunc)(int, char*, int));

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

/************************************************************
    @brief:
        注册服务函数
    @param:
        eiodp_fd:eiodp句柄
        funcode：服务函数代码
        callbackFunc:服务函数
    @return:
        <0 - 失败（error code）
         0 - 成功
*************************************************************/
int eiodpRegister(eIODP_TYPE* eiodp_fd,uint16 funcode,
                int (*callbackFunc)(uint16 len, void* data,uint16* retlen,void* retdata));

/************************************************************
    @brief:
        打印已经注册的服务函数
    @param:
        eiodp_fd:eiodp句柄
    @return:
        <0 - 失败（error code）
         0 - 成功
*************************************************************/
int eiodpShowRegFunc(eIODP_TYPE* eiodp_fd);
/************************************************************
    @brief:
        调用服务端的API接口，该接口由服务端的eiodpRegister注册的callbackFunc函数来提供
    @param:
        eiodp_fd:eiodp句柄
        code：API代码
        argsize：入参arg的长度
        arg：API入参
        retarg：用户需要提供返回参数的容器
    @return:
        <0 - 失败（error code）
        >0 - 成功 返回参数长度
*************************************************************/
int eiodpFunction(eIODP_TYPE* eiodp_fd, uint16 code, 
        uint16 argsize,void* arg, void* retarg);


/************************************************************
    @brief:
        接收服务函数-任务(无操作系统，连续字符read) 在无操作系统的程序里 需要连续调用
    @param:
        eiodp_fd:eiodp句柄
*************************************************************/
int eiodp_recvProcessTask_nos(eIODP_TYPE* eiodp_fd);


int checkpktcrc(unsigned char* data,unsigned int size);
int updatepktcrc(unsigned char* data,unsigned int size);

void crc32_init();



//-----------------------------------------------------------------------------------
//                               eiodp ring buffer
//

eIODP_RING* creat_ring(uint32 size);

void delate_ring(eIODP_RING* p);

int put_ring(eIODP_RING* p,uint8* buf,uint32 size);
int get_ring(eIODP_RING* p,uint8* buf,uint32 size);
uint16 size_ring(eIODP_RING* p);

#endif
