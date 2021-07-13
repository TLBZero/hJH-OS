/*
 * @Author: Yinwhe
 * @Date: 2021-07-10 20:06:58
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-07-14 00:33:45
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */
#pragma once

#include "types.h"

void* memset(void *dst, int c, size_t n);
void* memmove(void *dst, void *src, size_t n);
void* memcpy(void *dst, const void *src, size_t n);

uint64  strlen(const char* str);
char*   strcpy(char* dst, const char* src);
char*   strncpy(char *dst, const char *src, int64 n);
char*   strchr(char *str, char c);
int     strcmp(const char *str1, const char *str2);
int     strncmp(const char *str1, const char *str2, int64 n);