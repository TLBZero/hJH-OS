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

/* these are defined by POSIX and also present in glibc's dirent.h */
#define DT_UNKNOWN	0
#define DT_FIFO		1
#define DT_CHR		2
#define DT_DIR		4
#define DT_BLK		6
#define DT_REG		8
#define DT_LNK		10
#define DT_SOCK		12
#define DT_WHT		14

/* File Structure */
struct file {
    uint8 f_type;   /* File type */
    int f_perm;     /* Permission Control */
    struct dirent* f_entry;
    int f_count;    /* Reference Count */
    uint32 f_pos;   /* File Pointer */
};

struct kdirent {
    int64           d_ino;    /* 64-bit inode number */
    int64           d_off;    /* 64-bit offset to next structure */
    unsigned short  d_reclen; /* Size of this dirent */
    unsigned char   d_type;   /* File type */
    char            d_name[]; /* Filename (null-terminated) */
};

/* File status */
struct kstat {
	dev_t       st_dev;         /* ID of device containing file */
	ino_t       st_ino;         /* inode number */
	mode_t      st_mode;        /* protection */
	nlink_t     st_nlink;       /* number of hard links */
	uid_t       st_uid;         /* user ID of owner */
	gid_t       st_gid;         /* group ID of owner */
	dev_t       st_rdev;        /* device ID (if special file) */
	unsigned long __pad;
	off_t       st_size;        /* total size, in bytes */
	blksize_t   st_blksize;     /* blocksize for file system I/O */
	int         __pad2;
	blkcnt_t    st_blocks;      /* number of 512B blocks allocated */
	long        st_atime_sec;   /* time of last access */
	long        st_atime_nsec;
	long        st_mtime_sec;   /* time of last modification */
	long        st_mtime_nsec;
	long        st_ctime_sec;   /* time of last status change */
	long        st_ctime_nsec;
	unsigned    __unused[2];
};

void sysfile_init();
void sysfile_test();