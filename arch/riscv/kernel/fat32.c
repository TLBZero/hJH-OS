#include "fat32.h"
#include "buffer.h"
#include "put.h"
#include "string.h"
#include "sched.h"
#define DEBUG
union dentry {
    short_name_entry_t  sne;
    long_name_entry_t   lne;
};

static struct{
    struct {
        uint16 BytsPerSec;
        uint8  SecPerClus;
        uint16 RsvdSecCnt;
        uint8  NumFATs;
        uint16 RootEntCnt;
        uint32 HiddSec;
        uint32 TotSec32;
        uint32 FATSz32;
        uint32 RootClus;
    }BPB;

    uint32  FirstDataSec;
    uint32  DataSecCnt;
    uint32  DataClusCnt;
    uint32  BytSPerClus;
}FAT;

static struct entry_cache {
    struct spinlock lock;
    struct dirent entries[ENTRY_CACHE_NUM];
} ecache;

/* root也作为cache的head */
static struct dirent root;

static uint getClusCnt(uint32 firstClus);
/**
 * @brief Initialize fat32 fs
 * 
 * @return 0 for success and -1 for errors
 */
int fat_init(){
    struct buffer_head *bh = bread(0, 0);
    if (strncmp((char const*)(bh->buf + 82), "FAT32", 5))
        panic("fat_init error, not FAT32!");
    // Read in
    memcpy(&FAT.BPB.BytsPerSec, bh->buf + 11, 2);
    FAT.BPB.SecPerClus = *(bh->buf + 13);
    FAT.BPB.RsvdSecCnt = *(uint16 *)(bh->buf + 14);
    FAT.BPB.NumFATs = *(bh->buf + 16);
    FAT.BPB.HiddSec = *(uint32 *)(bh->buf + 28);
    FAT.BPB.TotSec32 = *(uint32 *)(bh->buf + 32);
    FAT.BPB.FATSz32 = *(uint32 *)(bh->buf + 36);
    FAT.BPB.RootClus = *(uint32 *)(bh->buf + 44);
    FAT.FirstDataSec = FAT.BPB.RsvdSecCnt + FAT.BPB.NumFATs * FAT.BPB.FATSz32;
    FAT.DataSecCnt = FAT.BPB.TotSec32 - FAT.FirstDataSec;
    FAT.DataClusCnt = FAT.DataSecCnt / FAT.BPB.SecPerClus;
    FAT.BytSPerClus = FAT.BPB.SecPerClus * FAT.BPB.BytsPerSec;

    brelse(bh);

    #ifdef DEBUG
    printf("[FAT32 init]bytes per sector: %d\n", FAT.BPB.BytsPerSec);
    printf("[FAT32 init]root cluster: %d\n", FAT.BPB.RootClus);
    printf("[FAT32 init]sector per clus: %d\n", FAT.BPB.SecPerClus);
    printf("[FAT32 init]fat cnt: %d\n", FAT.BPB.NumFATs);
    printf("[FAT32 init]fat sz: %d\n", FAT.BPB.FATSz32);
    printf("[FAT32 init]hidden sec: %d\n", FAT.BPB.HiddSec);
    printf("[FAT32 init]reserved sec: %d\n", FAT.BPB.RsvdSecCnt);
    printf("[FAT32 init]total sec: %d\n", FAT.BPB.TotSec32);
    printf("[FAT32 init]byte per cluster: %d\n", FAT.BytSPerClus);
    printf("[FAT32 init]first data sector: %d\n", FAT.FirstDataSec);
    printf("[FAT32 init]]data sector cnt: %d\n", FAT.DataSecCnt);
    printf("[FAT32 init]data clus cnt: %d\n",FAT.DataClusCnt);
    #endif

    // make sure that byts_per_sec has the same value with BSIZE 
    if (BSIZE != FAT.BPB.BytsPerSec) 
        panic("fat_init error, BytsPerSec not equals to BSIZE!");

    initlock(&ecache.lock, "ecache");
    memset(&root, 0, sizeof(root));
    initsleeplock(&root.lock, "root");
    root.attribute = (ATTR_DIRECTORY | ATTR_SYSTEM);
    root.first_clus = root.cur_clus = FAT.BPB.RootClus;
    root.dev = 0;
    root.dirty = 0;
    root.parent = &root; //Vital
    root.refcnt = 1;
    root.uptodate = 1;
    root.clus_cnt = getClusCnt(root.first_clus);
    strncpy(root.filename, "/", FAT32_MAX_FILENAME);
    root.prev = &root;
    root.next = &root;
    for(struct dirent *de = ecache.entries; de < ecache.entries + ENTRY_CACHE_NUM; de++) {
        de->dev = 0;
        de->uptodate = 0;
        de->refcnt = 0;
        de->dirty = 0;
        de->parent = NULL;
        initsleeplock(&de->lock, "entry");
        //Insert after root
        de->next = root.next;
        de->prev = &root;
        root.next->prev = de;
        root.next = de;
    }
    #ifdef DEBUG
    printf("[fat_init]fat init done!\n");
    #endif
    return 0;
}

