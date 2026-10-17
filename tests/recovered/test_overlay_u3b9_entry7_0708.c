/* Synthetic tests for the U3B9 +0708 (entry 7) whole-function model. The
 * message literal is a synthetic 30-character stand-in, not the original. */
#include "poolrad/recovered/overlay_u3b9_entry7_0708.h"

#include <stdio.h>
#include <string.h>

static int failures;
#define CHECK(cond) do { if (!(cond)) { fprintf(stderr, "FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); failures++; } } while (0)

static const uint8_t MSG[31] = {30u, 'A','B','C','D','E','F','G','H','I','J','K','L','M','N','O',
                                'P','Q','R','S','T','U','V','W','X','Y','Z','0','1','2','3'};

/* Host far memory: the caller's record at 1000:0100 and one object at 2000:0010. */
#define REC_SEG 0x1000u
#define REC_OFF 0x0100u
#define OBJ_SEG 0x2000u
#define OBJ_OFF 0x0010u

typedef struct {
    char log[32];
    size_t n;
    uint8_t record[PR_U3B9_0708_RECORD_SIZE];
    uint8_t object[16];
    uint16_t copy_seg, copy_off, copy_count;
    uint16_t file_ds, width;
    size_t msg_span;
    uint16_t a[8];                /* the 0x0D71 words in push order */
    int fail_copy, fail_read_at, reads, fail_draw;
    int alias_copy_index, alias_halt_index;   /* -1 = none, otherwise new low byte */
} fake;

static void put(fake *f, char c) {
    if (f->n + 1u < sizeof f->log) { f->log[f->n++] = c; f->log[f->n] = '\0'; }
}

static void set_word(uint8_t *p, uint16_t v) { p[0] = (uint8_t)(v & 0xFFu); p[1] = (uint8_t)(v >> 8); }

static int h_copy(void *ctx, pr_u3b9_0708_frame *fr, uint16_t seg, uint16_t off, uint8_t *dst, uint16_t count) {
    fake *f = ctx;
    put(f, 'C');
    f->copy_seg = seg; f->copy_off = off; f->copy_count = count;
    if (seg == REC_SEG && off == REC_OFF && count == PR_U3B9_0708_RECORD_SIZE) {
        memcpy(dst, f->record, count);
    }
    if (f->alias_copy_index >= 0) {
        fr->index_word = (uint16_t)((fr->index_word & 0xFF00u) | (uint16_t)f->alias_copy_index);
    }
    return f->fail_copy;
}

static int h_00ff(void *ctx, pr_u3b9_0708_frame *fr) { (void)fr; put(ctx, 'E'); return 0; }

static int h_write(void *ctx, pr_u3b9_0708_frame *fr, uint16_t file, const uint8_t *s, size_t span, uint16_t width) {
    fake *f = ctx;
    (void)fr;
    put(f, 'W');
    f->file_ds = file; f->width = width; f->msg_span = span;
    return memcmp(s, MSG, sizeof MSG) == 0 ? 0 : 1;
}

static int h_writeln(void *ctx, pr_u3b9_0708_frame *fr, uint16_t file) {
    (void)fr;
    put(ctx, 'L');
    return file == PR_U3B9_0708_OUTPUT_DS ? 0 : 1;
}

static int h_halt(void *ctx, pr_u3b9_0708_frame *fr) {
    fake *f = ctx;
    put(f, 'H');
    if (f->alias_halt_index >= 0) {
        fr->index_word = (uint16_t)((fr->index_word & 0xFF00u) | (uint16_t)f->alias_halt_index);
    }
    return 0;                                     /* returns: execution reaches +074E */
}

static int h_read(void *ctx, pr_u3b9_0708_frame *fr, uint16_t seg, uint16_t off, uint16_t *value) {
    fake *f = ctx;
    (void)fr;
    put(f, 'R');
    f->reads++;
    if (f->reads == f->fail_read_at) {
        return 1;
    }
    if (seg != OBJ_SEG || off < OBJ_OFF || off + 2u > OBJ_OFF + sizeof f->object) {
        return 1;
    }
    *value = (uint16_t)(f->object[off - OBJ_OFF] | ((uint16_t)f->object[off - OBJ_OFF + 1u] << 8));
    return 0;
}

