
//#include <QCoreApplication>

#include <eiodp.h>

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <sys/types.h>

#include <sys/fcntl.h>





unsigned long recvByte=0;

int zwrite(int fd,char* buf,int size)
{
    return write(fd,buf,size);
}

int zread(int fd,char* buf,int size)
{
    return read(fd,buf,size);
}


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



#define testpkt_len 100
int main(int argc, char *argv[])
{
    //QCoreApplication a(argc, argv);
    fflush(stdout);
    printf("Init Server!\n");
    int serialfd = open("com3",O_RDWR);
    int M_serialfd = open("com2",O_RDWR);






    eIODP_TYPE* pServer;
    pServer = eiodp_init(serialfd,zread,zwrite);
    eiodpRegister(pServer,0x666,func_sum);

    printf("Init Master!\n");

    eIODP_TYPE* pdev=eiodp_init(M_serialfd,zread,zwrite);



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
        if(cnt%10==0)printf("errorcnt=%d cnt=%d \n",errorcnt,cnt);
        //i=1000000;
        //while(i--);
        usleep(100);

    }




    return 1;
}


/*
int main()
{
    int sfd = open("com2",O_RDWR);
    int rfd = open("com3",O_RDWR);
    unsigned char sdbuf[1024];
    for(int i=0 ; i<43;i++)
    {
        sdbuf[i]=i;
    }

    zwrite(sfd,sdbuf,43);
    unsigned char rbuf[1024];
    zread(rfd,rbuf,30);
    printf(rbuf);
}
*/

