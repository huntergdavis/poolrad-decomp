#ifndef POOLRAD_REPLAY_SLOT_A_PARTY_ENTRY_H
#define POOLRAD_REPLAY_SLOT_A_PARTY_ENTRY_H

/* GAME.OVR unit 0x6F:+0x191E..+0x1964, after the post-transfer state step.
 * This is the local-count copy and the first party-loop decision. It stops
 * before the runtime string load at +0x1965 and does not load a record or
 * execute the later party/sidecar/menu effects. */
#include <stdint.h>

#include "poolrad/replay/slot_a_native_save.h"

typedef enum {
    PR_SLOT_A_PARTY_ENTRY_OK = 0,
    PR_SLOT_A_PARTY_ENTRY_ARGUMENT
} pr_slot_a_party_entry_status;

typedef struct {
    uint8_t local_count;       /* +0x191E..+0x1925: [BP-0x212] */
    uint8_t index_one_based;   /* +0x1931: 1, or 0 when the loop is skipped */
    uint16_t slot_offset;      /* offset into name_slots; 0 for the first slot */
    const uint8_t *slot;       /* borrowed first Pascal slot, NULL when count is 0 */
    uint16_t next_pc;         /* +0x1965 or +0x19F7 */
} pr_slot_a_party_entry;

/* Does not mutate world. NULL arguments leave out unchanged. The DOS count
 * is an unchecked byte loop bound: even a value above the eight stored slots
 * reaches the first slot. This step makes no claim about later iterations. */
pr_slot_a_party_entry_status pr_slot_a_party_entry_begin(
    const pr_slot_a_native_state *world, pr_slot_a_party_entry *out);

#endif
