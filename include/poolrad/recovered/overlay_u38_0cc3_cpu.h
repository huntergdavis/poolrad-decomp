#ifndef POOLRAD_RECOVERED_OVERLAY_U38_0CC3_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_0CC3_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_00ba_display_1693_cpu.h"
#include "poolrad/recovered/overlay_u9e_entry16_10e4.h"

typedef pr_u9e_10e4_cpu pr_u38_0cc3_regs;
typedef enum {
    PR_U38_0CC3_RETURNED = 0,
    PR_U38_0CC3_ARGUMENT,   /* null/size/IP/SP outside the contract; nothing changed */
    PR_U38_0CC3_CODE,       /* a far call site is not the original opcode or offset; IP at the call, before its return frame */
    PR_U38_0CC3_LIMIT,      /* the host budget ran out at the next unexecuted instruction of this body (or FreeMem's walk cap) */
    PR_U38_0CC3_TRANSFER,   /* a return went elsewhere than expected (RETF 0Ah popped another address, or a callee returned elsewhere) */
    PR_U38_0CC3_CALLEE,     /* a callee stopped otherwise; diagnostic.callee and callee_status say which and how */
    PR_U38_0CC3_RUNERROR    /* SYSTEM FreeMem raised RunError 204 (an invalid or duplicate block): CPU at SYSTEM:00E2, AX = 204, as in DOS */
} pr_u38_0cc3_status;

typedef struct { size_t clear, rect, printer, glyph; uint64_t delay; unsigned freemem_steps; } pr_u38_0cc3_limits;
typedef struct {
    int callee;               /* -1 none yet; 1 StrLoad, 2 resident 1693, 3 FreeMem */
    int callee_status;
    uint16_t site;
    pr_r00ba_1693_diagnostic d1693;
} pr_u38_0cc3_diagnostic;

/* GAME.OVR unit 0x38 [0CC3,0FA2): 735 bytes / 266 instructions (the queue's 745-byte bound runs into the next routine's 9-byte
 * string at 0FA2), entry 34 (stub 0041:00CA), the U9E dispatcher's slot-40 handler. Enter after the far call at IP 0CC3.
 * Frame: SP IP, SP+2 CS, SP+4 a byte (unused), SP+6/SP+8 the far pointer to an object record, SP+10/SP+12 a 32-bit key; RETF 0Ah.
 * The body walks the far-pointer list headed at DS:[6DBB] for the node whose first dword is the key and whose byte +12h is the
 * record's byte +3 >> 4. If none matches it returns. Otherwise it loads the 26-byte message at CS:0CA8 and shows it through
 * resident 00BA:1693, then for each of the node's four cells i = 1..4 with byte +0Bh+i nonzero: the cell (x, y) is the node's
 * signed bytes +10h/+11h plus the signed offsets DS:[2736h + k] / DS:[273Fh + k], k = DS:[290Dh + i]; the map byte +7 of the cell
 * at the far pointer DS:[6716] + 50*y + x becomes 1Fh when an entry j = 1..DS:[6715] of the 7-byte table at DS:66D6 (nonzero
 * dword, signed bytes x at +4, y at +5) sits on the cell, otherwise the node's saved byte +7+i. It unlinks the node from the list
 * (a list whose head is the node advances the head; otherwise the walk from the head looks for the predecessor and never stops
 * if there is none), frees it with SYSTEM FreeMem (13h bytes), and re-stamps 1Eh on every cell of every remaining node.
 * Every access through a far pointer the body did not create (the record, the list nodes, the map) goes THROUGH THE BUS; the
 * stack, the code and DS globals are flat; StrLoad, the 1693 closure and FreeMem keep their own contracts. Declared exclusion: FreeMem
 * reaches the freed node FLAT, so a corrupt list node in the A0000..AFFFF aperture is freed through RAM, not the planes (as for +0EC4). The flags follow the
 * pinned DOSBox-X normal core: MUL keeps SF/AF (pinned MULW), SHR sets AF for a nonzero count. Each instruction of this body
 * consumes one unit of *budget; StrLoad, 1693 and FreeMem do not (1693 and FreeMem have their own caps in *limits). Exactly 1 MiB
 * of A20-off RAM; entry SP in [0200h,FFF2h]. CPU, budget, limits, bus and diagnostic storage are distinct. Code is immutable. */
pr_u38_0cc3_status pr_u38_0cc3_cpu(pr_bus *bus, size_t ram_size, pr_u38_0cc3_regs *cpu, size_t *budget, const pr_u38_0cc3_limits *limits,
                                   pr_u38_0cc3_diagnostic *diagnostic);
#endif
