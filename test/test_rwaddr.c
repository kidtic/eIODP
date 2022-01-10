#include <stdio.h>


#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <eiodp.h>

unsigned long recvByte=0;
extern int udpsend(int fd,char* buf,int len);
extern int udpread(int fd,char* buf,int len);

void recvspeed(void)
{
    unsigned long start,end;

    while(1)
    {
        start=recvByte;
        sleep(1);
        end=recvByte;
        printf("recv speed:%.3f MByte/s\n",(double)(end-start)/(1024.0*1024.0));
    }

}
void socket_test(void){
    printf("Hello World!\n");
    //socket_recv();
    int sockt = udpopen();

    pthread_t speedtd;

    pthread_create(&speedtd,NULL,recvspeed,0);

    char recvbuf[1024*256]={0};
    int recvlen=0;

    while(1){
        //udpsend(sockt,"helloworld",10);
        recvlen = udpread(sockt,recvbuf,1);
        if(recvlen>0) printf(recvbuf);
        else{
            printf("recvlen:%d\n",recvlen);
        }
        recvByte+=recvlen;
    }
}


int start_send(void)
{
    printf("Hello World master!\n");
    //socket_recv();
    int sockt = udpopen(7777,8888);
    eIODP_TYPE* pdev=eiodp_init(sockt,udpread,udpsend);
    srand((int)time(0));
    int errorcnt=0;
    int cnt=0;

    unsigned char buf[100];
    unsigned char recvbuf[100];
    int i=0;
    int retlen=0;
    while(1){
        cnt++;
        for(i=0;i<100;i++){
            buf[i]=rand()%100;
        }
        eiodpWriteAddr(pdev,0,100,buf);

        retlen = eiodpReadAddr(pdev,0,100,recvbuf);
        if(retlen!=100){
            printf("retlen!=100");
        }

        int ret = memcmp(buf,recvbuf,100);
        if(ret!=0){
            errorcnt++;
            //printf("errorcnt=%d\n",errorcnt);
        }
        printf("errorcnt=%d cnt=%d \n",errorcnt,cnt);
        //i=10000;
        //while(i--);
        usleep(10);


    }
}

#define testpkt_len 100
int main()
{


    int socktServer = udpopen(8888,7777);
    eIODP_TYPE* pServer;
    pServer = eiodp_init(socktServer,udpread,udpsend);

    printf("Hello World master!\n");

    int sockt = udpopen(7777,8888);
    eIODP_TYPE* pdev=eiodp_init(sockt,udpread,udpsend);
    srand((int)time(0));
    int errorcnt=0;
    int cnt=0;

    unsigned char buf[testpkt_len];
    unsigned char recvbuf[testpkt_len];
    int i=0;
    int retlen=0;
    while(1){
        cnt++;
        for(i=0;i<testpkt_len;i++){
            buf[i]=rand()%testpkt_len;
        }

        int randlen = rand()%testpkt_len+1;
        eiodpWriteAddr(pdev,0,randlen,buf);

        retlen = eiodpReadAddr(pdev,0,randlen,recvbuf);
        if(retlen!=randlen){
            printf("retlen!=100");
        }

        int ret = memcmp(buf,recvbuf,randlen);
        if(ret!=0){
            errorcnt++;
            printf("errorcnt=%d cnt=%d \n",errorcnt,cnt);
        }
        if(cnt%10000==0)printf("errorcnt=%d cnt=%d \n",errorcnt,cnt);
        //i=1000000;
        //while(i--);
        usleep(10);

    }




    return 0;
}
