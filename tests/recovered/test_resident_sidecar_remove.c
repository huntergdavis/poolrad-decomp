/* Synthetic path tests for START.EXE RES_00BA_1509. Expected traces were
 * transcribed from docs/re/RESIDENT_SIDECAR_REMOVE.md, not generated from the
 * C control flow. No DOS records, strings, or executable bytes are used. */
#include "poolrad/recovered/resident_sidecar_remove.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

typedef struct {
    uint16_t segment, offset, value;
} test_word;

typedef struct {
    test_word words[64];
    size_t word_count;
    char trace[128];
    size_t trace_len;
    uint16_t copied_segment, copied_literal;
    uint8_t presented_parameter;
    pr_rec_far_ptr released;
    uint16_t released_bytes;
    unsigned release_calls;
    int reject_release;
} fixture;

static pr_rec_far_ptr ptr(uint16_t offset, uint16_t segment) {
    pr_rec_far_ptr result = {offset, segment};
    return result;
}

static void event(fixture *state, char code) {
    if (state->trace_len + 1u < sizeof state->trace) {
        state->trace[state->trace_len++] = code;
        state->trace[state->trace_len] = '\0';
    }
}

static test_word *find_word(fixture *state, uint16_t segment, uint16_t offset) {
    size_t i;
    for (i = 0; i < state->word_count; ++i) {
        if (state->words[i].segment == segment &&
            state->words[i].offset == offset) {
            return &state->words[i];
        }
    }
    return NULL;
}

static int put_word(fixture *state, uint16_t segment, uint16_t offset,
                    uint16_t value) {
    test_word *cell = find_word(state, segment, offset);
    if (cell == NULL) {
        if (state->word_count == sizeof state->words / sizeof state->words[0]) {
            return 0;
        }
        cell = &state->words[state->word_count++];
        cell->segment = segment;
        cell->offset = offset;
    }
    cell->value = value;
    return 1;
}

static int put_far(fixture *state, pr_rec_far_ptr base, uint16_t field,
                   pr_rec_far_ptr value) {
    return put_word(state, base.segment, (uint16_t)(base.offset + field),
                    value.offset) &&
           put_word(state, base.segment, (uint16_t)(base.offset + field + 2u),
                    value.segment);
}

static int equals_far(fixture *state, pr_rec_far_ptr base, uint16_t field,
                      pr_rec_far_ptr expected) {
    test_word *low = find_word(state, base.segment,
                               (uint16_t)(base.offset + field));
    test_word *high = find_word(state, base.segment,
                                (uint16_t)(base.offset + field + 2u));
    return low != NULL && high != NULL && low->value == expected.offset &&
           high->value == expected.segment;
}

static int on_read(void *context, uint16_t segment, uint16_t offset,
                   uint16_t *value) {
    fixture *state = context;
    test_word *cell;
    event(state, 'R');
    cell = find_word(state, segment, offset);
    if (cell == NULL) return 0;
    *value = cell->value;
    return 1;
}

static int on_write(void *context, uint16_t segment, uint16_t offset,
                    uint16_t value) {
    fixture *state = context;
    test_word *cell;
    event(state, 'W');
    cell = find_word(state, segment, offset);
    if (cell == NULL) return 0;
    cell->value = value;
    return 1;
}

static void on_copy(void *context, uint16_t segment, uint16_t literal) {
    fixture *state = context;
    event(state, 'C');
    state->copied_segment = segment;
    state->copied_literal = literal;
}

static void on_present(void *context, uint8_t parameter) {
    fixture *state = context;
    event(state, 'P');
    state->presented_parameter = parameter;
}

static int on_release(void *context, pr_rec_far_ptr block, uint16_t bytes) {
    fixture *state = context;
    event(state, 'F');
    state->released = block;
    state->released_bytes = bytes;
    state->release_calls++;
    return !state->reject_release;
}

static pr_rec_remove_io make_io(fixture *state, size_t max_nodes) {
    pr_rec_remove_io io = {state, on_read, on_write, on_copy,
                          on_present, on_release, max_nodes};
    return io;
}

static int test_null_target(void) {
    fixture state = {0};
    pr_rec_remove_io io = make_io(&state, 4u);
    CHECK(pr_rec_res_00ba_1509(&io, ptr(0u, 0u), ptr(0x100u, 0x200u)) ==
          PR_REC_REMOVE_REPORTED_NULL);
    CHECK(strcmp(state.trace, "CP") == 0);
    CHECK(state.copied_segment == 0xBAu);
    CHECK(state.copied_literal == PR_REC_REMOVE_NULL_LITERAL);
    CHECK(state.presented_parameter == PR_REC_REMOVE_MESSAGE_PARAMETER);
    CHECK(state.release_calls == 0u);
    return 0;
}

static int test_empty_head(void) {
    fixture state = {0};
    pr_rec_remove_io io = make_io(&state, 4u);
    pr_rec_far_ptr owner = ptr(0x100u, 0x200u);
    CHECK(put_far(&state, owner, 0xC8u, ptr(0u, 0u)));
    CHECK(pr_rec_res_00ba_1509(&io, ptr(0x300u, 0x400u), owner) ==
          PR_REC_REMOVE_REPORTED_MISSING);
    CHECK(strcmp(state.trace, "RRCP") == 0);
    CHECK(state.copied_literal == PR_REC_REMOVE_MISSING_LITERAL);
    CHECK(state.release_calls == 0u);
    return 0;
}

