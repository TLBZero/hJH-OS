/*
 * @Author: Yinwhe
 * @Date: 2021-07-10 20:06:58
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-07-14 00:42:30
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */
#include "string.h"
#include "types.h"
#include "put.h"

void *memset(void *dst, int ch, size_t num)
{
    char *dst_tp = dst;
    for(size_t i = 0; i <num; i++)
        *dst_tp++ = ch;
    return dst;
}

void *memcpy(void *dst, const void *src, size_t num){
    char *dst_tp = (char *) dst;
    char *src_tp = (char *) src;
    for(size_t i = 0; i < num; i++)
        *dst_tp++ = *src_tp++;
    return dst;
};


void* memmove(void *dst, void *src, size_t num){
    const char *s;
    char *d;

    s = src;
    d = dst;
    if(s < d && s + num > d){
        s += num;
        d += num;
        while(num-- > 0)
        *--d = *--s;
    } else
        while(num-- > 0)
        *d++ = *s++;

    return dst;
}

char* strcpy(char* dst, const char* src){
    char* r=dst;
    while((*r++ = *src++)!='\0');
    return dst;
}

char* strncpy(char *dst, const char *src, int64 n){
    char *r=dst;
    while(n-- > 0 && (*r++ = *src++) != '\0');
    while(n-- > 0) *r++ = '\0';
    return dst;
}

char* strchr(char *s, char c)
{
    while(*s != '\0' && *s != c){
        ++s;
    }
    return ((*s)==c) ? s : NULL;
}

uint64 strlen(const char *str){
    uint64 num = 0;
    while(*str++ != '\0') num++;
    return num;
}

int strncmp(const char *str1, const char *str2, int64 n){
    for(;n>0 && *str1!='\0' && *str2!='\0';n--, str1++, str2++){
        if(*str1 != *str2) break;
    }
    if(n==0) return 0;
    return *str1 - *str2;
}

int strcmp(const char *str1, const char *str2){
    for(; *str1!='\0' && *str2!='\0'; str1++, str2++){
        if(*str1 != *str2) break;
    }
    return *str1 - *str2;
}