/**
 * @brief 给定任何有效的簇号cluster，计算其第一个sector号
 * @param cluster 给定的有效簇号
 * 
 * @return sector号
 */
static inline uint32 first_sec_of_clus(uint32 cluster){
    return ((cluster - 2) * FAT.BPB.SecPerClus) + FAT.FirstDataSec;
}

/**
 * @brief 给定任何有效的簇号cluster，计算在FAT中该簇号的entry所在的sector
 * 
 * @param whichFAT 指定要读取的FAT
 */
static inline uint32 fat_sec_of_clus(uint32 cluster, uint8 whichFAT){
    return (FAT.BPB.RsvdSecCnt + (cluster << 2)/(FAT.BPB.BytsPerSec) + (whichFAT - 1) * FAT.BPB.FATSz32);
}

/**
 * @brief 给定任何有效的簇号cluster，计算在FAT中该簇号的entry在sector中的offset
 */
static inline uint32 fat_offset_of_clus(uint32 cluster)
{
    return ((cluster << 2) % FAT.BPB.BytsPerSec);
}

/**
 * @brief 读取给定cluster的FAT entry
 * 
 * @param cluster 要读取FAT的cluster
 */
static uint32 readFAT(uint32 cluster){
    //Overflow
    //if(cluster <=1 || cluster > FAT.DataClusCnt + 1) return EOC;

    struct buffer_head *bh;
    
    uint32 sec = fat_sec_of_clus(cluster, 1);
    uint32 off = fat_offset_of_clus(cluster);
    bh = bread(0, sec);
 
    uint32 res =  *(uint32 *)(bh->buf + off);
    brelse(bh);
    return res&ENTRY_MASK;
}

/**
 * @brief 将content写入指定的cluster的FAT entry中
 * 
 * @param cluster 指定的clsuter
 * @param content 要写入entry的内容
 * @return 操作成功则返回 0，否则返回 -1
 */
static int writeFAT(uint32 cluster, uint32 content){
    //Overflow
    if(cluster <=1 || cluster > FAT.DataClusCnt + 1)
        return -1;

    struct buffer_head *bh;
    
    uint32 sec = fat_sec_of_clus(cluster, 1);
    uint32 off = fat_offset_of_clus(cluster);
    bh = bread(0, sec);
    //Write into buffer first
    *((uint32 *)(bh->buf + off)) = content&ENTRY_MASK;
    bwrite(bh);
    brelse(bh);

    return 0;
}

/**
 * @brief 清空cluster的内容，也即置0
 * 
 * @param cluster 要清空的cluster
 */
static void emptyClus(uint32 cluster){
    //Overflow
    if(cluster <=1 || cluster > FAT.DataClusCnt + 1)
        panic("Empty non-existing cluster");

    struct buffer_head *bh;
    
    uint32 sec = first_sec_of_clus(cluster);
    for(int i=0;i<FAT.BPB.SecPerClus;i++, sec++){
        bh = bread(0, sec);
        memset(bh->buf, 0, BSIZE);
        bwrite(bh);
        brelse(bh);
    }
}

/**
 * @brief 分配一个cluster
 * 
 * @param dev 设备号(这里应该都是0)
 * @return 分配的cluster number
 */
static uint32 allocClus(uint8 dev){
    struct buffer_head *bh;
    uint32 sec = FAT.BPB.RsvdSecCnt;    //Start sector of FAT1
    const uint32 EntryNumPerSec = FAT.BPB.BytsPerSec / 32;

    for(int i=0;i<FAT.BPB.FATSz32;i++, sec++){
        bh = bread(dev, sec);
        uint32 *p = bh->buf;
        for(int j=0;j<EntryNumPerSec;j++){
            if(p[j]==0){
                p[j] = EOC; //Set used
                bwrite(bh);
                brelse(bh);
                uint32 cluster = i * EntryNumPerSec + j;
                emptyClus(cluster);
                #ifdef DEBUG
                printf("alloc Clus:%d\n", cluster);
                #endif
                return cluster;
            }
        }
        brelse(bh);
    }
    panic("No empty cluster on sdcard!");
}

/**
 * @brief 释放cluster
 * 
 * @param cluster 
 */
static void freeClus(uint32 cluster){
    // Just clear it's FAT entry
    writeFAT(cluster, 0);
}

/**
 * @brief 根据off重新定位entry中的cur_clus
 * 
 * @param entry 要重新定位的entry
 * @param off   指定的off
 * @param alloc 如果超出了文件，1则分配新的clus，0则不分配
 * @return 相对于当前cluster的偏移量
 */