static int test_head(void) {
    fixture state = {0};
    pr_rec_remove_io io = make_io(&state, 4u);
    pr_rec_far_ptr owner = ptr(0x100u, 0x200u);
    pr_rec_far_ptr target = ptr(0x300u, 0x400u);
    pr_rec_far_ptr next = ptr(0x500u, 0x400u);
    CHECK(put_far(&state, owner, 0xC8u, target));
    CHECK(put_far(&state, target, 0x2Au, next));
    CHECK(pr_rec_res_00ba_1509(&io, target, owner) == PR_REC_REMOVE_OK);
    CHECK(equals_far(&state, owner, 0xC8u, next));
    CHECK(equals_far(&state, target, 0x2Au, ptr(0u, 0u)));
    CHECK(strcmp(state.trace, "RRRRWWRRWWWWF") == 0);
    CHECK(state.release_calls == 1u && state.released_bytes == 0x3Fu);
    CHECK(state.released.offset == target.offset &&
          state.released.segment == target.segment);
    return 0;
}

static int test_middle_and_tail(void) {
    fixture state = {0};
    pr_rec_remove_io io = make_io(&state, 4u);
    pr_rec_far_ptr owner = ptr(0x100u, 0x200u);
    pr_rec_far_ptr a = ptr(0x300u, 0x400u);
    pr_rec_far_ptr b = ptr(0x400u, 0x400u);
    pr_rec_far_ptr c = ptr(0x500u, 0x400u);
    CHECK(put_far(&state, owner, 0xC8u, a));
    CHECK(put_far(&state, a, 0x2Au, b));
    CHECK(put_far(&state, b, 0x2Au, c));
    CHECK(put_far(&state, c, 0x2Au, ptr(0u, 0u)));
    CHECK(pr_rec_res_00ba_1509(&io, b, owner) == PR_REC_REMOVE_OK);
    CHECK(equals_far(&state, owner, 0xC8u, a));
    CHECK(equals_far(&state, a, 0x2Au, c));
    CHECK(equals_far(&state, b, 0x2Au, ptr(0u, 0u)));
    CHECK(strcmp(state.trace, "RRRRRRWWWWF") == 0);

    memset(state.trace, 0, sizeof state.trace);
    state.trace_len = 0u;
    CHECK(pr_rec_res_00ba_1509(&io, c, owner) == PR_REC_REMOVE_OK);
    CHECK(equals_far(&state, a, 0x2Au, ptr(0u, 0u)));
    CHECK(equals_far(&state, c, 0x2Au, ptr(0u, 0u)));
    CHECK(strcmp(state.trace, "RRRRRRWWWWF") == 0);
    CHECK(state.release_calls == 2u);
    return 0;
}

static int test_not_found_and_cycle_guard(void) {
    fixture state = {0};
    pr_rec_remove_io io = make_io(&state, 3u);
    pr_rec_far_ptr owner = ptr(0x100u, 0x200u);
    pr_rec_far_ptr a = ptr(0x300u, 0x400u);
    pr_rec_far_ptr b = ptr(0x400u, 0x400u);
    pr_rec_far_ptr target = ptr(0x500u, 0x400u);
    CHECK(put_far(&state, owner, 0xC8u, a));
    CHECK(put_far(&state, a, 0x2Au, b));
    CHECK(put_far(&state, b, 0x2Au, ptr(0u, 0u)));
    CHECK(pr_rec_res_00ba_1509(&io, target, owner) ==
          PR_REC_REMOVE_REPORTED_MISSING);
    CHECK(strcmp(state.trace, "RRRRRRRRRRCP") == 0);
    CHECK(state.copied_literal == PR_REC_REMOVE_MISSING_LITERAL);
    CHECK(equals_far(&state, a, 0x2Au, b));
    CHECK(state.release_calls == 0u);

    memset(state.trace, 0, sizeof state.trace);
    state.trace_len = 0u;
    CHECK(put_far(&state, b, 0x2Au, a));
    io.max_nodes = 2u;
    CHECK(pr_rec_res_00ba_1509(&io, target, owner) == PR_REC_REMOVE_LIMIT);
    CHECK(strcmp(state.trace, "RRRRRRRRRR") == 0);
    CHECK(state.release_calls == 0u);
    return 0;
}

static int test_wrap_and_failures(void) {
    fixture state = {0};
    pr_rec_remove_io io = make_io(&state, 2u);
    pr_rec_far_ptr owner = ptr(0xFF50u, 0x200u);
    pr_rec_far_ptr target = ptr(0u, 0x400u); /* one zero word is non-null */
    CHECK(put_far(&state, owner, 0xC8u, target));
    CHECK(put_far(&state, target, 0x2Au, ptr(0u, 0u)));
    state.reject_release = 1;
    CHECK(pr_rec_res_00ba_1509(&io, target, owner) ==
          PR_REC_REMOVE_RELEASE_ABORT);
    CHECK(strcmp(state.trace, "RRRRWWRRWWWWF") == 0);
    CHECK(equals_far(&state, owner, 0xC8u, ptr(0u, 0u)));
    CHECK(state.release_calls == 1u);

    memset(&state, 0, sizeof state);
    io = make_io(&state, 2u);
    CHECK(pr_rec_res_00ba_1509(&io, target, owner) == PR_REC_REMOVE_MEMORY);
    CHECK(strcmp(state.trace, "R") == 0);
    CHECK(pr_rec_res_00ba_1509(NULL, target, owner) == PR_REC_REMOVE_ARGUMENT);
    io.max_nodes = 0u;
    CHECK(pr_rec_res_00ba_1509(&io, target, owner) == PR_REC_REMOVE_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_null_target() || test_empty_head() || test_head() ||
        test_middle_and_tail() || test_not_found_and_cycle_guard() ||
        test_wrap_and_failures()) {
        return 1;
    }
    puts("resident sidecar removal synthetic paths passed");
    return 0;
}
