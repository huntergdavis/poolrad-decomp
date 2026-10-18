#ifndef POOLRAD_RECOVERED_OVERLAY_U25_ALLOC_1951_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U25_ALLOC_1951_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U25_1951_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u25_1951_cpu;

typedef enum {
    PR_U25_1951_NO_CALLEE = -1,
    PR_U25_1951_GETMEM = 0,     /* SYSTEM GetMem 023F, the merged pr_system_getmem_023f */
    PR_U25_1951_WORD_06AF       /* U25 +06AF, the merged pr_u25_word_06af */
} pr_u25_1951_callee;

typedef enum {
    PR_U25_1951_OK = 0,
    PR_U25_1951_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 1951h; nothing changed */
    PR_U25_1951_CODE,      /* a reached GetMem LCALL is not 9A 3F 02; stopped at the site, nothing pushed */
    PR_U25_1951_CALLEE,    /* GetMem stopped (RunError 203, a custom HeapError, its walk cap, a return guard) or +06AF
                              rejected its entry; *callee_status holds that status */
    PR_U25_1951_TRANSFER   /* GetMem's RETF 2 or +06AF's RETF 4 popped a CS:IP other than the call's next instruction */
} pr_u25_1951_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x25 routine
 * +1951..+19DA (138 bytes, 47 instructions), entered after a far call (or a
 * near CALL preceded by PUSH CS): SS:[SP] return IP, [SP+2] return CS, then
 * an index byte at [SP+4]; RETF 2. It pushes a 6-byte node onto the list
 * whose far head is DS:6F70h/6F72h. After PUSH BP; MOV BP,SP:
 *  1. If the head is nil, head := GetMem(6) and its link (+2/+4) := nil;
 *     otherwise the old head is kept at DS:6F74h/6F76h, head := GetMem(6)
 *     and its link := the old head.
 *  2. The node's word +0 := DS:[49EDh].
 *  3. DS:[49EDh] := +06AF of the bytes DS:[7085h + index] and DS:[7045h +
 *     index], which is (DS:[7045h + index] << 8) + DS:[7085h + index].
 *  4. POP BP; RETF 2.
 * GetMem (LCALL xxxx:023F, its segment read from RAM) is the merged
 * pr_system_getmem_023f with max_heap_steps for its free-list walk; it keeps
 * its own contract (its default HeapError is xxxx:00A9; a failed allocation
 * stops at RunError with AX = 203, which this routine reports as CALLEE).
 * +06AF (PUSH CS; CALL near) is the merged pr_u25_word_06af and needs its
 * original offset, so the entry must be +1951. Every operand is reread from
 * RAM in source order, so the list words, the node and the tables may
 * alias. Flags: the final flags are +06AF's (its ADD); at a GetMem site they
 * are those of OR AX,[6F72h]; after a callee they are the callee's.
 * Offsets wrap at 16 bits, physical addresses at 1 MiB; LES pairs, POP and
 * this routine's RETF frame are read physically contiguous. *callee and
 * *callee_status (either may be NULL) receive the last callee run and its
 * status, or NO_CALLEE and -1. */
pr_u25_1951_status pr_u25_alloc_1951_cpu(uint8_t *ram, size_t ram_size, pr_u25_1951_cpu *cpu, unsigned max_heap_steps,
                                         pr_u25_1951_callee *callee, int *callee_status);

#endif
