#pragma once

#include "types.h"
#include "sleeplock.h"

/* Attributions */
#define ATTR_READ_ONLY      0x01
#define ATTR_HIDDEN         0x02
#define ATTR_SYSTEM         0x04
#define ATTR_VOLUME_ID      0x08
#define ATTR_DIRECTORY      0x10
#define ATTR_ARCHIVE        0x20
#define ATTR_LONG_NAME      0x0F

/* Cluster Marks */
#define BAD_CLUSTER         0x0FFFFFF7
#define FAT32_EOC           0x0FFFFFF8
#define EOC                 0x0FFFFFFF

/* Dir Marks */
#define DIR_ENTRY_FREE      0xE5
#define DIR_ENTRY_END       0x00

/* Some other Specifications */
#define ENTRY_MASK          0x0FFFFFFF
#define ENTRY_CACHE_NUM     50

#define LONG_NAME_LEN       13
#define SHORT_NAME_LEN      11
#define LAST_LONG_ENTRY     0x40

#define FAT32_MAX_FILENAME  255
#define FAT32_MAX_PATH      260
#define FAT32_MAX_FILESIZE  0x100000000
#define ENTRY_CACHE_NUM     50

/* This struct only exist in memory, not in disk */
struct dirent {
    char  filename[FAT32_MAX_FILENAME + 1];
    uint8   attribute;
    // uint8   create_time_tenth;
    // uint16  create_time;
    // uint16  create_date;
    // uint16  last_access_date;
    uint32  first_clus;
    // uint16  write_time;
    // uint16  write_date;
    uint32  file_size;

    uint32  cur_clus;
    uint32  clus_cnt;

    /* for OS */
    uint8   dev;
    uint8   dirty;
    uint8   uptodate;
    int     refcnt;
    uint32  off;            // offset in the parent dir entry, for writing convenience
    struct dirent *parent;  // because FAT32 doesn't have such thing like inum, use this for cache trick
    struct dirent *next;
    struct dirent *prev;
    struct sleeplock lock;
};

typedef struct short_name_entry {
    char        name[SHORT_NAME_LEN];
    uint8       attr;
    uint8       _nt_res;
    uint8       _crt_time_tenth;
    uint16      _crt_time;
    uint16      _crt_date;
    uint16      _lst_acce_date;
    uint16      fst_clus_hi;
    uint16      _lst_wrt_time;
    uint16      _lst_wrt_date;
    uint16      fst_clus_lo;
    uint32      file_size;
} __attribute__((packed, aligned(4))) short_name_entry_t;

typedef struct long_name_entry {
    uint8       order;
    wchar       name1[5];
    uint8       attr;
    uint8       _type;
    uint8       checksum;
    wchar       name2[6];
    uint16      _fst_clus_lo;
    wchar       name3[2];
} __attribute__((packed, aligned(4))) long_name_entry_t;


int fat_init();
int eread(struct dirent *entry, uint64 dst, uint off, uint num);
struct dirent *edup(struct dirent *entry);
int ewrite(struct dirent *entry, uint64 src, uint off, uint num);
void eput(struct dirent *entry);
char *formatname(char *name);
uint8 cal_checksum(uchar* shortname);
struct dirent *ealloc(struct dirent *dp, char *name, int attr);
void eupdate(struct dirent *entry);
void etrunc(struct dirent *entry);
void eremove(struct dirent *entry);
int enext(struct dirent *dp, struct dirent *ep, uint off, int *count);
struct dirent *dirlookup(struct dirent *dp, char *filename, uint *poff);
struct dirent *ename(char *path);
struct dirent *enameparent(char *path, char* name);
int epath(struct dirent* ep, char *buf, uint size);
uint elen(struct dirent* ep);