static uint reallocCurClus(struct dirent* entry, uint off, uint8 alloc){
    uint32 rlvClus = off/FAT.BytSPerClus + 1;  // Start from 1
    off = off % FAT.BytSPerClus;
    uint cnt;
    if(rlvClus >= entry->clus_cnt){
        // Go to the last one first
        while(readFAT(entry->cur_clus)<FAT32_EOC)
            entry->cur_clus = readFAT(entry->cur_clus);
        while(rlvClus > entry->clus_cnt){
            if(!alloc) return -1;
            uint32 newClus = allocClus(0);
            writeFAT(entry->cur_clus, newClus);
            entry->cur_clus = newClus;
            entry->clus_cnt++;
        }
        return off;
    }

    entry->cur_clus = entry->first_clus;
    cnt = 1;
    while(rlvClus > cnt){
        entry->cur_clus = readFAT(entry->cur_clus);
        cnt++;
    }
    return off;
}

/**
 * @brief 对给定的cluster进行读/写操作
 * 
 * @param cluster 指定cluster
 * @param write   1：写，0：读
 * @param addr    对于写操作，相当于source address；对于读操作相当于destination address
 * @param off     偏移量
 * @param num     读/写字符数
 * @return        成功读/写字符数
 */
static uint rwClus(uint32 cluster, int write, void* addr, uint off, uint num){
    if(off >= FAT.BytSPerClus) return 0;
    num = (off + num <= FAT.BytSPerClus) ? (num) : (FAT.BytSPerClus - off);
    
    uint32 total, sec, m;
    sec = first_sec_of_clus(cluster) + off / FAT.BPB.BytsPerSec;
    off = off % FAT.BPB.BytsPerSec;
    
    struct buffer_head* bh;
    for(total=0;total<num;total+=m, addr+=m, sec++){
        bh = bread(0,sec);
        m = (num - total + off > FAT.BPB.BytsPerSec) ? (FAT.BPB.BytsPerSec - off) : (num - total);
        if(write){
            memcpy(bh->buf+off, addr, m);
            bwrite(bh);
        }else{
            memcpy(addr, bh->buf+off, m);
        }
        off = (off + m) % FAT.BPB.BytsPerSec;
        brelse(bh);
    }
    return total;
}

static void inline liftEntry(struct dirent *ep){
    acquire(&ecache.lock);
    //Remove ep from original place
    ep->prev->next = ep->next;
    ep->next->prev = ep->prev;

    //Insert after head
    ep->next = root.next;
    ep->prev = &root;
    root.next = ep;
    ep->next->prev = ep;
    release(&ecache.lock);
}

static void inline downEntry(struct dirent *ep){
    acquire(&ecache.lock);
    //Remove ep from original place
    ep->prev->next = ep->next;
    ep->next->prev = ep->prev;

    //Insert before head
    ep->next = &root;
    ep->prev = root.prev;
    root.prev = ep;
    ep->prev->next = ep;
    release(&ecache.lock);
}

struct dirent *edup(struct dirent *entry){
    if (entry != 0) {
        acquire(&ecache.lock);
        entry->refcnt++;
        release(&ecache.lock);
    }
    #ifdef DEBUG
    printf("%s edup success! ref:%d\n", entry->filename, entry->refcnt);
    #endif
    return entry;
}

/**
 * @brief 将entry指向的file内容读取到指定的地址上
 * 
 * @param entry 指定的entry
 * @param dst   指定的地址
 * @param off   文件内的偏移量
 * @param num   读取的字符数
 * @return 成功读取的字符数，或错误返回-1
 */
int eread(struct dirent *entry, uint64 dst, uint off, uint num){
    if(entry->attribute & ATTR_DIRECTORY){
        #ifdef DEBUG
        printf("eread prompt, read dir!\n");
        #endif
        return -1;
    }

    if(off>entry->file_size || off + num < off) return -1;
    // read to the end of file, at most
    num = (off + num <= entry->file_size) ? (num) : (entry->file_size - off);
    off = reallocCurClus(entry, off, 0);

    uint total, m;
    for(total=0;total<num;total+=m, dst+=m){
        m = (num - total + off > FAT.BytSPerClus) ? (FAT.BytSPerClus - off) : (num - total);
        if (m != rwClus(entry->cur_clus, 0, dst, off, m))
            panic("eread error, read num inconsistent!");
        if(m + off >= FAT.BytSPerClus)
            entry->cur_clus = readFAT(entry->cur_clus);
        off = (off + m) % FAT.BytSPerClus;
    }

    return total;
}

/**
 * @brief 向entry指向的file中写入用户指定的地址上的内容
 * 
 * @param entry 指定的entry
 * @param src   指定的地址
 * @param off   偏移量
 * @param num   写入的字符数
 * @return 成功写入的字符数，或错误返回-1
 */
