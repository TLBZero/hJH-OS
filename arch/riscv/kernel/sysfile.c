#include "sysfile.h"
#include "sched.h"
#include "vm.h"
#include "put.h"
#include "string.h"
#include "spinlock.h"
// #define DEBUG
extern struct dirent root;
struct file SysFTable[SYSOFILENUM];
struct spinlock SysFLock;

void sysfile_init(){
    initlock(&SysFLock, "sysfile");
    for(struct file *file=SysFTable;file<SysFTable+SYSOFILENUM;file++)
        memset(file, 0, sizeof(struct file));
    #ifdef DEBUG
    printf("[sysfile]file init done!\n");
    #endif
}

/**
 * @brief 分配一个空闲的file struct
 * 
 * @return 返回该空闲file的fd，如果失败则返回-1
 */
uint falloc(){
    acquire(&SysFLock);
    for(int sfd=0;sfd<SYSOFILENUM;sfd++)
        if(SysFTable[sfd].f_count == 0){
            SysFTable[sfd].f_count = 1;
            release(&SysFLock);
            return sfd;
        }
    release(&SysFLock);
    return -1;
}

/**
 * @brief 释放指定的file struct
 */
void frelease(struct file* file){
    if(!file) return;
    
    acquire(&SysFLock);
    if(file->f_count == 0) 
        panic("frelease error, tring to free freed file!");
    if(--file->f_count == 0){
        eput(file->f_entry);
        file->f_entry = NULL;
    }
    release(&SysFLock);
}

/**
 * @brief 分配一个fd
 * @return 返回分配的fd，如失败则返回-1
 */
int locate_fd(){
    int res = 0;
    acquire(&current->lk);
    for(;res<PROCOFILENUM;res++)
        if(current->FTable[res]==NULL){
            release(&current->lk);
            return res;
        }
    release(&current->lk);
    return -1;
}

/**
 * @brief 检查fd是否被占用
 * @return 如果没有被占用则返回fd，否则返回-1
 */
int check_fd(int fd){
    acquire(&current->lk);
    if(current->FTable[fd]==NULL){
        release(&current->lk);
        return fd;
    }
    release(&current->lk);
    return -1;
}

void install_fd(int fd, struct file* file){
    acquire(&current->lk);
    current->FTable[fd] = file;
    release(&current->lk);
}

void remove_fd(int fd){
    acquire(&current->lk);
    current->FTable[fd] = NULL;
    release(&current->lk);
}

/**
 * @brief 从文件指针处读取文件内容
 * 
 * @param file 指定的文件
 * @param dst 读取内容存储的目标地址
 * @param num 读取的字符数
 * @return 成功读取的字符数
 */
int fread(struct file* file, void* dst, int num){
    if(!file) panic("fread error, read null file!");

    if(file->f_perm&O_WRONLY)// Not readable
        return -1;
    int r;
    switch(file->f_type){
        case REG:{
            acquiresleep(&file->f_entry->lock);
            r = eread(file->f_entry, dst, file->f_pos, num);
            if(r>0) file->f_pos += r; // Move file pointer
            releasesleep(&file->f_entry->lock);
            break;
        }
        default:panic("fread error, unknow file type!");break;
    }
    return r;
}

/**
 * @brief 从文件指针处写入文件内容
 * 
 * @param file 指定的文件
 * @param src 要写入内容的地址
 * @param num 写入的字符数
 * @return 成功写入的字符数
 */
int fwrite(struct file* file, void* src, int num){
    if(!file) panic("fwrite error, write null file!");

    if(!(file->f_perm&(O_WRONLY|O_RDWR))) // Not writeable
        return -1;

    int w;
    switch (file->f_type){
        case REG:{
            acquiresleep(&file->f_entry->lock);
            w = ewrite(file->f_entry, src, file->f_pos, num);
            if(w>0) file->f_pos+=w;
            releasesleep(&file->f_entry->lock);
        }
    }
    return w;
}

/**
 * @brief 新建一个文件或目录
 * 
 * @param pathname 绝对路径与相对路径皆可
 * @param type REG 或 DIR
 * @param flags 
 * @return struct dirent* 
 */
struct dirent* fcreate(char *pathname, uint type, uint flags){
    struct dirent *ep, *dp;
    char name[FAT32_MAX_FILENAME + 1];
    int attr = 0;

    // Path error
    if((dp=enameparent(pathname, name)) == NULL)
        return NULL;

    if (type == C_DIR)
        attr = ATTR_DIRECTORY;
    else if (flags & O_RDONLY) // But O_RDONLY is always 0
        attr = ATTR_READ_ONLY;

