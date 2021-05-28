#pragma once
#include "types.h"
#include "fat32.h"

#define SYSOFILENUM     100
#define PROCOFILENUM    16

/* Flags */
#define O_RDONLY        00000000
#define O_WRONLY        00000001
#define O_RDWR          00000002
#define O_ACCMODE       00000003
#define O_CREAT         00000040
#define O_EXCL          00000080
#define O_NOCTTY        00000100
#define O_TRUNC         00000200
#define O_APPEND        00000400
#define O_NONBLOCK      00000800
#define O_DSYNC         00001000
#define FASYNC          00002000
#define O_NOFOLLOW      00020000

#define AT_FDCWD		    -100    /* Special value used to indicate
                                        openat should use the current
                                        working directory. */
#define AT_SYMLINK_NOFOLLOW	0x100   /* Do not follow symbolic links.  */
#define AT_REMOVEDIR		0x200   /* Remove directory instead of
                                        unlinking file.  */
#define AT_SYMLINK_FOLLOW	0x400   /* Follow symbolic links.  */
#define AT_NO_AUTOMOUNT		0x800	/* Suppress terminal automount traversal */
#define AT_EMPTY_PATH		0x1000	/* Allow empty relative pathname */

#define C_REG       0x01
#define C_DIR       0x02

/* File Structure */
struct file {
    enum { REG, DIR, LNK, BLK, CHR, PIPE, SOCK} f_type;
    int f_perm;   // Permission Control
    struct dirent* f_entry;
    int f_count;    // Reference Count
    uint32 f_pos;   // File Pointer
};



void sysfile_init();
void sysfile_test();