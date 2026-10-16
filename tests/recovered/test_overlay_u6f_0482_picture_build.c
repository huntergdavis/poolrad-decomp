#include "poolrad/recovered/overlay_u6f_0482_picture_build.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* Synthetic literals and tables of the originals' lengths (the real bytes stay private). */
static const char k_prefix_a_s[] = "AAAAA";
static const char k_prefix_b_s[] = "BBBBB";
#define k_prefix_a ((const uint8_t *)k_prefix_a_s)
#define k_prefix_b ((const uint8_t *)k_prefix_b_s)
static const uint8_t k_letters[4] = { 'W', 'X', 'Y', 'Z' };            /* DS:0x08C5.. (synthetic) */
static const uint8_t k_index[9] = { 0, 1, 2, 3, 4, 6, 7, 0, 0 };       /* DS:0x3CF5.. (synthetic; entries 1..6 used) */

typedef struct {
    char log[64];
    int fail_push, fail_load, fail_merge, fail_recolor, fail_release, fail_restore, fail_flush;
    int fail_load_at;                         /* 0 = never, 1 = first load, 2 = second */
    uint8_t push_byte;
    uint8_t names[2][16]; uint8_t p3[2]; uint8_t slots[2]; size_t load_count;
    uint8_t merge_slots[2]; uint8_t merge_which[2]; size_t merge_count;
    uint8_t recolor_a[2][16]; uint8_t recolor_b[2][16]; uint8_t recolor_slots[2]; uint8_t recolor_which[2]; size_t recolor_count;
    uint8_t released_slot;
} fake_t;

static void put(fake_t *f, const char *s) { if (strlen(f->log) + strlen(s) < sizeof f->log) strcat(f->log, s); }
static int h_push(void *c, uint8_t b) { fake_t *f = c; put(f, "S"); f->push_byte = b; return f->fail_push; }
static int h_load(void *c, const uint8_t *n, size_t span, uint8_t p3, uint8_t slot) {
    fake_t *f = c; put(f, "L");
    if (f->load_count < 2) { memcpy(f->names[f->load_count], n, span < 16 ? span : 16); f->p3[f->load_count] = p3; f->slots[f->load_count] = slot; }
    f->load_count++;
    if (f->fail_load_at == (int)f->load_count) return 1;
    return f->fail_load;
}
static int h_merge(void *c, uint8_t slot, uint8_t which) { fake_t *f = c; put(f, "M"); if (f->merge_count < 2) { f->merge_slots[f->merge_count] = slot; f->merge_which[f->merge_count] = which; } f->merge_count++; return f->fail_merge; }
static int h_recolor(void *c, const uint8_t *a, const uint8_t *b, uint8_t slot, uint8_t which) {
    fake_t *f = c; put(f, "R");
    if (f->recolor_count < 2) { memcpy(f->recolor_a[f->recolor_count], a, 16); memcpy(f->recolor_b[f->recolor_count], b, 16); f->recolor_slots[f->recolor_count] = slot; f->recolor_which[f->recolor_count] = which; }
    f->recolor_count++; return f->fail_recolor;
}
static int h_release(void *c, uint8_t slot) { fake_t *f = c; put(f, "E"); f->released_slot = slot; return f->fail_release; }
static int h_restore(void *c) { fake_t *f = c; put(f, "A"); return f->fail_restore; }
static int h_flush(void *c) { fake_t *f = c; put(f, "F"); return f->fail_flush; }

static uint8_t g_record[0x120];

static void reset(fake_t *f, pr_u6f_0482_hooks *h) {
    memset(f, 0, sizeof *f); memset(h, 0, sizeof *h); h->context = f;
    h->selector_push = h_push; h->load_picture = h_load; h->merge_picture = h_merge; h->recolor_picture = h_recolor;
    h->release_slot = h_release; h->restore_archive_number = h_restore; h->flush_keys = h_flush;
    memset(g_record, 0xEE, sizeof g_record);
    g_record[0xBD] = 0x21; g_record[0xBE] = 0x42; g_record[0xBF] = 5; g_record[0xC0] = 2;   /* letter index 2 -> 'Y' */
    g_record[0xC1] = 0x1A; g_record[0xC2] = 0x2B; g_record[0xC3] = 0x3C; g_record[0xC4] = 0x4D; g_record[0xC5] = 0x5E; g_record[0xC6] = 0x6F;
}

static int pascal_equals(const uint8_t *p, const char *s) { size_t n = strlen(s); return p[0] == n && memcmp(p + 1, s, n) == 0; }

#define CALL(flag, hp, rp) pr_u6f_0482_picture_build((flag), g_record, sizeof g_record, k_letters, sizeof k_letters, k_index, sizeof k_index, k_prefix_a, 5, k_prefix_b, 5, (hp), (rp))

