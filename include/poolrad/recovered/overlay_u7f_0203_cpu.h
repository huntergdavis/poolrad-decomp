#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_0203_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_0203_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/overlay_u7f_0054_cpu.h"
#include "poolrad/recovered/resident_00ba_display_1693_cpu.h"

typedef pr_u9e_10e4_cpu pr_u7f_0203_regs;
typedef enum {
    PR_U7F_0203_RETURNED = 0,
    PR_U7F_0203_ARGUMENT,   /* null/size/IP/SP outside the contract; nothing changed */
    PR_U7F_0203_CODE,       /* a call site is not the original opcode, displacement or offset; IP at the call, before its return frame */
    PR_U7F_0203_LIMIT,      /* the host budget ran out at the next unexecuted instruction (this body, +0054, +0000 or the 13AB closure) */
    PR_U7F_0203_TRANSFER,   /* a return went elsewhere than expected (RETF 2 popped another address, or a callee returned elsewhere) */
    PR_U7F_0203_CALLEE      /* a callee stopped otherwise; diagnostic.callee and callee_status say which and how */
} pr_u7f_0203_status;

typedef struct { size_t clear, rect, printer, glyph; uint64_t delay; } pr_u7f_0203_limits;   /* the 1693 closure's loop-back budgets, as in pr_r00ba_display_1693_cpu */
typedef struct {
    int callee;               /* -1 none yet; 0 +0054, 1 StrLoad, 2 resident 1693, 3 the first +003A, 4 the second +003A */
    int callee_status;
    uint16_t site;
    pr_u7f_0054_diagnostic d0054;
    pr_r00ba_1693_diagnostic d1693;
} pr_u7f_0203_diagnostic;

/* GAME.OVR unit 0x7F [0203,0294): 145 bytes. Enter after the call at IP 0203. Frame: SP IP, SP+2 CS, SP+4 the word AMOUNT; RETF 2.
 * G is the far pointer at DS:[5D92] (offset) and DS:[5D94] (segment). +0054(record G, AMOUNT, &local) returns AL = 1 when the record's word at +0102h plus AMOUNT is above the +0000 value, and
 * stores the excess in the local at BP-2. When AL is nonzero the body loads the message at CS:01DB, shows it through resident 00BA:1693 (the 1693 closure: clear, print, delay, clear), adds the
 * excess to the record word at G+90h, calls +003A(G, excess) and subtracts the excess from AMOUNT into the 32-bit counter at DS:[6804]; otherwise it adds AMOUNT to G+90h and calls +003A(G, AMOUNT).
 * The record word at G+90h is read and written THROUGH THE BUS (a caller-supplied pointer). Stack, code and DS globals are flat; +0054's own accesses through its pointers go through the bus;
 * +003A, StrLoad and the 1693 closure keep their own contracts (flat RAM, and for 1693 an attached EGA and the windows it names). Each instruction of this body, of +0054, +0000 and the 13AB closure
 * consumes one unit of *budget; the instructions of StrLoad, +003A and the 1693 closure do not (the 1693 closure has its own loop-back limits). Exactly 1 MiB of A20-off RAM; entry SP in
 * [0200h,FFF8h]. CPU, budget, limits, bus and diagnostic storage are distinct. Code is immutable. */
pr_u7f_0203_status pr_u7f_0203_cpu(pr_bus *bus, size_t ram_size, pr_u7f_0203_regs *cpu, size_t *budget, const pr_u7f_0203_limits *limits, pr_u7f_0203_diagnostic *diagnostic);
#endif
