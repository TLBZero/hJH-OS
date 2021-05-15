#pragma once

#include "types.h"
/* Attributions */
#define ATTR_READ_ONLY      0x01
#define ATTR_HIDDEN         0x02
#define ATTR_SYSTEM         0x04
#define ATTR_VOLUME_ID      0x08
#define ATTR_DIRECTORY      0x10
#define ATTR_ARCHIVE        0x20
#define ATTR_LONG_NAME      ATTR_READ_ONLY | ATTR_HIDDEN | ATTR_SYSTEM | ATTR_VOLUME_ID

#define BAD_CLUSTER         0x0FFFFFF7
#define FAT32_EOC           0x0FFFFFF8
#define EOC                 0x0FFFFFFF

#define DIR_ENTRY_FREE      0xE5
#define DIR_ENTRY_END       0x00

#define ENTRY_MASK          0x0FFFFFFF
#define ENTRY_CACHE_NUM     50

#define LONG_NAME_LEN       13
#define SHORT_NAME_LEN      11

#define FAT32_MAX_FILENAME  255
#define FAT32_MAX_PATH      260
#define FAT32_MAX_FILESIZE  0x100000000
#define ENTRY_CACHE_NUM     50

struct dir_entry {
    char  filename[FAT32_MAX_FILENAME + 1];
    uint8   attribute;
    // uint8   create_time_tenth;
    // uint16  create_time;
    // uint16  create_date;
    // uint16  last_access_date;
    uint32  first_clus;
    // uint16  last_write_time;
    // uint16  last_write_date;
    uint32  file_size;

    uint32  cur_clus;
    uint    clus_cnt;

    /* for OS */
    uint8   dev;
    uint8   dirty;
    short   valid;
    int     ref;
    uint32  off;            // offset in the parent dir entry, for writing convenience
    struct dir_entry *parent;  // because FAT32 doesn't have such thing like inum, use this for cache trick
    struct dir_entry *next;
    struct dir_entry *prev;
    struct sleeplock lock;
};