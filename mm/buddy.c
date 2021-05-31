#include "vm.h"
#include "riscv.h"
#include "memlayout.h"
struct buddy Buddy;
unsigned buddy_bitmap[BUDDY_BITMAP_SIZE];
// #define DEBUG
/**
 * @brief Initialize buddy system
 */
void init_buddy_system(void)
{
    Buddy.bitmap = buddy_bitmap;
    Buddy.size = (MEM_SIZE>>PAGE_SHIFT);

    #ifdef DEBUG
    printf("[init_buddy_system]Buddy.size:%x, _end:%p\n", Buddy.size, _end);
    #endif

    for (uint size = Buddy.size, num = 1, i = 0; size > 0; size /= 2, num *= 2)
        for (int j = 0; j < num; j++)
            Buddy.bitmap[i++] = size;
    alloc_pages(PAGENUM_ROUNDUP(KERNELSIZE));
    
    #ifdef DEBUG
    printf("[init_buddy_system]Buddy Init Down!\n", Buddy.size);
    #endif
}

/**
 * @brief align up to power of 2
 * @param x: number needed to align up.
 */
static inline uint64 next_pow_of_2(uint64 x){
	if(IS_POW_OF_2(x)) return x;
	x |= x>>1;
	x |= x>>2;
	x |= x>>4;
	x |= x>>8;
	x |= x>>16;
    x |= x>>32;
    return x+1;
}

static inline uint64 from_index_to_X(uint64 index){
    return (index + 1) * Buddy.bitmap[index] - Buddy.size;
}

static inline void* get_va_of_pageX(uint64 index){
    uintptr_t t = (from_index_to_X(index)<<PAGE_SHIFT) + SBI_HIGH_BASE;
    return (void *)t;
}

static inline uint64 get_X_of_va(void *va){
    uint64 t = *(uint64 *)&va;
    return (t-SBI_HIGH_BASE)>>PAGE_SHIFT;
}


/**
 * @brief This function will allocate int number of physical continuous memory space and return the VA start;
 *        If no matched frames, return null(0);
 * @param num: number of continuous physical frames needed.
 */
void *alloc_pages(int num)
{
    int search=0, size = next_pow_of_2(num), node_size;
    if(size<=0||size>Buddy.bitmap[0]) return 0;
    for(node_size = Buddy.size; node_size != size; node_size /= 2 ) {
        if(Buddy.bitmap[LEFT_CHILD(search)]>=size) search=LEFT_CHILD(search);
        else search = RIGHT_CHILD(search);
    }
    void* va = get_va_of_pageX(search);
    Buddy.bitmap[search] = 0;
    /* trace back and set parent's bitmap. */
    int trace = search;
    while (trace)
    {
        trace=PARENT(trace);
        Buddy.bitmap[trace] = ((Buddy.bitmap[LEFT_CHILD(trace)])>(Buddy.bitmap[RIGHT_CHILD(trace)]))?(Buddy.bitmap[LEFT_CHILD(trace)]):(Buddy.bitmap[RIGHT_CHILD(trace)]);
    }
    #ifdef DEBUG
    printf("[alloc_pages]size:%x, va:%p, pa:%p\n", size, va, K_VA2PA((uint64)va));
    #endif
    return va;
}

/**
 * @brief This function will free VA and merge it back;
 * @param VA: Virtual Address to be freed.
 */
void free_pages(void * VA){
    int index = get_X_of_va(VA)+Buddy.size-1;
    int currentSize = 1;
    while(Buddy.bitmap[index]) index=PARENT(index), currentSize*=2;
    Buddy.bitmap[index]=currentSize;

    /* trace back and set parent's bitmap */
    while(index){
        index = PARENT(index);
        if((Buddy.bitmap[LEFT_CHILD(index)]==currentSize)&&(Buddy.bitmap[RIGHT_CHILD(index)]==currentSize))
            Buddy.bitmap[index] = 2*currentSize;
        else Buddy.bitmap[index] = ((Buddy.bitmap[LEFT_CHILD(index)])>(Buddy.bitmap[RIGHT_CHILD(index)]))?(Buddy.bitmap[LEFT_CHILD(index)]):(Buddy.bitmap[RIGHT_CHILD(index)]);
        currentSize*=2;
    }
}