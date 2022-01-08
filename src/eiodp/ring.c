//循环缓冲区
#include "type.h"
#include "ring.h"
#include "stdlib.h"
//#include "rt_heap.h"


RING* creat_ring(uint32 size)
{
    RING* pRet=(RING*)MOONOS_MALLOC(sizeof (RING));
    pRet->buf=(uint8*)MOONOS_MALLOC(size);
    if(pRet->buf!=nullptr)
    {
        pRet->bufSize=size;
        pRet->pIn=0;
        pRet->pOut=0;

    }
    else {
        MOONOS_FREE(pRet);
        pRet=nullptr;
    }

    return pRet;
}

void delate_ring(RING* p)
{
    MOONOS_FREE(p->buf);
    MOONOS_FREE(p);
}

uint16 size_ring(RING* p)
{
    if(p->pIn >= p->pOut){
        return p->pIn-p->pOut;
    }
    else{
        return p->bufSize-(p->pOut-p->pIn);
    }
}

int put_ring(RING* p,uint8* buf,uint32 size)
{
		int i;
    //首先要判断是否会写满
    if(size+size_ring(p) >= p->bufSize){
        return -1;
    }

    
    for(i=0;i<size;i++){
        p->buf[p->pIn]=buf[i];

        if((p->pIn+1) >= p->bufSize){
            p->pIn=0;
        }
        else {
            p->pIn++;
        }
    }
    return i;

}

int get_ring(RING* p,uint8* buf,uint32 size)
{
	int i;
    if(p->pIn == p->pOut) return 0;

    
    for(i=0;i<size;i++){
        buf[i]=p->buf[p->pOut];

        if((p->pOut+1) >= p->bufSize){
            p->pOut=0;
        }
        else {
            p->pOut++;
        }


        if(p->pIn == p->pOut){
            return i+1;
        }
    }
    return i;
}
