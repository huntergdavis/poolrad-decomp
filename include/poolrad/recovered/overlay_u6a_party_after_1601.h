#ifndef POOLRAD_RECOVERED_OVERLAY_U6A_PARTY_AFTER_1601_H
#define POOLRAD_RECOVERED_OVERLAY_U6A_PARTY_AFTER_1601_H
#include <stddef.h>
#include <stdint.h>
#define PR_U6A_1601_RAM_SIZE (1u << 20)
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_u6a_1601_cpu;
typedef enum { PR_U6A_1601_OK = 0, PR_U6A_1601_ARGUMENT } pr_u6a_1601_status;
/* Complete +1601..+171F, entered after CALL; no arguments, RETF.
 * For a disjoint finite party containing active, moves active one place
 * toward the tail, wrapping a tail member to the head. Models ordered
 * shared-RAM effects and terminating alias paths. Source traversal can
 * fail to return for malformed/cyclic lists; no host cap is added.
 * Undefined AF after logical instructions is represented as zero. */
pr_u6a_1601_status pr_u6a_party_after_1601(
    uint8_t *ram, size_t ram_size, pr_u6a_1601_cpu *cpu);
#endif