int ewrite(struct dirent *entry, uint64 src, uint off, uint num){
    if(entry->attribute & ATTR_DIRECTORY){
        #ifdef DEBUG
        printf("ewrite prompt, write content of dir!\n");
        #endif
        return -1;
    }
    if(off + num < off || entry->attribute & ATTR_READ_ONLY) return -1;

    //Empty file
    if(entry->first_clus == 0){
        entry->cur_clus = entry->first_clus = allocClus(entry->dev);
        entry->clus_cnt = 1;
        entry->dirty = 1;
    }

    uint total, m, clusOff;
    for(total=0;total<num;total+=m, src+=m, off+=m){
        clusOff = reallocCurClus(entry, off, 1);
        m = (num - total + clusOff > FAT.BytSPerClus) ? (FAT.BytSPerClus - clusOff) : (num - total);
        if(m!=rwClus(entry->cur_clus, 1, src, clusOff, m))
            panic("ewrite error, write num inconsistent!");
    }
    if(off>entry->file_size) entry->file_size = off;
    entry->dirty = 1;

    return total;
}


/**
 * @brief 在parent指定的目录下寻找或新建一个cache entry；
 * 
 * @param parent 指定的父目录
 * @param name 要寻找的entry名称，如果为空或者未找到则新建
 */
static struct dirent *eget(struct dirent *parent, char *name){
    struct dirent *ep;
    acquire(&ecache.lock);
    if(name){// Name given, search all cache
        for(ep=root.next;ep!=&root;ep=ep->next)
            if(ep->uptodate && ep->parent == parent && strncmp(ep->filename, name, FAT32_MAX_FILENAME) == 0){
                if(ep->refcnt++==0) ep->parent->refcnt++; //Vital
                #ifdef DEBUG
                printf("%s eget success! ref:%d, par:%p %s\n", ep->filename, ep->refcnt, ep->parent, ep->parent->filename);
                #endif
                release(&ecache.lock);
                return ep;
            }
    }
    for(ep=root.prev;ep!=&root;ep=ep->prev){    //LRU
        if(ep->refcnt==0){
            ep->refcnt=1;
            ep->parent=parent;
            ep->parent->refcnt++;
            ep->dev=0;
            ep->dirty=0;
            ep->uptodate=0;
            if(name) strncpy(ep->filename, name, FAT32_MAX_FILENAME);
            #ifdef DEBUG
            printf("%s eget success! ref:%d, par:%p %s\n", ep->filename, ep->refcnt, ep->parent, ep->parent->filename);
            #endif
            release(&ecache.lock);
            return ep;
        }
    }
    panic("eget error, no free ecache found!");
}

/**
 * @brief 释放掉cache entry
 */
void eput(struct dirent *entry){
    acquire(&ecache.lock);
    if (entry != &root && entry->refcnt == 1){
        release(&ecache.lock);

        acquiresleep(&entry->lock);
        if(entry->dirty) eupdate(entry);
        downEntry(entry);
        releasesleep(&entry->lock);
        entry->refcnt--;

        struct dirent *parent = entry->parent;
        if(--parent->refcnt==0) eput(parent);
        #ifdef DEBUG
        printf("%s eput success! ref:%d\n", entry->filename, entry->refcnt);
        #endif
        return;
    }

    if(entry->refcnt-- == 0)
        panic("eput error, trying to put released entry cache!");
    release(&ecache.lock);
    #ifdef DEBUG
    printf("%s eput success! ref:%d\n", entry->filename, entry->refcnt);
    #endif
}

char *formatname(char *name){
    static char illegal[] = { '\"', '*', '/', ':', '<', '>', '?', '\\', '|', 0 };
    char *p;
    while (*name == ' ' || *name == '.') { name++; }
    for (p = name; *p; p++) {
        char c = *p;
        if (c < 0x20 || strchr(illegal, c))
            return 0;
    }
    while (p-- > name) {
        if (*p != ' ') {
            p[1] = '\0';
            break;
        }
    }
    return name;
}

static void generate_shortname(char *shortname, char *name){
    static char illegal[] = { '+', ',', ';', '=', '[', ']', 0 };   // these are legal in l-n-e but not s-n-e
    int i = 0;
    char c, *p = name;
    for (int j = strlen(name) - 1; j >= 0; j--) {
        if (name[j] == '.') {
            p = name + j;
            break;
        }
    }
    while (i < SHORT_NAME_LEN && (c = *name++)) {
        if (i == 8 && p) {
            if (p + 1 < name) { break; }            // no '.'
            else {
                name = p + 1, p = 0;
                continue;
            }
        }
        if (c == ' ') { continue; }
        if (c == '.') {
            if (name > p) {                    // last '.'
                memset(shortname + i, ' ', 8 - i);
                i = 8, p = 0;
            }
            continue;
        }
        if (c >= 'a' && c <= 'z') {
            c += 'A' - 'a';
        } else {
            if (strchr(illegal, c) != NULL) {
                c = '_';
            }
        }
        shortname[i++] = c;
    }
    while (i < SHORT_NAME_LEN) {
        shortname[i++] = ' ';
    }
}

