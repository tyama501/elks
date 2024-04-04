/*
 * ELKS shared instrumentation functions for ia16-elf-gcc
 *
 * June 2022 Greg Haerr
 * Mar  2024 Added backwards scan w/o symbol table using _get_fn_start_address
 */
#include <stdio.h>
#include "instrument.h"
#include "syms.h"

#define _get_csbyte(ip) __extension__ ({        \
        unsigned char _v;                       \
        asm volatile ("mov %%cs:(%%bx),%%al"    \
            :"=Ral" (_v)                        \
            :"b" (ip));                         \
        _v; })

/*
 * Calculate function start address containing addr.
 * Proper operation requires compilation with -fno-omit-frame-pointer
 * and -fno-optimize-sibling-calls.
 */
int * noinstrument _get_fn_start_address(int *addr)
{
    char *ip = (char *)addr;
    int i = 0;

    /* adjust forward if addr is function start (hack) */
    if (_get_csbyte(ip) == 0x56) {      /* push %si */
        i = -1;
        if (_get_csbyte(ip+1) == 0x57)  /* push %di */
            i = -2;
    }

    /* look backwards for prologue: push %bp/mov %sp,%bp (55 89 e5) */
    for(;;) {
        /* main called at 0x37 from crt0.S which has no prologue at _start */
        if ((unsigned int)ip-i <= 0x37)
            return 0;                   /* _start address or start address not found */

        if (_get_csbyte(ip-i+0) == 0x55 &&          /* push %bp */
            _get_csbyte(ip-i+1) == 0x89 &&          /* mov %sp,%bp */
            _get_csbyte(ip-i+2) == 0xe5) {
                ip = ip - i;
                /* prologue possibly prededed by optional push %si/%di */
                if ((i = _get_csbyte(ip-1)) == 0x57) /* push %di */
                    return (int *)(ip-2);           /* start is push %si,push %di */
                if (i == 0x56)                      /* push %si */
                    return (int *)(ip-1);           /* start is push %si */
                return (int *)ip;                   /* start is push %bp */
        }
        i++;
    }
}

/*
 * Return pushed word count and register bitmask by function at passed address,
 * used to traverse BP chain and display registers.
 */
int noinstrument _get_push_count(int *fnstart)
{
    char *fp = (char *)fnstart;
    int count = 0;

    int opcode = _get_csbyte(fp++);
    if (opcode == 0x56)         /* push %si */
        count = (count+1) | SI_PUSHED, opcode = _get_csbyte(fp++);
    if (opcode == 0x57)         /* push %di */
        count = (count+1) | DI_PUSHED, opcode = _get_csbyte(fp++);
    if (opcode == 0x55 ||       /* push %bp */
       (opcode == 0x59 && (unsigned int)fp < 0x40)) /* hack for crt0.S 'pop %cx' start */
        count = (count + 1) | BP_PUSHED, opcode = _get_csbyte(fp);
    //printf("%s (%x) pushes %x\n", sym_text_symbol(addr, 1), (int)addr, count);
    return count;
}
