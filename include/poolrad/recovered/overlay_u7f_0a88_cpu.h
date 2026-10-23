#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_0A88_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_0A88_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/overlay_u7f_0054_cpu.h"
#include "poolrad/recovered/resident_00ba_display_1693_cpu.h"

typedef pr_u9e_10e4_cpu pr_u7f_0a88_regs;
typedef enum {
    PR_U7F_0A88_RETURNED = 0,
    PR_U7F_0A88_ARGUMENT,   /* null/size/IP/SP outside the contract; nothing changed */
    PR_U7F_0A88_CODE,       /* a call site is not the original opcode, displacement or offset; IP at the call, before its return frame */
    PR_U7F_0A88_LIMIT,      /* the host budget ran out at the next unexecuted instruction (this body, +0054, +0000 or the 13AB closure) */
    PR_U7F_0A88_TRANSFER,   /* a return went elsewhere than expected (RETF 8 popped another address, or a callee returned elsewhere) */
    PR_U7F_0A88_CALLEE      /* a callee stopped otherwise; diagnostic.callee and callee_status say which and how */
} pr_u7f_0a88_status;

typedef struct { size_t clear, rect, printer, glyph; uint64_t delay; } pr_u7f_0a88_limits;   /* the 1693 closure's loop-back budgets, as in pr_r00ba_display_1693_cpu */
typedef struct {
    int callee;               /* -1 none yet; 0 +0054, 1 StrLoad, 2 resident 1693, 3 +003A */
    int callee_status;
    uint16_t site;
    pr_u7f_0054_diagnostic d0054;
    pr_r00ba_1693_diagnostic d1693;
} pr_u7f_0a88_diagnostic;

/* GAME.OVR unit 0x7F [0A88,0B5A): 210 bytes / 87 instructions (the queue's 267-byte extent runs on into the unit's string data at 0B5A). Enter after the call at IP 0A88.
 * Frame: SP IP, SP+2 CS, SP+4 the word whose low byte is the signed index I, SP+6 the word AMOUNT, SP+8 the record pointer's offset, SP+10 its segment; RETF 8.
 * P is the 32-bit pool at DS:[67F4h+4I] (low word) and DS:[67F6h+4I] (high word). +0054(record, AMOUNT, &local) returns AL = 1 when the record's word at +0102h plus AMOUNT is above the +0000 value;
 * then the body loads the message at CS:0A7D into its frame, shows it through resident 00BA:1693 (the 1693 closure: clear, print, delay, clear) and returns with nothing else changed.
 * Otherwise it clamps AMOUNT to P's low word when P (signed compare of the high words, then unsigned low words) is below AMOUNT, subtracts the clamped AMOUNT from P, adds it to the record's word
 * at +88h+2I and calls +003A(record, AMOUNT), which adds it to the record's word at +0102h.
 * The record word at +88h+2I is read and written THROUGH THE BUS (a caller-supplied pointer). Stack, code and DS globals (P) are flat; +0054's own accesses through its pointers go through the bus;
 * +003A, StrLoad and the 1693 closure keep their own contracts (flat RAM, and for 1693 an attached EGA and the windows it names). Each instruction of this body, of +0054, +0000 and the 13AB closure
 * consumes one unit of *budget; the instructions of StrLoad, +003A and the 1693 closure do not (the 1693 closure has its own loop-back limits). Exactly 1 MiB of A20-off RAM; entry SP in
 * [0200h,FFF4h]. CPU, budget, limits, bus and diagnostic storage are distinct. Code is immutable. */
pr_u7f_0a88_status pr_u7f_0a88_cpu(pr_bus *bus, size_t ram_size, pr_u7f_0a88_regs *cpu, size_t *budget, const pr_u7f_0a88_limits *limits, pr_u7f_0a88_diagnostic *diagnostic);
#endif
