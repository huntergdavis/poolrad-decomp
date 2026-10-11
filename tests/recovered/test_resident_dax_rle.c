#include "poolrad/recovered/resident_dax_rle.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

#define MEMORY_SIZE (1024u * 1024u)

typedef struct {
    uint8_t *bytes;
    unsigned reads;
    unsigned writes;
    int fail_read;
    int fail_write;
} fixture;

static size_t physical(pr_rec_dax_far p) {
    return (((size_t)p.segment << 4) + p.offset) & (MEMORY_SIZE - 1u);
}

static pr_rec_dax_far far_at(uint16_t segment, uint16_t offset) {
    pr_rec_dax_far p = {segment, offset};
    return p;
}

static pr_rec_dax_far offset(pr_rec_dax_far p, unsigned delta) {
    p.offset = (uint16_t)(p.offset + delta);
    return p;
}

static int read8(void *ctx, pr_rec_dax_far p, uint8_t *value) {
    fixture *f = ctx;
    ++f->reads;
    if (f->fail_read && f->reads == (unsigned)f->fail_read) return 1;
    *value = f->bytes[physical(p)];
    return 0;
}

static int write8(void *ctx, pr_rec_dax_far p, uint8_t value) {
    fixture *f = ctx;
    ++f->writes;
    if (f->fail_write && f->writes == (unsigned)f->fail_write) return 1;
    f->bytes[physical(p)] = value;
    return 0;
}

static void put_word(fixture *f, pr_rec_dax_far p, uint16_t value) {
    f->bytes[physical(p)] = (uint8_t)value;
    f->bytes[physical(offset(p, 1))] = (uint8_t)(value >> 8);
}

static void put_far(fixture *f, pr_rec_dax_far p, pr_rec_dax_far value) {
    put_word(f, p, value.offset);
    put_word(f, offset(p, 2), value.segment);
}

static int setup(fixture *f, pr_rec_dax_memory *memory,
                 pr_rec_dax_rle_args *args, pr_rec_dax_far source,
                 pr_rec_dax_far destination, uint16_t size) {
    memset(f, 0, sizeof *f);
    f->bytes = calloc(MEMORY_SIZE, 1);
    if (!f->bytes) return 1;
    memory->ctx = f;
    memory->read8 = read8;
    memory->write8 = write8;
    args->source_pointer = far_at(0x1000, 0);
    args->destination_pointer = far_at(0x1000, 4);
    args->packed_size = far_at(0x1000, 8);
    args->unused_output_size = far_at(0xFFFF, 0xFFFF);
    put_far(f, args->source_pointer, source);
    put_far(f, args->destination_pointer, destination);
    put_word(f, args->packed_size, size);
    return 0;
}

static int test_controls_and_frame(void) {
    fixture f;
    pr_rec_dax_memory memory;
    pr_rec_dax_rle_args args;
    pr_rec_dax_rle_state state = {0};
    pr_rec_dax_far src = far_at(0x2000, 0);
    pr_rec_dax_far dst = far_at(0x4000, 0);
    unsigned i;
    CHECK(!setup(&f, &memory, &args, src, dst, 4));
    f.bytes[physical(src)] = 0;
    f.bytes[physical(offset(src, 1))] = 0x31;
    f.bytes[physical(offset(src, 2))] = 0xFF;
    f.bytes[physical(offset(src, 3))] = 0x32;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 4, &state) == PR_REC_DAX_RLE_OK);
    CHECK(state.iterations == 2 && state.source_index == 4 &&
          state.destination_index == 2 && state.direction_flag == 0);
    CHECK(f.bytes[physical(dst)] == 0x31 &&
          f.bytes[physical(offset(dst, 1))] == 0x32);
    free(f.bytes);

    CHECK(!setup(&f, &memory, &args, src, dst, 128));
    f.bytes[physical(src)] = 0x7E;
    for (i = 0; i < 127; ++i)
        f.bytes[physical(offset(src, i + 1))] = (uint8_t)i;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 1, &state) == PR_REC_DAX_RLE_OK);
    CHECK(state.source_index == 128 && state.destination_index == 127);
    for (i = 0; i < 127; ++i)
        CHECK(f.bytes[physical(offset(dst, i))] == (uint8_t)i);
    free(f.bytes);

    CHECK(!setup(&f, &memory, &args, src, dst, 2));
    f.bytes[physical(src)] = 0x81;
    f.bytes[physical(offset(src, 1))] = 0xA5;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 1, &state) == PR_REC_DAX_RLE_OK);
    CHECK(state.source_index == 2 && state.destination_index == 127);
    for (i = 0; i < 127; ++i)
        CHECK(f.bytes[physical(offset(dst, i))] == 0xA5);
    free(f.bytes);

    /* The fourth far argument is never dereferenced; the size check is
     * after the first control, even if the size word starts at zero. */
    CHECK(!setup(&f, &memory, &args, src, dst, 0));
    f.bytes[physical(src)] = 0;
    f.bytes[physical(offset(src, 1))] = 0x5C;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 1, &state) == PR_REC_DAX_RLE_OK);
    CHECK(state.iterations == 1 && state.destination_index == 1 &&
          f.bytes[physical(dst)] == 0x5C);
    free(f.bytes);
    return 0;
}

