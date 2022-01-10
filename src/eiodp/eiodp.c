/*
    文件名：eiodp.c
    作者：zouzhiqiang

    说明：
                                《嵌入式通用io数据协议框架》
    通过制定一套协议框架，让用户快速实现主从设备的交互，尤其是让从设备能够轻松构建服务响应（类似http）
    框架适用于所有支持标准io（read write）的通讯类型。可以在eiodp_config.h中配置相关接口。

    数据包协议通过一个头帧+包类型来确定一个数据包。头帧为2字节：0xeb90,包类型也占2字节。而数据包尾则占
    4个字节。包尾根据不同的包类型来确定内容：固定尾帧、校验和、crc32。

    框架会开启一个接收数据服务线程，该线程会根据接收的数据做响应的处理。（同一个硬件设备可以同时做主从，这
    里的主从设备是虚拟的，io必须要是双工的）

    接受线程会根据TYPE的不同来进行不同的操作、发送不同的信号量、压入不同的ringbuf

    数据包格式与占用长度
    +------+------+------+-------------+------+------+
    |  2B  |  2B  |  2B  |      nB     |      4B     |
    +------+------+------+-------------+------+------+
    | eb90 | size | TYPE |     DATA    |     CHECK   |
    +------+------+------+-------------+------+------+

    根据type的不同 data与check也有所不同

    type最高位代表是发送包还是返回包，1是发送包；0是返回包
    type
    ---------------------------------
    | | 1b  | 1b  |
    +-+-----+-----+
    |1|发送包| OK  |
    +-+-----+-----+
    |0|返回包|ERROR|
    +-+-----+-----|

【设备空间读写操作】TYPE=0xEC01、0xEC02 
    从设备一般会分配一些内存空间，用于记录配置与设备信息等等。主设备可以通过读写地址操作来操作从设备的空
    间地址数据。

    [写地址数据操作]将长度为len字节的数据，写入从设备地址为addr往后的len位上。
    +------+------+------+------+------+-------------+------+------+
    |  2B  |  2B  |  2B  |  2B  |  2B  |     lenB    |      4B     |
    +------+------+------+------+------+-------------+------+------+
    | eb90 | size | EC01 | addr |  len |     DATA    |     CRC32   |
    +------+------+------+------+------+-------------+------+------+

    [读地址数据操作]读取从设备从addr地址往后的len位数据，该操作有返回响应。
    +------+------+------+------+------+------+------+
    |  2B  |  2B  |  2B  |  2B  |  2B  |      4B     |
    +------+------+------+------+------+------+------+
    | eb90 | size | EC02 | addr |  len |     CRC32   |
    +------+------+------+------+------+------+------+
        返回正确
        +------+------+------+------+------+-------------+------+------+
        |  2B  |  2B  |  2B  |  2B  |  2B  |     lenB    |      4B     |
        +------+------+------+------+------+-------------+------+------+
        | eb90 | size | 6C02 | addr |  len |     DATA    |     CRC32   |
        +------+------+------+------+------+-------------+------+------+
        返回错误，eCODE代表错误码:0x1地址非法
        +------+------+------+-----+------+------+
        |  2B  |  2B  |  2B  | 1B  |      4B     |
        +------+------+------+-----+------+------+
        | eb90 | size | 2C02 |eCODE|     CRC32   |
        +------+------+------+-----+------+------+

【function】TYPE=0xEC03
    通用函数接口。有返回。
    +------+------+------+------+------+-------------+------+------+
    |  2B  |  2B  |  2B  |  2B  |  2B  |     lenB    |      4B     |
    +------+------+------+------+------+-------------+------+------+
    | eb90 | size | EC03 | func |  len |     arg     |     CRC32   |
    +------+------+------+------+------+-------------+------+------+
        返回正确
        +------+------+------+------+------+-------------+------+------+
        |  2B  |  2B  |  2B  |  2B  |  2B  |     lenB    |      4B     |
        +------+------+------+------+------+-------------+------+------+
        | eb90 | size | 6C03 | func |  len | returnDATA  |     CRC32   |
        +------+------+------+------+------+-------------+------+------+
        返回错误，eCODE代表错误码
        +------+------+------+-----+------+------+
        |  2B  |  2B  |  2B  | 1B  |      4B     |
        +------+------+------+-----+------+------+
        | eb90 | size | 2C03 |eCODE|     CRC32   |
        +------+------+------+-----+------+------+


*/

