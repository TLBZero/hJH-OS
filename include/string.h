#pragma once

#include "types.h"

int   memcmp(const void*, const void*, uint);
int   strlen(const char*);
void* memset(void *dst, int c, uint n);
void* memmove(void *dst, const void *src, uint n);
void* memcpy(void *dst, void *src, uint size);