static int test_recolor_path(void) {
    fake_t f; pr_u6f_0482_hooks h; pr_u6f_0482_result r;
    static const uint8_t identity[16] = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15 };
    static const uint8_t expect_b[16] = { 0, 0xA, 0xB, 0xC, 0xD, 5, 0xE, 0xF, 8, 1, 2, 3, 4, 13, 5, 6 };   /* low nibbles at 1,2,3,4,6,7; high at +8 */
    reset(&f, &h);
    CHECK(CALL(1, &h, &r) == PR_U6F_0482_OK);
    CHECK(strcmp(f.log, "SLLMMRREAF") == 0);
    CHECK(f.push_byte == 3 && r.letter == 'Y');
    CHECK(pascal_equals(f.names[0], "AAAAAY") && f.p3[0] == 0x21 && f.slots[0] == 0x0B);
    CHECK(pascal_equals(f.names[1], "BBBBBY") && f.p3[1] == 0x42 && f.slots[1] == 5);
    CHECK(f.merge_slots[0] == 5 && f.merge_which[0] == 0 && f.merge_slots[1] == 5 && f.merge_which[1] == 1);
    CHECK(memcmp(f.recolor_a[0], identity, 16) == 0 && memcmp(f.recolor_b[0], expect_b, 16) == 0);
    CHECK(memcmp(f.recolor_a[1], identity, 16) == 0 && memcmp(f.recolor_b[1], expect_b, 16) == 0);
    CHECK(f.recolor_slots[0] == 5 && f.recolor_which[0] == 0 && f.recolor_slots[1] == 5 && f.recolor_which[1] == 1);
    CHECK(f.released_slot == 0x0B);
    CHECK(r.loads == 2 && r.merges == 2 && r.recolors == 2 && r.released == 1 && r.restored == 1 && r.flushed == 1);
    CHECK(memcmp(r.map_a, identity, 16) == 0 && memcmp(r.map_b, expect_b, 16) == 0 && r.next_offset == PR_U6F_0482_END_OFFSET);
    /* only the low byte of the word argument matters; any non-zero value recolors */
    reset(&f, &h); CHECK(CALL(0x80, &h, &r) == PR_U6F_0482_OK && r.recolors == 2);
    return 0;
}

static int test_plain_path(void) {
    fake_t f; pr_u6f_0482_hooks h; pr_u6f_0482_result r;
    reset(&f, &h);
    CHECK(CALL(0, &h, &r) == PR_U6F_0482_OK);
    CHECK(strcmp(f.log, "SLLMMEAF") == 0 && r.recolors == 0 && r.loads == 2 && r.merges == 2 && r.released == 1 && r.flushed == 1);
    /* the recolor path's record bytes and the index table are not read here */
    reset(&f, &h);
    CHECK(pr_u6f_0482_picture_build(0, g_record, 0xC1, k_letters, sizeof k_letters, k_index, 2, k_prefix_a, 5, k_prefix_b, 5, &h, &r) == PR_U6F_0482_OK);
    /* letter index 0 and the last table entry */
    reset(&f, &h); g_record[0xC0] = 0; CHECK(CALL(0, &h, &r) == PR_U6F_0482_OK && r.letter == 'W' && pascal_equals(f.names[1], "BBBBBW"));
    reset(&f, &h); g_record[0xC0] = 3; CHECK(CALL(0, &h, &r) == PR_U6F_0482_OK && r.letter == 'Z');
    return 0;
}

