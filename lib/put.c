#include "put.h"
#include "rustsbi.h"
int puts(const char *s)
{
    while (*s != '\0')
    {
        console_putchar(*s);
        s++;
    }
    return 0;
}

void putchar(const char ch){
    console_putchar(ch);
}

static char hex[] = "0123456789abcdef";

static void printptr(unsigned long x)
{
    int i;
    puts("0x");
    for (i = 0; i < (sizeof(unsigned long) * 2); i++, x <<= 4)
        putchar(hex[x >> (sizeof(unsigned long) * 8 - 4)]);
}

static void printint(int xx, int base, int sign)
{
    char buf[17];
    int i;
    unsigned int x;

    if(sign && (sign = xx < 0)) x = -xx;
    else x = xx;

    i = 0;
    do {
        buf[i++] = hex[x % base];
    } while((x /= base) != 0);

    if(sign) buf[i++] = '-';

    while(--i >= 0) putchar(buf[i]);
}

void printf(char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    while (*fmt)
    {
        if (*fmt == '%')
            switch (*(++fmt))
            {
            case 'd': //int
                printint(va_arg(ap,int),10,1);
                break;
            case 'x'://hex
                puts("0x");
                printint(va_arg(ap,int),16,0);
                break;
            case 'p': //address
                printptr(va_arg(ap,unsigned long));
                break;
            case 's':
                puts(va_arg(ap, char*));
                break;
            }
        else putchar(*fmt);
        fmt++;
    }
    va_end(ap);
}

void panic(const char *s)
{
    printf("[Kernel Panic]: %s\n", s);
    while(1);
}

void print_logo(){
	printf(" _         ___   _   _ \n\
| |       |_  | | | | |\n\
| |__       | | | |_| |\n\
| '_ \\      | | |  _  |\n\
| | | | /\\__/ / | | | |\n\
|_| |_| \\____/  \\_| |_/\n");
}