#include "eiodp.h"
#include "eiodp_config.h"


int eiodp_recvpushTask(eIODP_TYPE* eiodp_fd);
int eiodp_recvProcessTask(eIODP_TYPE* eiodp_fd);

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
eIODP_TYPE* eiodp_init(unsigned int fd, 
                int (*readfunc)(int, char*, int),
                int (*writefunc)(int, char*, int))
{
    crc32_init();
    //入参检查
    if(fd==NULL){
        printf("error: eiodp_init fd = NULL\n");
        return NULL;
    }
    if(readfunc == NULL || writefunc == NULL){
        printf("error: eiodp_init func = NULL\n");
        return NULL;
    }

    eIODP_TYPE* pDev = MOONOS_MALLOC(sizeof(eIODP_TYPE));
    if(pDev == NULL)return NULL;
    pDev->recv_ringbuf=creat_ring(IODP_RECV_MAX_LEN);
    if(pDev->recv_ringbuf == NULL){
        MOONOS_FREE(pDev);
        printf("recv_ringbuf melloc error\n");
        return NULL;
    }

    pDev->retbuf_func=creat_ring(IODP_RETURN_BUFFER);
    if(pDev->retbuf_func == NULL){
        delate_ring(pDev->recv_ringbuf);
        MOONOS_FREE(pDev);
        printf("retbuf_func melloc error\n");
        return NULL;
    }

    pDev->retbuf_readaddr=creat_ring(IODP_RETURN_BUFFER);
    if(pDev->retbuf_readaddr == NULL){
        delate_ring(pDev->retbuf_func);
        delate_ring(pDev->recv_ringbuf);
        MOONOS_FREE(pDev);
        printf("retbuf_func melloc error\n");
        return NULL;
    }

    pDev->configmemSize=IODP_CONFIGMEM_SIZE;
    pDev->iodevHandle=fd;
    pDev->iodevRead = readfunc;
    pDev->iodevWrite = writefunc;
    

#if (IODP_OS==IODP_OS_LINUX)
    sem_init(&pDev->readaddr_retsem, 0, 0);
    sem_init(&pDev->func_retsem, 0, 0);
    sem_init(&pDev->sem_recvSync, 0, 0);
    pthread_mutex_init(&(pDev->mutex_recv),NULL);
#elif (IODP_OS==IODP_OS_FREERTOS)
#endif


    //开启服务
#if (IODP_OS==IODP_OS_LINUX)
    pthread_create(&pDev->ptRecvPushTask,NULL,eiodp_recvpushTask,pDev);
    pthread_create(&pDev->ptRecvProcessTask,NULL,eiodp_recvProcessTask,pDev);
#elif (IODP_OS==IODP_OS_FREERTOS)
#endif

    return pDev;
}

//找到帧头
static char* findhead(char* buf,int buflen)
{
    int i=0;
    for(i=0;i<buflen-1;i++){
        if(buf[i]==0xeb && buf[i+1]==0x90){
            return &buf[i];
        }
    }
    return NULL;
}
/************************************************************
    @brief:
        写地址处理函数 type EC01
    @param:
        eiodp_fd：eiodp句柄
        pktbuf：需要处理的数据包，这是已经解了eb90的数据包
        pktsize：数据包长度
    @return:
        -1为帧头错误 
        0为地址溢出错误（会有返回iodp） 
        1为正确
*************************************************************/
static int writeaddr_Process(eIODP_TYPE* eiodp_fd, unsigned char* pktbuf, int pktsize){
    if(pktbuf[0]!=0xec || pktbuf[1]!=0x01)return -1;
    unsigned short addr = ((unsigned short)pktbuf[2] << 8) | ((unsigned short)pktbuf[3]) ;
    unsigned short len = ((unsigned short)pktbuf[4] << 8) | ((unsigned short)pktbuf[5]) ;
    int i=0;
    for(i=0;i<len;i++){
        if((addr+i)>=eiodp_fd->configmemSize)return 0;
        eiodp_fd->configmem[addr+i]=pktbuf[i+6];
    }
    return 1;
}

