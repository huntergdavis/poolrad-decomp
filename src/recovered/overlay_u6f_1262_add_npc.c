#include "poolrad/recovered/overlay_u6f_1262_add_npc.h"

/* DS:[table + race*stride + roll] with the engine's 16-bit index arithmetic
 * (race sign-extended by cbw, the roll zero-extended), checked against the span. */
static int table_read(const pr_u6f_1262_table *table, int race, unsigned stride, uint8_t roll, uint8_t *value) {
    long index = (long)race * (long)stride + (long)roll;
    if (table->bytes == NULL || index < 0 || (unsigned long)index >= table->size) {
        return -1;
    }
    *value = table->bytes[index];
    return 0;
}

pr_u6f_1262_status pr_u6f_1262_add_npc(
    uint8_t id,
    const pr_u6f_1262_state *state,
    const pr_u6f_1262_hooks *hooks,
    pr_u6f_1262_result *out) {
    uint8_t *rec = NULL;                                        /* SS:BP-8 (and the copy at SS:BP-0x12) */
    size_t rec_size = 0u;
    uint8_t count;                                              /* SS:BP-9 */
    uint8_t al = 0u;
    int race;
    const pr_u6f_1262_table *table;
    uint16_t table_offset;
    pr_u6f_1262_result r;
    r.skipped = 0u; r.record = NULL; r.initialized = 0u; r.random_value = 0u; r.class_count = 0u;
    for (size_t i = 0; i <= PR_U6F_1262_CLASS_SLOTS; ++i) { r.list[i] = 0u; }
    r.selected_class = 0u; r.class_roll = 0u; r.picture_roll = 0u; r.inserted = 0u; r.built = 0u;
    r.next_offset = 0x1262u;
    if (out != NULL) {
        *out = r;
    }
    if (out == NULL || state == NULL || hooks == NULL) {
        return PR_U6F_1262_ARGUMENT;
    }
    /* +1268..+1274: the party count word above 7 jumps to +1420 */
    if (state->party_count > PR_U6F_1262_PARTY_MAX) {
        r.skipped = 1u;
        r.next_offset = PR_U6F_1262_END_OFFSET;
        *out = r;
        return PR_U6F_1262_OK;
    }
    /* +1277..+1283: rec := GetMem(0x11D) */
    r.next_offset = 0x127Bu;
    if (hooks->alloc_record == NULL) { *out = r; return PR_U6F_1262_NEED_HOOK; }
    if (hooks->alloc_record(hooks->context, &rec, &rec_size) != 0) { *out = r; return PR_U6F_1262_HOST_FAILURE; }
    if (rec == NULL || rec_size < PR_U6F_1262_RECORD_SIZE) { *out = r; return PR_U6F_1262_ARGUMENT; }   /* the engine never tests the result */
    r.record = rec;
    /* +1286..+1291: push cs; call +0EC4 with (rec, id) */
    r.next_offset = 0x1291u;
    if (hooks->init_record == NULL) { *out = r; return PR_U6F_1262_NEED_HOOK; }
    if (hooks->init_record(hooks->context, rec, rec_size, id) != 0) { *out = r; return PR_U6F_1262_HOST_FAILURE; }
    r.initialized = 1u;
    /* +129D..+12A9: rec[0xAB] := Random(0x100) */
    r.next_offset = 0x12A1u;
    if (hooks->random_byte == NULL) { *out = r; return PR_U6F_1262_NEED_HOOK; }
    if (hooks->random_byte(hooks->context, PR_U6F_1262_RANDOM_RANGE, &al) != 0) { *out = r; return PR_U6F_1262_HOST_FAILURE; }
    rec[PR_U6F_1262_OFF_RANDOM] = al;
    r.random_value = al;
    /* +12AE..+12E0: the seven class slots; a positive signed byte appends its index */
    count = 0u;
    for (uint8_t k = 0; k < PR_U6F_1262_CLASS_SLOTS; ++k) {
        if ((int8_t)rec[PR_U6F_1262_OFF_CLASS_BASE + k] > 0) {
            count++;
            r.list[count] = k;
        }
    }
    /* +12E2..+12F3: no positive slot: one entry, class 2 */
    if (count == 0u) {
        count = 1u;
        r.list[1] = PR_U6F_1262_DEFAULT_CLASS;
    }
    r.class_count = count;
    /* +12F7..+130D: sel := list[Roll(1, count)] */
    r.next_offset = 0x12FEu;
    if (hooks->roll == NULL) { *out = r; return PR_U6F_1262_NEED_HOOK; }
    if (hooks->roll(hooks->context, 1u, count, &al) != 0) { *out = r; return PR_U6F_1262_HOST_FAILURE; }
    if (al == 0u || al > count) { *out = r; return PR_U6F_1262_ARGUMENT; }   /* list[0] and list[count+1..] are unwritten in the engine */
    r.selected_class = r.list[al];
    /* +1310..+13DE: the class picture byte from one of four tables by race*5 + Roll(1, 5) */
    race = (int8_t)rec[PR_U6F_1262_OFF_RACE];
    if (r.selected_class == 0u) { table = &state->class_table_0; table_offset = 0x1314u; }
    else if (r.selected_class == 5u) { table = &state->class_table_5; table_offset = 0x134Bu; }
    else if (r.selected_class == 6u) { table = &state->class_table_6; table_offset = 0x1381u; }
    else { table = &state->class_table_other; table_offset = 0x13B3u; }
    r.next_offset = (uint16_t)(table_offset + 6u);              /* the lcall inside the branch */
    if (hooks->roll(hooks->context, 1u, 5u, &al) != 0) { *out = r; return PR_U6F_1262_HOST_FAILURE; }
    r.class_roll = al;
    {
        uint8_t value;
        if (table_read(table, race, 5u, al, &value) != 0) { *out = r; return PR_U6F_1262_ARGUMENT; }
        rec[PR_U6F_1262_OFF_PICTURE_CLASS] = value;
    }
    /* +13E3..+140A: rec[0xBB] := DS:[0x08C7 + race*8 + Roll(1, 8)] */
    r.next_offset = 0x13E9u;
    if (hooks->roll(hooks->context, 1u, 8u, &al) != 0) { *out = r; return PR_U6F_1262_HOST_FAILURE; }
    r.picture_roll = al;
    {
        uint8_t value;
        if (table_read(&state->picture_table, race, 8u, al, &value) != 0) { *out = r; return PR_U6F_1262_ARGUMENT; }
        rec[PR_U6F_1262_OFF_PICTURE_B] = value;
    }
    /* +140F..+1416: push cs; call +1446 with rec */
    r.next_offset = 0x1416u;
    if (hooks->insert_party == NULL) { *out = r; return PR_U6F_1262_NEED_HOOK; }
    if (hooks->insert_party(hooks->context, rec) != 0) { *out = r; return PR_U6F_1262_HOST_FAILURE; }
    r.inserted = 1u;
    /* +1419..+141D: push 1; push cs; call +0482 */
    r.next_offset = 0x141Du;
    if (hooks->build_pictures == NULL) { *out = r; return PR_U6F_1262_NEED_HOOK; }
    if (hooks->build_pictures(hooks->context, 1u) != 0) { *out = r; return PR_U6F_1262_HOST_FAILURE; }
    r.built = 1u;
    r.next_offset = PR_U6F_1262_END_OFFSET;                     /* +1420..+1423: mov sp,bp; pop bp; retf 2 */
    *out = r;
    return PR_U6F_1262_OK;
}
