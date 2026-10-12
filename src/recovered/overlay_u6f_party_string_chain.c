#include "poolrad/recovered/overlay_u6f_party_string_chain.h"

#include "poolrad/recovered/resident_string_0356.h"
#include "poolrad/recovered/resident_string_0425.h"

#include <string.h>

pr_u6f_party_string_status pr_u6f_party_string_first(
    const pr_slot_a_native_state *world, const pr_slot_a_party_entry *entry,
    const uint8_t *literal, size_t literal_size,
    const uint8_t *table, size_t table_size,
    pr_u6f_party_string_frame *out)
{
    pr_u6f_party_string_frame result;
    size_t length;

    if (!world || !entry || !out) return PR_U6F_PARTY_STRING_ARGUMENT;
    memset(&result, 0, sizeof result);

    /* +0x1926 skips the entire call slice when local count is zero. Input
     * callbacks/data are gated at their first reached instruction. */
    if (world->character_count == 0u) {
        if (entry->local_count != 0u || entry->index_one_based != 0u ||
            entry->slot != NULL || entry->slot_offset != 0u ||
            entry->next_pc != 0x19f7u)
            return PR_U6F_PARTY_STRING_ENTRY;
        result.next_pc = 0x19f7u;
        *out = result;
        return PR_U6F_PARTY_STRING_OK;
    }

    /* The existing entry model selects the save-derived first 41-byte slot
     * at +0x1952..+0x1959, including when count exceeds eight. */
    if (entry->local_count != world->character_count ||
        entry->index_one_based != 1u || entry->slot_offset != 0u ||
        entry->slot != world->name_slots || entry->next_pc != 0x1965u)
        return PR_U6F_PARTY_STRING_ENTRY;
    if (!literal || !table) return PR_U6F_PARTY_STRING_ARGUMENT;
    if (literal_size < 2u || literal[0] != 1u || table_size < 10u ||
        entry->slot[0] > 40u)
        return PR_U6F_PARTY_STRING_BOUNDS;

    /* +0x1965 SYSTEM 0x0B0B loads the private compiled literal into the
     * two-byte local. Its RETF 4 retains that local's far pointer. */
    memcpy(result.literal_local, literal, sizeof result.literal_local);

    /* +0x196A consumes the local needle and the selected name slot, then
     * RETF 8 retains the -0x314 far output for the next resident call. */
    if (pr_rec_res_0802_0356(entry->slot, PR_SLOT_A_NAMES_SIZE / 8u,
                             result.literal_local, sizeof result.literal_local,
                             result.after_0356, sizeof result.after_0356)
        != PR_REC_0802_0356_OK)
        return PR_U6F_PARTY_STRING_BOUNDS;

    /* +0x196F consumes -0x314 and retains -0x414; the ten DS bytes are
     * runtime input, never a constant baked into this translation. */
    if (pr_rec_res_0802_0425(result.after_0356,
                             (size_t)result.after_0356[0] + 1u,
                             table, table_size, result.after_0425,
                             sizeof result.after_0425) != PR_REC_0802_0425_OK)
        return PR_U6F_PARTY_STRING_BOUNDS;

    /* +0x1974..+0x1982: SYSTEM 0x0B25 copies the retained -0x414 output
     * to the 41-byte basename local with cap 0x28, then pops all arguments. */
    length = result.after_0425[0];
    if (length > 0x28u) length = 0x28u;
    result.basename[0] = (uint8_t)length;
    memcpy(result.basename + 1u, result.after_0425 + 1u, length);
    result.next_pc = 0x1983u;
    *out = result;
    return PR_U6F_PARTY_STRING_OK;
}
