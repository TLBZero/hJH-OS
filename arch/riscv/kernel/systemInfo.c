#include "systemInfo.h"

struct utsname info={"hJH","0.00001","k210"};

void uname(struct utsname *uts)
{
    int i;
    for(i=0;i<3*_UTSNAME_SYSNAME_LENGTH;i++)
    {
        *((char *)uts+i)=*((char *)&info+i);
    }
}