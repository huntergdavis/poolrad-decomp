#ifndef POOLRAD_RECOVERED_OVERLAY_U38_HELPER_CLUSTER_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_HELPER_CLUSTER_CPU_H

#include <stddef.h>
#include <stdint.h>

#define PR_U38C_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u38c_cpu;

typedef enum {
    PR_U38C_OK = 0,
    PR_U38C_ARGUMENT,  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
    PR_U38C_TRANSFER,  /* a composed helper call's RETF 2 popped a CS:IP other than the call's next instruction */
    PR_U38C_LIMIT      /* +1490 only: host cap on loop-backs; CS:IP = CS:(entry IP + 21h) */
} pr_u38c_status;

/* Raw-CPU, full-state models of seven complete GAME.OVR unit 0x38 routines
 * (unit at file 0x0EC5B). The scalar-state translations of #77
 * (overlay_unit38_helper.h) are unchanged. Every operand is reread from RAM
 * in source order; offsets wrap at 16 bits, physical addresses at 1 MiB;
 * the RETF frame is read physically contiguous. Flag writes that are
 * overwritten before any read or return are folded (SUB SP; INC/SUB/ADD in
 * +1490); AND/OR leave AF clear, as both oracles do.
 *
 * Helper +0000 (31 bytes, 11 instructions), entered after PUSH AX; PUSH
 * CS; CALL +0000 (a far frame: [SP] IP, [SP+2] CS, [SP+4] word whose low
 * byte is the key k): if k is 0, or DS:6817 (loaded into AL) equals k,
 * DS:6818 := 0 and DS:6817 := 0. Final flags: the last CMP (k with 0, or
 * AL with k). RETF 2. */
pr_u38c_status pr_u38_helper_0000_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu);

/* The public entries below are entered after the far CALL with [SP] IP,
 * [SP+2] CS and ten argument bytes, which they never read; each ends with
 * RETF 0Ah. Each helper call pushes AX (AL = the key, AH as left), CS and
 * the next IP relative to the entry IP, runs the helper at entry IP minus
 * the entry's unit offset, and requires its return (else TRANSFER). The
 * final flags are the helper's, or the routine's last OR/CMP when it
 * returns without calling it.
 *
 * Entry 50, +1490 (91 bytes, 36 instructions): if DS:6819 bit 0 is set,
 * n := DS:681C ([BP-2]); for i = 1..n ([BP-1]): DS:6818 -= 2, then
 * DS:6818 := DS:681C if it is below it; DS:6816 += 4; then unless DS:6819
 * bit 3 is set, helper(0). max_loopbacks caps the taken JNE +14CE -> +14B1
 * loop-backs; on the next one the model stops before the INC with IP =
 * entry IP + 21h, SP = BP - 2 and every earlier effect kept (LIMIT). n
 * passes take n - 1 loop-backs unless DS stores alias the frame bytes.
 * Entry 92, +29B6 (21 bytes, 12 instructions): helper(0Bh), helper(35h).
 * Entry 93, +29CB (14 bytes, 8 instructions): helper(34h).
 * Entry 94, +29D9 (23 bytes, 12 instructions): helper(0) if DS:6819 bit 1.
 * Entry 95, +29F0 (33 bytes, 15 instructions): helper(37h), helper(34h),
 *   then DS:6816 := 64h if DS:682A is 0 (final flags: CMP [682A],0).
 * Entry 96, +2A11 (23 bytes, 12 instructions): helper(0) if DS:6819 bit 0. */
pr_u38c_status pr_u38_entry50_1490_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu, unsigned max_loopbacks);
pr_u38c_status pr_u38_entry92_29b6_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu);
pr_u38c_status pr_u38_entry93_29cb_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu);
pr_u38c_status pr_u38_entry94_29d9_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu);
pr_u38c_status pr_u38_entry95_29f0_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu);
pr_u38c_status pr_u38_entry96_2a11_cpu(uint8_t *ram, size_t ram_size, pr_u38c_cpu *cpu);

#endif
