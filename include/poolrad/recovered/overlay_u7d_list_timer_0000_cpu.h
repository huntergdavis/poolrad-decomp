#ifndef POOLRAD_RECOVERED_OVERLAY_U7D_LIST_TIMER_0000_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U7D_LIST_TIMER_0000_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U7D_0000_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u7d_0000_cpu;

typedef enum {
    PR_U7D_0000_OK = 0,
    PR_U7D_0000_ARGUMENT,  /* null RAM/CPU, RAM that is not exactly 1 MiB, or IP not 0000h; nothing changed */
    PR_U7D_0000_CODE,      /* a reached LCALL is not 9A A8 16 (FillChar, +0019) or 9A 2A 00 (+01FE); stopped at it */
    PR_U7D_0000_CALLEE,    /* FillChar rejected its entry (cannot happen once the arguments are valid) */
    PR_U7D_0000_TRANSFER,  /* FillChar's RETF 8 popped a CS:IP other than +001E in this CS */
    PR_U7D_0000_PENDING,   /* reached +01FE, LCALL 00B0:002A (U9E +0026, not modeled here) with its five words pushed */
    PR_U7D_0000_LIMIT      /* max_loopbacks backward jumps were taken; stopped at the next jump's target */
} pr_u7d_0000_status;

/* Raw-CPU, full-state model of GAME.OVR unit 0x7D +0000..+02F1 (754 bytes,
 * 248 instructions) up to its one unmodeled call. Entered after a far call
 * (or PUSH CS; CALL): SS:[SP] return IP, [SP+2] return CS, then the bytes u
 * ([SP+4]) and n ([SP+6]); RETF 4. After PUSH BP; MOV BP,SP; SUB SP,1Ch:
 *  1. If DS:[49F3h] is not 2, FillChar(DS:471Eh, 48h, 1) (the merged
 *     pr_system_fillchar_16a8). Otherwise the flags DS:[471Dh+i] are scanned
 *     from i = 1 while i <= the word at [DS:49D6h]+67Ch; none set returns.
 *  2. The amount n is multiplied (MUL, low word) by DS:[363Ah + 2(k-2)] for
 *     k = u down to 2 (signed byte loop).
 *  3. While the amount is above 0: step := min(amount, 10); the party list
 *     (head DS:5D96h, link +104h/+106h) is walked with i from 1; a member
 *     whose flag DS:[471Dh+i] is set has it cleared, the tail of its list
 *     (head +7Fh/+81h, link +5/+7) found, and its nodes visited until the
 *     tail: a word +1 of 0 is skipped; one above the step is reduced by it
 *     and sets the flag again; one at or below the step reaches +01FE (stop
 *     PENDING after pushing the member, the node's byte +0 over AH and the
 *     node). After the visit, the remaining nodes with word +1 above 0 set
 *     the flag. Then the amount drops by 10 (or to 0).
 *  4. MOV SP,BP; POP BP; RETF 4.
 * Every operand is reread from RAM in source order (each LES reloads its
 * pair); LES pairs, word operands, POP and the RETF frame are read
 * physically contiguous; offsets wrap at 16 bits, physical addresses at
 * 1 MiB. Flags follow every flag-setting instruction (MUL's undefined ones
 * are overwritten by DEC before any stop). The lists can be cyclic in RAM,
 * so max_loopbacks caps the taken backward jumps (+0028, +006A, +008E, +00BA,
 * +0137, +015B, +024B, +028E). The code must sit at its original offsets
 * (IP = 0000h). */
pr_u7d_0000_status pr_u7d_list_timer_0000_cpu(uint8_t *ram, size_t ram_size, pr_u7d_0000_cpu *cpu, unsigned max_loopbacks);

#endif
