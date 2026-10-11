#include "poolrad/recovered/resident_ecl_dax_block.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #expr); return 1; \
} } while (0)

typedef struct {
    uint8_t file[64];
    size_t file_size;
    int missing;
    int fail_alloc;
    int allocations;
    size_t allocation_sizes[3];
    size_t free_sizes[3];
    int frees;
    int closes;
    char events[16];
    size_t event_count;
    char name[PR_REC_RES_ECL_DAX_NAME_MAX + 1];
    char ext[PR_REC_RES_ECL_DAX_NAME_MAX + 1];
} fixture;

static void event(fixture *f, char c) {
    if (f->event_count < sizeof f->events - 1) {
        f->events[f->event_count++] = c;
        f->events[f->event_count] = '\0';
    }
}

static void word(uint8_t *p, unsigned v) {
    p[0] = (uint8_t)v;
    p[1] = (uint8_t)(v >> 8);
}

static void dword(uint8_t *p, unsigned v) {
    word(p, v);
    word(p + 2, v >> 16);
}

static void entry(uint8_t *p, uint8_t id, unsigned offset,
                  unsigned unpacked, unsigned packed) {
    p[0] = id;
    dword(p + 1, offset);
    word(p + 5, unpacked);
    word(p + 7, packed);
}

static void make_file(fixture *f) {
    memset(f, 0, sizeof *f);
    word(f->file, 18);
    entry(f->file + 2, 3, 0, 3, 4);
    entry(f->file + 11, 7, 4, 5, 2);
    f->file[20] = 2;
    f->file[21] = 0x11;
    f->file[22] = 0x22;
    f->file[23] = 0x33;
    f->file[24] = 0xFB;
    f->file[25] = 0x44;
    f->file_size = 26;
}

static int open_archive(void *ctx, const char *name, const char *ext,
                        const uint8_t **data, size_t *size) {
    fixture *f = ctx;
    event(f, 'O');
    strcpy(f->name, name);
    strcpy(f->ext, ext);
    if (f->missing) return 1;
    *data = f->file;
    *size = f->file_size;
    return 0;
}

static void close_archive(void *ctx) {
    fixture *f = ctx;
    ++f->closes;
    event(f, 'C');
}

static void *allocate(void *ctx, size_t size) {
    fixture *f = ctx;
    ++f->allocations;
    f->allocation_sizes[f->allocations - 1] = size;
    event(f, 'A');
    if (f->allocations == f->fail_alloc) return NULL;
    return malloc(size);
}

static void deallocate(void *ctx, void *ptr, size_t size) {
    fixture *f = ctx;
    f->free_sizes[f->frees++] = size;
    event(f, 'F');
    free(ptr);
}

static pr_rec_res_ecl_dax_io io_for(fixture *f) {
    pr_rec_res_ecl_dax_io io = {f, open_archive, close_archive,
                                allocate, deallocate};
    return io;
}

static int test_literal_and_repeat(void) {
    fixture f;
    pr_rec_res_ecl_dax_io io;
    const uint8_t *block = NULL;
    uint16_t size = 0;
    size_t i;
    make_file(&f);
    io = io_for(&f);
    CHECK(pr_rec_res_0802_0716(&io, "ABCDEFGHIJKLMNOPQRSTUVW", ".dax", 3,
                               &size, &block) == PR_REC_RES_ECL_DAX_OK);
    CHECK(size == 3 && block && block[0] == 0x11 && block[1] == 0x22 &&
          block[2] == 0x33);
    CHECK(strlen(f.name) == 20 && strcmp(f.name, "ABCDEFGHIJKLMNOPQRST") == 0);
    CHECK(strcmp(f.ext, ".dax") == 0);
    CHECK(f.allocations == 2 && f.allocation_sizes[0] == 8 &&
          f.allocation_sizes[1] == 3);
    CHECK(f.closes == 1 && f.frees == 1 && f.free_sizes[0] == 8);
    CHECK(strcmp(f.events, "OAACF") == 0);
    pr_rec_res_0802_0716_release(&io, block, size);
    CHECK(f.frees == 2 && f.free_sizes[1] == 3 &&
          strcmp(f.events, "OAACFF") == 0);

    make_file(&f);
    io = io_for(&f);
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 7, &size, &block) ==
          PR_REC_RES_ECL_DAX_OK);
    CHECK(size == 5);
    for (i = 0; i < size; ++i) CHECK(block[i] == 0x44);
    CHECK(f.allocation_sizes[0] == 8 && f.allocation_sizes[1] == 5);
    pr_rec_res_0802_0716_release(&io, block, size);
    return 0;
}

