#ifndef POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_UNLINK_3291_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U6C_RECORD_UNLINK_3291_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/overlay_u3ce_bitmap_pair_release_014e_cpu.h"
#include "poolrad/recovered/overlay_u6c_record_release_0000_cpu.h"
#define PR_U6C_3291_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags;
} pr_u6c_3291_cpu;
typedef struct {
    unsigned max_walk_loopbacks;  /* taken JMP +006F->0034 edges */
    unsigned max_list_loopbacks;  /* +0000's shared list budget, per invocation */
    unsigned max_heap_steps;      /* FreeMem heap-walk budget, per FreeMem invocation */
} pr_u6c_3291_limits;
typedef enum {
    PR_U6C_3291_NO_CALLEE = -1,
    PR_U6C_3291_PAIR_RELEASE = 0,    /* U3CE +014E through the 03E9:002F stub */
    PR_U6C_3291_RECORD_RELEASE       /* U6C +0000 */
} pr_u6c_3291_callee;
typedef struct {
    pr_u6c_3291_callee callee;       /* last callee reached, or NO_CALLEE */
    int callee_status;               /* its status; -1 before any call */
    pr_u3ce_014e_diagnostic pair;    /* latest +014E invocation */
    pr_u6c_0000_diagnostic record;   /* latest +0000 invocation */
    unsigned pair_calls, record_calls;
    uint16_t call_site;              /* last reached call site; zero before any */
} pr_u6c_3291_diagnostic;
typedef enum {
    PR_U6C_3291_OK = 0,
    PR_U6C_3291_ARGUMENT,  /* bad host object or profile violation; nothing changed */
    PR_U6C_3291_CODE,      /* reached call opcode/operand malformed (stopped at the site, nothing pushed), or
                              the 03E9:002F stub is not a loaded EA jump to +014E (stopped at the stub, after the
                              call's pushes: a cold INT 3Fh stub included) */
    PR_U6C_3291_CALLEE,    /* a callee stopped; diagnostic names it and holds its status; effects kept */
    PR_U6C_3291_TRANSFER,  /* a callee's RETF popped a CS:IP other than the call's next instruction */
    PR_U6C_3291_LIMIT      /* walk budget: IP at +0034 with every earlier effect kept */
} pr_u6c_3291_status;
/* Complete GAME.OVR U6C [3291,33D7), 326 bytes / 95 instructions, RETF 4
 * (the queue row's 342 bytes includes 16 trailing data bytes). Entered by a
 * far call: [SP] return IP, [SP+2] return CS, [SP+4] word whose low byte is
 * B6 (SS:BP+6), [SP+6] word whose low byte is B8 (SS:BP+8); only the low
 * bytes are read. The DS
 * globals are the list head (5D96/5D98), the active record (5D92/5D94) and
 * the window pointer (49D6): each record's next link is at +104h/+106h, its
 * slot byte at +0BFh. Unlinks the active record and releases it:
 *  - head nil: return. head == active: head := active.next; otherwise walk
 *    from the head to the node whose next == active (nil next first: return);
 *    prev.next := active.next.
 *  - if B6, LCALL 03E9:002F with the active record's +0BFh byte (the loaded
 *    stub EA jumps to U3CE +014E, pr_u3ce_bitmap_pair_release_014e_cpu);
 *    then PUSH CS / CALL +0000 on the active record
 *    (pr_u6c_record_release_0000_cpu); active := the node saved in BP-4 (the
 *    predecessor in the walk case, the old next link in the head case); if
 *    !B8, DEC WORD [window+67Ch]. B6 only gates the LCALL.
 * Raw CPU14, exactly 1 MiB A20-off RAM, entry IP=3291, SP in [008C,FFF8].
 * Code immutable; the 0000 and 014E closures' own contracts apply to the
 * reached callees (resident and SYSTEM segments read live or passed in).
 * CPU, limits and diagnostic are distinct host objects outside RAM. Operands
 * are reread from RAM in source order, so records, globals, frame and heap
 * may alias; LES, words and the RETF frame are physically contiguous. Reached
 * call opcodes/operands are checked; the stub is read after the pushes.
 * Flags follow DOSBox-X normal/386 (OR/XOR/CMP/DEC AF as the oracle); at a
 * stop they are those the original leaves. A callee that stops keeps its
 * state (CALLEE); an unexpected callee return is TRANSFER; no rollback.
 * resident_cs is the segment of RES 0464 (+2595) and system_cs the SYSTEM
 * FreeMem segment of the +014E closure; both must satisfy +014E's contract
 * or +014E reports ARGUMENT, surfaced as CALLEE. A null diagnostic is allowed. */
pr_u6c_3291_status pr_u6c_record_unlink_3291_cpu(uint8_t *ram,size_t ram_size,
    pr_u6c_3291_cpu *cpu,uint16_t resident_cs,uint16_t system_cs,
    const pr_u6c_3291_limits *limits,pr_u6c_3291_diagnostic *diagnostic);
#endif
