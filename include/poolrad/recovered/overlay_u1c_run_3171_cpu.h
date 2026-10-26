#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_RUN_3171_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_RUN_3171_CPU_H

#include <stddef.h>
#include <stdint.h>

/* Raw-state C model of GAME.OVR unit 0x1C +[3171,31B3) (66 bytes, 22 instructions,
 * RETF 2), the ECL run loop. It is a candidate-scout draft, not a credit claim.
 *
 * Far Pascal: word entry address at [BP+6], return IP/CS beneath it, RETF 2.
 *   PC ([0x49ED]) := entry; stop byte [0x442E] := 0.
 *   while [0x442E] == 0 and [0x49FF] == 0:
 *       [0x6F7E] := [0x6F7F]   previous opcode
 *       ax := PC; di,es := far ptr [0x49DE]; di += ax
 *       al := es:[di + 0x6700]; [0x6F7F] := al   fetch and latch
 *       push cs; call near +2F2D   dispatch, held a callback
 *   [0x442E] := 0; POP BP; RETF 2.
 *
 * All data references use DS, stack words use SS at (SP mod 64K), offsets wrap at 16
 * bits and physical addresses wrap at 1 MiB. The dispatcher is a callback boundary:
 * the audit supplies a deterministic double on both the original and the C side, so
 * this model is checked up to that boundary, not across the real 61-case dispatch.
 */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u1c_3171_cpu;

typedef int (*pr_u1c_3171_dispatch_fn)(uint8_t *ram, size_t ram_size,
                                       pr_u1c_3171_cpu *cpu, void *context);

/* Deterministic dispatcher double used identically on both sides of the differential:
 * opcode 0x00 sets the stop byte, 0xFF sets the second stop byte, else PC += opcode. */
int pr_u1c_3171_dispatch_double(uint8_t *ram, size_t ram_size,
                                pr_u1c_3171_cpu *cpu, void *context);

/* caller_cs is the CS the RETF 2 returns to. Returns 0 on success. */
int pr_u1c_run_3171(uint8_t *ram, size_t ram_size, pr_u1c_3171_cpu *cpu,
                    uint16_t caller_cs, pr_u1c_3171_dispatch_fn dispatch,
                    void *context);

#endif
