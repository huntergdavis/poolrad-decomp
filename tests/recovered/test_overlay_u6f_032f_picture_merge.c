#include "poolrad/recovered/overlay_u6f_032f_picture_merge.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)

/* One synthetic 64 KiB segment for objects and planes, and a small DS segment. */
static uint8_t g_seg[0x10000];
static uint8_t g_seg2[0x10000];
static uint8_t g_seg3[0x10000];
static uint8_t g_ds[0x6000];                                     /* the DS view: 0x4A1E and 0x5EB2 lie inside */
static pr_u6f_032f_segment g_views[4];
#define DS_SEG 0x0C7C

static void put_word(uint8_t *seg, uint16_t off, uint16_t v) { seg[off] = (uint8_t)v; seg[(uint16_t)(off + 1)] = (uint8_t)(v >> 8); }
static void put_far(uint8_t *seg, uint16_t off, pr_u6f_032f_far f) { put_word(seg, off, f.offset); put_word(seg, (uint16_t)(off + 2), f.segment); }
/* Build an object at `at` with span, aux pointer and data bytes. */
static void make_object(uint8_t *seg, uint16_t at, uint16_t span, pr_u6f_032f_far aux, const uint8_t *data, size_t n) {
    put_word(seg, (uint16_t)(at + 0x11), span); put_far(seg, (uint16_t)(at + 0x13), aux);
    for (size_t i = 0; i < n; ++i) seg[(uint16_t)(at + 0x17 + i)] = data[i];
}
static pr_u6f_032f_far far(uint16_t seg, uint16_t off) { pr_u6f_032f_far f; f.offset = off; f.segment = seg; return f; }

static void reset(pr_u6f_032f_state *s, uint8_t mode) {
    memset(g_seg, 0, sizeof g_seg); memset(g_seg2, 0, sizeof g_seg2); memset(g_seg3, 0, sizeof g_seg3);
    g_views[0].segment = 0x1000; g_views[0].bytes = g_seg; g_views[0].size = sizeof g_seg;
    g_views[1].segment = 0x2000; g_views[1].bytes = g_seg2; g_views[1].size = sizeof g_seg2;
    g_views[2].segment = 0x3000; g_views[2].bytes = g_seg3; g_views[2].size = sizeof g_seg3;
    memset(g_ds, 0, sizeof g_ds); g_ds[0x4A1E] = mode; put_far(g_ds, 0x5EB2, far(0x1000, 0x3000));
    g_views[3].segment = DS_SEG; g_views[3].bytes = g_ds; g_views[3].size = sizeof g_ds;
    memset(s, 0, sizeof *s); s->ds_segment = DS_SEG; s->segments = g_views; s->segment_count = 4;
}

static int test_mode0_and_2(void) {
    pr_u6f_032f_state s; pr_u6f_032f_result r;
    static const uint8_t sd[4] = { 0x01, 0x10, 0xF0, 0x0F }, dd[4] = { 0x02, 0x10, 0x0F, 0xFF };
    static const uint8_t sa[4] = { 0xF0, 0xFF, 0x0F, 0x00 }, da[4] = { 0xFF, 0x0F, 0xFF, 0xFF };
    for (uint8_t mode = 0; mode <= 2; mode += 2) {
        reset(&s, mode);
        make_object(g_seg, 0x0100, 4, far(0x2000, 0x0040), sd, 4);   /* src */
        make_object(g_seg, 0x0200, 9, far(0x2000, 0x0080), dd, 4);   /* dst: its own span is not used */
        memcpy(g_seg2 + 0x40, sa, 4); memcpy(g_seg2 + 0x80, da, 4);
        g_seg[0x0200 + 0x17 + 4] = 0x55; g_seg2[0x84] = 0x55;         /* bytes past the span stay */
        CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_OK);
        CHECK(r.route == 0 && r.src_span == 4 && r.data_bytes == 4 && r.aux_bytes == 4 && r.next_offset == PR_U6F_032F_END_OFFSET);
        CHECK(g_seg[0x217] == 0x03 && g_seg[0x218] == 0x10 && g_seg[0x219] == 0xFF && g_seg[0x21A] == 0xFF && g_seg[0x21B] == 0x55);
        CHECK(g_seg2[0x80] == 0xF0 && g_seg2[0x81] == 0x0F && g_seg2[0x82] == 0x0F && g_seg2[0x83] == 0x00 && g_seg2[0x84] == 0x55);
        CHECK(g_seg[0x117] == 0x01 && g_seg2[0x40] == 0xF0);           /* the source is not written */
    }
    return 0;
}

