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

char* strcpy(char* dst, const char* src){
    char* r=dst;
    while((*r++ = *src++)!='\0');
    return dst;
}

char* strncpy(char *dst, const char *src, uint n){
    char *r=dst;
    while(n-- > 0 && (*r++ = *src++) != '\0');
    while(n-- > 0) *r++ = '\0';
    return dst;
}

char* strchr(const char *s, char c)
{
    while(*s != '\0' && *s != c){
        ++s;
    }
    return *s==c ? s : NULL;
}

int strlen(const char *str){
    int num = 0;
    while(*str++ != '\0') num++;
    return num;
}

int strncmp(const char *str1, const char *str2, uint n){
    for(;n>0 && *str1!='\0' && *str2!='\0';n--, str1++, str2++){
        if(*str1 != *str2) break;
    }
    if(n==0) return 0;
    return *str1 - *str2;
}