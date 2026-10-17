#ifndef POOLRAD_RECOVERED_OVERLAY_U5E_DIRECTION_263D_H
#define POOLRAD_RECOVERED_OVERLAY_U5E_DIRECTION_263D_H

#include <stddef.h>
#include <stdint.h>

#define PR_U5E_263D_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u5e_263d_cpu;

typedef struct {
    uint16_t slots_cs;     /* loaded U3C6 code, entries +0BE7/+0C0F */
    uint16_t predicate_cs; /* loaded U3C2 code, entry +057B */
} pr_u5e_263d_layout;

typedef enum {
    PR_U5E_263D_OK = 0,
    PR_U5E_263D_ARGUMENT,
    PR_U5E_263D_CODE_ALIAS,
    PR_U5E_263D_LIMIT,
    PR_U5E_263D_CALLEE,
    PR_U5E_263D_TRANSFER
} pr_u5e_263d_status;

/* Complete U5E +263D caller, composed with U3C6 entries 12, 13, 15 and
 * U3C2 +057B. RAM, CPU and layout must be distinct stable objects. Enter
 * after the far CALL, IP=263D, with two far-pointer arguments on SS:SP.
 * layout binds the already-loaded far-jump stubs to their code segments;
 * overlay loading itself is outside this routine. Code is immutable.
 *
 * Each slot-search iteration and each predicate attempt consumes one unit
 * of max_iterations. LIMIT stops before +0C71 or +267F, exposing reached
 * CPU/RAM. It is a host boundary, not a DOS return or termination claim.
 * Initial stack windows crossing offset zero are rejected, as are modeled
 * reserved stack windows overlapping a composed code span; rejection is atomic.
 * Logical/shift AF is chosen as zero (architecturally undefined). Native
 * saved-route reachability and strict-credit review remain separate.
 */
pr_u5e_263d_status pr_u5e_direction_263d(
    uint8_t *ram, size_t ram_size, pr_u5e_263d_cpu *cpu,
    const pr_u5e_263d_layout *layout, size_t max_iterations);

#endif
