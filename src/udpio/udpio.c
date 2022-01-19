#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>


#define UDP_REMOTEIP "127.0.0.1"
#define UDPFD_MAXNUM 30

//#define UDP_RECVSIG

#ifdef UDP_RECVSIG
#include <pthread.h>
#include <ring.h>
#endif


struct sockaddr_in from_addr;
int from_addr_len = sizeof(from_addr);

int target_addr_len = sizeof(from_addr);

struct udpfdtype{
    unsigned long fd;
    struct sockaddr_in target_addr;
#ifdef UDP_RECVSIG
    RING* recvbuf;
    pthread_t pt;
#endif
};

struct udpfdtype udpfd_list[UDPFD_MAXNUM];
int udpfd_list_head=1;


#ifdef UDP_RECVSIG
void udp_recvTask(struct udpfdtype* udpfd)
{
    char buf[2048];
    while(1)
    {
        int rlen = recvfrom(udpfd->fd,buf,2048,0,(struct sockaddr *)&from_addr,&from_addr_len);
        put_ring(udpfd->recvbuf,buf,rlen);
    }
    
}
#endif

unsigned long udpopen(unsigned int local_port,unsigned int remote_port){

    int sockt = socket(AF_INET, SOCK_DGRAM, 0);
    int retfd =0;
    if(sockt < 0)
    {
            printf("socket error !");
            return 0;
    }

    struct sockaddr_in serv_addr;
    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    serv_addr.sin_port = htons(local_port);

    udpfd_list[udpfd_list_head].fd=sockt;
    memset(&udpfd_list[udpfd_list_head].target_addr,0,target_addr_len);
    udpfd_list[udpfd_list_head].target_addr.sin_family = AF_INET;
    udpfd_list[udpfd_list_head].target_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    udpfd_list[udpfd_list_head].target_addr.sin_port = htons(remote_port);
    retfd = udpfd_list_head;
#ifdef UDP_RECVSIG
    udpfd_list[udpfd_list_head].recvbuf = creat_ring(1024*512);
    pthread_create(&udpfd_list[udpfd_list_head].pt,NULL,udp_recvTask,&udpfd_list[udpfd_list_head]); 
#endif

    udpfd_list_head++;

    if(bind(sockt,(struct sockaddr*)&serv_addr,sizeof(serv_addr))<0){
        printf("bind error\n");
    }
    return retfd;
}

int udpsend(int fd,char* buf,int len)
{
    return sendto(udpfd_list[fd].fd,buf,len,0,(struct sockaddr *)&udpfd_list[fd].target_addr,target_addr_len);
}

int udpread(int fd,char* buf,int len)
{
#ifdef UDP_RECVSIG
    return get_ring(udpfd_list[fd].recvbuf,buf,len);
#else
    return recvfrom(udpfd_list[fd].fd,buf,len,0,(struct sockaddr *)&from_addr,&from_addr_len);
#endif
}
