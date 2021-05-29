#ifndef _TYPES_H
#define _TYPES_H

typedef unsigned int    uint;
typedef unsigned short  ushort;
typedef unsigned char   uchar;
typedef unsigned short  wchar;

typedef unsigned char   uint8;
typedef unsigned short  uint16;
typedef unsigned int    uint32;
typedef unsigned long   uint64;

typedef char            int8;
typedef short           int16;
typedef int             int32;
typedef long long       int64;

typedef unsigned long   uintptr_t;
typedef unsigned long   size_t;
typedef unsigned long   __off_t;

typedef int     pid_t;
typedef int64   off_t;

typedef uint32  mode_t;
typedef int32   dev_t;
typedef uint64  ino_t;
typedef uint16  nlink_t;
typedef uint32  uid_t;
typedef uint32  gid_t;
typedef int32   blksize_t;
typedef int64   blkcnt_t;

#define NULL ((void*)0)

#endif