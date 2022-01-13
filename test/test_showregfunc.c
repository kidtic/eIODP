#include <stdio.h>


#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>

#include <eiodp.h>


extern int udpsend(int fd,char* buf,int len);
extern int udpread(int fd,char* buf,int len);

int func1(uint16 len, void* data,uint16* retlen,void* retdata){
    return 1;
}
int func2(uint16 len, void* data,uint16* retlen,void* retdata){
    return 1;
}
int func3(uint16 len, void* data,uint16* retlen,void* retdata){
    return 1;
}
int func4(uint16 len, void* data,uint16* retlen,void* retdata){
    return 1;
}

int main()
{

    int socktServer = udpopen(8888,7777);
    eIODP_TYPE* pServer;
    pServer = eiodp_init(socktServer,udpread,udpsend);

    eiodpRegister(pServer,0x245,func1);
    eiodpRegister(pServer,0x284,func2);
    eiodpRegister(pServer,0x258,func3);
    eiodpRegister(pServer,0x7777,func4);

    printf("func1:%x\n",func1);
    printf("func2:%x\n",func2);
    printf("func3:%x\n",func3);
    printf("func4:%x\n",func4);

    eiodpShowRegFunc(pServer);


}