#include "string.h"
#include "types.h"
#include "put.h"

void *memset(void *dst, int init, uint num)
{
    char *dst_tp = (char *) dst;
    for(uint i = 0; i < num; i++)
        dst_tp[i] = init;
    return dst;
}

void *memcpy(void *dst, void *src, uint size){
    char *dst_tp = (char *) dst;
    char *src_tp = (char *) src;
    for(uint i = 0; i < size; i++)
        dst_tp[i]=src_tp[i];
    return dst;
};


void* memmove(void *dst, const void *src, uint n){
    memcpy(dst, src, n);
    memset(src, 0, n);
}