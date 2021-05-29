#ifndef _SYSTEMINFO_H
#define _SYSTEMINFO_H

#define _UTSNAME_SYSNAME_LENGTH 20

struct utsname
{
    char sysname[_UTSNAME_SYSNAME_LENGTH];
    char version[_UTSNAME_SYSNAME_LENGTH];
    char machine[_UTSNAME_SYSNAME_LENGTH];
};

void uname(struct utsname *);

#endif