static int h_draw(void *ctx, pr_u3b9_0708_frame *fr, uint16_t x, uint16_t y, uint16_t k, uint16_t flags,
                  uint16_t s_seg, uint16_t s_off, uint16_t d_seg, uint16_t d_off) {
    fake *f = ctx;
    (void)fr;
    put(f, 'D');
    f->a[0] = x; f->a[1] = y; f->a[2] = k; f->a[3] = flags;
    f->a[4] = s_seg; f->a[5] = s_off; f->a[6] = d_seg; f->a[7] = d_off;
    return f->fail_draw;
}

static int h_flush(void *ctx, pr_u3b9_0708_frame *fr) { (void)fr; put(ctx, 'F'); return 0; }

static void setup(fake *f, pr_u3b9_0708_hooks *h, pr_u3b9_0708_frame *fr, uint16_t index_word) {
    memset(f, 0, sizeof *f);
    f->alias_copy_index = -1; f->alias_halt_index = -1;
    /* slot i (1..8) far pointer at record offsets 8i-2 (offset word) and 8i (segment word) */
    set_word(&f->record[2u * 8u - 2u], OBJ_OFF); set_word(&f->record[2u * 8u], OBJ_SEG);   /* slot 2 */
    set_word(&f->record[5u * 8u - 2u], OBJ_OFF); set_word(&f->record[5u * 8u], OBJ_SEG);   /* slot 5 */
    set_word(&f->record[1u * 8u - 2u], OBJ_OFF); set_word(&f->record[1u * 8u], OBJ_SEG);   /* slot 1 */
    set_word(&f->object[4], 0x0030u);             /* p^[4] */
    set_word(&f->object[6], 0x12FEu);             /* p^[6]: y = 0x1300, AH carried into k/flags */
    {
        pr_u3b9_0708_hooks hh = {f, h_copy, h_00ff, h_write, h_writeln, h_halt, h_read, h_draw, h_flush};
        *h = hh;
    }
    memset(fr, 0xEE, sizeof *fr);                 /* uninitialized-stack stand-in */
    fr->record_seg = REC_SEG; fr->record_off = REC_OFF; fr->index_word = index_word;
}

static const pr_u3b9_0708_state STATE = {0x0040u, 0xA000u};   /* DS:[0x4A20] offset, [0x4A22] segment */

static void test_draw_slot2(void) {
    fake f; pr_u3b9_0708_hooks h; pr_u3b9_0708_frame fr; pr_u3b9_0708_result r;
    setup(&f, &h, &fr, 0x0002u);
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_OK);
    CHECK(strcmp(f.log, "CRRDF") == 0);
    CHECK(f.copy_seg == REC_SEG && f.copy_off == REC_OFF && f.copy_count == 0x42u);
    CHECK(f.a[0] == 0x0032u && f.a[1] == 0x1300u && f.a[2] == 0x1300u && f.a[3] == 0x1301u);
    CHECK(f.a[4] == OBJ_SEG && f.a[5] == OBJ_OFF && f.a[6] == 0xA000u && f.a[7] == 0x0040u);
    CHECK(fr.locals[0] == 0x10u && fr.locals[1] == 0x00u && fr.locals[2] == 0x00u && fr.locals[3] == 0x20u);
    CHECK(r.copied && !r.error_path && r.drawn && r.flushed && r.next_offset == 0x07C0u);
}

static void test_nil_slot_and_word_arg(void) {
    fake f; pr_u3b9_0708_hooks h; pr_u3b9_0708_frame fr; pr_u3b9_0708_result r;
    setup(&f, &h, &fr, 0xAB03u);                  /* slot 3 is nil; AH 0xAB is not read */
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_OK);
    CHECK(strcmp(f.log, "C") == 0 && !r.drawn && r.next_offset == 0x07C0u);
}

