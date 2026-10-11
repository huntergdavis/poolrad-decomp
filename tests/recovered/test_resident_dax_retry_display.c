#include "poolrad/recovered/resident_dax_retry_display.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

typedef struct {
    char events[64];
    size_t event_count;
    unsigned calls;
    unsigned fail_at;
    uint8_t columns[64];
    uint8_t glyphs[64];
    uint8_t codes[64];
    uint8_t spans[64];
    uint8_t rows[64];
    uint8_t key;
    uint8_t final_al;
} fixture;

static int display_command(void *ctx, uint8_t column, uint8_t row,
                           uint8_t code, uint8_t count, uint8_t glyph,
                           uint8_t *al) {
    fixture *f = ctx;
    size_t n = f->event_count;
    if (n >= sizeof f->events - 1) return 1;
    f->events[n] = 'D';
    f->events[n + 1] = '\0';
    f->columns[n] = column;
    f->rows[n] = row;
    f->codes[n] = code;
    f->spans[n] = count;
    f->glyphs[n] = glyph;
    f->event_count++;
    f->calls++;
    *al = f->final_al;
    return f->calls == f->fail_at;
}

static int read_key(void *ctx, uint8_t *key) {
    fixture *f = ctx;
    size_t n = f->event_count;
    if (n >= sizeof f->events - 1) return 1;
    f->events[n] = 'K';
    f->events[n + 1] = '\0';
    f->event_count++;
    f->calls++;
    *key = f->key;
    return f->calls == f->fail_at;
}

static pr_rec_dax_retry_io io_for(fixture *f) {
    pr_rec_dax_retry_io io = {f, display_command, read_key};
    return io;
}

static int test_empty_and_frame(void) {
    fixture f = {0};
    pr_rec_dax_nested_string message = {0};
    pr_rec_dax_retry_io io = io_for(&f);
    uint8_t al = 0x55;
    f.key = 0xA7;
    f.final_al = 0x3C;
    CHECK(pr_rec_res_0802_0193(&io, &message, 0xAB0E, &al) ==
          PR_REC_DAX_RETRY_OK);
    CHECK(strcmp(f.events, "DKD") == 0);
    CHECK(f.columns[0] == 0 && f.columns[2] == 0);
    CHECK(f.rows[0] == 24 && f.rows[2] == 24);
    CHECK(f.codes[0] == 0 && f.codes[2] == 0);
    CHECK(f.spans[0] == 40 && f.spans[2] == 40);
    CHECK(f.glyphs[0] == 0xDB && f.glyphs[2] == 0xDB);
    CHECK(al == 0x3C); /* final display AL, not the key byte */
    return 0;
}

static int test_message_and_cap(void) {
    fixture f = {0};
    pr_rec_dax_nested_string message = {0};
    pr_rec_dax_retry_io io = io_for(&f);
    uint8_t al = 0;
    unsigned i;
    message.length = 43;
    for (i = 0; i < message.length; ++i) message.bytes[i] = (uint8_t)(i + 1);
    f.final_al = 0x7E;
    CHECK(pr_rec_res_0802_0193(&io, &message, 0x120E, &al) ==
          PR_REC_DAX_RETRY_OK);
    CHECK(f.event_count == 43 && f.events[0] == 'D' &&
          f.events[41] == 'K' && f.events[42] == 'D');
    for (i = 0; i < 40; ++i) {
        CHECK(f.events[i + 1] == 'D');
        CHECK(f.columns[i + 1] == i && f.rows[i + 1] == 24 &&
              f.codes[i + 1] == 0x0E && f.spans[i + 1] == 1 &&
              f.glyphs[i + 1] == i + 1);
    }
    CHECK(al == 0x7E);
    return 0;
}

static int test_boundary_failures(void) {
    unsigned fail;
    for (fail = 1; fail <= 5; ++fail) {
        fixture f = {0};
        pr_rec_dax_nested_string message = {2, {0x11, 0x22}};
        pr_rec_dax_retry_io io = io_for(&f);
        uint8_t al = 0x66;
        f.fail_at = fail;
        CHECK(pr_rec_res_0802_0193(&io, &message, 0x0E, &al) ==
              PR_REC_DAX_RETRY_BOUNDARY);
        CHECK(f.calls == fail && f.event_count == fail && al == 0x66);
    }
    return 0;
}

static int test_invalid_arguments(void) {
    fixture f = {0};
    pr_rec_dax_nested_string message = {0};
    pr_rec_dax_retry_io io = io_for(&f);
    uint8_t al = 0x66;
    CHECK(pr_rec_res_0802_0193(NULL, &message, 0, &al) ==
          PR_REC_DAX_RETRY_ARGUMENT);
    CHECK(pr_rec_res_0802_0193(&io, NULL, 0, &al) ==
          PR_REC_DAX_RETRY_ARGUMENT);
    CHECK(pr_rec_res_0802_0193(&io, &message, 0, NULL) ==
          PR_REC_DAX_RETRY_ARGUMENT);
    io.display_command = NULL;
    CHECK(pr_rec_res_0802_0193(&io, &message, 0, &al) ==
          PR_REC_DAX_RETRY_ARGUMENT);
    io = io_for(&f);
    io.read_key = NULL;
    CHECK(pr_rec_res_0802_0193(&io, &message, 0, &al) ==
          PR_REC_DAX_RETRY_ARGUMENT);
    CHECK(f.calls == 0 && al == 0x66);
    return 0;
}

int main(void) {
    if (test_empty_and_frame() || test_message_and_cap() ||
        test_boundary_failures() || test_invalid_arguments()) return 1;
    puts("resident DAX retry display tests passed");
    return 0;
}