uint8 cal_checksum(uchar* shortname){
    uint8 sum = 0;
    for (int i = SHORT_NAME_LEN; i != 0; i--) {
        sum = ((sum & 1) ? 0x80 : 0) + (sum >> 1) + *shortname++;
    }
    return sum;
}


/**
 * @brief 将一个新的entry写到disk上
 * 
 * @param dp 该entry所在的目录
 * @param ep 即指定的entry
 * @param off entry在目录下的偏移量
 */
static void emake(struct dirent *dp, struct dirent *ep, uint off){
    if (!(dp->attribute & ATTR_DIRECTORY))
        panic("emake error, not dir!");
    if (off%32)
        panic("emake error, offset not aligned!");
    #ifdef DEBUG
    if(ep) printf("emake dp:%s, ep:%s, off:%d\n", dp->filename, ep->filename, off);
    else printf("emake dp:%s, off:%d\n", dp->filename, off);
    #endif
    union dentry de;
    memset(&de, 0, 32);
    if(off<=32 && dp!=&root){
        if(off == 0){
            strncpy(de.sne.name, ".          ", SHORT_NAME_LEN);
            de.sne.file_size = dp->file_size;
            de.sne.fst_clus_hi = dp->first_clus >> 16;
            de.sne.fst_clus_lo = dp->first_clus &0xFFFF;
        }
        else{
            strncpy(de.sne.name, "..         ", SHORT_NAME_LEN);
            de.sne.file_size = dp->parent->file_size;
            de.sne.fst_clus_hi = dp->parent->first_clus >> 16;
            de.sne.fst_clus_lo = dp->parent->first_clus &0xFFFF;
        }
        de.sne.attr = ATTR_DIRECTORY;
        off = reallocCurClus(dp, off, 1);
        if(rwClus(dp->cur_clus, 1, &de, off, 32)!=32)
            panic("emake error, rwClus inconsistent");
    }
    else{//Long entry by default
        uint32 lneCnt = (strlen(ep->filename) + LONG_NAME_LEN - 1 ) / LONG_NAME_LEN;    //Round up
        char shortName[SHORT_NAME_LEN+1];
        memset(shortName, 0, SHORT_NAME_LEN+1);
        generate_shortname(shortName, ep->filename);
        de.lne.attr = ATTR_LONG_NAME;
        de.lne.checksum = cal_checksum(shortName);
        for(int i=lneCnt;i>=1;i--){
            if((de.lne.order=i)==lneCnt)
                de.lne.order |= LAST_LONG_ENTRY;
            char *p = ep->filename + (i-1)*LONG_NAME_LEN;
            uint8 *name=de.lne.name1, end=0;
            for(int j=0;j<LONG_NAME_LEN;j++){
                if(j==5) name = de.lne.name2;
                if(j==11) name = de.lne.name3;
                if(end){
                    *name++ = 0xff;
                    *name++ = 0xff;
                }else{
                    *name++ = *p++;
                    *name++ = 0;
                }
                if(*(p-1) == '\0') end = 1;// Write an extra bit into the name
            }
            uint32 clusOff = reallocCurClus(dp, off, 1);
            rwClus(dp->cur_clus, 1, &de, clusOff, 32); 
            off += 32;
        }
        //Add a short name entry
        memset(&de, 0, 32);
        de.sne.attr = ep->attribute;
        de.sne.file_size = ep->file_size;
        strncpy(de.sne.name, shortName, SHORT_NAME_LEN);
        de.sne.fst_clus_hi = ep->first_clus >> 16;
        de.sne.fst_clus_lo = ep->first_clus & 0xFFFF;
        off = reallocCurClus(dp, off, 1);
        rwClus(dp->cur_clus, 1, &de, off, 32);
    }
}

/**
 * @brief 根据指定的name和attr，在指定的目录下分配一个entry并写到disk上
 * 
 * @param dp 该entry所在的目录
 * @param name 名称
 * @param attr 属性
 * @return 返回已经写入的entry
 */
struct dirent *ealloc(struct dirent *dp, char *name, int attr){
    if(!(dp->attribute&ATTR_DIRECTORY))
        panic("ealloc error, dp not dir!");
    if(!dp->uptodate)
        panic("ealloc error, dp not latest version!");
    if((name=formatname(name))==NULL)
        panic("ealloc error, name not valid!");
    