/************************************************************
    @brief:
        读地址处理函数 type EC02
    @param:
        eiodp_fd：eiodp句柄
        pktbuf：需要处理的数据包，这是已经解了eb90的数据包
        pktsize：数据包长度
    @return:
        -1为帧头错误 
        0为地址溢出错误（会有返回iodp） 
        1为正确
*************************************************************/
static int readaddr_Process(eIODP_TYPE* eiodp_fd, unsigned char* pktbuf, int pktsize)
{
    if(pktbuf[0]!=0xec || pktbuf[1]!=0x02)return -1;
    unsigned short addr = ((unsigned short)pktbuf[2] << 8) | ((unsigned short)pktbuf[3]) ;
    unsigned short len = ((unsigned short)pktbuf[4] << 8) | ((unsigned short)pktbuf[5]) ;
    unsigned int devfd=eiodp_fd->iodevHandle;

    if(addr>=eiodp_fd->configmemSize){
        unsigned char retbuf[11];
        unsigned short retpktsize=7;
        retbuf[0]=0xeb;
        retbuf[1]=0x90;
        retbuf[2]=(unsigned char)(retpktsize>>8)&0xff;
        retbuf[3]=(unsigned char)(retpktsize)&0xff;
        retbuf[4]=0x2c; //type error pkt
        retbuf[5]=0x02;
        retbuf[6]=0x01; //error code
        updatepktcrc(retbuf,11);
        //IOWRITE(devfd,retbuf,11);
        eiodp_fd->iodevWrite(devfd,retbuf,11);
        return 0;
    }

    unsigned char *retbuf = MOONOS_MALLOC(14+len);
    unsigned short retlen = len;
    if(len>(eiodp_fd->configmemSize-addr))retlen = (eiodp_fd->configmemSize-addr);
    unsigned short retpktsize=10+retlen;
    retbuf[0]=0xeb;
    retbuf[1]=0x90;
    retbuf[2]=(unsigned char)(retpktsize>>8)&0xff;
    retbuf[3]=(unsigned char)(retpktsize)&0xff;
    retbuf[4]=0x6c;
    retbuf[5]=0x02;
    retbuf[6]=(unsigned char)(addr>>8)&0xff;
    retbuf[7]=(unsigned char)(addr)&0xff;
    retbuf[8]=(unsigned char)(retlen>>8)&0xff;
    retbuf[9]=(unsigned char)(retlen)&0xff;
    memcpy(&retbuf[10],&(eiodp_fd->configmem[addr]),retlen);
    updatepktcrc(retbuf,retpktsize+4);
    //IOWRITE(devfd,retbuf,retpktsize+4);
    eiodp_fd->iodevWrite(devfd,retbuf,retpktsize+4);
    MOONOS_FREE(retbuf);
    return 1;
}

/************************************************************
    @brief:
        接受数据压入循环缓存-任务
    @param:
        eiodp_fd:eiodp句柄
*************************************************************/
int eiodp_recvpushTask(eIODP_TYPE* eiodp_fd)
{
    unsigned int devfd=eiodp_fd->iodevHandle;
    unsigned char recvbuf[1024]={0};
    int recvlen=0;
    int putret=0;
    while(1){
        //recvlen=IOREAD(devfd,recvbuf,1024);
        recvlen = eiodp_fd->iodevRead(devfd,recvbuf,1024);
        if(recvlen<=0) continue;
        putret = put_ring(eiodp_fd->recv_ringbuf,recvbuf,recvlen);
        if(putret == -1){
            IODP_LOGMSG("put_ring out\n");
        }
    }
}


