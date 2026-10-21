#ifndef POOLRAD_RECOVERED_OVERLAY_U20_LIST_WALK_11AC_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U20_LIST_WALK_11AC_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/system_freemem_0254_cpu.h"
#include "poolrad/recovered/overlay_u6c_record_unlink_3291_cpu.h"
#define PR_U20_11AC_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_u20_11ac_cpu;
typedef struct {
    unsigned max_walk_loopbacks;  /* taken 1293 -> 11CA edges */
    unsigned max_callee_steps;    /* passed to FreeMem's max_steps */
    pr_u6c_3291_limits unlink;    /* passed unchanged to the +3291 closure */
} pr_u20_11ac_limits;
typedef enum {
    PR_U20_11AC_NO_CALLEE = -1,
    PR_U20_11AC_UNLINK = 0,   /* U6C +3291 through the 0078:002A stub */
    PR_U20_11AC_FREEMEM       /* SYSTEM 0AF8:0254 */
} pr_u20_11ac_callee;
typedef struct {
    pr_u20_11ac_callee callee;     /* last callee reached, or NO_CALLEE */
    int callee_status;             /* its status; -1 before any call */
    pr_u6c_3291_diagnostic unlink; /* latest +3291 invocation */
    pr_system_freemem_status freemem_status; /* latest FreeMem status */
    unsigned unlink_calls, freemem_calls;
    uint16_t call_site;            /* last reached call site; zero before any */
} pr_u20_11ac_diagnostic;
typedef enum {
    PR_U20_11AC_OK = 0,
    PR_U20_11AC_ARGUMENT,  /* bad host object or profile violation; nothing changed */
    PR_U20_11AC_CODE,      /* reached call opcode/operand malformed (stopped at the
                              site, nothing pushed) */
    PR_U20_11AC_CALLEE,    /* a callee stopped; diagnostic names it and holds its
                              status; effects kept */
    PR_U20_11AC_TRANSFER,  /* a callee's RETF popped a CS:IP other than the call's
                              next instruction */
    PR_U20_11AC_LIMIT      /* walk budget: IP at 11CA with every earlier effect kept */
} pr_u20_11ac_status;
/* Complete GAME.OVR U20 [11AC,12A8), 252 bytes / 77 instructions, final RETF at
 * 12A7 (the queue row's 316 bytes includes 64 trailing data bytes; see
 * docs/re/U20_LIST_WALK_11AC_CPU.md).
 *
 * Behaviour: clears the word at window(+590h) where window is the far pointer
 * at DS:49D6, then walks the doubly-linked list whose head is DS:5D96/5D98.
 * For each node (reached through the far next link at node+108h):
 *  - if node+13h == 1, or node+10Eh == 1:
 *      - if node+10Dh != 1, increment window(+590h);
 *      - activate the node (DS:5D92/5D94 := node), remember node+104h/+106h in
 *        BP-8/BP-6, and LCALL 0078:002A (the loaded stub jumps to U6C +3291)
 *        with B6 = 1 and B8 = (node's fresh next+13h == 1); continue at the
 *        remembered node+104h;
 *  - otherwise, if either half of node+108h is nonzero, FreeMem(offset
 *    node+108h, segment node+10Ah, size 16h) and zero both halves; then
 *    continue at node+104h/+106h.
 * The walk ends when the current far pointer is nil; DS:5D92/5D94 are then set
 * to the head, and the routine returns with RETF.
 *
 * Raw CPU14, exactly 1 MiB A20-off RAM, entry IP=11AC, SP in [0058,FFF8].
 * Code immutable; the 3291 and FreeMem contracts apply to the reached callees
 * (resident and SYSTEM segments read live or passed in). CPU, limits and
 * diagnostic are distinct host objects outside RAM. Operands are reread from
 * RAM in source order, so list nodes, globals and frame may alias; LES, words
 * and the RETF frame are physically contiguous. Reached call opcodes/operands
 * are checked. Flags follow DOSBox-X normal/386; at a stop they are those the
 * original leaves. A callee that stops keeps its state (CALLEE); an unexpected
 * callee return is TRANSFER; no rollback. resident_cs is the segment of the
 * 3291 closure's resident dependency and system_cs the SYSTEM FreeMem segment;
 * both must satisfy those contracts or the callee reports ARGUMENT, surfaced
 * here as CALLEE. A null diagnostic is allowed. */
pr_u20_11ac_status pr_u20_list_walk_11ac_cpu(uint8_t *ram,size_t ram_size,
    pr_u20_11ac_cpu *cpu,uint16_t resident_cs,uint16_t system_cs,
    const pr_u20_11ac_limits *limits,pr_u20_11ac_diagnostic *diagnostic);
#endif