    struct dirent* ep;
    uint off = 0;
    if((ep=dirlookup(dp, name, &off))!=NULL){ //Already exist
        #ifdef DEBUG
        printf("[ealloc]ealloc exist, name:%s, first_clus:%d, clus_cnt:%d\n", ep->filename, ep->first_clus, ep->clus_cnt);
        #endif
        eput(ep);
        return NULL;
    }
    #ifdef DEBUG
    printf("[ealloc]offset:%x\n", off);
    #endif
    ep = eget(dp, name);
    acquiresleep(&ep->lock);
    ep->attribute = attr;
    ep->clus_cnt = 0;
    ep->cur_clus = 0;
    ep->dirty = 0;
    ep->file_size = 0;
    ep->first_clus = 0;
    ep->off = off;
    if(attr&ATTR_DIRECTORY){
        ep->first_clus = ep->cur_clus = allocClus(ep->dev);
        ep->clus_cnt = 1;
        emake(ep, NULL, 0);
        emake(ep, NULL, 32);
    }else ep->attribute |= ATTR_ARCHIVE;
    emake(dp, ep, off);

    liftEntry(ep);
    ep->dirty = 1;
    eupdate(ep); // Write updates into disk
    ep->uptodate = 1;
    releasesleep(&ep->lock);
    #ifdef DEBUG
    printf("[ealloc]ealloc success, name:%s, first_clus:%d, clus_cnt:%d\n", ep->filename, ep->first_clus, ep->clus_cnt);
    #endif

    return ep;
}

/**
 * @brief 将entry的内容更新到disk上
 * 
 * @param entry 指定的entry
 */
void eupdate(struct dirent *entry){
    if(!entry->dirty){
        #ifdef DEBUG
        printf("eupdate prompt, already updated!");
        #endif
        return;
    }
    uint8 lneCnt;
    uint32 off = reallocCurClus(entry->parent, entry->off, 0);
    rwClus(entry->parent->cur_clus, 0, &lneCnt, off, 1);
    lneCnt &= ~LAST_LONG_ENTRY;//Get count of long name entry
    union dentry de;
    memset(&de, 0, 32);
    off = reallocCurClus(entry->parent, entry->off+lneCnt*32, 0);// Only data in sne shall be updated
    rwClus(entry->parent->cur_clus, 0, &de, off, 32);
    de.sne.attr = entry->attribute;
    de.sne.file_size = entry->file_size;
    de.sne.fst_clus_hi = entry->first_clus >> 16;
    de.sne.fst_clus_lo = entry->first_clus & 0xFFFF;
    rwClus(entry->parent->cur_clus, 1, &de, off, 32);
    entry->dirty = 0;
}

/**
 * @brief 清空一个entry指定的文件
 */
void etrunc(struct dirent *entry){
    if (!entry->uptodate)
        panic("etrunc error, entry not latest version!");
    for (uint32 clus=entry->first_clus; clus>=2 && clus < FAT32_EOC;){
        uint32 next = readFAT(clus);
        freeClus(clus);
        clus = next;
    }
    entry->file_size = 0;
    entry->first_clus = 0;
    entry->clus_cnt = 0;
    entry->dirty = 1;
}

/**
 * @brief 释放文件的entry（文件本身没有删除）
 */
void eremove(struct dirent *entry){
    if (!entry->uptodate)
        panic("eremove error, entry not latest version!");

    uint8 entcnt = 0;
    uint32 off = entry->off;
    uint32 clusOff = reallocCurClus(entry->parent, off, 0);
    rwClus(entry->parent->cur_clus, 0, &entcnt, clusOff, 1);
    entcnt &= ~LAST_LONG_ENTRY;
    uint8 flag = DIR_ENTRY_FREE;
    for (int i = 0; i <= entcnt; i++) {
        rwClus(entry->parent->cur_clus, 1, &flag, clusOff, 1);
        off += 32;
        clusOff = reallocCurClus(entry->parent, off, 0);
    }
    entry->dirty = 0;
    memset(entry->filename, 0, FAT32_MAX_FILENAME+1); // In case mistaken as cache
}

static void read_long_name(char *filename, union dentry *de){
    char *name = de->lne.name1;
    uint8 end=0;
    for(int i=0;i<LONG_NAME_LEN;i++){
        if(i==5) name = de->lne.name2;
        if(i==11) name = de->lne.name3;
        if(*name == 0xff) end = 1;

        if(end){
            filename[i] = '\0';
        }
        else{
            filename[i] = *name++;
            name++;
        }
    }
}

static uint getClusCnt(uint32 firstClus){
    uint cnt = 0, next;
    if(firstClus == 0 || firstClus >= FAT32_EOC)
        return cnt;
    
    next = readFAT(firstClus);
    for(cnt=1;next<FAT32_EOC;cnt++){
        firstClus = next;
        next = readFAT(next);
    }
    return cnt;
}

/**
 * @brief 读取off后面的一个entry
 * 
 * @param dp 指定的父目录
 * @param ep info将被写入到ep中
 * @param off 偏移量
 * @param count 相关的数量
 * @return -1 读到dir结尾了；
 *         0  读到空entry，此时count为空entry的个数；
 *         1  读到了file entry，此时count是entry的数量（包括sne）
 */
