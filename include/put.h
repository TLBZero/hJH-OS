#ifndef PUT_H
#define PUT_H
typedef __builtin_va_list va_list;

#define va_start(v,l) __builtin_va_start(v,l)
#define va_end(v) __builtin_va_end(v)
#define va_arg(v,l) __builtin_va_arg(v,l)

int puts(const char *s);
void printf_init();
void printf(char *fmt, ...);
void print_logo();
void panic(const char *s);

#endif