static int test_mode1(void) {
    pr_u6f_032f_state s; pr_u6f_032f_result r;
    static const uint8_t sd[3] = { 0x80, 0x01, 0x00 }, dd[3] = { 0x01, 0x01, 0x00 };
    reset(&s, 1);
    make_object(g_seg, 0x0100, 3, far(0x2000, 0x0040), sd, 3);
    make_object(g_seg, 0x0200, 3, far(0x2000, 0x0080), dd, 3);
    make_object(g_seg, 0x3000, 9, far(0, 0), NULL, 0);               /* slot-11 first picture: span 9 -> 9 >> 2 = 2 aux bytes */
    memset(g_seg2 + 0x40, 0x0F, 8); memset(g_seg2 + 0x80, 0xFF, 8);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_OK);
    CHECK(r.route == 1 && r.src_span == 3 && r.base_span == 9 && r.data_bytes == 3 && r.aux_bytes == 2);
    CHECK(g_seg[0x217] == 0x81 && g_seg[0x218] == 0x01 && g_seg[0x219] == 0x00 && g_seg[0x21A] == 0x00);
    CHECK(g_seg2[0x80] == 0x0F && g_seg2[0x81] == 0x0F && g_seg2[0x82] == 0xFF);   /* two bytes ANDed, the third untouched */
    /* the auxiliary count comes from the slot-11 picture, not from the source */
    reset(&s, 1);
    make_object(g_seg, 0x0100, 1, far(0x2000, 0x0040), sd, 1);
    make_object(g_seg, 0x0200, 1, far(0x2000, 0x0080), dd, 1);
    make_object(g_seg, 0x3000, 20, far(0, 0), NULL, 0);              /* 20 >> 2 = 5 */
    memset(g_seg2 + 0x40, 0xF0, 8); memset(g_seg2 + 0x80, 0xFF, 8);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_OK && r.data_bytes == 1 && r.aux_bytes == 5);
    CHECK(g_seg2[0x84] == 0xF0 && g_seg2[0x85] == 0xFF);
    return 0;
}

static int test_other_modes(void) {
    pr_u6f_032f_state s; pr_u6f_032f_result r;
    static const uint8_t sd[2] = { 0xFF, 0xFF }, dd[2] = { 0x00, 0x00 };
    for (unsigned mode = 3; mode < 256; ++mode) {
        reset(&s, (uint8_t)mode);
        make_object(g_seg, 0x0100, 2, far(0x2000, 0x0040), sd, 2);
        make_object(g_seg, 0x0200, 2, far(0x2000, 0x0080), dd, 2);
        CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_OK);
        CHECK(r.route == 2 && r.data_bytes == 0 && r.aux_bytes == 0 && g_seg[0x217] == 0 && r.next_offset == PR_U6F_032F_END_OFFSET);
    }
    /* mode 255 is "other" too: only the mode byte is read, so unmapped objects are fine */
    reset(&s, 0xFF);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x7000, 0), far(0x7000, 0), &r) == PR_U6F_032F_OK && r.route == 2 && r.mode == 0xFF);
    /* with no DS view at all the mode read itself is out of bounds */
    reset(&s, 0); s.segment_count = 3;
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_BOUNDS && r.next_offset == 0x0335 && r.data_bytes == 0);
    return 0;
}