int enext(struct dirent *dp, struct dirent *ep, uint off, int *count){
    if(!dp->attribute&ATTR_DIRECTORY)
        panic("enext error, not a directory!");
    if(!dp->uptodate)
        panic("enext error, dp not latest version!");
    if(ep->uptodate)
        panic("enext error, ep may be used by others!");
    if(off%32)
        panic("enext error, offset not aligned!");

    union dentry de;
    memset(ep->filename, 0, FAT32_MAX_FILENAME+1);
    uint emptyCnt=0, entryCnt=0;
    for(uint32 clusOff;(clusOff=reallocCurClus(dp, off, 0))!=-1;off+=32){
        if(rwClus(dp->cur_clus, 0, &de, clusOff, 32)!=32)
            panic("enext error, rwClus inconsistent!");
            
        if(de.lne.order == DIR_ENTRY_END)
            return -1;
             
        if(de.lne.order == DIR_ENTRY_FREE){
            emptyCnt++;
            continue;
        }
        else if(emptyCnt){
            *count = emptyCnt;
            return 0;
        }

        if(de.lne.attr == ATTR_LONG_NAME){
            uint order = de.lne.order & (~LAST_LONG_ENTRY);
            if(de.lne.order & LAST_LONG_ENTRY)
                entryCnt = order + 1;
            #ifdef DEBUG
            printf("Long Name Entry! order:%x, entryCnt:%d\n", de.lne.order, entryCnt);
            #endif
            read_long_name(ep->filename + (order-1)*LONG_NAME_LEN, &de);
        }
        else{
            #ifdef DEBUG
            printf("Short Name Entry! first cluser:%d\n", de.sne.fst_clus_hi<<16|de.sne.fst_clus_lo);
            #endif
            if(entryCnt==0){ // '.' or '..' dir
                entryCnt = 1;
                strncpy(ep->filename, de.sne.name, SHORT_NAME_LEN);
            }
            // Get relevent info
            ep->attribute = de.sne.attr;
            ep->file_size = de.sne.file_size;
            ep->first_clus = (uint32)(de.sne.fst_clus_hi << 16) | de.sne.fst_clus_lo;
            ep->cur_clus = ep->first_clus;
            ep->clus_cnt = 0;//Update in dirlookup
            *count = entryCnt;
            return 1;
        }
    }
    return -1;
}

/**
 * @brief 在指定的目录中寻找file entry，如果存在则返回；否则返回NULL，
 *        如果存在空余位置可以放下该entry，poff将是第一个empty entry的offset
 * @param dp 指定的父目录
 * @param filename 文件名
 * @param poff
 */
struct dirent *dirlookup(struct dirent *dp, char *filename, uint *poff){
    if(!(dp->attribute&ATTR_DIRECTORY))
        panic("dirlookup error, dp not a dir!");
    if(!dp->uptodate)
        panic("dirlookup error, dp not latest version!");
    
    if(strncmp(filename, ".", FAT32_MAX_FILENAME)==0)
        return edup(dp);
    if(strncmp(filename, "..", FAT32_MAX_FILENAME)==0)
        return edup(dp->parent);

    struct dirent* ep = eget(dp, filename);
    if(ep->uptodate){
        #ifdef DEBUG
        printf("dirlookup prompts, found in cache\n");
        #endif
        return ep;  // Found in cache
    }

    // Or look through the directory
    uint off = reallocCurClus(dp, 0, 0);
    uint count = 0;
    uint type = 0;
    uint entCnt = (strlen(filename) + LONG_NAME_LEN - 1) / LONG_NAME_LEN + 1;
    while((type=enext(dp, ep, off, &count))!=-1){
        #ifdef DEBUG
        printf("type:%d, filename:%s, epfilename:%s\n", type, filename, ep->filename);
        #endif
        if(type==0&&count>=entCnt&&poff){
            *poff = off;
            poff = 0;
        }
        else if(strncmp(filename, ep->filename, FAT32_MAX_FILENAME)==0){
            ep->clus_cnt = getClusCnt(ep->first_clus);
            ep->off = off;
            ep->uptodate = 1;
            return ep;
        }
        off += (count<<5);
    }
    if(poff) *poff = off;
    memset(ep->filename, 0, FAT32_MAX_FILENAME+1);// Not found, shall clear it's name
    eput(ep);
    return NULL;
}

static int pathNextName(char **path, char* name){
    memset(name, 0, FAT32_MAX_FILENAME+1);
    while ((**path)!='/'){
        *name++ = *(*path)++;
        if(**path == '\0') return 0;
    }
    (*path)++;
    return 1;
}

static int getPPath(char *path, char *name){
    memset(name, 0, FAT32_MAX_FILENAME+1);

    char *p, *end;
    p = end = path + strlen(path);
    while(*--p!='/'&&p>path);

    if(*p!='/'){ // CWD is the parent
        strcpy(name, path);
        return 0;
    }

    strcpy(name, p+1);
    if(p==path) p++; // Root is the parent

    while(p<end) *p++='\0';
    #ifdef DEBUG
    printf("[getPPath]ppath:%s\n", path);
    #endif
    return 1;
}

