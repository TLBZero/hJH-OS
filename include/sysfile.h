#pragma once
#include "types.h"
#include "fat32.h"
#include "pipe.h"

#define SYSOFILENUM     100
#define PROCOFILENUM    16
#define CONSOLE 1

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
                                     * openat should use the current
                                     * working directory. */
#define AT_SYMLINK_NOFOLLOW	0x100   /* Do not follow symbolic links.  */
#define AT_REMOVEDIR		0x200   /* Remove directory instead of
                                     * unlinking file.  */
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

/* File Permission Control */
#define READABLE        0x01
#define WRITABLE        0x02
#define READ_AND_WRITE  0x03

struct task_struct;

/* File Structure */
struct file {
    uint8 	f_type;   	/* File type */
    int 	f_perm;     /* Permission Control */
    struct dirent* f_entry;
    int 	f_count;    /* Reference Count */
    uint32 	f_pos;   	/* File Pointer */
    uint8 	major;    	/* Device Distinguisher */

    struct pipe *pipe;
};

struct linux_dirent64 {
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

/* Map major device number to device functions. */
struct devsw {
    int (*read)(int, uint64, int);
    int (*write)(int, uint64, int);
};

extern struct spinlock SysFLock;
extern struct file SysFTable[SYSOFILENUM];

void	sysfile_init();
void 	procfile_init(struct task_struct* task);
uint 	falloc();
void 	frelease(struct file* file);
int 	fread(struct file* file, void* dst, int num);
int 	fwrite(struct file* file, void* src, int num);
struct dirent* fcreate(char *pathname, uint type, uint flags);
struct file* fdup(struct file* file);
void 	fseek(struct file* file, uint pos);
int 	dirnext(struct file *file, struct dirent* ep);

/* syscall in fs */ 
char *sys_getcwd(uintptr_t *regs);
int sys_dup(uintptr_t *regs);
int sys_dup3(uintptr_t *regs);
int sys_chdir(uintptr_t *regs);
int sys_openat(uintptr_t *regs);
int sys_close(uintptr_t *regs);
int sys_getdents64(uintptr_t *regs);
int sys_read(uintptr_t *regs);
int sys_write(uintptr_t *regs);
int sys_unlinkat(uintptr_t *regs);
int sys_mkdirat(uintptr_t *regs);
int sys_fstat(uintptr_t *regs);
int sys_pipe2(uintptr_t *regs);