static int test_missing_and_scan_order(void) {
    fixture f;
    pr_rec_res_ecl_dax_io io;
    const uint8_t *block = (const uint8_t *)f.file;
    uint16_t size = 99;
    make_file(&f);
    io = io_for(&f);
    f.missing = 1;
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_MISSING_FILE);
    CHECK(size == 0 && block == NULL && f.closes == 0 &&
          strcmp(f.events, "O") == 0);

    make_file(&f);
    io = io_for(&f);
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 99, &size, &block) ==
          PR_REC_RES_ECL_DAX_MISSING_ID);
    CHECK(size == 0 && block == NULL && f.closes == 1 &&
          strcmp(f.events, "OC") == 0);

    make_file(&f);
    io = io_for(&f);
    word(f.file + 2 + 7, PR_REC_RES_ECL_DAX_PACKED_LIMIT + 1);
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 7, &size, &block) ==
          PR_REC_RES_ECL_DAX_PACKED_LIMIT_ERROR);
    CHECK(size == 0 && block == NULL && f.closes == 1 &&
          strcmp(f.events, "OC") == 0);

    make_file(&f);
    io = io_for(&f);
    word(f.file + 11 + 7, PR_REC_RES_ECL_DAX_PACKED_LIMIT + 1);
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_OK);
    pr_rec_res_0802_0716_release(&io, block, size);

    make_file(&f);
    io = io_for(&f);
    entry(f.file + 11, 3, 4, 5, 2); /* DOS selects the first matching ID. */
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_OK);
    CHECK(size == 3 && block[0] == 0x11);
    pr_rec_res_0802_0716_release(&io, block, size);
    return 0;
}

static int test_bounded_errors(void) {
    fixture f;
    pr_rec_res_ecl_dax_io io;
    const uint8_t *block = NULL;
    uint16_t size = 0;
    int control;
    for (control = 0x7F; control <= 0x80; ++control) {
        make_file(&f);
        io = io_for(&f);
        f.file[20] = (uint8_t)control;
        CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
              PR_REC_RES_ECL_DAX_RLE_STALL);
        CHECK(size == 0 && block == NULL && f.closes == 1 && f.frees == 2);
        CHECK(strcmp(f.events, "OAACFF") == 0);
    }
    make_file(&f);
    io = io_for(&f);
    word(f.file + 2 + 7, 2);
    f.file[20] = 2; /* Needs three literals but only one follows. */
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_RLE);
    make_file(&f);
    io = io_for(&f);
    f.file[20] = 3; /* Four decoded bytes exceed the declared three. */
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_RLE);
    make_file(&f);
    io = io_for(&f);
    word(f.file + 2 + 7, 2);
    f.file[20] = 0; /* One literal leaves the declared output underfilled. */
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_RLE);
    make_file(&f);
    io = io_for(&f);
    entry(f.file + 2, 3, 0, 3, 1);
    f.file[20] = 0xFF; /* A repeat without its value byte. */
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_RLE);
    make_file(&f);
    io = io_for(&f);
    word(f.file, 17); /* Not a multiple of nine. */
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_INDEX);
    make_file(&f);
    io = io_for(&f);
    dword(f.file + 2 + 1, 999);
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_RANGE);
    make_file(&f);
    io = io_for(&f);
    word(f.file + 2 + 5, 0);
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_RANGE);
    return 0;
}

static int test_allocation_and_arguments(void) {
    fixture f;
    pr_rec_res_ecl_dax_io io;
    const uint8_t *block = NULL;
    uint16_t size = 0;
    make_file(&f);
    io = io_for(&f);
    f.fail_alloc = 1;
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_ALLOC);
    CHECK(f.allocations == 1 && f.frees == 0 && f.closes == 1 &&
          strcmp(f.events, "OAC") == 0);
    make_file(&f);
    io = io_for(&f);
    f.fail_alloc = 2;
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_ALLOC);
    CHECK(f.allocations == 2 && f.frees == 1 && f.free_sizes[0] == 8 &&
          f.closes == 1 && strcmp(f.events, "OAACF") == 0);
    make_file(&f);
    io = io_for(&f);
    size = 99;
    block = f.file;
    CHECK(pr_rec_res_0802_0716(NULL, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_ARGUMENT);
    CHECK(size == 99 && block == f.file);
    io.deallocate = NULL;
    CHECK(pr_rec_res_0802_0716(&io, "N", "X", 3, &size, &block) ==
          PR_REC_RES_ECL_DAX_ARGUMENT);
    CHECK(size == 99 && block == f.file);
    pr_rec_res_0802_0716_release(&io, NULL, 0);
    return 0;
}

int main(void) {
    if (test_literal_and_repeat() || test_missing_and_scan_order() ||
        test_bounded_errors() || test_allocation_and_arguments()) return 1;
    puts("resident ECL DAX block model tests passed");
    return 0;
}
