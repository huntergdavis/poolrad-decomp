#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_SLOT_OFFSETS_0068_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_SLOT_OFFSETS_0068_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3C6_0068_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c6_0068_cpu;

typedef enum {
    PR_U3C6_0068_OK = 0,
    PR_U3C6_0068_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U3C6_0068_LIMIT      /* host cap: one more loop-back than max_loopbacks; CS:IP = CS:(entry IP + 18h) */
} pr_u3c6_0068_status;

/* Complete GAME.OVR unit 0x3C6:+0068..+00E2 (123 bytes, 51 instructions,
 * two conditional branches, no calls), entered at CS:IP = unit:+0068
 * after the far CALL with SS:[SP] return IP and [SP+2] return CS and no
 * arguments; RETF. With n = DS:5F2A copied to the frame byte [BP-2], for
 * slots s = 1..n (none when n is 0) it stores
 * DS:[604A+s] := DS:[5F27+4s] - sign(G[+2]) and DS:[6092+s] :=
 * DS:[5F28+4s] - sign(G[+3]) (low bytes of CBW operands), where G is the
 * far pointer DS:6716/6718, reread through LES for every store. The slot
 * counter [BP-1] is reread for each index and compared with [BP-2] after
 * each pass. Once n exceeds 48h the slot tables run into the stored
 * bytes, which are reread as DOS would. The final flags are the last
 * CMP AL,[BP-2]'s (from MOV AL,1 when n is 0); AH, DX and ES:DI return
 * as the last pass left them. Every operand is reread from RAM; offsets
 * wrap at 16 bits, physical addresses at 1 MiB; LES and the RETF frame
 * are read physically contiguous.
 *
 * max_loopbacks: the number of taken JNE +00DD -> +0080 loop-backs the
 * model follows. On the next one it stops before the INC at +0080 with
 * CS unchanged, IP = entry IP + 18h, SP = BP - 2 and every earlier effect
 * kept (status LIMIT), which is the original's state on reaching +0080
 * that time. A call whose stores do not alias the frame bytes [BP-1] or
 * [BP-2] makes n passes and n - 1 <= 254 loop-backs, so a cap of 254 or
 * more never stops it; an aliasing store can make the original loop
 * forever. */
pr_u3c6_0068_status pr_u3c6_slot_offsets_0068(uint8_t *ram, size_t ram_size, pr_u3c6_0068_cpu *cpu,
                                              unsigned max_loopbacks);

#endif
