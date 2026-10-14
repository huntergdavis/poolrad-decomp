#ifndef POOLRAD_RECOVERED_OVERLAY_U6A_PARTY_BEFORE_14A9_H
#define POOLRAD_RECOVERED_OVERLAY_U6A_PARTY_BEFORE_14A9_H
#include <stddef.h>
#include <stdint.h>
#define PR_U6A_14A9_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6a_14a9_cpu;
typedef enum { PR_U6A_14A9_OK = 0, PR_U6A_14A9_ARGUMENT } pr_u6a_14a9_status;
/* Complete +14A9..+1600, entered after CALL; no arguments, RETF.
 * With a disjoint, finite party containing the active member, moves that
 * member one place toward the head, wrapping the head to the tail.
 * Models all ordered shared-RAM effects and terminating alias paths.
 * Like DOS, malformed/cyclic traversals can fail to return; no loop cap.
 * Undefined AF after logical instructions is represented as zero. */
pr_u6a_14a9_status pr_u6a_party_before_14a9(
    uint8_t *ram, size_t ram_size, pr_u6a_14a9_cpu *cpu);
#endif
