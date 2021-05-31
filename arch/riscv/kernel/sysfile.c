#include "sysfile.h"
#include "sched.h"
#include "vm.h"
#include "put.h"
#include "string.h"
#include "spinlock.h"
#include "system.h"
#include "pipe.h"
// #define DEBUG

struct devsw devsw[NDEV];
struct file SysFTable[SYSOFILENUM];
struct spinlock SysFLock;

/**
 * @brief 创建管道
 * 
 * @param regs struct task的栈
 * @return 成功执行，返回0。失败，返回-1
 */
int sys_pipe2(uintptr_t *regs)
{
    int *fd;
    fd=(int*)regs[REG_A(0)];
    int ret=pipealloc(fd, fd+1);
    return ret;
}

void sysfile_init(){
    initlock(&SysFLock, "sysfile");
    for(struct file *file=SysFTable;file<SysFTable+SYSOFILENUM;file++){
        memset(file, 0, sizeof(struct file));
    }
    // STDIN
    SysFTable[0].f_type = DT_CHR;
    SysFTable[0].f_perm = READABLE;
    SysFTable[0].f_count = 1;
    SysFTable[0].major = CONSOLE;
    // STDOUT
    SysFTable[1].f_type = DT_CHR;
    SysFTable[1].f_perm = WRITABLE;
    SysFTable[1].f_count = 1;
    SysFTable[1].major = CONSOLE;
    // STDERR
    SysFTable[2].f_type = DT_CHR;
    SysFTable[2].f_perm = WRITABLE;
    SysFTable[2].f_count = 1;
    SysFTable[2].major = CONSOLE;
    #ifdef DEBUG
    printf("[sysfile]file init done!\n");
    #endif
}