static int test_stall_and_bound(void) {
    fixture f;
    pr_rec_dax_memory memory;
    pr_rec_dax_rle_args args;
    pr_rec_dax_rle_state state = {0};
    pr_rec_dax_far src = far_at(0x2000, 0);
    pr_rec_dax_far dst = far_at(0x4000, 0);
    int control;
    for (control = 0x7F; control <= 0x80; ++control) {
        CHECK(!setup(&f, &memory, &args, src, dst, 1));
        f.bytes[physical(src)] = (uint8_t)control;
        state.direction_flag = 1;
        CHECK(pr_rec_res_0802_050f(&memory, &args, 2, &state) ==
              PR_REC_DAX_RLE_STALLED);
        CHECK(state.iterations == 1 && state.source_index == 0 &&
              state.destination_index == 0 && state.direction_flag == 1 &&
              f.writes == 0);
        put_word(&f, args.packed_size, 0);
        CHECK(pr_rec_res_0802_050f(&memory, &args, 1, &state) ==
              PR_REC_DAX_RLE_OK);
        free(f.bytes);
    }
    CHECK(!setup(&f, &memory, &args, src, dst, 4));
    f.bytes[physical(src)] = 0;
    f.bytes[physical(offset(src, 1))] = 1;
    f.bytes[physical(offset(src, 2))] = 0;
    f.bytes[physical(offset(src, 3))] = 2;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 1, &state) ==
          PR_REC_DAX_RLE_ITERATION_LIMIT);
    CHECK(state.iterations == 1 && state.source_index == 2 &&
          state.destination_index == 1);
    free(f.bytes);
    return 0;
}

static int test_overlap_and_offset_wrap(void) {
    fixture f;
    pr_rec_dax_memory memory;
    pr_rec_dax_rle_args args;
    pr_rec_dax_rle_state state = {0};
    pr_rec_dax_far src = far_at(0x4000, 0x100);
    pr_rec_dax_far dst = far_at(0x4000, 0x102);
    CHECK(!setup(&f, &memory, &args, src, dst, 4));
    f.bytes[physical(src)] = 2;
    f.bytes[physical(offset(src, 1))] = 0x11;
    f.bytes[physical(offset(src, 2))] = 0x22;
    f.bytes[physical(offset(src, 3))] = 0x33;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 1, &state) == PR_REC_DAX_RLE_OK);
    CHECK(state.direction_flag == 1 && state.destination_index == 3);
    CHECK(f.bytes[physical(dst)] == 0x11 &&
          f.bytes[physical(offset(dst, 1))] == 0x22 &&
          f.bytes[physical(offset(dst, 2))] == 0x33);
    free(f.bytes);

    /* The two far pointers alias physical memory, but their offsets order
     * in the opposite direction. DOS Move compares offsets only, so it
     * copies forward and overwrites later source bytes. */
    src = far_at(0x3000, 0x200);
    dst = far_at(0x3010, 0x102);
    CHECK(!setup(&f, &memory, &args, src, dst, 4));
    f.bytes[physical(src)] = 2;
    f.bytes[physical(offset(src, 1))] = 0x11;
    f.bytes[physical(offset(src, 2))] = 0x22;
    f.bytes[physical(offset(src, 3))] = 0x33;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 1, &state) == PR_REC_DAX_RLE_OK);
    CHECK(state.direction_flag == 0 && state.destination_index == 3);
    CHECK(f.bytes[physical(dst)] == 0x11 &&
          f.bytes[physical(offset(dst, 1))] == 0x11 &&
          f.bytes[physical(offset(dst, 2))] == 0x11);
    free(f.bytes);

    src = far_at(0x3000, 0xFFFE);
    dst = far_at(0x4000, 0xFFFF);
    CHECK(!setup(&f, &memory, &args, src, dst, 3));
    f.bytes[physical(src)] = 1;
    f.bytes[physical(offset(src, 1))] = 0x41;
    f.bytes[physical(offset(src, 2))] = 0x42;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 1, &state) == PR_REC_DAX_RLE_OK);
    CHECK(state.source_index == 3 && state.destination_index == 2);
    CHECK(f.bytes[physical(dst)] == 0x41 &&
          f.bytes[physical(offset(dst, 1))] == 0x42);
    free(f.bytes);
    return 0;
}