/************************************************************
    @brief:
        接收服务函数-任务
    @param:
        eiodp_fd:eiodp句柄
*************************************************************/
int eiodp_recvProcessTask(eIODP_TYPE* eiodp_fd)
{
    unsigned int devfd=eiodp_fd->iodevHandle;
    unsigned char recvbuf[1024]={0};
    int recvlen=0;
    unsigned short pktlen=0;
    while(1)
    {
        recvlen=get_ring(eiodp_fd->recv_ringbuf, recvbuf,1);
        if(recvlen<=0){continue;}
        //确定帧头,并持续接受4字节
        if(recvbuf[0]!=0xeb){IODP_LOGMSG("recvbuf[0]!=0xeb\n");continue;}
        while(recvlen<4){
            recvlen+=get_ring(eiodp_fd->recv_ringbuf,&recvbuf[recvlen],4-recvlen);
        }
        if(recvbuf[1]!=0x90){IODP_LOGMSG("recvbuf[1]!=0x90\n");continue;}
        //此时已经读取了4位数据
        pktlen = ((unsigned short)recvbuf[2] << 8) | ((unsigned short)recvbuf[3]) ;
        if(pktlen>=1024-4){IODP_LOGMSG("pktlen>=1024-4\n");continue;}
        pktlen += 4;
        while(recvlen<pktlen){
            recvlen+=get_ring(eiodp_fd->recv_ringbuf,&recvbuf[recvlen],pktlen-recvlen);
        }

        //------------------确定包类型
        if(((recvbuf[4])&IODP_TYPEBIT_SR_MASK)==0)//确定包为返回类型
        {
            //接受到返回类型的包，先校验，再根据返回类型确定不同的返回缓冲区，再给信号
            if(checkpktcrc(recvbuf,recvlen)==0){
                IODP_LOGMSG("retpkt crc error\n");
                continue;
            }
            if(recvbuf[5]==0x2)//readaddr
            {
                put_ring(eiodp_fd->retbuf_readaddr,&recvbuf[4],recvlen-8);//去掉头和crc
#if (IODP_OS==IODP_OS_LINUX)
                sem_post(&(eiodp_fd->readaddr_retsem));
#elif (IODP_OS==IODP_OS_FREERTOS)
#endif
            }
            else if(recvbuf[5]==0x3)//function
            {
                put_ring(eiodp_fd->retbuf_func,&recvbuf[4],recvlen-8);//去掉头和crc
#if (IODP_OS==IODP_OS_LINUX)
                sem_post(&(eiodp_fd->func_retsem));
#elif (IODP_OS==IODP_OS_FREERTOS)
#endif
            }
            else {IODP_LOGMSG("pkt recvbuf[5] no match \n");continue;}
        }
        else                                 //确定包为发送类型
        {
            //接受到发送类型的包需要 更具type代码分别转向不同的服务类型
            if(recvbuf[5]==0x01)//write addr
            {
                if(checkpktcrc(recvbuf,recvlen)==0){IODP_LOGMSG("write addr pkt crc error\n");continue;}
                writeaddr_Process(eiodp_fd,&recvbuf[4],recvlen-8);
            }
            else if(recvbuf[5]==0x02)//readaddr
            {
                if(checkpktcrc(recvbuf,recvlen)==0){IODP_LOGMSG("readaddr pkt crc error\n");continue;}
                readaddr_Process(eiodp_fd,&recvbuf[4],recvlen-8);
            }
            else {IODP_LOGMSG("retpkt recvbuf[5] no match \n");continue;}
        }

    }
}

