#include "poolrad/recovered/resident_464_block_wrapper.h"
#include "poolrad/recovered/resident_ecl_dax_block.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr, "%s:%d: %s\n", \
    __FILE__, __LINE__, #x); return 1; } } while (0)

typedef struct {
    uint8_t block[24];
    uint8_t archive[40];
    uint16_t size;
    int reader_mode; /* 0=synthetic block, 1=zero size, 2=error, 3=real reader */
    int fail_make, fail_process, fail_flush;
    unsigned released_blocks;
    uint8_t id, mode, variant;
    uint16_t width, height;
    char name[21], extension[21], events[32];
    size_t events_len;
    pr_rec_res_ecl_dax_io reader_io;
} fixture;

static void event(fixture *f, char c) {
    if (f->events_len < sizeof f->events - 1) {
        f->events[f->events_len++] = c;
        f->events[f->events_len] = '\0';
    }
}

static void put_word(uint8_t *p, unsigned value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static int open_archive(void *ctx, const char *name, const char *ext,
                        const uint8_t **data, size_t *size) {
    fixture *f = ctx;
    event(f, 'O');
    CHECK(strcmp(name, f->name) == 0 && strcmp(ext, f->extension) == 0);
    *data = f->archive;
    *size = 29;
    return 0;
}

static void close_archive(void *ctx) { event(ctx, 'C'); }

static int read_block(void *ctx, const char *name, const char *extension,
                      uint8_t id, uint16_t *size, const uint8_t **block) {
    fixture *f = ctx;
    event(f, 'R');
    f->id = id;
    strcpy(f->name, name);
    strcpy(f->extension, extension);
    if (f->reader_mode == 2) return 9;
    if (f->reader_mode == 1) {
        *size = 0;
        *block = NULL;
        return 0;
    }
    if (f->reader_mode == 3)
        return pr_rec_res_0802_0716(&f->reader_io, name, extension, id,
                                    size, block);
    *size = f->size;
    *block = f->block;
    return 0;
}

static int release_prior(void *ctx, pr_r464_block_object **dest) {
    fixture *f = ctx;
    event(f, 'P');
    free((*dest)->bytes);
    free(*dest);
    *dest = NULL;
    return 0;
}

static int make_object(void *ctx, uint16_t width, uint16_t height,
                       uint8_t planes, uint8_t mode,
                       pr_r464_block_object **dest) {
    fixture *f = ctx;
    pr_r464_block_object *o;
    event(f, 'M');
    f->width = width; f->height = height; f->mode = mode;
    if (f->fail_make) return 3;
    o = malloc(sizeof *o);
    if (!o) return 4;
    o->bytes = calloc(23, 1);
    if (!o->bytes) { free(o); return 4; }
    o->capacity = 23;
    put_word(o->bytes, width);
    put_word(o->bytes + 2, height);
    o->bytes[8] = planes;
    *dest = o;
    return 0;
}

static int process_block(void *ctx, const uint8_t *block, uint16_t size,
                         uint16_t data_offset, uint8_t mode, uint8_t variant,
                         pr_r464_block_object *object) {
    fixture *f = ctx;
    event(f, 'H');
    CHECK(block != NULL && size == f->size &&
          data_offset == PR_R464_BLOCK_DATA_OFFSET && object && object->bytes);
    f->mode = mode; f->variant = variant;
    return f->fail_process ? 5 : 0;
}

static void release_block(void *ctx, const uint8_t *block, uint16_t size) {
    fixture *f = ctx;
    event(f, 'F');
    ++f->released_blocks;
    if (f->reader_mode == 3)
        pr_rec_res_0802_0716_release(&f->reader_io, block, size);
}

static int flush(void *ctx) {
    fixture *f = ctx;
    event(f, 'K');
    return f->fail_flush ? 6 : 0;
}

static pr_r464_wrapper_env env_for(fixture *f) {
    pr_r464_wrapper_env e = {
        f, "test", read_block, release_prior, make_object,
        process_block, release_block, flush
    };
    return e;
}

static void init(fixture *f) {
    size_t i;
    memset(f, 0, sizeof *f);
    f->size = 17;
    for (i = 0; i < 17; ++i) f->block[i] = (uint8_t)(i + 1);
    f->reader_io.ctx = f;
    f->reader_io.open_archive = open_archive;
    f->reader_io.close_archive = close_archive;
}

static void dispose(pr_r464_block_object **o) {
    if (!*o) return;
    free((*o)->bytes);
    free(*o);
    *o = NULL;
}

static int test_full_and_zero(void) {
    fixture f;
    pr_r464_wrapper_env e;
    pr_r464_block_object *o;
    pr_r464_wrapper_report r;
    uint8_t name[256] = {25};
    size_t i;
    init(&f);
    for (i = 0; i < 25; ++i) name[i + 1] = (uint8_t)('A' + i);
    e = env_for(&f);
    o = malloc(sizeof *o);
    CHECK(o != NULL);
    o->bytes = calloc(23, 1);
    CHECK(o->bytes != NULL);
    o->capacity = 23;
    CHECK(pr_r464_block_wrapper(&e, name, 0xAB03, 7, 2, &o, &r) ==
          PR_R464_WRAPPER_OK);
    CHECK(strcmp(f.events, "PRMHFK") == 0 && r.call_count == 6 &&
          f.released_blocks == 1 && r.held_block == NULL);
    CHECK(f.id == 3 && f.mode == 2 && f.variant == 7);
    CHECK(strcmp(f.name, "ABCDEFGHIJKLMNOPQRST") == 0);
    CHECK(f.width == 0x0201 && f.height == 0x0403);
    CHECK(o && memcmp(o->bytes, f.block, 17) == 0);
    dispose(&o);

    init(&f); e = env_for(&f);
    f.reader_mode = 1;
    o = malloc(sizeof *o);
    CHECK(o != NULL);
    o->bytes = calloc(23, 1);
    CHECK(o->bytes != NULL);
    o->capacity = 23;
    CHECK(pr_r464_block_wrapper(&e, name, 3, 7, 2, &o, &r) ==
          PR_R464_WRAPPER_OK);
    CHECK(strcmp(f.events, "PR") == 0 && o == NULL &&
          r.call_count == 2 && r.block_size == 0);
    return 0;
}

static int test_reached_stops(void) {
    fixture f;
    pr_r464_wrapper_env e;
    pr_r464_block_object *o = NULL;
    pr_r464_wrapper_report r;
    uint8_t name[256] = {1, 'N'};
    init(&f); e = env_for(&f);
    o = malloc(sizeof *o);
    CHECK(o != NULL);
    o->bytes = calloc(23, 1);
    CHECK(o->bytes != NULL);
    o->capacity = 23;
    e.release_prior = NULL;
    CHECK(pr_r464_block_wrapper(&e, name, 1, 0, 0, &o, &r) ==
          PR_R464_WRAPPER_HOOK);
    CHECK(strcmp(f.events, "") == 0 && o != NULL &&
          r.stopped_at == PR_R464_CALL_RELEASE_PRIOR);
    dispose(&o);

    init(&f); e = env_for(&f);
    o = malloc(sizeof *o);
    CHECK(o != NULL);
    o->bytes = calloc(23, 1);
    CHECK(o->bytes != NULL);
    o->capacity = 23;
    e.read_block = NULL;
    CHECK(pr_r464_block_wrapper(&e, name, 1, 0, 0, &o, &r) ==
          PR_R464_WRAPPER_HOOK);
    CHECK(strcmp(f.events, "P") == 0 && o == NULL &&
          r.stopped_at == PR_R464_CALL_READ_BLOCK);

    init(&f); e = env_for(&f);
    e.release_prior = NULL;
    CHECK(pr_r464_block_wrapper(&e, name, 1, 0, 0, &o, &r) ==
          PR_R464_WRAPPER_OK);
    CHECK(strcmp(f.events, "RMHFK") == 0);
    dispose(&o);

    init(&f); e = env_for(&f); f.reader_mode = 2;
    CHECK(pr_r464_block_wrapper(&e, name, 1, 0, 0, &o, &r) ==
          PR_R464_WRAPPER_READER);
    CHECK(strcmp(f.events, "R") == 0 && r.reader_status == 9 &&
          r.stopped_at == PR_R464_CALL_READ_BLOCK && o == NULL);

    init(&f); e = env_for(&f);
    name[0] = 3; name[1] = 'A'; name[2] = 0; name[3] = 'B';
    o = malloc(sizeof *o);
    CHECK(o != NULL);
    o->bytes = calloc(23, 1);
    CHECK(o->bytes != NULL);
    o->capacity = 23;
    CHECK(pr_r464_block_wrapper(&e, name, 1, 0, 0, &o, &r) ==
          PR_R464_WRAPPER_NAME);
    CHECK(strcmp(f.events, "P") == 0 && o == NULL &&
          r.stopped_at == PR_R464_CALL_READ_BLOCK);
    name[0] = 1; name[1] = 'N';

    init(&f); e = env_for(&f); f.size = 16;
    CHECK(pr_r464_block_wrapper(&e, name, 1, 0, 0, &o, &r) ==
          PR_R464_WRAPPER_BLOCK);
    CHECK(strcmp(f.events, "R") == 0 && r.held_block == f.block && o == NULL);

    init(&f); e = env_for(&f); e.make_object = NULL;
    CHECK(pr_r464_block_wrapper(&e, name, 1, 0, 0, &o, &r) ==
          PR_R464_WRAPPER_HOOK);
    CHECK(strcmp(f.events, "R") == 0 &&
          r.stopped_at == PR_R464_CALL_MAKE_OBJECT && r.held_block == f.block);

    init(&f); e = env_for(&f); f.fail_make = 1;
    CHECK(pr_r464_block_wrapper(&e, name, 1, 0, 0, &o, &r) ==
          PR_R464_WRAPPER_CALLBACK);
    CHECK(strcmp(f.events, "RM") == 0 && r.callback_status == 3 &&
          r.held_block == f.block);

    init(&f); e = env_for(&f); f.fail_process = 1;
    CHECK(pr_r464_block_wrapper(&e, name, 1, 4, 5, &o, &r) ==
          PR_R464_WRAPPER_CALLBACK);
    CHECK(strcmp(f.events, "RMH") == 0 && r.callback_status == 5 &&
          r.held_block == f.block && o &&
          memcmp(o->bytes, f.block, 17) == 0);
    dispose(&o);

    init(&f); e = env_for(&f); f.fail_flush = 1;
    CHECK(pr_r464_block_wrapper(&e, name, 1, 4, 5, &o, &r) ==
          PR_R464_WRAPPER_CALLBACK);
    CHECK(strcmp(f.events, "RMHFK") == 0 && r.callback_status == 6 &&
          r.held_block == NULL && f.released_blocks == 1 && o);
    dispose(&o);
    return 0;
}

static int test_merged_reader_callback(void) {
    fixture f;
    pr_r464_wrapper_env e;
    pr_r464_block_object *o = NULL;
    pr_r464_wrapper_report r;
    uint8_t name[256] = {4, 'T', 'E', 'S', 'T'};
    size_t i;
    init(&f); e = env_for(&f);
    f.reader_mode = 3;
    /* One synthetic DAX index entry, then one 17-byte literal packet. */
    put_word(f.archive, 9);
    f.archive[2] = 3;
    put_word(f.archive + 3, 0);
    put_word(f.archive + 5, 0);
    put_word(f.archive + 7, 17);
    put_word(f.archive + 9, 18);
    f.archive[11] = 16;
    for (i = 0; i < 17; ++i) f.archive[12 + i] = f.block[i];
    CHECK(pr_r464_block_wrapper(&e, name, 0xFF03, 1, 2, &o, &r) ==
          PR_R464_WRAPPER_OK);
    CHECK(strcmp(f.events, "ROCMHFK") == 0 &&
          strcmp(f.name, "TEST") == 0 &&
          memcmp(o->bytes, f.block, 17) == 0 &&
          r.held_block == NULL && f.released_blocks == 1);
    dispose(&o);
    return 0;
}

int main(void) {
    if (test_full_and_zero() || test_reached_stops() ||
        test_merged_reader_callback()) return 1;
    return 0;
}
