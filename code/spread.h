// short shellcode for memory malloc
// prepare for inject the entire program
// reduce the probability of problems with multi-threaded programs

#ifndef _SPREAD_H
#define _SPREAD_H

#include "icrt_mem.h"

void spread_beg() {};

_syscall3(SYS_mprotect, _mprotect_inline,  long,       void*, long, int)

#if __i386__ || __arm__
_syscall6(SYS_mmap2, _mmap_inline, void *, void *, long, int, int, int, long)
#else
_syscall6(SYS_mmap, _mmap_inline, void *, void *, long, int, int, int, long)
#endif

static inline void * malloc_inline(size_t size)
{
    size_t alloc_size;
    unsigned long mem;

    if(!size)
        return NULL;

    alloc_size = size + IXTY_SIZE_HDR;
    if(alloc_size % IXTY_PAGE_SIZE)
        alloc_size = ((alloc_size / IXTY_PAGE_SIZE) + 1) * IXTY_PAGE_SIZE;

    mem = (unsigned long)_mmap_inline(NULL, alloc_size, PROT_READ|PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    if(mem < 0)
    {
        printf("> memory allocation error (0x%x bytes)\n", alloc_size);
        return NULL;
    }

    mem += IXTY_SIZE_HDR;
    IXTY_SIZE_USER(mem) = size;
    IXTY_SIZE_ALLOC(mem) = alloc_size;

    return (void*)mem;
}

// malloc memory in remote process
void *spread_main(unsigned long size)
{
    if (!size)
        return NULL;

    void * mem = malloc_inline(size);

    if (!mem)
        return NULL;

    if(_mprotect_inline(mem - IXTY_SIZE_HDR, IXTY_SIZE_ALLOC(mem), PROT_READ | PROT_EXEC | PROT_WRITE) < 0)
        return NULL;

    return mem;
}

INJ_ENTRY(spread_start, spread_main)

void spread_end() {};

#endif
