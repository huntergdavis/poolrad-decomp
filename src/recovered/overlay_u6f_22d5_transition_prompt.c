#include "poolrad/recovered/overlay_u6f_22d5_transition_prompt.h"

/* 0xAF8:0x0B25: copy at most `limit` characters of a Pascal string. */
static void pascal_store(uint8_t *dst, const uint8_t *src, size_t limit) {
    size_t len = src[0];
    if (len > limit) {
        len = limit;
    }
    dst[0] = (uint8_t)len;
    for (size_t i = 0; i < len; ++i) {
        dst[1u + i] = src[1u + i];
    }
}

/* 0xAF8:0x1366 with width 0 into a string[1]: the first decimal digit. */
static uint8_t first_digit(uint8_t value) {
    unsigned v = value;
    while (v >= 10u) {
        v /= 10u;
    }
    return (uint8_t)('0' + v);
}

/* +2389..+23A5 (and +23B0..+23CC): sub, then the two signed byte tests. */
static uint8_t step_coordinate(uint8_t coordinate, uint8_t delta) {
    uint8_t c = (uint8_t)(coordinate - delta);
    if ((int8_t)c < 0) {
        c = PR_U6F_22D5_COORD_MAX;
    }
    if ((int8_t)c > (int8_t)PR_U6F_22D5_COORD_MAX) {
        c = 0u;
    }
    return c;
}

pr_u6f_22d5_status pr_u6f_22d5_transition_prompt(
    const uint8_t *file_name, size_t file_name_size,
    const uint8_t *message, size_t message_size,
    pr_u6f_22d5_state *state,
    const pr_u6f_22d5_hooks *hooks,
    pr_u6f_22d5_result *out) {
    uint8_t name[PR_U6F_22D5_NAME_LIMIT + 1u];                  /* SS:BP-0x29, string[40] */
    uint8_t menu[PR_U6F_22D5_NAME_LIMIT + 1u];                  /* SS:BP-0x52, string[40] */
    uint8_t digit[2];                                           /* SS:BP-0x54, string[1] */
    uint8_t full[256];                                          /* SS:BP-0x156, string[255] */
    const uint8_t title[1] = { 0u };                            /* SS:BP-0x57, string[0] loaded from CS:0x22D4 */
    uint8_t extended = 0u;                                      /* SS:BP-0x56 */
    uint8_t al = 0u;                                            /* SS:BP-0x55 */
    pr_u6f_22d5_result r = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0u, 0x22D5u};
    if (out != NULL) {
        *out = r;
    }
    if (out == NULL || hooks == NULL || state == NULL || file_name == NULL || file_name_size == 0u ||
        (size_t)file_name[0] + 1u > file_name_size ||
        message == NULL || message_size != PR_U6F_22D5_MESSAGE_LENGTH) {
        return PR_U6F_22D5_ARGUMENT;
    }
    /* +22DC..+22EA: name := Copy(arg, 1, 0x28) */
    pascal_store(name, file_name, PR_U6F_22D5_NAME_LIMIT);
    /* +22EF..+22F4: resident 0x802:0x02EB with the local name; +22F9..+22FD: AL <> 0 jumps to +23DA */
    r.next_offset = 0x22F4u;
    if (hooks->select_file == NULL) { *out = r; return PR_U6F_22D5_NEED_HOOK; }
    if (hooks->select_file(hooks->context, name, (size_t)name[0] + 1u, &al) != 0) { *out = r; return PR_U6F_22D5_HOST_FAILURE; }
    if (al != 0u) {
        r.selected = 1u;
        r.next_offset = PR_U6F_22D5_END_OFFSET;                 /* +23DA..+23DD: frame restore; retf 4 */
        *out = r;
        return PR_U6F_22D5_OK;
    }
    /* +2300..+2315: digit := Str(zero-extended DS:[0x5376], width 0) into the string[1] */
    digit[0] = 1u;
    digit[1] = first_digit(state->archive_number);
    /* +231A..+232F: full := CS:0x22B7 + digit; +2334..+233D: menu := Copy(full, 1, 0x28) */
    full[0] = PR_U6F_22D5_MESSAGE_LENGTH + 1u;
    for (size_t i = 0; i < PR_U6F_22D5_MESSAGE_LENGTH; ++i) {
        full[1u + i] = message[i];
    }
    full[1u + PR_U6F_22D5_MESSAGE_LENGTH] = digit[1];
    pascal_store(menu, full, PR_U6F_22D5_NAME_LIMIT);
    /* +2342..+234C: title := CS:0x22D4 (length 0), translated as the constant above */
    /* +2351..+236F: unit 0x3B0 entry 2 (title, menu, 0x0D, 0x0A, 0x0F, 0, 0, var extended) -> AL */
    r.next_offset = 0x236Au;
    if (hooks->key_input == NULL) { *out = r; return PR_U6F_22D5_NEED_HOOK; }
    r.prompted = 1u;
    if (hooks->key_input(hooks->context, title, 1u, menu, (size_t)menu[0] + 1u,
                         PR_U6F_22D5_ROW, PR_U6F_22D5_ARG_A, PR_U6F_22D5_ARG_B, 0u, 0u,
                         &extended, &al) != 0) { *out = r; return PR_U6F_22D5_HOST_FAILURE; }
    r.key = al;
    r.extended = extended;
    /* +2372..+237C: an extended key, or a key other than 0x53, jumps to +23DA */
    if (extended != 0u || al != PR_U6F_22D5_SAVE_KEY) {
        r.next_offset = PR_U6F_22D5_END_OFFSET;
        *out = r;
        return PR_U6F_22D5_OK;
    }
    /* +237E..+23C7: x and y stepped by the facing tables with the 0..15 wrap */
    r.next_offset = 0x237Eu;
    if (state->dx_table == NULL || state->dy_table == NULL ||
        (size_t)state->facing >= state->dx_table_size || (size_t)state->facing >= state->dy_table_size) {
        *out = r; return PR_U6F_22D5_ARGUMENT;                  /* the table byte at the facing must be supplied */
    }
    state->x = step_coordinate(state->x, state->dx_table[state->facing]);
    state->y = step_coordinate(state->y, state->dy_table[state->facing]);
    r.moved = 1u;
    /* +23CC: resident 0x802:0x095D */
    r.next_offset = 0x23CCu;
    if (hooks->restore_archive_number == NULL) { *out = r; return PR_U6F_22D5_NEED_HOOK; }
    if (hooks->restore_archive_number(hooks->context) != 0) { *out = r; return PR_U6F_22D5_HOST_FAILURE; }
    r.restored = 1u;
    /* +23D1..+23D2: push cs; call +1D58 */
    r.next_offset = 0x23D2u;
    if (hooks->save == NULL) { *out = r; return PR_U6F_22D5_NEED_HOOK; }
    if (hooks->save(hooks->context) != 0) { *out = r; return PR_U6F_22D5_HOST_FAILURE; }
    r.saved = 1u;
    /* +23D5: resident 0x899:0x0000 */
    r.next_offset = 0x23D5u;
    if (hooks->exit_game == NULL) { *out = r; return PR_U6F_22D5_NEED_HOOK; }
    if (hooks->exit_game(hooks->context) != 0) { *out = r; return PR_U6F_22D5_HOST_FAILURE; }
    r.exited = 1u;
    r.next_offset = PR_U6F_22D5_END_OFFSET;                     /* +23DA..+23DD: mov sp,bp; pop bp; retf 4 */
    *out = r;
    return PR_U6F_22D5_OK;
}