void procfile_init(struct task_struct* task){
    task->cwd = ename("/");
	memset(task->FTable, 0, sizeof(struct file*)*PROCOFILENUM);
    for(int i=0;i<3;i++){
        task->FTable[i] = &SysFTable[i];
        SysFTable[i].f_count++;
    }
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
    if(file->f_count < 1) 
        panic("frelease error, tring to free freed file!");
    if(--file->f_count > 0){
        release(&SysFLock);
        return;
    }
    file->f_count = 0;
    if(file->f_type == DT_FIFO)
        pipeclose(file);
    else if(file->f_type & (DT_REG|DT_DIR))
        eput(file->f_entry);
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
            current->FTable[res] = 1;
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

    if(!(file->f_perm&READABLE))// Not readable
        return -1;
    int r;
    switch(file->f_type){
        case DT_REG:{
            acquiresleep(&file->f_entry->lock);
            r = eread(file->f_entry, dst, file->f_pos, num);
            if(r>0) file->f_pos += r; // Move file pointer
            releasesleep(&file->f_entry->lock);
            break;
        }
        case DT_FIFO:{
            r = piperead(file, dst, num);
            break;
        }
        case DT_CHR:{
            if(file->major < 0 || file->major >= NDEV ||!devsw[file->major].read)
                return -1;
            r = devsw[file->major].read(0, dst, num);
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

    if(!(file->f_perm&WRITABLE)) // Not writeable
        return -1;

    int w;
    switch (file->f_type){
        case DT_REG:{
            acquiresleep(&file->f_entry->lock);
            w = ewrite(file->f_entry, src, file->f_pos, num);
            if(w>0) file->f_pos+=w;
            releasesleep(&file->f_entry->lock);
            break;
        }
        case DT_FIFO:{
            w = pipewrite(file, src, num);
            break;
        }
        case DT_CHR:{
            if(file->major < 0 || file->major >= NDEV ||!devsw[file->major].write)
                return -1;
            w = devsw[file->major].write(0, src, num);
            break;
        }
        default:panic("fwrite error, unknow file type!");break;
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

    if (type == DT_DIR)
        attr = ATTR_DIRECTORY;
    else if (!(flags&(O_WRONLY|O_RDWR))) // Just readable
        attr = ATTR_READ_ONLY;

    acquiresleep(&dp->lock);
    if((ep = ealloc(dp, name, attr)) == NULL){
        releasesleep(&dp->lock);
        eput(dp);
        return NULL;
    }

    if ((type == DT_DIR && !(ep->attribute & ATTR_DIRECTORY)) ||
        (type == DT_REG && (ep->attribute & ATTR_DIRECTORY)))
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
    acquire(&SysFLock);
    file->f_pos = pos;
    release(&SysFLock);
}

/**
 * @brief 读取目录文件中的下一项
 * 
 * @param file 指定的目录文件
 * @param ep 读取到的entry将被在ep里
 * @return 读取成功则返回1，读到结尾返回0，否则失败，返回-1
 */
int dirnext(struct file *file, struct dirent* ep){
    if(!(file->f_perm&READABLE)|| !(file->f_entry->attribute&ATTR_DIRECTORY))
        return -1;

    struct kstat st;
    int count = 0;
    int ret;
    acquiresleep(&file->f_entry->lock);
    while ((ret = enext(file->f_entry, ep, file->f_pos, &count)) == 0) // Empty entry
        file->f_pos += count * 32;
    releasesleep(&file->f_entry->lock);
    if (ret == -1)// Dir end
        return 0;

    file->f_pos += count * 32;
    return 1;
}

int isdirempty(struct dirent *dp)
{
  struct dirent ep;
  int count;
  int ret = enext(dp, &ep, 64, &count);   // skip the "." and ".."
  return ret == -1;
}


/**
 * @brief 获取当前的工作目录
 * 
 * @param buf 指定存储目录路径的缓冲区，若为空则由os分配
 * @param size buffer的大小
 * @return 指向当前工作目录的字符串的指针；若失败，则返回NULL。
 */
char *sys_getcwd(uintptr_t *regs){
    char *buf=(char *)regs[REG_A(0)];
    uint size=(uint)regs[REG_A(1)];
    // Note, buf shall be allocated from it's heap rather than kmalloc
    if (!buf){
        if ((buf = (char *)kmalloc(size)) == NULL)
            return NULL;
    }
    memset(buf, 0 ,size);
    if(epath(current->cwd, buf, size)==-1)
        return NULL;
    return buf;
}

/**
 * @brief 复制文件描述符
 * 
 * @return 返回的新的描述符，若失败则返回-1
 */
int sys_dup(uintptr_t *regs){
    int fd=regs[REG_A(0)];
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
int sys_dup3(uintptr_t *regs){
    int old=regs[REG_A(0)];
    int new=regs[REG_A(1)];
    if(check_fd(new)<0) return -1;

    struct file* file = fdup(current->FTable[old]);
    if(!file) return -1;

    install_fd(new, file);
    return new;
}

int cd(const char *pathname){
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


int sys_chdir(uintptr_t *regs){
    const char *pathname=(const char *)regs[REG_A(0)];
    return cd(pathname);
}

/**
 * @brief 打开或创建一个文件
 * 
 * @param pathname 路径名称
 * @param flags 控制位
 * @param mode Unused
 * @return 打开或创建文件的文件描述符，如失败则返回-1
 */
int open(char *pathname, int flags, mode_t mode){
    int len = strlen(pathname);
    if(len<=0||len>FAT32_MAX_PATH)
        return -1;

    char path[FAT32_MAX_PATH];// For possible name changes
    strcpy(path, pathname);

    struct dirent* ep;
    struct file* file;
    int sfd, fd;

    if(flags&O_CREAT){   // Create file
        ep = fcreate(path, DT_REG, flags);
        if(!ep) return -1;// Create fails
    }else{
        ep = ename(path);
        if(!ep) return -1;// Open fails
    }

    acquiresleep(&ep->lock);
    if((sfd=falloc())==-1 || (fd=locate_fd())==-1){// No fd can use
        if(sfd!=-1) frelease(&SysFTable[sfd]);
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
    file->f_perm = 0;
    if(flags&O_WRONLY) file->f_perm = WRITABLE;
    file->f_perm |= READABLE;
    file->f_pos = (flags&O_APPEND) ? ep->file_size:0;
    file->f_type = (ep->attribute&ATTR_DIRECTORY) ? DT_DIR:DT_REG;
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
int sys_openat(uintptr_t *regs){
    int dirfd=regs[REG_A(0)];
    const char *pathname=(const char *)regs[REG_A(1)];
    int flags=regs[REG_A(2)];
    mode_t mode=(mode_t)regs[REG_A(3)];

    int res;
    if(*pathname!='/'&&dirfd!=AT_FDCWD){// Case 3
        struct dirent* ep = current->cwd;
        current->cwd = current->FTable[dirfd]->f_entry; // Quite dirty way I guess
        res = open(pathname, flags ,mode);
        current->cwd = ep;
    }else res = open(pathname, flags, mode);// Case 1 and 2

    return res;
}

/**
 * @brief 关闭一个文件描述符
 * 
 * @param fd 要关闭的文件描述符
 * @return 成功执行返回0，失败返回-1
 */
int sys_close(uintptr_t *regs){
    int fd=regs[REG_A(0)];
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
int sys_read(uintptr_t *regs){
    int fd=regs[REG_A(0)];
    char *buf=(char *)regs[REG_A(1)];
    int count=regs[REG_A(2)];
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
int sys_write(uintptr_t *regs){
    int fd=regs[REG_A(0)];
    char* buf=(char *)regs[REG_A(1)];
    int count=regs[REG_A(2)];
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
int mkdir(char *pathname, mode_t mode){
    int len = strlen(pathname);
    if(len<=0||len>FAT32_MAX_PATH)
        return -1;

    char path[FAT32_MAX_PATH];// For possible name changes
    strcpy(path, pathname);

    struct dirent* ep = fcreate(path, DT_DIR, 0);
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
int sys_mkdirat(uintptr_t *regs){
    int dirfd= regs[REG_A(0)];
    const char *pathname= (const char *)regs[REG_A(1)];
    mode_t mode=(mode_t)regs[REG_A(2)];
    int res;
    if(*pathname!='/'&&dirfd!=AT_FDCWD){// Case 3
        struct dirent* ep = current->cwd;
        current->cwd = current->FTable[dirfd]->f_entry;
        res = mkdir(pathname ,mode);
        current->cwd = ep;
    }else res = mkdir(pathname, mode);// Case 1 and 2

    return res;
}

/**
 * @brief 获取目录的条目
 * 
 * @param fd 所要读取目录的文件描述符
 * @param buf 用于存放目录信息的缓冲区
 * @param len buf大小
 * @return 成功执行，返回读取的字节数。当到目录结尾，则返回0。失败，则返回-1
 */
int sys_getdents64(uintptr_t *regs){
    int fd=regs[REG_A(0)];
    char *buf=(char *)regs[REG_A(1)];
    int len=regs[REG_A(2)];
    struct file* file = current->FTable[fd];
    if(!file || !buf )// Null file!
        return -1;

    memset(buf, 0, len);

    struct dirent ep;
    int ret = dirnext(file, &ep);
    if(ret<=0) return ret;

    struct linux_dirent64* ldr = (struct linux_dirent64*)buf;
    ldr->d_ino = ep.first_clus;
    ldr->d_off = ep.off;
    ldr->d_type = file->f_type;
    ldr->d_reclen = sizeof(struct linux_dirent64) + strlen(ep.filename);
    if(len<ldr->d_reclen)
        return -1;// Buffer too small;
    strcpy(ldr->d_name, ep.filename);

    return ldr->d_reclen;
}

/**
 * @brief 获取文件状态
 * 
 * @param fd 文件描述符
 * @param st 接受保存文件状态的指针
 * @return 成功返回0，失败返回-1
 */
int fstat(int fd, struct kstat* st){
    struct file* file = current->FTable[fd];
    if(!file) return -1; // Null file!

    st->st_dev = file->f_entry->dev;
    st->st_ino = file->f_entry->first_clus;
    st->st_mode = 0;
    st->st_nlink = 1;
    st->st_uid = 0;
    st->st_gid = 0;
    st->st_rdev = 0;
    st->st_size = file->f_entry->file_size;
    st->st_blksize = 512;
    st->st_blocks = file->f_entry->clus_cnt;
    st->st_atime_sec = 0;
    st->st_atime_nsec = 0;
    st->st_mtime_sec = 0;
    st->st_mtime_nsec = 0;
    st->st_ctime_sec = 0;
    st->st_ctime_nsec = 0;
    return 0;
}

int sys_fstat(uintptr_t *regs){
    int fd=regs[REG_A(0)];
    struct kstat *st=(struct kstat *)regs[REG_A(1)];
    return fstat(fd, st);
}

/**
 * @brief 删除文件
 * 
 * @param pathname 指定的文件路径
 * @return 成功则返回0，否则返回-1
 */
int rm(const char* pathname, int flag){
    if(strlen(pathname)<=0)
        return -1;

    char path[FAT32_MAX_PATH];
    strcpy(path, pathname);

    struct dirent *ep = ename(path);
    acquiresleep(&ep->lock);
    if(!ep || ep->attribute&ATTR_SYSTEM)
        return -1;
    if(ep->attribute&ATTR_DIRECTORY && flag!=AT_REMOVEDIR
        || !(ep->attribute&ATTR_DIRECTORY)&&flag==AT_REMOVEDIR)
        return -1;
    
    etrunc(ep);
    acquiresleep(&ep->parent->lock);
    eremove(ep);
    releasesleep(&ep->parent->lock);
    releasesleep(&ep->lock);
    eput(ep);
    return 0;
}

int sys_unlinkat(uintptr_t *regs){
    int dirfd=regs[REG_A(0)];
    const char* pathname=(const char *)regs[REG_A(1)];
    int flag=regs[REG_A(2)];
    int res;
    if(*pathname!='/'&&dirfd!=AT_FDCWD){// Case 3
        struct dirent* ep = current->cwd;
        current->cwd = current->FTable[dirfd]->f_entry;
        res = rm(pathname, flag);
        current->cwd = ep;
    }else res = rm(pathname, flag);// Case 1 and 2

    return res;
}

void sysfile_test(){

    printf("[sysfile_test]test done!\n");
    while(1);
}