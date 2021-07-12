/*
 * @Author: Yinwhe
 * @Date: 2021-07-10 20:06:58
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-07-12 13:24:23
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */
#pragma once

#include "types.h"

int   memcmp(const void*, const void*, uint);
void* memset(void *dst, int c, uint n);
void* memmove(void *dst, void *src, uint n);
void* memcpy(void *dst, const void *src, uint size);

int   strlen(const char* str);
char* strcpy(char* dst, const char* src);
char* strncpy(char *dst, const char *src, int n);
char* strchr(char *str, char c);
int   strncmp(const char *str1, const char *str2, int n);