#include "pipe.h"
#include "sysfile.h"
#include "spinlock.h"
#include "slub.h"
#include "sched.h"
#include "put.h"

extern struct file SysFTable[SYSOFILENUM];
extern struct spinlock SysFLock;

int pipealloc(int *sfd0, int *sfd1)
{
    struct pipe *pi;
    pi=NULL;
    if((*sfd0 = falloc()) == -1 || (*sfd1 = falloc()) == -1)
        goto bad;

    if((pi = (struct pipe*)kmalloc(sizeof(struct pipe))) == NULL)
        goto bad;

    pi->readopen = 1;
    pi->writeopen = 1;
    pi->nwrite = 0;
    pi->nread = 0;
    initlock(&pi->lock, "pipe");
    struct file *f0 = &SysFTable[*sfd0];
    struct file *f1 = &SysFTable[*sfd1];

    acquire(&SysFLock);
    f0->f_type = DT_FIFO;
    f0->f_perm = READABLE;
    f0->pipe = pi;
    f1->f_type = DT_FIFO;
    f1->f_perm = WRITABLE;
    f1->pipe = pi;
    release(&SysFLock);

    return 0;

    bad:
    if(pi) kfree((char*)pi);
    if(sfd0!=-1) frelease(&SysFTable[*sfd0]);
    if(sfd1!=-1) frelease(&SysFTable[*sfd1]);
    return -1;
}


int pipeclose(struct file *f)
{
    //struct file *f=current->FTable[fd];
    struct pipe *pi=f->pipe;
    acquire(&pi->lock);
    if(f->f_perm==WRITABLE)
    {
        pi->writeopen=0;
        wakeup(&pi->nread);
    }
    else if(f->f_perm==READABLE)
    {
        pi->readopen = 0;
        wakeup(&pi->nwrite);
    }
    else return -1;

    if(pi->readopen == 0 && pi->writeopen == 0)
    {
        release(&pi->lock);
        kfree((char*)pi);
    } 
    else
        release(&pi->lock);
    
    return 0;
}


int pipewrite(struct file *f, char *str, int n)
{
	int i;
    //struct file *f=current->FTable[fd];
    struct pipe *pi=f->pipe;
    

    acquire(&pi->lock);
    if(f->f_perm!=WRITABLE) 
    {
        release(&pi->lock);
        return -1;
    }

    if(pi->writeopen == 0) 
    {
        release(&pi->lock);
        return -2;
    }

    for(i = 0; i < n; i++){
        while(pi->nwrite == pi->nread + PIPESIZE){  //DOC: pipewrite-full
            if(pi->readopen == 0){
                release(&pi->lock);
                return -1;
            }
            wakeup(&pi->nread);
            sleep(&pi->nwrite, &pi->lock);
        }
        //if((uint64)str+i>=SZ) break;
        pi->data[pi->nwrite++ % PIPESIZE] = str[i];
    }
	printf("%d\n",pi->nwrite);
	
    wakeup(&pi->nread);
    release(&pi->lock);
    return i;
}


int piperead(struct file *f, char *str, int n)
{
    //struct file *f=current->FTable[fd];
    struct pipe *pi=f->pipe;
    int i;
    
    acquire(&pi->lock);

    if(f->f_perm!=READABLE) 
    {
        release(&pi->lock);
        return -1;
    }

    if(pi->readopen == 0) 
    {
        release(&pi->lock);
        return -2;
    }
	

    while(pi->nread == pi->nwrite && pi->writeopen){  //DOC: pipe-empty
        sleep(&pi->nread, &pi->lock); //DOC: piperead-sleep
    }


    for(i = 0; i < n; i++){  //DOC: piperead-copy

        if(pi->nread == pi->nwrite)
            break;

        //if(str+i>=SZ) break;
        str[i]=pi->data[pi->nread++ % PIPESIZE];
    }

    wakeup(&pi->nwrite);  //DOC: piperead-wakeup
    release(&pi->lock);
    return i;
}


void pipe_test()
{
    // int _pipe[2];
    // int ret=pipealloc(_pipe,_pipe+1);	
    // if(ret<0)
    // {
    //     printf("pipe error\n");
    // }

    // char mesg1[100];
    // char *mesg2;
    // mesg2="ABCDEF";
    // //pipeclose(&SysFTable[_pipe[1]]);
    // printf("1:%d\n",pipewrite(&SysFTable[_pipe[1]],mesg2,7));
    // printf("2:%d\n",piperead(&SysFTable[_pipe[0]],mesg1,7));

	// printf("%s\n",mesg1);

    // while(1);
}

