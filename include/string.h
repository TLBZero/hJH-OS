#pragma once

#include "types.h"

int   memcmp(const void*, const void*, uint);
void* memset(void *dst, int c, uint n);
void* memmove(void *dst, const void *src, uint n);
void* memcpy(void *dst, void *src, uint size);

int   strlen(const char* str);
char* strcpy(char* dst, const char* src);
char* strncpy(char *dst, const char *src, int n);
char* strchr(const char *str, char c);
int strncmp(const char *str1, const char *str2, int n);