    acquiresleep(&dp->lock);
    if((ep = ealloc(dp, name, attr)) == NULL){
        releasesleep(&dp->lock);
        eput(dp);
        return NULL;
    }

    if ((type == C_DIR && !(ep->attribute & ATTR_DIRECTORY)) ||
        (type == C_REG && (ep->attribute & ATTR_DIRECTORY)))
            panic("fcreate error!");// Some strange error

    releasesleep(&dp->lock);
    eput(dp);
    return ep;
}

struct file* fdup(struct file* file){
    if(!file) return NULL;
    acquire(&SysFLock);
    file->f_count++;
    release(&SysFLock);

    return file;
}

void fseek(struct file* file, uint pos){
    file->f_pos = pos;
}

/**
 * @brief 获取当前的工作目录
 * 
 * @param buf 指定存储目录路径的缓冲区，若为空则由os分配
 * @param size buffer的大小
 * @return 指向当前工作目录的字符串的指针；若失败，则返回NULL。
 */
char *sys_getcwd(char *buf, uint size)
{
    // Note, buf shall be allocated from it's heap rather than kmalloc
    if (!buf){
        if ((buf = (char *)kmalloc(size)) == NULL)
            return NULL;
    }
    memset(buf, 0 ,size);
    if(epath(current->cwd, buf, size)==-1) return NULL;
    return buf;
}

/**
 * @brief 复制文件描述符
 * 
 * @return 返回的新的描述符，若失败则返回-1
 */
int sys_dup(int fd){
    int nfd = locate_fd();
    if(nfd<0) return -1; // Fails
    fdup(current->FTable[fd]);
    
    return nfd;
}

/**
 * @brief 同dup，但指定了新的文件描述符
 * 
 * @return 返回的新的描述符，若失败则返回-1
 */
int sys_dup3(int old, int new){
    if(check_fd(new)<0) return -1;

    struct file* file = fdup(current->FTable[old]);
    if(!file) return -1;

    install_fd(new, file);
    return new;
}

int sys_chdir(const char *pathname){
    int len = strlen(pathname);
    if(len<=0||len>FAT32_MAX_PATH)
        return -1;// path error
    
    char path[FAT32_MAX_PATH];
    strcpy(path, pathname);

    struct dirent* ep;
    if((ep=ename(path))==NULL)
        return -1;
    
    acquiresleep(&ep->lock);
    if(!(ep->attribute&ATTR_DIRECTORY)){
        releasesleep(&ep->lock);
        eput(ep);
        return -1;
    }

    releasesleep(&ep->lock);
    eput(current->cwd);
    current->cwd = ep;  //Change dir! Just so easy!
    return 0;
}

/**
 * @brief 打开或创建一个文件
 * 
 * @param pathname 路径名称
 * @param flags 控制位
 * @param mode Unused
 * @return 打开或创建文件的文件描述符，如失败则返回-1
 */
int sys_open(char *pathname, int flags, mode_t mode){
    int len = strlen(pathname);
    if(len<=0||len>FAT32_MAX_PATH)
        return -1;

    char path[FAT32_MAX_PATH];// For possible name changes
    strcpy(path, pathname);

    struct dirent* ep;
    struct file* file;
    int sfd, fd;

    if(flags&O_CREAT){   // Create file
        ep = fcreate(path, C_REG, flags);
        if(!ep) return -1;// Create fails
    }else{
        ep = ename(path);
        if(!ep) return -1;// Open fails
    }

    acquiresleep(&ep->lock);
    if((sfd=falloc())==-1 || (fd=locate_fd())==-1){// No fd can use
        if(sfd!=-1) frelease(sfd);
        releasesleep(&ep->lock);
        eput(ep);
        return -1;
    }

    if(!(ep->attribute&ATTR_DIRECTORY)&&(flags&O_TRUNC))
        etrunc(ep);
    releasesleep(&ep->lock);

    acquire(&SysFLock);
    file = &SysFTable[sfd];
    file->f_count = 1;
    file->f_entry = ep;
    file->f_perm = flags;
    file->f_pos = (flags&O_APPEND) ? ep->file_size:0;
    file->f_type = (ep->attribute&ATTR_DIRECTORY) ? DIR : REG;
    release(&SysFLock);
    install_fd(fd, file);

    return fd;
}