static int test_boundaries_and_arguments(void) {
    fake_t f; pr_u6f_0482_hooks h; pr_u6f_0482_result r;
    /* each boundary without its callback, in reach order */
    reset(&f, &h); h.selector_push = NULL;
    CHECK(CALL(1, &h, &r) == PR_U6F_0482_NEED_HOOK && r.next_offset == 0x048C && strcmp(f.log, "") == 0);
    reset(&f, &h); h.load_picture = NULL;
    CHECK(CALL(1, &h, &r) == PR_U6F_0482_NEED_HOOK && r.next_offset == 0x04D8 && strcmp(f.log, "S") == 0 && r.letter == 'Y');
    reset(&f, &h); h.merge_picture = NULL;
    CHECK(CALL(1, &h, &r) == PR_U6F_0482_NEED_HOOK && r.next_offset == 0x0546 && r.loads == 2);
    reset(&f, &h); h.recolor_picture = NULL;
    CHECK(CALL(1, &h, &r) == PR_U6F_0482_NEED_HOOK && r.next_offset == 0x062A && r.merges == 2 && r.map_b[1] == 0xA);
    reset(&f, &h); h.recolor_picture = NULL;                     /* not reached without the flag */
    CHECK(CALL(0, &h, &r) == PR_U6F_0482_OK);
    reset(&f, &h); h.release_slot = NULL;
    CHECK(CALL(1, &h, &r) == PR_U6F_0482_NEED_HOOK && r.next_offset == 0x065F && r.recolors == 2);
    reset(&f, &h); h.restore_archive_number = NULL;
    CHECK(CALL(0, &h, &r) == PR_U6F_0482_NEED_HOOK && r.next_offset == 0x0664 && r.released == 1);
    reset(&f, &h); h.flush_keys = NULL;
    CHECK(CALL(0, &h, &r) == PR_U6F_0482_NEED_HOOK && r.next_offset == 0x0669 && r.restored == 1 && r.flushed == 0);
    /* host failures keep the earlier effects */
    reset(&f, &h); f.fail_load_at = 2;
    CHECK(CALL(1, &h, &r) == PR_U6F_0482_HOST_FAILURE && r.next_offset == 0x0520 && r.loads == 1 && r.merges == 0 && strcmp(f.log, "SLL") == 0);
    reset(&f, &h); f.fail_merge = 1;
    CHECK(CALL(1, &h, &r) == PR_U6F_0482_HOST_FAILURE && r.next_offset == 0x0546 && r.loads == 2 && r.merges == 0);
    reset(&f, &h); f.fail_recolor = 1;
    CHECK(CALL(1, &h, &r) == PR_U6F_0482_HOST_FAILURE && r.next_offset == 0x062A && r.merges == 2 && r.recolors == 0);
    reset(&f, &h); f.fail_flush = 1;
    CHECK(CALL(0, &h, &r) == PR_U6F_0482_HOST_FAILURE && r.restored == 1 && r.flushed == 0 && strcmp(f.log, "SLLMMEAF") == 0);
    reset(&f, &h); f.fail_push = 1;
    CHECK(CALL(0, &h, &r) == PR_U6F_0482_HOST_FAILURE && r.next_offset == 0x048C && r.loads == 0);
    /* arguments: record spans, table spans, literal lengths, nulls */
    reset(&f, &h);
    CHECK(pr_u6f_0482_picture_build(0, g_record, 0xC0, k_letters, 4, k_index, 9, k_prefix_a, 5, k_prefix_b, 5, &h, &r) == PR_U6F_0482_ARGUMENT && strcmp(f.log, "") == 0);
    CHECK(pr_u6f_0482_picture_build(0, g_record, 0xC1, k_letters, 4, k_index, 9, k_prefix_a, 4, k_prefix_b, 5, &h, &r) == PR_U6F_0482_ARGUMENT);
    CHECK(pr_u6f_0482_picture_build(0, g_record, 0xC1, k_letters, 4, k_index, 9, k_prefix_a, 5, k_prefix_b, 6, &h, &r) == PR_U6F_0482_ARGUMENT);
    CHECK(pr_u6f_0482_picture_build(0, g_record, 0xC1, k_letters, 4, k_index, 9, k_prefix_a, 5, k_prefix_b, 5, NULL, &r) == PR_U6F_0482_ARGUMENT);
    CHECK(pr_u6f_0482_picture_build(0, g_record, 0xC1, k_letters, 4, k_index, 9, k_prefix_a, 5, k_prefix_b, 5, &h, NULL) == PR_U6F_0482_ARGUMENT);
    CHECK(strcmp(f.log, "") == 0);
    /* the letter index past the table: after the push, before any load */
    reset(&f, &h); g_record[0xC0] = 4;
    CHECK(CALL(0, &h, &r) == PR_U6F_0482_ARGUMENT && r.next_offset == 0x04B4 && strcmp(f.log, "S") == 0 && r.loads == 0);
    /* recolor spans: a record too short for 0xC6, a short index table, an index above 7: after the merges, before any remap */
    reset(&f, &h);
    CHECK(pr_u6f_0482_picture_build(1, g_record, 0xC6, k_letters, 4, k_index, 9, k_prefix_a, 5, k_prefix_b, 5, &h, &r) == PR_U6F_0482_ARGUMENT && r.next_offset == 0x059F && r.merges == 2 && r.recolors == 0);
    reset(&f, &h);
    CHECK(pr_u6f_0482_picture_build(1, g_record, sizeof g_record, k_letters, 4, k_index, 6, k_prefix_a, 5, k_prefix_b, 5, &h, &r) == PR_U6F_0482_ARGUMENT && r.next_offset == 0x059F);
    { static const uint8_t bad_index[9] = { 0, 1, 2, 3, 4, 6, 8, 0, 0 }; reset(&f, &h);
      CHECK(pr_u6f_0482_picture_build(1, g_record, sizeof g_record, k_letters, 4, bad_index, 9, k_prefix_a, 5, k_prefix_b, 5, &h, &r) == PR_U6F_0482_ARGUMENT && r.next_offset == 0x059F && r.recolors == 0 && strcmp(f.log, "SLLMM") == 0); }
    return 0;
}

int main(void) {
    int rc = 0;
    rc |= test_recolor_path();
    rc |= test_plain_path();
    rc |= test_boundaries_and_arguments();
    if (rc == 0) {
        puts("overlay U6F +0482 picture-build tests passed");
    }
    return rc;
}
