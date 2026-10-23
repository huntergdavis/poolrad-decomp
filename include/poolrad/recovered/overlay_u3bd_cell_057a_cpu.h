#ifndef POOLRAD_RECOVERED_OVERLAY_U3BD_CELL_057A_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3BD_CELL_057A_CPU_H
#include "poolrad/recovered/memory_bus.h"

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3bd_057a_cpu;

typedef enum {
    PR_U3BD_057A_RETURNED = 0,
    PR_U3BD_057A_ARGUMENT,   /* null bus/RAM/CPU, RAM not exactly 1 MiB, IP not 057Ah, entry SP outside [0010h,FFF8h], or the +031F leaf rejected its entry; nothing changed */
    PR_U3BD_057A_TRANSFER    /* the +031F leaf's RETF, or this body's RETF 4, popped a return address other than its own frame's; the CPU is left at the popped CS:IP */
} pr_u3bd_057a_status;

/* GAME.OVR unit 0x3BD [057A,05F3): public entry 4, the cell lookup, 121 bytes / 45 instructions (plus the 49-byte / 18-instruction +031F leaf it calls). Enter after the far call at IP 057A.
 * Frame: SP IP, SP+2 CS, SP+4 the y word slot ([BP+6]), SP+6 the x word slot ([BP+8]); each is read and rewritten as its low byte. RETF 4.
 * PUSH BP; MOV BP,SP; SUB SP,2; the +031F range leaf (PUSH x, PUSH y, PUSH CS, CALL near; it returns AL=1 when both bytes are signed 0..15);
 * when AL is 0 and the map byte DS:[84DC] is 0 or 0Ah the result is 0; otherwise x and y are wrapped in place into 0..15 (signed: above 15 -> 0,
 * below 0 -> 15) and the result is the byte at ES:[DI+200h], DI = offset(DS:[6A5C]) + (sign(y) SHL 4) + sign(x), ES = segment(DS:[6A5C]). The result
 * goes to [BP-1] and returns in AL; AH keeps CBW's sign byte on the lookup path. Every operand is reread from RAM in source order (the two DS:84DC
 * reads, the wrapped bytes). The buffer byte is read THROUGH THE BUS (its far pointer comes from memory); the frame, the leaf's frame and the DS
 * globals are flat RAM. Flags at return are those of the last flag-setting instruction executed: ADD DI,DX on the lookup path, the gate's CMP on
 * the zero path, computed as DOSBox-X's 386 core does (SHL leaves AF set for a non-zero count; ADD overwrites it). Exactly 1 MiB of A20-off RAM;
 * entry SP in [0010h,FFF8h] so the leaf's frame and the return frame never straddle the segment end. Code is immutable. */
pr_u3bd_057a_status pr_u3bd_cell_057a_cpu(pr_bus *bus, size_t ram_size, pr_u3bd_057a_cpu *cpu);
#endif
