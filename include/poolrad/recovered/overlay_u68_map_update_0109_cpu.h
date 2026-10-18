#ifndef POOLRAD_RECOVERED_OVERLAY_U68_MAP_UPDATE_0109_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U68_MAP_UPDATE_0109_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U68_0109_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u68_0109_cpu;

typedef enum {
    PR_U68_0109_OK = 0,
    PR_U68_0109_ARGUMENT  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 0109h; nothing changed */
} pr_u68_0109_status;

/* Raw-CPU, full-state model of GAME.OVR unit 0x68 +0109..+0235 (301 bytes,
 * 125 instructions, call-free), the CPU/stack-effect form of the semantic
 * pr_u68 map update (overlay_u68_map_update.h). Entered after a far call (or
 * PUSH CS; CALL): SS:[SP] return IP, [SP+2] return CS, then the words s
 * ([SP+4]), y ([SP+6]) and x ([SP+8]), of which the low bytes are used;
 * RETF 6. After PUSH BP; MOV BP,SP:
 *  1. Signed bytes x or y outside 0..15 return (CMP; JL/JG/JLE).
 *  2. For s = 6, 4, 2 or 0 (MOV AL,s; CMP AL,k), the map byte
 *     ES:[DI+300h], with ES:DI := the far pointer DS:[6A5Ch] plus 16y + x
 *     (CBW; SHL AX,4; ADD DI,AX; ADD DI,DX), becomes (byte AND 3Fh) OR 40h,
 *     (AND CFh) OR 10h, (AND F3h) OR 04h or (AND FCh) OR 01h; the address is
 *     computed again (LES reread) before the store. Another s returns.
 *  3. POP BP; RETF 6.
 * After a store AX = 16y, DX = x, CL = 4 (CH kept), BL = the new byte,
 * ES:DI = the map pointer plus 16y + x, and the arithmetic flags are ADD
 * DI,DX's (SHL's undefined ones never survive). On the early returns AL =
 * s when s was loaded, and the flags are the last CMP's. Every operand is
 * reread from RAM in source order; the LES pair, POP and the RETF frame are
 * read physically contiguous; offsets wrap at 16 bits, physical addresses
 * at 1 MiB. The code must sit at its original offsets (IP = 0109h). */
pr_u68_0109_status pr_u68_map_update_0109_cpu(uint8_t *ram, size_t ram_size, pr_u68_0109_cpu *cpu);

#endif
