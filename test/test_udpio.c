#include <stdio.h>

#include <fcntl.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>

#include <eiodp.h>


unsigned long recvByte=0;
extern int udpopen(unsigned int local_port,unsigned int remote_port);
extern int udpsend(int fd,char* buf,int len);
extern int udpread(int fd,char* buf,int len);

void slaver(void)
{
     printf("Init Server!\n");
    int socktServer = udpopen(8888,7777);
    int rlen=0;
    char buf[1024];
    while(1)
    {
        rlen = udpread(socktServer,buf,1024);
        if(rlen>0)
        {
            printf("recv: %d  %s\n",rlen,buf);
        }
    }
}


void master(void)
{
    printf("Init Master!\n");
    int sockt = udpopen(7777,8888);

    char buf[1024]= "hello world!!!!";
    int i=0;
    int retlen=0;
    while(1){
        udpsend(sockt,buf,1024);
        usleep(100000);

    }




    return 0;
}



int main()
{
    pthread_t t1,t2;
    pthread_create(&t1,NULL,slaver,NULL); 
    usleep(100*1000);
    pthread_create(&t2,NULL,master,NULL); 

    while(1)
    {

    }
   

}