/**
 * @brief 根据给定路径返回file entry
 * 
 * @param path 路径
 */
struct dirent *ename(char *path){
    struct dirent *entry, *next;
    char name[FAT32_MAX_FILENAME+1];
    if (*path == '/'){
        entry = edup(&root);
        path++;
        if(*path == '\0')
            return entry;
    }
    else if (*path != '\0') 
        entry = edup(current->cwd);
    else return NULL;

    uint8 flag = 1;
    while(flag){
        flag = pathNextName(&path, name);
        #ifdef DEBUG
        printf("[ename]entry:%s, name:%s\n", entry->filename, name);
        #endif
        acquiresleep(&entry->lock);
        if(!(entry->attribute&ATTR_DIRECTORY)){ // Dir invalid
            #ifdef DEBUG
            printf("ename prompt, enter non-dir\n");
            #endif
            return NULL;
        }
        if((next=dirlookup(entry, name, 0))==NULL){ // File not exist
            #ifdef DEBUG
            printf("ename prompt, path invalid\n");
            #endif
            return NULL;
        }
        releasesleep(&entry->lock);
        eput(entry);
        entry = next;
    }
    return entry;
}

// name size shall be no less than 255
struct dirent* enameparent(char* path, char* name){
    if(getPPath(path, name)==0)
        return edup(current->cwd);

    return ename(path);
}

/**
 * @brief 获取entry所在的路径
 * 
 * @param buf 指定存储路径的缓冲区
 * @param size buffer的大小
 * @return 返回0表示成功，否则返回-1
 */
int epath(struct dirent* ep, char *buf, uint size){
    if(!ep || !buf || !size) return -1;

    char path[FAT32_MAX_PATH];
    memset(path, 0, FAT32_MAX_PATH);
    char *s=path+FAT32_MAX_PATH-1;
    uint len;
    if (ep->parent == ep){// Means Root Dir
        *s-- = '\0';
        *s = '/';
    }
    else{
        *s = '\0';
        while (ep->parent!=ep){
            len = strlen(ep->filename);
            s -= len;
            if (s <= path) // Path too long !
                return -1;
            strncpy(s, ep->filename, len);
            *--s = '/';
            ep = ep->parent;
        }
    }
    
    len = strlen(s);
    if(size <= len) return -1; // Buffer too small !

    strncpy(buf, s, len);
    return 0;
}

/**
 * @brief entry的长度(不是大小)
 */
uint elen(struct dirent* ep){
    return ep->clus_cnt*FAT.BytSPerClus;
}


void fat_test(){
    // struct buffer_head* bh = bread(0, first_sec_of_clus(2));
    // for(int i=1;i<=BSIZE;i++){
    //     printf("%x ", bh->buf[i-1]);
    //     if(i%16==0) printf("\n");
    // }
    // brelse(bh);

    // struct dirent *dp = edup(&root);
    // char name[FAT32_MAX_FILENAME] = "dirTest";
    // struct dirent *file = ealloc(dp, name, ATTR_DIRECTORY);
    
    // char test[100] = "this for test!";
    // ewrite(file, test, 0, 100);
    // eput(file);

    // struct dirent *file = ename("/createTest.txt");
    // eremove(file);
    // printf("%s %d %d\n", file->filename, file->first_clus, file->clus_cnt);
    // memset(test, 0, 100);
    // eread(file, test, 0, 100);
    // printf("test:%s\n", test);


    // char dirname[FAT32_MAX_FILENAME] = "testDir";
    // char filename[FAT32_MAX_FILENAME] = "testFile.txt";
    // struct dirent* dp = ealloc(&root, dirname, ATTR_DIRECTORY);
    // struct dirent* ep = ealloc(dp, filename, 0);

    // char fileContent[1024];
    // for(int i=0;i<509;i++) fileContent[i] = 'x';
    // fileContent[510] = 't';
    // fileContent[511] = 'e';
    // fileContent[512] = 's';
    // fileContent[513] = 't';
    // for(int i=514;i<1024;i++) fileContent[i] = 'y';

    // ewrite(ep, fileContent, 0, 1024);
    // printf("%s %d %d\n", ep->filename, ep->first_clus, ep->clus_cnt);

    // memset(fileContent, 0, 1024);
    // eread(ep, fileContent, 0, 1024);
    // printf("%s\n", fileContent);
    // eput(ep);

    printf("fat_test!");
    struct dirent* ep = ename("hello.bin");
    printf("%s, %d, %d", ep->filename, ep->first_clus, ep->clus_cnt);
    char buf[1000];
    eread(ep, buf, 0, 1000);
    for(int i=1;i<=1000;i++){
        printf("%x", buf[i-1]);
        if(i%16==0) printf("\n");
    }
    printf("[fat_test]done!\n");
    while(1);
}