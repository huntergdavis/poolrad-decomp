#include "poolrad/replay/slot_a_party_entry.h"

pr_slot_a_party_entry_status pr_slot_a_party_entry_begin(
    const pr_slot_a_native_state *world, pr_slot_a_party_entry *out)
{
    pr_slot_a_party_entry result;
    if (!world || !out) return PR_SLOT_A_PARTY_ENTRY_ARGUMENT;

    /* The byte copied at +0x191E is the ninth world-save transfer. */
    result.local_count = world->character_count;
    result.index_one_based = 0u;
    result.slot_offset = 0u;
    result.slot = NULL;
    result.next_pc = 0x19F7u;

    /* +0x1926..+0x1935: unsigned 1 <= count enters the loop. The first
     * source at +0x1952..+0x1959 is BP-0x171 + 1*0x29 = BP-0x148,
     * the start of the transferred 0x148-byte name-slot array. */
    if (result.local_count != 0u) {
        result.index_one_based = 1u;
        result.slot = world->name_slots;
        result.next_pc = 0x1965u;
    }
    *out = result;
    return PR_SLOT_A_PARTY_ENTRY_OK;
}