static int test_span_zero_runs_whole_segment(void) {
    pr_u6f_032f_state s; pr_u6f_032f_result r;
    /* the unsigned entry test never skips: span 0 means n = 0xFFFF and all 65,536 offsets (with
     * wrap). The source and the slot-11 picture live in segment 0x1000, the destination in 0x2000
     * (so the sweep never rewrites a header it later rereads) and both auxiliary planes in 0x3000. */
    reset(&s, 1);
    make_object(g_seg, 0x0100, 0, far(0x3000, 0x0000), NULL, 0);
    make_object(g_seg2, 0x0200, 0, far(0x3000, 0x8000), NULL, 0);
    make_object(g_seg, 0x3000, 3, far(0, 0), NULL, 0);               /* 3 >> 2 = 0 -> m = 0xFFFF as well */
    memset(g_seg3, 0xFF, 0x8000); memset(g_seg3 + 0x8000, 0x3C, 0x8000);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x2000, 0x0200), &r) == PR_U6F_032F_OK);
    CHECK(r.route == 1 && r.src_span == 0 && r.base_span == 3 && r.data_bytes == 65536u && r.aux_bytes == 65536u);
    /* the OR sweep copied the source segment (shifted by 0x100) into the destination segment:
     * the source header landed at 0x0211.. (same values) and the base span at 0x3111 */
    CHECK(g_seg2[0x0211] == 0 && g_seg2[0x0213] == 0x00 && g_seg2[0x0214] == 0x80 && g_seg2[0x0216] == 0x30 && g_seg2[0x3111] == 3);
    /* the AND loop wrote dst aux 0x8000.. and wrapped into 0x0000..0x7FFF: every plane byte is now 0x3C */
    CHECK(g_seg3[0x8000] == 0x3C && g_seg3[0x0000] == 0x3C && g_seg3[0x7FFF] == 0x3C && g_seg3[0xFFFF] == 0x3C);
    CHECK(g_seg[0x0111] == 0 && g_seg[0x0116] == 0x30 && g_seg[0x3011] == 3);   /* the source segment is never written */
    return 0;
}

static int test_same_segment_sweep_propagates(void) {
    pr_u6f_032f_state s; pr_u6f_032f_result r;
    /* both objects in one segment, span 0: every byte written at +0x100 becomes the source of a
     * later step, so values travel through the whole segment across the wrap. The base span (3)
     * reaches the source span word, and the destination's auxiliary offset (0x8000) reaches the
     * source's auxiliary offset, so the AND loop then ANDs the plane with itself (no change). The
     * engine does exactly this; the model reproduces it. */
    reset(&s, 1);
    make_object(g_seg, 0x0100, 0, far(0x3000, 0x0000), NULL, 0);
    make_object(g_seg, 0x0200, 0, far(0x3000, 0x8000), NULL, 0);
    make_object(g_seg, 0x3000, 3, far(0, 0), NULL, 0);
    memset(g_seg3, 0xFF, 0x8000); memset(g_seg3 + 0x8000, 0x3C, 0x8000);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_OK);
    CHECK(r.data_bytes == 65536u && r.aux_bytes == 65536u);
    CHECK(g_seg[0x0111] == 3 && g_seg[0x0114] == 0x80);             /* propagated through the wrap */
    CHECK(g_seg3[0x0000] == 0xFF && g_seg3[0x8000] == 0x3C);        /* the plane ANDed with itself */
    return 0;
}