static void test_error_path_then_continue(void) {
    fake f; pr_u3b9_0708_hooks h; pr_u3b9_0708_frame fr; pr_u3b9_0708_result r;
    setup(&f, &h, &fr, 0x0005u);                  /* above 3: message, then slot 5 (inside the copy) */
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_OK);
    CHECK(strcmp(f.log, "CEWLHRRDF") == 0);
    CHECK(f.file_ds == 0x8608u && f.width == 0u && f.msg_span == 31u);
    CHECK(r.error_path && r.drawn);
    setup(&f, &h, &fr, 0x0000u);                  /* zero: slot words are locals[2..5] (temp segment, copy[0..1]) */
    fr.locals[2] = 0u; fr.locals[3] = 0u;
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_OK);
    CHECK(strcmp(f.log, "CEWLH") == 0 && !r.drawn);
    setup(&f, &h, &fr, 0x0009u);                  /* beyond the locals image */
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_UNMODELED_FRAME);
    CHECK(strcmp(f.log, "CEWLH") == 0 && r.next_offset == 0x074Eu && r.error_path);
}

static void test_live_frame_aliases(void) {
    fake f; pr_u3b9_0708_hooks h; pr_u3b9_0708_frame fr; pr_u3b9_0708_result r;
    setup(&f, &h, &fr, 0x0000u);
    f.alias_copy_index = 2;                       /* the copy changes [BP+0xA] before +0721 */
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_OK);
    CHECK(strcmp(f.log, "CRRDF") == 0 && !r.error_path);
    setup(&f, &h, &fr, 0x0007u);
    f.alias_halt_index = 1;                       /* 0x899:0x0000 changes it before +074E */
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_OK);
    CHECK(strcmp(f.log, "CEWLHRRDF") == 0 && f.a[4] == OBJ_SEG && f.a[5] == OBJ_OFF);
}

static void test_wrap_and_failures(void) {
    fake f; pr_u3b9_0708_hooks h; pr_u3b9_0708_frame fr; pr_u3b9_0708_result r;
    setup(&f, &h, &fr, 0x0001u);
    set_word(&f.object[4], 0xFFFFu);              /* x wraps to 0x0001 */
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_OK && f.a[0] == 0x0001u);
    setup(&f, &h, &fr, 0x0002u); f.fail_copy = 1;
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_HOST_FAILURE);
    CHECK(!r.copied && r.next_offset == 0x071Cu);
    setup(&f, &h, &fr, 0x0002u); f.fail_read_at = 2;
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_HOST_FAILURE);
    CHECK(r.next_offset == 0x0784u && r.x == 0x0032u && !r.drawn);
    setup(&f, &h, &fr, 0x0002u); f.fail_draw = 1;
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_HOST_FAILURE);
    CHECK(r.next_offset == 0x07ACu && !r.drawn && !r.flushed);
    setup(&f, &h, &fr, 0x0002u); h.resident_464_1b52 = NULL;
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_NEED_HOOK);
    CHECK(r.drawn && !r.flushed && r.next_offset == 0x07B5u);
    setup(&f, &h, &fr, 0x0002u); h.resident_464_00ff = NULL;   /* not reached on a valid index */
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_OK);
    setup(&f, &h, &fr, 0x0004u); h.resident_464_00ff = NULL;
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_NEED_HOOK);
    CHECK(r.next_offset == 0x072Du && !r.error_path);
}

static void test_arguments(void) {
    fake f; pr_u3b9_0708_hooks h; pr_u3b9_0708_frame fr; pr_u3b9_0708_result r;
    static const uint8_t SHORT[30] = {29u};
    setup(&f, &h, &fr, 0x0002u);
    CHECK(pr_u3b9_0708_show(NULL, &STATE, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_ARGUMENT);
    CHECK(pr_u3b9_0708_show(&fr, NULL, MSG, sizeof MSG, &h, &r) == PR_U3B9_0708_ARGUMENT);
    CHECK(pr_u3b9_0708_show(&fr, &STATE, SHORT, sizeof SHORT, &h, &r) == PR_U3B9_0708_ARGUMENT);
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, NULL, &r) == PR_U3B9_0708_ARGUMENT);
    CHECK(pr_u3b9_0708_show(&fr, &STATE, MSG, sizeof MSG, &h, NULL) == PR_U3B9_0708_ARGUMENT);
    CHECK(f.n == 0u);
}

int main(void) {
    test_draw_slot2();
    test_nil_slot_and_word_arg();
    test_error_path_then_continue();
    test_live_frame_aliases();
    test_wrap_and_failures();
    test_arguments();
    if (failures != 0) {
        fprintf(stderr, "%d failure(s)\n", failures);
        return 1;
    }
    puts("u3b9 entry7 0708: all tests passed");
    return 0;
}
