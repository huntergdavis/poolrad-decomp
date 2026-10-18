#ifndef POOLRAD_RECOVERED_OVERLAY_U5E_NEIGHBOR_1BF8_H
#define POOLRAD_RECOVERED_OVERLAY_U5E_NEIGHBOR_1BF8_H
#include <stddef.h>
#include <stdint.h>
#define PR_U5E_1BF8_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_u5e_1bf8_cpu;
typedef struct {
    uint16_t slots_cs;  /* already loaded U3C6 +0BE7/+0C0F/+0C5F/+04BE */
    uint16_t system_cs; /* already loaded SYSTEM set IN +0DC0 */
} pr_u5e_1bf8_layout;
typedef enum {
    PR_U5E_1BF8_OK = 0,
    PR_U5E_1BF8_ARGUMENT,
    PR_U5E_1BF8_CODE_ALIAS,
    PR_U5E_1BF8_LIMIT,
    PR_U5E_1BF8_CALLEE,
    PR_U5E_1BF8_TRANSFER
} pr_u5e_1bf8_status;

/* Complete U5E [1BF8,1E16), RETF 8: after the far CALL, SS:SP contains
 * return IP/CS, output far-pointer address [BP+6], and subject far pointer
 * [BP+0A]. Reads nine live displacements, slot coordinates/cell occupants,
 * candidate records and the original 32-byte set at caller CS:1BD8.
 * Composes real U3C6 cell/slot/search and SYSTEM set-IN instructions; no
 * result-only callbacks. Layout binds already-loaded overlay stubs; actual
 * overlay loading is outside this entry.
 *
 * Exactly 1 MiB A20-off RAM; CPU/RAM/layout must be distinct stable objects.
 * Loaded bodies and CS:1BD8 set data must be original, immutable, mutually
 * compatible. Data writes must not alter the far-jump bindings. Entry SP
 * [002A,FFF4] avoids all initial/nested frame and stack-LES seams. A reserved
 * stack window overlapping modelled code/set data rejects atomically.
 * Effective offsets wrap16; multi-byte/LES operands use consecutive physical
 * bytes. RETF uses the contiguous profile; initial frames do not wrap.
 * Arbitrary indirect LES seams are this declared Unicorn profile, not a
 * universal native-CPU claim.
 *
 * Each reached outer header +1C17, record header +1D42 and slot-search
 * header +0C71 consumes one shared max_iterations unit. LIMIT stops before
 * that instruction, retaining reached CPU/RAM. It is not a successful DOS
 * return or termination guarantee. Output writes into modelled code/set data
 * stop before the word at IP (CODE_ALIAS); prior effects remain.
 * CALLEE preserves a nested cell/runtime rejection or partial stop.
 * TRANSFER preserves an unexpected callee return. Logical/shift AF is zero
 * by convention; MUL follows DOSBox-X normal/386's undefined-flag profile.
 * Native route reachability and independent credit remain separate.
 */
pr_u5e_1bf8_status pr_u5e_neighbor_1bf8(
    uint8_t *ram, size_t ram_size, pr_u5e_1bf8_cpu *cpu,
    const pr_u5e_1bf8_layout *layout, size_t max_iterations);
#endif
