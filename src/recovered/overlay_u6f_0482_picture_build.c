#include "poolrad/recovered/overlay_u6f_0482_picture_build.h"

/* 0xAF8:0x0B0B + 0x0C3B + 0x0B98: prefix (5 characters) followed by one character. */
static void make_name(uint8_t *dst, const uint8_t *prefix, uint8_t letter) {
    dst[0] = (uint8_t)(PR_U6F_0482_PREFIX_LENGTH + 1u);
    for (size_t i = 0; i < PR_U6F_0482_PREFIX_LENGTH; ++i) {
        dst[1u + i] = prefix[i];
    }
    dst[1u + PR_U6F_0482_PREFIX_LENGTH] = letter;
}

pr_u6f_0482_status pr_u6f_0482_picture_build(
    uint8_t recolor,
    const uint8_t *record, size_t record_size,
    const uint8_t *letter_table, size_t letter_table_size,
    const uint8_t *index_table, size_t index_table_size,
    const uint8_t *prefix_a, size_t prefix_a_size,
    const uint8_t *prefix_b, size_t prefix_b_size,
    const pr_u6f_0482_hooks *hooks,
    pr_u6f_0482_result *out) {
    uint8_t name[PR_U6F_0482_PREFIX_LENGTH + 2u];               /* the composed string, SS:BP-0x229 */
    uint8_t slot;                                               /* rec[0xBF] */
    pr_u6f_0482_result r;
    for (size_t i = 0; i < PR_U6F_0482_MAP_SIZE; ++i) { r.map_a[i] = 0u; r.map_b[i] = 0u; }
    r.loads = 0u; r.merges = 0u; r.recolors = 0u; r.released = 0u; r.restored = 0u; r.flushed = 0u;
    r.letter = 0u; r.next_offset = 0x0482u;
    if (out != NULL) {
        *out = r;
    }
    if (out == NULL || hooks == NULL || record == NULL || record_size < PR_U6F_0482_RECORD_MIN ||
        letter_table == NULL || letter_table_size == 0u || index_table == NULL ||
        prefix_a == NULL || prefix_a_size != PR_U6F_0482_PREFIX_LENGTH ||
        prefix_b == NULL || prefix_b_size != PR_U6F_0482_PREFIX_LENGTH) {
        return PR_U6F_0482_ARGUMENT;
    }
    /* +0489..+048C: 0x802:0x094A with AL = 3 */
    r.next_offset = 0x048Cu;
    if (hooks->selector_push == NULL) { *out = r; return PR_U6F_0482_NEED_HOOK; }
    if (hooks->selector_push(hooks->context, 3u) != 0) { *out = r; return PR_U6F_0482_HOST_FAILURE; }
    /* +0491..+0498: rec := DS:[0x5D92]; +04B4..+04BD: the letter DS:[0x08C5 + rec[0xC0]] */
    r.next_offset = 0x04B4u;
    if ((size_t)record[PR_U6F_0482_OFF_LETTER] >= letter_table_size) { *out = r; return PR_U6F_0482_ARGUMENT; }
    r.letter = letter_table[record[PR_U6F_0482_OFF_LETTER]];
    slot = record[PR_U6F_0482_OFF_SLOT];
    /* +049B..+04D8: name := CS:0x0476 + Chr(letter); loader (name, rec[0xBD], 0x0B) */
    make_name(name, prefix_a, r.letter);
    r.next_offset = 0x04D8u;
    if (hooks->load_picture == NULL) { *out = r; return PR_U6F_0482_NEED_HOOK; }
    if (hooks->load_picture(hooks->context, name, (size_t)name[0] + 1u, record[PR_U6F_0482_OFF_P3_FIRST], PR_U6F_0482_TEMP_SLOT) != 0) { *out = r; return PR_U6F_0482_HOST_FAILURE; }
    r.loads++;
    /* +04DD..+0520: name := CS:0x047C + Chr(letter); loader (name, rec[0xBE], rec[0xBF]) */
    make_name(name, prefix_b, r.letter);
    r.next_offset = 0x0520u;
    if (hooks->load_picture(hooks->context, name, (size_t)name[0] + 1u, record[PR_U6F_0482_OFF_P3_SECOND], slot) != 0) { *out = r; return PR_U6F_0482_HOST_FAILURE; }
    r.loads++;
    /* +0525..+0546: +032F (DS:[0x5EB2], slot first); +0549..+056A: +032F (DS:[0x5EB6], slot second) */
    r.next_offset = 0x0546u;
    if (hooks->merge_picture == NULL) { *out = r; return PR_U6F_0482_NEED_HOOK; }
    if (hooks->merge_picture(hooks->context, slot, 0u) != 0) { *out = r; return PR_U6F_0482_HOST_FAILURE; }
    r.merges++;
    r.next_offset = 0x056Au;
    if (hooks->merge_picture(hooks->context, slot, 1u) != 0) { *out = r; return PR_U6F_0482_HOST_FAILURE; }
    r.merges++;
    /* +056D..+0573: a zero argument byte jumps to +065C */
    if (recolor != 0u) {
        /* +0576..+059D: both maps := identity (16 entries) */
        r.next_offset = 0x0576u;
        for (size_t k = 0; k < PR_U6F_0482_MAP_SIZE; ++k) {
            r.map_a[k] = (uint8_t)k;
            r.map_b[k] = (uint8_t)k;
        }
        /* +059F..+0600: for k := 1 to 6: i := DS:[0x3CF5 + k]; map_b[i] := low nibble; map_b[i+8] := high nibble */
        r.next_offset = 0x059Fu;
        if (record_size < PR_U6F_0482_RECORD_RECOLOR_MIN || index_table_size < PR_U6F_0482_COLOR_COUNT + 1u) { *out = r; return PR_U6F_0482_ARGUMENT; }
        for (size_t k = 1; k <= PR_U6F_0482_COLOR_COUNT; ++k) {
            uint8_t color = record[PR_U6F_0482_OFF_LETTER + k];
            uint8_t i = index_table[k];
            if (i > 7u) { *out = r; return PR_U6F_0482_ARGUMENT; }   /* i + 8 would leave the engine's 16-byte local */
            r.map_b[i] = (uint8_t)(color & 0x0Fu);
            r.map_b[i + 8u] = (uint8_t)(color >> 4);
        }
        /* +0602..+062A: 0x464:0x1E5C (map_a, map_b, 0, 0, slot first); +062F..+0657: the same for the second */
        r.next_offset = 0x062Au;
        if (hooks->recolor_picture == NULL) { *out = r; return PR_U6F_0482_NEED_HOOK; }
        if (hooks->recolor_picture(hooks->context, r.map_a, r.map_b, slot, 0u) != 0) { *out = r; return PR_U6F_0482_HOST_FAILURE; }
        r.recolors++;
        r.next_offset = 0x0657u;
        if (hooks->recolor_picture(hooks->context, r.map_a, r.map_b, slot, 1u) != 0) { *out = r; return PR_U6F_0482_HOST_FAILURE; }
        r.recolors++;
    }
    /* +065C..+065F: unit 0x3CE entry 3 with 0x0B */
    r.next_offset = 0x065Fu;
    if (hooks->release_slot == NULL) { *out = r; return PR_U6F_0482_NEED_HOOK; }
    if (hooks->release_slot(hooks->context, PR_U6F_0482_TEMP_SLOT) != 0) { *out = r; return PR_U6F_0482_HOST_FAILURE; }
    r.released = 1u;
    /* +0664: 0x802:0x095D */
    r.next_offset = 0x0664u;
    if (hooks->restore_archive_number == NULL) { *out = r; return PR_U6F_0482_NEED_HOOK; }
    if (hooks->restore_archive_number(hooks->context) != 0) { *out = r; return PR_U6F_0482_HOST_FAILURE; }
    r.restored = 1u;
    /* +0669: 0x899:0x0104 */
    r.next_offset = 0x0669u;
    if (hooks->flush_keys == NULL) { *out = r; return PR_U6F_0482_NEED_HOOK; }
    if (hooks->flush_keys(hooks->context) != 0) { *out = r; return PR_U6F_0482_HOST_FAILURE; }
    r.flushed = 1u;
    r.next_offset = PR_U6F_0482_END_OFFSET;                     /* +066E..+0671: mov sp,bp; pop bp; retf 2 */
    *out = r;
    return PR_U6F_0482_OK;
}
