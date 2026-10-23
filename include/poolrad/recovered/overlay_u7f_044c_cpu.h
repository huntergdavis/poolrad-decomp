#ifndef POOLRAD_RECOVERED_OVERLAY_U7F_044C_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7F_044C_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/overlay_u7f_entry0_0000_cpu.h"
#include "poolrad/recovered/resident_00ba_display_1693_cpu.h"

typedef pr_u9e_10e4_cpu pr_u7f_044c_regs;
typedef enum {
    PR_U7F_044C_RETURNED = 0,
    PR_U7F_044C_ARGUMENT,   /* null/size/IP/SP outside the contract; nothing changed */
    PR_U7F_044C_CODE,       /* a call site is not the original opcode, displacement or offset; IP at the call, before its return frame */
    PR_U7F_044C_LIMIT,      /* the host budget ran out at the next unexecuted counted instruction (this body, +0000 or the 13AB closure) */
    PR_U7F_044C_TRANSFER,   /* a return went elsewhere than expected (RETF 000Ch popped another address, or a callee returned elsewhere) */
    PR_U7F_044C_CALLEE      /* a callee stopped otherwise; diagnostic.callee and callee_status say which and how */
} pr_u7f_044c_status;

typedef struct { size_t clear, rect, printer, glyph; uint64_t delay; } pr_u7f_044c_limits;
typedef struct {
    int callee;               /* -1 none yet; 0 +0000, 1 +0020, 2 +003A, 3 StrLoad, 4 resident 1693 */
    int callee_status;
    uint16_t site;
    pr_u7f_0000_diagnostic d0000;
    pr_r00ba_1693_diagnostic d1693;
} pr_u7f_044c_diagnostic;

/* GAME.OVR unit 0x7F [044C,0506): 188 bytes. Enter after the call at IP 044C. Frame (RETF 000Ch pops all twelve argument bytes): SP IP, SP+2 CS, SP+4 INDEX (a byte in a word slot), SP+6 EXTRA,
 * SP+8/+10 the far pointer PP to a far pointer to the destination record P, SP+12/+14 the far pointer QQ to a far pointer to the source record Q.
 * With V = pr_u7f_entry0_0000_cpu(*PP) and W = the word at (*PP)+0102h: if W + EXTRA (16-bit) is above V the body shows a message through resident 1693 (StrLoad, then the display) and returns;
 * otherwise it subtracts EXTRA from the word at (*QQ)+88h+2*INDEX, calls +0020(*QQ, EXTRA), adds EXTRA to the word at (*PP)+88h+2*INDEX and calls +003A(*QQ, EXTRA).
 * EVERY access through PP, QQ, *PP and *QQ (the pointer words themselves and the data words) is through a caller-supplied pointer and goes through the bus; the pointer chains are re-read before every use.
 * Stack, code and the DS-independent frame are flat. +0020, +003A and StrLoad keep their flat contracts; the 1693 closure needs an attached EGA. Each instruction of this body, of +0000 and the 13AB
 * closure consumes one unit of *budget; those of +0020, +003A, StrLoad and the 1693 closure do not. Exactly 1 MiB of A20-off RAM; entry SP in [0200h,FFEEh]. CPU, budget, limits, bus and diagnostic
 * storage are distinct. Code is immutable. */
pr_u7f_044c_status pr_u7f_044c_cpu(pr_bus *bus, size_t ram_size, pr_u7f_044c_regs *cpu, size_t *budget, const pr_u7f_044c_limits *limits, pr_u7f_044c_diagnostic *diagnostic);
#endif
