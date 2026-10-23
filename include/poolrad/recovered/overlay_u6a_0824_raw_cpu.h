#ifndef POOLRAD_RECOVERED_OVERLAY_U6A_0824_RAW_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U6A_0824_RAW_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/overlay_u9e_entry16_10e4.h"

typedef pr_u9e_10e4_cpu pr_u6a_0824_raw_regs;
typedef enum {
    PR_U6A_0824_RAW_RETURNED = 0,
    PR_U6A_0824_RAW_ARGUMENT,   /* null/size/IP/SP outside the contract; nothing changed */
    PR_U6A_0824_RAW_LIMIT,      /* the host budget ran out at the next unexecuted instruction */
    PR_U6A_0824_RAW_TRANSFER    /* RETF popped a return address other than the entry frame's (the sort's own stores can reach it); the CPU is left at the popped CS:IP */
} pr_u6a_0824_raw_status;

/* GAME.OVR unit 0x6A [0824,08BF): 155 bytes / 57 instructions. Enter after the call at IP 0824. Frame: SP IP, SP+2 CS; no arguments (a plain RETF).
 * The far pointer G at DS:[5D92] (offset) and DS:[5D94] (segment) addresses an array of 21 bytes at G+17h..G+2Bh; the body sorts it into ascending order of the low seven bits with a
 * selection-exchange loop (i = 0..13h, j = i..14h, swap when the i-th low-seven-bit value is above the j-th), keeping the high bits with their bytes. G is RE-READ before every array access.
 * The array reads and writes go THROUGH A CALLER-SUPPLIED POINTER and so go through the bus; the loop counters i, j and the swap temporary live in the body's own frame (BP-1, BP-2, BP-3)
 * and an array placed over that frame changes the loop. Stack, code and the DS globals are flat RAM. Exactly 1 MiB of A20-off RAM; entry SP in [0008h,FFFCh]. Each instruction consumes one
 * unit of *budget; LIMIT leaves the next unexecuted instruction in cpu->ip. CPU, budget and bus storage are distinct. Code is immutable. */
pr_u6a_0824_raw_status pr_u6a_0824_raw_cpu(pr_bus *bus, size_t ram_size, pr_u6a_0824_raw_regs *cpu, size_t *budget);
#endif
