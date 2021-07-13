/*
 * @Author: Yinwhe
 * @Date: 2021-07-10 20:06:58
 * @LastEditors: Yinwhe
 * @LastEditTime: 2021-07-13 10:43:25
 * @Description: file information
 * @Copyright: Copyright (c) 2021
 */
#ifndef PUT_H
#define PUT_H
typedef __builtin_va_list va_list;

#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v,l) __builtin_va_arg(v,l)
extern int printlocking;
int  puts(const char *s);
void printf_init();
int  printf(char *fmt, ...);
void print_logo();
void panic(const char *s);

#endif

