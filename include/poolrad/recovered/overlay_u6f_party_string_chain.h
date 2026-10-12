#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_STRING_CHAIN_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_PARTY_STRING_CHAIN_H

#include <stddef.h>
#include <stdint.h>

#include "poolrad/replay/slot_a_party_entry.h"

/* One selected first-slot call slice at unit 0x6F:+0x1965..+0x1982.
 * Each array represents the live Pascal length and text in its DOS local;
 * bytes after the length are outside this model's observable contract. */
typedef struct {
    uint8_t literal_local[2];      /* [BP-0x214], after SYSTEM StrLoad */
    uint8_t after_0356[256];       /* [BP-0x314] */
    uint8_t after_0425[256];       /* [BP-0x414], live at +0x1974 */
    uint8_t basename[41];         /* [BP-0x1F6], after +0x1974 cap 0x28 */
    uint16_t next_pc;              /* +0x1983, or +0x19F7 when count=0 */
} pr_u6f_party_string_frame;

typedef enum {
    PR_U6F_PARTY_STRING_OK = 0,
    PR_U6F_PARTY_STRING_ARGUMENT,
    PR_U6F_PARTY_STRING_ENTRY,
    PR_U6F_PARTY_STRING_BOUNDS
} pr_u6f_party_string_status;

/* 'entry' must be the first-slot handoff returned by
 * pr_slot_a_party_entry_begin(world,...). A zero-count entry skips the calls
 * and does not need literal/table inputs. A reached entry requires the
 * caller's one-character Pascal literal and ten live DS table bytes.
 * Errors leave 'out' untouched; no later filename/file/record effect runs. */
pr_u6f_party_string_status pr_u6f_party_string_first(
    const pr_slot_a_native_state *world, const pr_slot_a_party_entry *entry,
    const uint8_t *literal, size_t literal_size,
    const uint8_t *table, size_t table_size,
    pr_u6f_party_string_frame *out);

#endif
