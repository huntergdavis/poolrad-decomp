#ifndef POOLRAD_REPLAY_SLOT_A_MOVEMENT_0747_H
#define POOLRAD_REPLAY_SLOT_A_MOVEMENT_0747_H

#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/resident_08ae_dispatch_00dc_cpu.h"

#define PR_SLOT_A_MOVEMENT_RAM_SIZE (1u << 20)
#define PR_SLOT_A_MOVEMENT_BACKED_SIZE 655360u
#define PR_SLOT_A_MOVEMENT_CODE_SIZE 0x5au

typedef enum {
    PR_SLOT_A_MOVEMENT_AT_LOOKUP = 0,
    PR_SLOT_A_MOVEMENT_REFUSED
} pr_slot_a_movement_status;

/* Fixed conditional run003 profile, 2E60:0747 through the argument pushes,
 * stopping before the lookup call at 07A1. The caller supplies a separately
 * authenticated immutable reference to the 90 loaded code bytes at 0747;
 * equality to that reference is checked here, but authentication is the
 * caller's responsibility. RAM and CPU must be distinct objects, and the
 * code reference must not overlap mutable RAM. Only the first 655360 bytes
 * are backed; no other byte is read or written. All profile/identity/backing
 * failures refuse before changing CPU or RAM. This is not a general movement
 * model or a claim of native, same-invocation, or full-Up parity. */
pr_slot_a_movement_status pr_slot_a_movement_0747(
    uint8_t *ram, size_t capacity, size_t backed,
    const uint8_t *authenticated_code, size_t code_size,
    pr_r08ae_00dc_cpu *cpu);

#endif
