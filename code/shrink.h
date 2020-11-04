// short shellcode for memory free

#ifndef _SHRINK_H
#define _SHRINK_H

#include "icrt_mem.h"

void shrink_beg() {};

_syscall2(SYS_munmap,   _munmap_inline,    long,       char*, int)

static inline void free_inline(void * ptr)
{
    char * page = (char *)(ptr) - IXTY_SIZE_HDR;

    if(!ptr)
        return;

    _munmap_inline(page, IXTY_SIZE_ALLOC(ptr));
}

// malloc memory in remote process
void shrink_main(void *ptr)
{
    free_inline(ptr);
}

INJ_ENTRY(shrink_start, shrink_main)

void shrink_end() {};

#endif