static int test_alias_and_wrap(void) {
    pr_u6f_032f_state s; pr_u6f_032f_result r;
    static const uint8_t d[3] = { 0xA5, 0x5A, 0x0F };
    /* src = dst: OR and AND with itself leave the bytes unchanged */
    reset(&s, 0);
    make_object(g_seg, 0x0100, 3, far(0x2000, 0x0040), d, 3); memcpy(g_seg2 + 0x40, d, 3);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0100), &r) == PR_U6F_032F_OK);
    CHECK(memcmp(g_seg + 0x117, d, 3) == 0 && memcmp(g_seg2 + 0x40, d, 3) == 0 && r.data_bytes == 3);
    /* overlapping auxiliary planes: dst aux = src aux + 1, processed in offset order */
    reset(&s, 2);
    { static const uint8_t seq[3] = { 0x01, 0x02, 0x04 };
      make_object(g_seg, 0x0100, 3, far(0x2000, 0x0040), seq, 3);
      make_object(g_seg, 0x0200, 3, far(0x2000, 0x0041), seq, 3);   /* dst aux = src aux + 1 */
      g_seg2[0x40] = 0x0F; g_seg2[0x41] = 0xF3; g_seg2[0x42] = 0xFF; g_seg2[0x43] = 0xFF;
      CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_OK);
      /* i=0: aux[0x41] &= aux[0x40] (0xF3 & 0x0F = 0x03); i=1: aux[0x42] &= aux[0x41] (now 0x03); i=2: aux[0x43] &= aux[0x42] (0x03) */
      CHECK(g_seg2[0x41] == 0x03 && g_seg2[0x42] == 0x03 && g_seg2[0x43] == 0x03); }
    /* offset wrap: an object at 0xFFF0 has its data at 0x0007.. */
    reset(&s, 0);
    { static const uint8_t one[2] = { 0x11, 0x22 };
      put_word(g_seg, 0x0001, 2); put_far(g_seg, 0x0003, far(0x2000, 0xFFFF)); g_seg[0x0007] = one[0]; g_seg[0x0008] = one[1];   /* src header at 0xFFF0 wraps */
      make_object(g_seg, 0x0200, 2, far(0x2000, 0x0080), one, 2);
      g_seg2[0xFFFF] = 0x0F; g_seg2[0x0000] = 0xF0; g_seg2[0x80] = 0xFF; g_seg2[0x81] = 0xFF;
      CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0xFFF0), far(0x1000, 0x0200), &r) == PR_U6F_032F_OK && r.src_span == 2);
      CHECK(g_seg[0x217] == 0x11 && g_seg[0x218] == 0x22 && g_seg2[0x80] == 0x0F && g_seg2[0x81] == 0xF0); }
    return 0;
}

static int test_or_pass_rewrites_slot11_pointer(void) {
    pr_u6f_032f_state s; pr_u6f_032f_result r;
    /* mode 1 with the destination object inside the DS segment so that its data bytes cover
     * DS:0x5EB2..0x5EB5: the OR pass rewrites the slot-11 far pointer, and the AND pass that
     * follows (+0412) must use the rewritten pointer, as the engine rereads it there. */
    static const uint8_t src_data[6] = { 0x00, 0x00, 0x00, 0x00, 0x20, 0x00 };   /* ORs 0x20 into the pointer's segment high byte... */
    static const uint8_t dst_data[6] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    reset(&s, 1);
    make_object(g_seg, 0x0100, 6, far(0x3000, 0x0040), src_data, 6);
    /* destination at DS:0x5E9B: its data starts at 0x5E9B + 0x17 = 0x5EB2, exactly the slot-11 pointer */
    make_object(g_ds, 0x5E9B, 6, far(0x3000, 0x0080), dst_data, 6);
    put_far(g_ds, 0x5EB2, far(0x1000, 0x3000));                  /* pointer (0x3000, 0x1000): data[0..3] = 00 30 00 10 */
    g_ds[0x5EB2 + 4] = 0x00; g_ds[0x5EB2 + 5] = 0x00;
    /* two candidate base pictures: segment 0x1000 (span 20 -> 5 aux bytes) and segment 0x3000 (span 8 -> 2) */
    make_object(g_seg, 0x3000, 20, far(0, 0), NULL, 0);
    make_object(g_seg3, 0x3000, 8, far(0, 0), NULL, 0);
    memset(g_seg3 + 0x40, 0x0F, 8); memset(g_seg3 + 0x80, 0xFF, 8);
    /* the OR pass: src data[2] = 0x00 and data[3] = 0x00 leave the offset/segment bytes, except
     * src_data[4..5] land past the pointer; so make the pointer change through data[3]: */
    g_seg[0x0117 + 3] = 0x20;                                     /* src data[3] = 0x20: segment 0x1000 | 0x2000 = 0x3000 */
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(DS_SEG, 0x5E9B), &r) == PR_U6F_032F_OK);
    CHECK(r.route == 1 && r.data_bytes == 6);
    CHECK(g_ds[0x5EB5] == 0x30);                                  /* the pointer's segment is now 0x3000 */
    CHECK(r.base.segment == 0x3000 && r.base.offset == 0x3000 && r.base_span == 8 && r.aux_bytes == 2);   /* the rewritten pointer was used */
    CHECK(g_seg3[0x80] == 0x0F && g_seg3[0x81] == 0x0F && g_seg3[0x82] == 0xFF);
    /* the same layout in mode 0 reads no DS pointer at all */
    reset(&s, 0);
    make_object(g_seg, 0x0100, 6, far(0x3000, 0x0040), src_data, 6);
    make_object(g_ds, 0x5E9B, 6, far(0x3000, 0x0080), dst_data, 6);
    put_far(g_ds, 0x5EB2, far(0x1000, 0x3000)); g_seg[0x0117 + 3] = 0x20;
    memset(g_seg3 + 0x40, 0x0F, 8); memset(g_seg3 + 0x80, 0xFF, 8);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(DS_SEG, 0x5E9B), &r) == PR_U6F_032F_OK && r.route == 0 && r.aux_bytes == 6 && g_ds[0x5EB5] == 0x30);
    return 0;
}

