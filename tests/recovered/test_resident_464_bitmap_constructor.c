#include "poolrad/recovered/resident_464_bitmap_constructor.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

typedef struct {
    uint8_t video[2], block[17];
    unsigned video_reads, allocations, fills;
    unsigned fail_allocate_at, fail_fill_at;
    uint16_t sizes[2];
    pr_r464_block_object *attached;
    char events[32];
    size_t event_count;
    pr_r464_ctor_report ctor_report;
    pr_r464_ctor_env ctor_env;
} fixture;

static void event(fixture *f, char c) {
    if (f->event_count < sizeof f->events - 1) {
        f->events[f->event_count++] = c;
        f->events[f->event_count] = '\0';
    }
}

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int read_video(void *ctx, uint8_t *value) {
    fixture *f = ctx;
    event(f, 'V');
    *value = f->video[f->video_reads < 2 ? f->video_reads : 1];
    ++f->video_reads;
    return 0;
}

static int allocate(void *ctx, uint16_t size, pr_r464_block_object **out) {
    fixture *f = ctx;
    pr_r464_block_object *o;
    event(f, 'A');
    f->sizes[f->allocations++] = size;
    if (f->allocations == f->fail_allocate_at) return 7;
    o = malloc(sizeof *o);
    if (!o) return 8;
    o->capacity = size;
    o->bytes = malloc(size ? size : 1u);
    if (!o->bytes) { free(o); return 8; }
    memset(o->bytes, 0xAA, size ? size : 1u);
    *out = o;
    return 0;
}

static int fill_zero(void *ctx, pr_r464_block_object *o, uint16_t size) {
    fixture *f = ctx;
    event(f, 'F');
    ++f->fills;
    if (f->fills == f->fail_fill_at) return 9;
    if (size) memset(o->bytes, 0, size);
    return 0;
}

static int attach_aux(void *ctx, pr_r464_block_object *primary,
                      pr_r464_block_object *auxiliary) {
    fixture *f = ctx;
    event(f, 'T');
    f->attached = auxiliary;
    /* A synthetic far-pointer token, supplied by the host mapping hook. */
    memset(primary->bytes + 0x13, 0x5A, 4);
    return 0;
}

static pr_r464_ctor_env env_for(fixture *f) {
    pr_r464_ctor_env e = {f, read_video, allocate, fill_zero, attach_aux};
    return e;
}

static void init(fixture *f) {
    size_t i;
    memset(f, 0, sizeof *f);
    for (i = 0; i < 17; ++i) f->block[i] = (uint8_t)(i + 1);
    f->ctor_env = env_for(f);
}

static void dispose(pr_r464_block_object *o) {
    if (!o) return;
    free(o->bytes);
    free(o);
}

static int test_primary_and_auxiliary(void) {
    fixture f;
    pr_r464_ctor_report r;
    pr_r464_block_object *primary = NULL;
    init(&f);
    CHECK(pr_r464_bitmap_construct(&f.ctor_env, 3, 4, 2, 2,
                                   &primary, &r) == PR_R464_CTOR_OK);
    CHECK(strcmp(f.events, "VAF") == 0 && r.call_count == 3 &&
          r.span_word == 24 && r.data_size == 48 && r.primary_size == 71 &&
          f.sizes[0] == 71 && f.video_reads == 1);
    CHECK(primary && word(primary->bytes) == 3 &&
          word(primary->bytes + 2) == 4 &&
          primary->bytes[8] == 2 && word(primary->bytes + 0x11) == 24 &&
          primary->bytes[0x13] == 0);
    dispose(primary);

    init(&f); primary = NULL;
    CHECK(pr_r464_bitmap_construct(&f.ctor_env, 0, 4, 2, 1,
                                   &primary, &r) == PR_R464_CTOR_OK);
    CHECK(strcmp(f.events, "VAFVATF") == 0 &&
          r.span_word == 0 && r.data_size == 0 &&
          r.primary_size == 23 && r.auxiliary_size == 0 &&
          f.sizes[0] == 23 && f.sizes[1] == 0);
    dispose(r.auxiliary); dispose(primary);

    init(&f); primary = NULL;
    f.video[0] = 0; f.video[1] = 1;
    CHECK(pr_r464_bitmap_construct(&f.ctor_env, 3, 4, 2, 3,
                                   &primary, &r) == PR_R464_CTOR_OK);
    CHECK(strcmp(f.events, "VAFVATF") == 0 && r.call_count == 7 &&
          r.span_word == 24 && r.data_size == 48 &&
          r.auxiliary_size == 12 && f.sizes[1] == 12 &&
          r.video_main == 0 && r.video_aux == 1);
    CHECK(r.auxiliary == f.attached &&
          primary->bytes[0x13] == 0x5A &&
          r.auxiliary->bytes[0] == 0);
    dispose(r.auxiliary); dispose(primary);

    init(&f); primary = NULL;
    f.video[0] = 2; f.video[1] = 2;
    CHECK(pr_r464_bitmap_construct(&f.ctor_env, 3, 4, 2, 1,
                                   &primary, &r) == PR_R464_CTOR_OK);
    CHECK(r.span_word == 48 && r.data_size == 96 &&
          r.primary_size == 119 && r.auxiliary_size == 96 &&
          f.sizes[0] == 119 && f.sizes[1] == 96);
    dispose(r.auxiliary); dispose(primary);
    return 0;
}

