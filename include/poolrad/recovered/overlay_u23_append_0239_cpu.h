#ifndef POOLRAD_RECOVERED_OVERLAY_U23_APPEND_0239_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U23_APPEND_0239_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/overlay_0077_275b_cpu.h"
#include "poolrad/recovered/resident_00ba_display_1693_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u23_0239_cpu;
typedef enum {
    PR_U23_0239_OK=0,
    PR_U23_0239_ARGUMENT,   /* contract violated; nothing changed */
    PR_U23_0239_CODE,       /* a call site's opcode or offset word is not the original's; IP at the call */
    PR_U23_0239_RESOLVE,    /* the overlay stub for the capacity check does not resolve to 275B; IP at the call */
    PR_U23_0239_CALLEE,     /* a composed callee stopped; diagnostic carries its call index and raw status */
    PR_U23_0239_TRANSFER,   /* a callee returned somewhere other than the instruction after its call */
    PR_U23_0239_LIMIT       /* the list walk took its backward jump more than walk_limit times; IP at 02FA */
} pr_u23_0239_status;
typedef enum {
    PR_U23_0239_NONE=-1, PR_U23_0239_CHECK=0, PR_U23_0239_STRLOAD=1, PR_U23_0239_DISPLAY=2, PR_U23_0239_GETMEM_HEAD=3,
    PR_U23_0239_MOVE_HEAD=4, PR_U23_0239_GETMEM_TAIL=5, PR_U23_0239_MOVE_TAIL=6, PR_U23_0239_REDRAW=7
} pr_u23_0239_callee;
typedef struct {
    size_t clear_loopbacks,rect_loopbacks,printer_loopbacks,glyph_loopbacks;
    uint64_t delay_loopbacks;
    size_t resident_budget;     /* instruction budget of the 0BB8 call at 0344 (275B resets its own for its two resident calls) */
    unsigned heap_steps;        /* GetMem free-list walk cap */
    unsigned walk_limit;        /* taken backward jumps of the list walk */
} pr_u23_0239_limits;
typedef struct {
    int callee,status;
    pr_ovl0077_275b_diagnostic check;
    pr_r00ba_1693_diagnostic display;
    unsigned walked;            /* backward jumps taken by the list walk */
} pr_u23_0239_diagnostic;
/* Original GAME.OVR unit 23h [0239,034F), 278 bytes / 94 instructions, RETF 8. The queue's 296-byte extent includes the 18
 * data bytes after the RETF (a Pascal string literal that is not executed); the 11 bytes before the entry are the literal
 * the over-capacity path displays. Entered after a far CALL: SS:[SP] return IP, [SP+2] return CS, [SP+4] far pointer
 * RESULT (offset, then segment), [SP+8] far pointer ITEM, a 63-byte (3Fh) record. DS:[5D92] is the far pointer to the
 * character object (offsets C8h/CAh hold the head of its singly linked list of 63-byte records, each record's next link at
 * +2Ah/+2Ch).
 *  1. The capacity check 275B (reached through the overlay stub at 0085:004D) is called with the character object and ITEM.
 *     Nonzero AL: the 10-character message literal at CS:022E is loaded into a local by raw 0AF8:0B0B and displayed by raw
 *     00BA:1693; *RESULT := 1; return (no append, no 0BB8).
 *  2. Otherwise *RESULT := 0 and the list head is read. Empty: GetMem(3Fh), Move(ITEM, node, 3Fh), node.next := nil, head := node.
 *     Non-empty: walk to the tail (next = nil), GetMem, Move, new.next := nil, tail.next := new.
 *  3. Raw 00BA:0BB8 is called on the character object and the function returns.
 * Exactly 1 MiB A20-off RAM, attached EGA, IP 0239h, SP in [0200h,FFE0h]. CS/DS/SS full 64 KiB windows exclude
 * A0000..AFFFF, because the flat-RAM callees (275B, 0BB8, GetMem, Move, StrLoad) read their operands from RAM. The pointers RESULT,
 * ITEM and the character object are NOT restricted for this body's own accesses (they go through the bus), but a record
 * or node that a flat callee reads or writes must lie in ordinary memory: that is a precondition, not an all-reads proof.
 * Code, the literal, the call-site bytes and the stub resolver are immutable. No interrupts, faults, self-modification or
 * arbitrary-PC resume. CPU, bus, EGA, limits and diagnostic are distinct host objects outside RAM.
 * Each call site's opcode 9A, offset word and (live) segment word are checked before its frame is pushed (CODE). The
 * capacity-check stub at the live segment must hold EA 5B 27 followed by a segment word (the overlay manager's load path is
 * a declared substitution, RESOLVE otherwise); that segment holds 275B. CALLEE keeps the nested stop (call index and raw
 * status); TRANSFER an unexpected successful return. ARGUMENT preserves CPU, RAM and EGA and resets the diagnostic. */
pr_u23_0239_status pr_u23_append_0239_cpu(pr_bus *bus,size_t ram_size,pr_u23_0239_cpu *cpu,
    const pr_u23_0239_limits *limits,pr_u23_0239_diagnostic *diagnostic);
#endif
