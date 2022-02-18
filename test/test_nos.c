#include <stdio.h>

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <eiodp.h>

#include <windows.h>


unsigned long recvByte=0;
extern int udpsend(int fd,char* buf,int len);
extern int udpread(int fd,char* buf,int len);


int func_sum(uint16 len, void* data,uint16* retlen,void* retdata){
    int sum = 0;
    unsigned char *ptr = (unsigned char *)data;
    for(int i=0; i<len; i++)
    {
        sum+=ptr[i];
    }

    *retlen=4;
    *(int*)retdata=sum;
    return 0;
}

void slaver(void)
{
    printf("Init Server!\n");
    int socktServer = udpopen(8888,7777);
    eIODP_TYPE* pServer;
    pServer = eiodp_init(socktServer,udpread,udpsend);
    eiodpRegister(pServer,0x666,func_sum);

    while(1)
    {
        #if (IODP_OS==IODP_OS_NULL)
        eiodp_recvProcessTask_nos(pServer);
        #endif
    }
}

#define testpkt_len 100
void master(void)
{
    printf("Init Master!\n");
    int sockt = udpopen(7777,8888);
    eIODP_TYPE* pdev=eiodp_init(sockt,udpread,udpsend);

        /*--------------------------------------*/
    srand((int)time(0));
    int errorcnt=0;
    int cnt=0;

    unsigned char buf[testpkt_len];
    unsigned char recvbuf[testpkt_len];
    int i=0;
    int retlen=0;
    while(1){
        //随机数生成
        cnt++;
        for(i=0;i<testpkt_len;i++){
            buf[i]=rand()%testpkt_len;
        }

        int randlen = rand()%testpkt_len+1;

        //API调用
        int funcret=0;
        int retlen = eiodpFunction(pdev,0x666,randlen,(unsigned char*)buf,&funcret);

        //自检cpu调用
        int sum=0;
        for(int i=0;i<randlen;i++){
            sum+=buf[i];
        }

        //对比
        if(retlen!=4){
            printf("retlen!=4");
        }

        if(sum != funcret){
            errorcnt++;
            printf("errorcnt=%d cnt=%d \n",errorcnt,cnt);
        }
        if(cnt%100==0)printf("errorcnt=%d cnt=%d \n",errorcnt,cnt);
        //i=1000000;
        //while(i--);
        Sleep(1);

    }




    return 0;
}

int main()
{
    pthread_t t1,t2;
    pthread_create(&t1,NULL,slaver,NULL); 
    Sleep(100);
    pthread_create(&t2,NULL,master,NULL); 

    while(1)
    {

    }
   

}