//---------------------------send cmd----------------------------

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
void eiodpWriteAddr(eIODP_TYPE* eiodp_fd,unsigned short addr,unsigned short len,unsigned char* sdbuf){
    unsigned int devfd=eiodp_fd->iodevHandle;

    unsigned short pktsize=10+len;
    unsigned char *sendbuf=MOONOS_MALLOC(pktsize+4);

    sendbuf[0]=0xeb;
    sendbuf[1]=0x90;
    sendbuf[2]=(unsigned char)(pktsize>>8)&0xff;
    sendbuf[3]=(unsigned char)(pktsize)&0xff;
    sendbuf[4]=0xec;
    sendbuf[5]=0x01;
    sendbuf[6]=(unsigned char)(addr>>8)&0xff;
    sendbuf[7]=(unsigned char)(addr)&0xff;
    sendbuf[8]=(unsigned char)(len>>8)&0xff;
    sendbuf[9]=(unsigned char)(len)&0xff;
    memcpy(&sendbuf[10],sdbuf,len);

    updatepktcrc(sendbuf,pktsize+4);

    //IOWRITE(devfd,sendbuf,pktsize+4);
    eiodp_fd->iodevWrite(devfd,sendbuf,pktsize+4);
    MOONOS_FREE(sendbuf);
}
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
int eiodpReadAddr(eIODP_TYPE* eiodp_fd,unsigned short addr,unsigned short len,unsigned char* recvbuf)
{
    unsigned int devfd=eiodp_fd->iodevHandle;
    int ret=0;

    unsigned short pktsize=10;
    unsigned char *sendbuf=MOONOS_MALLOC(pktsize+4);

    sendbuf[0]=0xeb;
    sendbuf[1]=0x90;
    sendbuf[2]=(unsigned char)(pktsize>>8)&0xff;
    sendbuf[3]=(unsigned char)(pktsize)&0xff;
    sendbuf[4]=0xec;
    sendbuf[5]=0x02;
    sendbuf[6]=(unsigned char)(addr>>8)&0xff;
    sendbuf[7]=(unsigned char)(addr)&0xff;
    sendbuf[8]=(unsigned char)(len>>8)&0xff;
    sendbuf[9]=(unsigned char)(len)&0xff;
    updatepktcrc(sendbuf,pktsize+4);

    //IOWRITE(devfd,sendbuf,pktsize+4);
    eiodp_fd->iodevWrite(devfd,sendbuf,pktsize+4);

    MOONOS_FREE(sendbuf);

    //等待返回
#if (IODP_OS==IODP_OS_LINUX)
    struct timespec tv;
    clock_gettime(CLOCK_REALTIME, &tv);
    tv.tv_sec += 3; // 这个是设置等待时长的。单位是秒
    int timeres=0;
    timeres = sem_timedwait(&(eiodp_fd->readaddr_retsem),&tv);
    if(timeres == -1){IODP_LOGMSG("time out\n");return -6;}
#elif (IODP_OS==IODP_OS_FREERTOS)
#endif
    unsigned char *retbuf=MOONOS_MALLOC(len+14);
    unsigned short recvlen=0;
    recvlen = get_ring(eiodp_fd->retbuf_readaddr,retbuf,len+14);
    if(recvlen<7){ret=-1;goto FAIL;}
    if(retbuf[0]==0x6c && retbuf[1]==0x02)
    {
        unsigned short retaddr = ((unsigned short)retbuf[2] << 8) | ((unsigned short)retbuf[3]) ;
        unsigned short retlen = ((unsigned short)retbuf[4] << 8) | ((unsigned short)retbuf[5]) ;
        if(retaddr!=addr){ret=-4;goto FAIL;}
        if(retlen!=recvlen-6){ret=-5;goto FAIL;}
        memcpy(recvbuf,&retbuf[6],retlen);
        MOONOS_FREE(retbuf);
        return retlen;
    }
    else if(retbuf[0]==0x2c && retbuf[1]==0x02)
    {
        printf("eiodpReadAddr return error code:0x%x\n",retbuf[2]);
        ret = -3;
        goto FAIL;
    }
    else{
        printf("eiodpReadAddr noreturn\n");
        ret = -2;
        goto FAIL;
    }



FAIL:
    MOONOS_FREE(retbuf);
    return ret;
}


//-----------------------------------crc32----------------------
static unsigned long table[256];
//位逆转
static unsigned long bitrev(unsigned long input, int bw)
{
    int i;
    unsigned long var;
    var = 0;
    for(i=0;i<bw;i++)
    {
        if(input & 0x01)
        {
            var |= 1<<(bw-1-i);
        }
        input>>=1;
    }
    return var;
}

//码表生成
//如:X32+X26+...X1+1,poly=(1<<26)|...|(1<<1)|(1<<0)
void crc32_init()
{
    int i;
    int j;
    unsigned long c;
    unsigned long poly = 0x4C11DB7;
    poly=bitrev(poly,32);
    for(i=0; i<256; i++)
    {
        c = i;
        for (j=0; j<8; j++)
        {
            if(c&1)
            {
                c=poly^(c>>1);
            }
            else
            {
                c=c>>1;
            }
        }
        table[i] = c;
    }
}

unsigned long crc32(void* input, int len)
{
    int i;
    unsigned char* pch;
    unsigned long crc = 0xFFFFFFFF;
    pch = (unsigned char*)input;
    for(i=0;i<len;i++)
    {
        crc = (crc>>8)^table[(unsigned char)(crc^*pch)];
        pch++;
    }
    crc ^= 0xFFFFFFFF;

    return crc;
}

int checkpktcrc(unsigned char* data,unsigned int size)
{
    unsigned int crcdata = (unsigned int)crc32(data,size-4);
    if(
       (data[size-2]==((crcdata>>8)&0xff)) && (data[size-1]==((crcdata)&0xff)) &&
       (data[size-4]==((crcdata>>24)&0xff)) && (data[size-3]==((crcdata>>16)&0xff))
      )
    {
        return 1;
    }
    else{
        return 0;
    }

}

int updatepktcrc(unsigned char* data,unsigned int size)
{
    unsigned int crcdata = (unsigned int)crc32(data,size-4);

    data[size-2]=((crcdata>>8)&0xff);
    data[size-1]=((crcdata)&0xff);
    data[size-4]=((crcdata>>24)&0xff);
    data[size-3]=((crcdata>>16)&0xff);

}