static int test_bounds_and_stops(void) {
    fixture f;
    pr_r464_ctor_report r;
    pr_r464_block_object *primary = NULL;
    init(&f);
    f.video[0] = 1;
    CHECK(pr_r464_bitmap_construct(&f.ctor_env, 0xFFFF, 2, 2, 0,
                                   &primary, &r) == PR_R464_CTOR_BOUNDS);
    CHECK(strcmp(f.events, "VA") == 0 && r.span_word == 0xFFF8 &&
          r.data_size == 0xFFF0 && r.primary_size == 7 &&
          r.stopped_at == PR_R464_CTOR_FILL_MAIN &&
          primary == r.primary);
    dispose(primary);

    init(&f); primary = NULL;
    f.ctor_env.fill_zero = NULL;
    CHECK(pr_r464_bitmap_construct(&f.ctor_env, 3, 4, 1, 0,
                                   &primary, &r) == PR_R464_CTOR_HOOK);
    CHECK(strcmp(f.events, "VA") == 0 && r.call_count == 2 &&
          r.stopped_at == PR_R464_CTOR_FILL_MAIN && primary);
    dispose(primary);

    init(&f); primary = NULL;
    f.fail_allocate_at = 2;
    CHECK(pr_r464_bitmap_construct(&f.ctor_env, 3, 4, 1, 1,
                                   &primary, &r) == PR_R464_CTOR_CALLBACK);
    CHECK(strcmp(f.events, "VAFVA") == 0 && r.callback_status == 7 &&
          r.stopped_at == PR_R464_CTOR_ALLOC_AUX && primary &&
          word(primary->bytes + 0x11) == 24 && r.auxiliary == NULL);
    dispose(primary);

    init(&f); primary = NULL;
    f.ctor_env.attach_aux = NULL;
    CHECK(pr_r464_bitmap_construct(&f.ctor_env, 3, 4, 1, 1,
                                   &primary, &r) == PR_R464_CTOR_HOOK);
    CHECK(strcmp(f.events, "VAFVA") == 0 &&
          r.stopped_at == PR_R464_CTOR_ATTACH_AUX &&
          r.auxiliary && primary->bytes[0x13] == 0);
    dispose(r.auxiliary); dispose(primary);

    init(&f); primary = NULL;
    f.fail_fill_at = 2;
    CHECK(pr_r464_bitmap_construct(&f.ctor_env, 3, 4, 1, 1,
                                   &primary, &r) == PR_R464_CTOR_CALLBACK);
    CHECK(strcmp(f.events, "VAFVATF") == 0 &&
          r.stopped_at == PR_R464_CTOR_FILL_AUX &&
          r.callback_status == 9 && r.auxiliary == f.attached);
    dispose(r.auxiliary); dispose(primary);
    return 0;
}

static int read_block(void *ctx, const char *name, const char *extension,
                      uint8_t id, uint16_t *size, const uint8_t **block) {
    fixture *f = ctx;
    event(f, 'R');
    if (strcmp(name, "N") || strcmp(extension, "test") || id != 3) return 1;
    *size = 17;
    *block = f->block;
    return 0;
}

static int make_from_wrapper(void *ctx, uint16_t word_0, uint16_t word_2,
                             uint8_t byte_8, uint8_t arg_c,
                             pr_r464_block_object **destination) {
    fixture *f = ctx;
    return pr_r464_bitmap_construct(&f->ctor_env, word_0, word_2, byte_8,
                                    arg_c, destination, &f->ctor_report);
}

static int process_block(void *ctx, const uint8_t *block, uint16_t size,
                         uint16_t data_offset, uint8_t arg_c, uint8_t arg_a,
                         pr_r464_block_object *object) {
    fixture *f = ctx;
    event(f, 'P');
    if (block != f->block || size != 17 ||
        data_offset != 0x11 || arg_c || arg_a != 4 || !object) return 2;
    return 0;
}

static void release_block(void *ctx, const uint8_t *block, uint16_t size) {
    fixture *f = ctx;
    (void)block;
    (void)size;
    event(f, 'L');
}

static int flush(void *ctx) { event(ctx, 'K'); return 0; }

static int test_wrapper_adapter(void) {
    fixture f;
    pr_r464_wrapper_env e;
    pr_r464_wrapper_report r;
    pr_r464_block_object *primary = NULL;
    uint8_t name[256] = {1, 'N'};
    init(&f);
    e.ctx = &f; e.extension = "test";
    e.read_block = read_block; e.release_prior = NULL;
    e.make_object = make_from_wrapper;
    e.process_block = process_block;
    e.release_block = release_block; e.flush_899_0104 = flush;
    CHECK(pr_r464_block_wrapper(&e, name, 0xAA03, 4, 0,
                                &primary, &r) == PR_R464_WRAPPER_OK);
    CHECK(strcmp(f.events, "RVAFPLK") == 0 && primary &&
          memcmp(primary->bytes, f.block, 17) == 0 &&
          word(primary->bytes + 0x11) == 0x1406 &&
          f.ctor_report.call_count == 3 && r.held_block == NULL);
    dispose(primary);
    return 0;
}

int main(void) {
    if (test_primary_and_auxiliary() || test_bounds_and_stops() ||
        test_wrapper_adapter()) return 1;
    return 0;
}