static int test_index_wrap_and_host_fault(void) {
    fixture f;
    pr_rec_dax_memory memory;
    pr_rec_dax_rle_args args;
    pr_rec_dax_rle_state state = {0};
    pr_rec_dax_far src = far_at(0x2000, 0);
    pr_rec_dax_far dst = far_at(0x7000, 0);
    unsigned i;
    CHECK(!setup(&f, &memory, &args, src, dst, 1040));
    for (i = 0; i < 520; ++i) {
        f.bytes[physical(offset(src, i * 2))] = 0x81;
        f.bytes[physical(offset(src, i * 2 + 1))] = 0x55;
    }
    CHECK(pr_rec_res_0802_050f(&memory, &args, 520, &state) == PR_REC_DAX_RLE_OK);
    CHECK(state.iterations == 520 && state.source_index == 1040 &&
          state.destination_index == 504 && state.direction_flag == 0);
    CHECK(f.bytes[physical(dst)] == 0x55 &&
          f.bytes[physical(offset(dst, 503))] == 0x55 &&
          f.bytes[physical(offset(dst, 65535))] == 0x55);
    free(f.bytes);

    CHECK(!setup(&f, &memory, &args, src, dst, 2));
    f.bytes[physical(src)] = 0xFF;
    f.bytes[physical(offset(src, 1))] = 9;
    f.fail_write = 1;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 1, &state) ==
          PR_REC_DAX_RLE_MEMORY);
    CHECK(f.writes == 1 && state.source_index == 0);
    CHECK(pr_rec_res_0802_050f(NULL, &args, 1, &state) ==
          PR_REC_DAX_RLE_ARGUMENT);
    CHECK(pr_rec_res_0802_050f(&memory, &args, 0, &state) ==
          PR_REC_DAX_RLE_ARGUMENT);
    free(f.bytes);
    return 0;
}

static int test_live_variables(void) {
    fixture f;
    pr_rec_dax_memory memory;
    pr_rec_dax_rle_args args;
    pr_rec_dax_rle_state state = {0};
    pr_rec_dax_far src = far_at(0x2000, 0);
    pr_rec_dax_far dst = far_at(0x1000, 0);
    CHECK(!setup(&f, &memory, &args, src, dst, 4));
    f.bytes[physical(src)] = 0;
    f.bytes[physical(offset(src, 1))] = 0x10;
    f.bytes[physical(offset(src, 0x12))] = 0;
    f.bytes[physical(offset(src, 0x13))] = 0x20;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 2, &state) == PR_REC_DAX_RLE_OK);
    CHECK(state.iterations == 2 && state.source_index == 4 &&
          state.destination_index == 2);
    CHECK(f.bytes[physical(args.source_pointer)] == 0x10 &&
          f.bytes[physical(offset(args.source_pointer, 1))] == 0x20);
    free(f.bytes);

    dst = far_at(0x1000, 8);
    CHECK(!setup(&f, &memory, &args, src, dst, 4));
    f.bytes[physical(src)] = 0;
    f.bytes[physical(offset(src, 1))] = 2;
    CHECK(pr_rec_res_0802_050f(&memory, &args, 2, &state) == PR_REC_DAX_RLE_OK);
    CHECK(state.iterations == 1 && state.source_index == 2 &&
          state.destination_index == 1 &&
          f.bytes[physical(args.packed_size)] == 2);
    free(f.bytes);
    return 0;
}

int main(void) {
    if (test_controls_and_frame() || test_stall_and_bound() ||
        test_overlap_and_offset_wrap() || test_index_wrap_and_host_fault() ||
        test_live_variables())
        return 1;
    puts("resident DAX RLE synthetic tests passed");
    return 0;
}