/**
 * @brief 打开或创建一个文件
 * 
 * @param dirfd 文件所在目录的文件描述符
 * @param filename 要打开或创建的文件名。
 *                 如为绝对路径，则忽略dirfd。
 *                 如为相对路径，且dirfd是AT_FDCWD，则filename是相对于当前工作目录来说的。
 *                 如为相对路径，且dirfd是一个文件描述符，则filename是相对于dirfd所指向的目录来说的。
 * @param flags 控制位
 * @param mode Unused
 * @return 打开或创建文件的文件描述符，如失败则返回-1
 */
int sys_openat(int dirfd, const char *pathname, int flags, mode_t mode){
    int res;
    if(*pathname!='/'&&dirfd!=AT_FDCWD){// Case 3
        struct dirent* ep = current->cwd;
        current->cwd = current->FTable[dirfd]->f_entry; // Quite dirty way I guess
        res = sys_open(pathname, flags ,mode);
        current->cwd = ep;
    }else res = sys_open(pathname, flags, mode);// Case 1 and 2

    return res;
}

/**
 * @brief 关闭一个文件描述符
 * 
 * @param fd 要关闭的文件描述符
 * @return 成功执行返回0，失败返回-1
 */
int sys_close(int fd){
    struct file* file = current->FTable[fd];
    if(!file) return -1;// Null file!

    remove_fd(fd);
    frelease(file);
    return 0;
}

/**
 * @brief 从一个文件描述符中读取
 * 
 * @param fd 要读取的文件描述符
 * @param buf 存放内容的缓冲区
 * @param count 要读取的字节数
 * @return 成功执行，返回读取的字节数。如为0，表示文件结束。错误，则返回-1
 */
int sys_read(int fd, char *buf, int count){
    struct file* file = current->FTable[fd];
    return fread(file, buf, count);
}

/**
 * @brief 从一个文件描述符中写入
 * 
 * @param fd 要写入的文件描述符
 * @param buf 存放写入内容的缓冲区
 * @param count 要写入的字节数
 * @return 成功执行，返回写入的字节数。错误，则返回-1
 */
int sys_write(int fd, char* buf, int count){
    struct file* file = current->FTable[fd];
    return fwrite(file, buf, count);
}

/**
 * @brief 创建一个目录
 * 
 * @param pathname 目录路径，绝对路径和相对路径皆可
 * @param mode Unused
 * @return 创建成功则返回0，否则返回-1
 */
int sys_mkdir(char *pathname, mode_t mode){
    int len = strlen(pathname);
    if(len<=0||len>FAT32_MAX_PATH)
        return -1;

    char path[FAT32_MAX_PATH];// For possible name changes
    strcpy(path, pathname);

    struct dirent* ep = fcreate(path, C_DIR, 0);
    if(!ep) return -1;

    eput(ep);
    return 0;
}

/**
 * @brief 创建目录
 * 
 * @param dirfd 目录所在位置的文件描述符
 * @param pathname 要创建的目录名
 *                 如为绝对路径，则忽略dirfd。
 *                 如为相对路径，且dirfd是AT_FDCWD，则pathname是相对于当前工作目录来说的。
 *                 如为相对路径，且dirfd是一个文件描述符，则pathname是相对于dirfd所指向的目录来说的。
 * @param mode Unused
 * @return 成功执行返回0。失败返回-1
 */
int sys_mkdirat(int dirfd, const char *pathname, mode_t mode){
    int res;
    if(*pathname!='/'&&dirfd!=AT_FDCWD){// Case 3
        struct dirent* ep = current->cwd;
        current->cwd = current->FTable[dirfd]->f_entry;
        res = sys_mkdir(pathname ,mode);
        current->cwd = ep;
    }else res = sys_mkdir(pathname, mode);// Case 1 and 2

    return res;
}

void sysfile_test(){
    char cwd[FAT32_MAX_PATH];
    char write[512]="fwrite test string!";
    char read[512]={0};
    sys_getcwd(cwd, FAT32_MAX_PATH);
    printf("%s\n", cwd);

    sys_mkdir("/testdir", 0);
    sys_chdir("/testdir");
    sys_getcwd(cwd, FAT32_MAX_PATH);
    printf("%s\n", cwd);

    int fd = sys_open("testfile.txt", O_CREAT|O_RDWR, 0);
    printf("fd:%d\n", fd);
    struct file* file = current->FTable[fd];
    printf("file ocnt:%d\n",file->f_count);
    
    fwrite(file, write, 30);
    file->f_pos = 10;
    fwrite(file, write, 30);
    file->f_pos = 0;
    fread(file, read, 30);
    printf("read:%s\n", read);

    sys_close(fd);

    printf("[sysfile_test]test done!\n");
    while(1);
}