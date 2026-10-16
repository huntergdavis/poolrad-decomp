#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_ENTRY5_0422_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_ENTRY5_0422_H

#include <stddef.h>
#include <stdint.h>

#define PR_U3C2_0422_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u3c2_0422_cpu;

typedef enum {
    PR_U3C2_0422_CALLEE_RETURNED = 0,
    PR_U3C2_0422_CALLEE_UNRESOLVED
} pr_u3c2_0422_callee_status;

/* Callee entry already contains the actual argument/far-return frame and
 * target IP=01A0 or02CC. Carry all RAM/CPU effects through the real return;
 * a scalar result without faithful body effects is insufficient. */
typedef pr_u3c2_0422_callee_status (*pr_u3c2_0422_callee)(
    void *user, uint8_t *ram, size_t ram_size, pr_u3c2_0422_cpu *cpu);

typedef struct {
    pr_u3c2_0422_callee entry2_01a0;
    pr_u3c2_0422_callee entry3_02cc;
    void *user;
} pr_u3c2_0422_hooks;

typedef enum {
    PR_U3C2_0422_OK = 0,
    PR_U3C2_0422_ARGUMENT,
    PR_U3C2_0422_CALLEE
} pr_u3c2_0422_status;

/* Candidate raw-RAM caller body [0422,057B), through RETF20.
 * Requires IP=0422, >=1MiB RAM distinct from CPU, immutable code and
 * synchronous RAM. Offsets wrap16; word/LES bytes are contiguous physical20.
 * A supplied hook descriptor must be stable and distinct from CPU/RAM.
 * Missing/rejected hooks or unexpected returned CS:IP stop at reached state.
 * Faithful hooks are required for returned behavior; no default is supplied.
 * There is no universal loop cap or raw-alias termination guarantee.
 * Undefined logical/SHL AF is zero by convention, without native AF proof;
 * other undefined MUL status bits are retained until overwritten.
 * RETF uses separately wrapped word pops; target native proof is pending.
 * Invalid initial arguments reject atomically. OK is model completion only,
 * not original/native parity or a strict/full-C certification. */
pr_u3c2_0422_status pr_u3c2_entry5_0422(
    uint8_t *ram, size_t ram_size, pr_u3c2_0422_cpu *cpu,
    const pr_u3c2_0422_hooks *hooks);

#endif
