#ifndef POOLRAD_RECOVERED_OVERLAY_U20_PARTY_AWARD_0355_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U20_PARTY_AWARD_0355_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U20_0355_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u20_0355_cpu;

typedef enum {
    PR_U20_0355_OK = 0,
    PR_U20_0355_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U20_0355_CODE,      /* a reached LCALL is not xxxx:0A76 (9A 76 0A); stopped before it, IP at the site */
    PR_U20_0355_CALLEE,    /* the divide model stopped (*callee_status); cannot occur, every divisor is 10, 2 or 3 */
    PR_U20_0355_TRANSFER,  /* the divide's RETF popped a CS:IP other than the call's next instruction; cannot occur,
                              the divide writes only its own stack slots below its frame */
    PR_U20_0355_LIMIT      /* host cap on the party walk's JMP +0368; IP at the loop head (entry IP + 13h) */
} pr_u20_0355_status;

/* Raw-CPU, full-state model of the complete GAME.OVR unit 0x20 routine
 * +0355..+04C8 (372 bytes, 126 instructions), entered after a far call with
 * SS:[SP] return IP, [SP+2] return CS and [SP+4]/[SP+6] a LongInt X (low,
 * high); RETF 4. After PUSH BP; MOV BP,SP; SUB SP,8, for p := the far
 * pointer DS:[5D96] while p <> nil (p := p^[104h]): when p^[10Dh] <> 0 and
 * p^[10Ch] <> 1, v := X; by c := p^[2Fh]:
 *   c = 0, 2, 5, 6 with p^[12h], p^[10h], p^[11h], p^[13h] respectively
 *     above 0Fh: v := X div 10 + X;
 *   c = 8, 10..14 (signed) or 16: v := X div 2;   c = 9 or 15: v := X div 3;
 * then the LongInt p^[0ACh] += v (ADD/ADC to memory). Every div is a real
 * far call LCALL xxxx:0A76 read from RAM and run by the raw-CPU
 * pr_system_longdiv_0a76 (truncating, signed). A cyclic party list loops
 * forever in DOS: max_loopbacks caps the taken JMP +0368 and the next one
 * stops at the loop head with LIMIT and every earlier effect kept. The
 * final flags are OR AX,[BP-2]'s; the memory ADC's flags are exact for a
 * LIMIT stop, XOR BX,BX's for a CODE stop and SUB SP,8's are computed;
 * other dead writes are folded. Every operand is reread from RAM in source
 * order; offsets wrap at 16 bits and physical addresses at 1 MiB; LES
 * pairs, POP and the RETF frame are read physically contiguous.
 * *callee_status (may be NULL) receives the last divide's status, or -1. */
pr_u20_0355_status pr_u20_party_award_0355_cpu(uint8_t *ram, size_t ram_size, pr_u20_0355_cpu *cpu, unsigned max_loopbacks,
                                               int *callee_status);

#endif
