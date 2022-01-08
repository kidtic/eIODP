#ifndef _RING_H_
#define _RING_H_

#include "type.h"

#define MOONOS_MALLOC(size) malloc(size)
#define MOONOS_FREE(P) free(P)

typedef struct
{
    uint32 bufSize;

    uint8 *buf;
    uint32 pIn; //环形缓冲的头指针
    uint32 pOut; //环形缓冲的尾指针

}RING;


RING* creat_ring(uint32 size);

void delate_ring(RING* p);

int put_ring(RING* p,uint8* buf,uint32 size);
int get_ring(RING* p,uint8* buf,uint32 size);
uint16 size_ring(RING* p);

#endif