static int test_bounds_and_arguments(void) {
    pr_u6f_032f_state s; pr_u6f_032f_result r;
    static const uint8_t d[2] = { 0x01, 0x02 };
    /* an unmapped auxiliary segment: the first data OR stands, the AND stops with BOUNDS */
    reset(&s, 0);
    make_object(g_seg, 0x0100, 2, far(0x5000, 0x0040), d, 2);
    make_object(g_seg, 0x0200, 2, far(0x2000, 0x0080), d, 2);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_BOUNDS);
    CHECK(r.data_bytes == 1 && r.aux_bytes == 0 && r.next_offset == 0x0385 && g_seg[0x217] == 0x01);
    /* a view shorter than the object: the span read fails */
    reset(&s, 0); g_views[0].size = 0x110;
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_BOUNDS && r.next_offset == 0x0346 && r.data_bytes == 0);
    /* mode 1: the slot-11 picture unmapped: the OR loop stands, the AND loop never starts */
    reset(&s, 1);
    make_object(g_seg, 0x0100, 2, far(0x2000, 0x0040), d, 2);
    make_object(g_seg, 0x0200, 2, far(0x2000, 0x0080), d, 2);
    put_far(g_ds, 0x5EB2, far(0x5000, 0));
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_BOUNDS && r.next_offset == 0x0416 && r.data_bytes == 2 && r.aux_bytes == 0 && r.base.segment == 0x5000);
    /* mode 1 with the DS view too short for 0x5EB2: BOUNDS at the pointer read itself */
    reset(&s, 1); g_views[3].size = 0x5000;
    make_object(g_seg, 0x0100, 2, far(0x2000, 0x0040), d, 2);
    make_object(g_seg, 0x0200, 2, far(0x2000, 0x0080), d, 2);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x1000, 0x0200), &r) == PR_U6F_032F_BOUNDS && r.next_offset == 0x0412 && r.data_bytes == 2);
    /* a destination segment that is not mapped */
    reset(&s, 2);
    make_object(g_seg, 0x0100, 2, far(0x2000, 0x0040), d, 2);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0x0100), far(0x4000, 0x0200), &r) == PR_U6F_032F_BOUNDS && r.next_offset == 0x035D);
    /* arguments */
    reset(&s, 0);
    CHECK(pr_u6f_032f_picture_merge(NULL, far(0x1000, 0), far(0x1000, 0), &r) == PR_U6F_032F_ARGUMENT);
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0), far(0x1000, 0), NULL) == PR_U6F_032F_ARGUMENT);
    s.segments = NULL; s.segment_count = 1;
    CHECK(pr_u6f_032f_picture_merge(&s, far(0x1000, 0), far(0x1000, 0), &r) == PR_U6F_032F_ARGUMENT);
    return 0;
}

int main(void) {
    int rc = 0;
    rc |= test_mode0_and_2();
    rc |= test_mode1();
    rc |= test_other_modes();
    rc |= test_span_zero_runs_whole_segment();
    rc |= test_same_segment_sweep_propagates();
    rc |= test_or_pass_rewrites_slot11_pointer();
    rc |= test_alias_and_wrap();
    rc |= test_bounds_and_arguments();
    if (rc == 0) {
        puts("overlay U6F +032F picture-merge tests passed");
    }
    return rc;
}
