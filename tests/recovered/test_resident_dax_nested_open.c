#include "poolrad/recovered/resident_dax_nested_open.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

typedef struct {
    char events[64];
    size_t event_count;
    int calls;
    int fail_at;
    int unexpected;
    uint16_t results[4];
    size_t result_count;
    size_t result_index;
    uint16_t last_global_offset;
    uint16_t last_global_value;
    uint16_t temp_reset_size;
    uint16_t first_reset_size;
    uint8_t temp_name_length;
    uint8_t first_name_length;
    uint8_t display_extension_length;
    uint8_t display_code;
    pr_rec_dax_nested_far first_record;
} fixture;

static int record_event(fixture *f, char code) {
    if (f->event_count < sizeof f->events - 1) {
        f->events[f->event_count++] = code;
        f->events[f->event_count] = '\0';
    }
    ++f->calls;
    return f->calls == f->fail_at;
}

static void check_record(fixture *f, pr_rec_dax_nested_record kind,
                         pr_rec_dax_nested_far address) {
    if (kind == PR_REC_DAX_NESTED_TEMP) {
        if (address.segment || address.offset) f->unexpected = 1;
    } else if (kind == PR_REC_DAX_NESTED_FIRST) {
        if (address.segment != f->first_record.segment ||
            address.offset != f->first_record.offset) f->unexpected = 1;
    } else f->unexpected = 1;
}

static int assign(void *ctx, pr_rec_dax_nested_record record,
                  pr_rec_dax_nested_far first_record,
                  const pr_rec_dax_nested_string *name) {
    fixture *f = ctx;
    check_record(f, record, first_record);
    if (record == PR_REC_DAX_NESTED_TEMP) {
        f->temp_name_length = name->length;
        return record_event(f, 'A');
    }
    f->first_name_length = name->length;
    return record_event(f, 'a');
}

static int reset(void *ctx, pr_rec_dax_nested_record record,
                 pr_rec_dax_nested_far first_record, uint16_t record_size) {
    fixture *f = ctx;
    check_record(f, record, first_record);
    if (record == PR_REC_DAX_NESTED_TEMP) {
        f->temp_reset_size = record_size;
        return record_event(f, 'R');
    }
    f->first_reset_size = record_size;
    return record_event(f, 'r');
}

static int close_temp(void *ctx) {
    return record_event(ctx, 'C');
}

static int io_result(void *ctx, uint16_t *result) {
    fixture *f = ctx;
    if (record_event(f, 'I')) return 1;
    if (f->result_index >= f->result_count) return 1;
    *result = f->results[f->result_index++];
    return 0;
}

static int store_word(void *ctx, uint16_t offset, uint16_t value) {
    fixture *f = ctx;
    f->last_global_offset = offset;
    f->last_global_value = value;
    return record_event(f, 'S');
}

static int display(void *ctx, uint8_t code,
                   const pr_rec_dax_nested_string *extension) {
    fixture *f = ctx;
    f->display_code = code;
    f->display_extension_length = extension->length;
    return record_event(f, 'D');
}

static pr_rec_dax_nested_io io_for(fixture *f) {
    pr_rec_dax_nested_io io = {f, assign, reset, close_temp, io_result,
                               store_word, display};
    return io;
}

static pr_rec_dax_nested_string string_of_size(unsigned size) {
    pr_rec_dax_nested_string s = {0};
    unsigned i;
    s.length = (uint8_t)size;
    for (i = 0; i < size; ++i) s.bytes[i] = (uint8_t)(i + 1u);
    return s;
}

static void init(fixture *f) {
    memset(f, 0, sizeof *f);
    f->first_record.segment = 0x1234;
    f->first_record.offset = 0x5678;
}

static int test_success_and_nonzero_mode(void) {
    fixture f;
    pr_rec_dax_nested_io io;
    pr_rec_dax_nested_string name = string_of_size(50);
    pr_rec_dax_nested_string ext = string_of_size(51);
    uint8_t al = 0x7E;
    init(&f);
    io = io_for(&f);
    f.results[0] = 0;
    f.result_count = 1;
    CHECK(pr_rec_res_0802_025d(&io, &name, &ext, 0, f.first_record,
                                1, &al) == PR_REC_DAX_NESTED_OK);
    CHECK(al == 1 && strcmp(f.events, "ARCISar") == 0 && !f.unexpected);
    CHECK(f.temp_name_length == 40 && f.first_name_length == 40 &&
          f.temp_reset_size == 0x80 && f.first_reset_size == 1 &&
          f.last_global_offset == 0x49CE && f.last_global_value == 0);

    init(&f);
    io = io_for(&f);
    f.results[0] = 5;
    f.result_count = 1;
    CHECK(pr_rec_res_0802_025d(&io, &name, &ext, 1, f.first_record,
                                1, &al) == PR_REC_DAX_NESTED_OK);
    CHECK(al == 0 && strcmp(f.events, "ARCIS") == 0 &&
          f.last_global_value == 5 && !f.unexpected);

    init(&f);
    io = io_for(&f);
    name = string_of_size(0);
    f.results[0] = 0;
    f.result_count = 1;
    CHECK(pr_rec_res_0802_025d(&io, &name, &ext, 0x101, f.first_record,
                                1, &al) == PR_REC_DAX_NESTED_OK);
    CHECK(al == 0 && strcmp(f.events, "ARCIS") == 0);
    return 0;
}

static int test_retry_and_bound(void) {
    fixture f;
    pr_rec_dax_nested_io io;
    pr_rec_dax_nested_string name = string_of_size(3);
    pr_rec_dax_nested_string ext = string_of_size(50);
    uint8_t al = 0x55;
    init(&f);
    io = io_for(&f);
    f.results[0] = 7;
    f.results[1] = 0;
    f.result_count = 2;
    CHECK(pr_rec_res_0802_025d(&io, &name, &ext, 0, f.first_record,
                                2, &al) == PR_REC_DAX_NESTED_OK);
    CHECK(al == 1 && strcmp(f.events, "ARCISDARCISar") == 0 &&
          f.display_code == 0x0E && f.display_extension_length == 40 &&
          f.last_global_value == 0);

    init(&f);
    io = io_for(&f);
    f.results[0] = 7;
    f.result_count = 1;
    al = 0x55;
    CHECK(pr_rec_res_0802_025d(&io, &name, &ext, 0x0100, f.first_record,
                                1, &al) == PR_REC_DAX_NESTED_RETRY_LIMIT);
    CHECK(al == 0x55 && strcmp(f.events, "ARCISD") == 0 &&
          f.last_global_value == 7);
    return 0;
}

static int test_boundaries_and_arguments(void) {
    fixture f;
    pr_rec_dax_nested_io io;
    pr_rec_dax_nested_string name = string_of_size(1);
    pr_rec_dax_nested_string ext = string_of_size(1);
    uint8_t al;
    int fail;
    for (fail = 1; fail <= 7; ++fail) {
        init(&f);
        io = io_for(&f);
        f.results[0] = 0;
        f.result_count = 1;
        f.fail_at = fail;
        al = 0xAA;
        CHECK(pr_rec_res_0802_025d(&io, &name, &ext, 1, f.first_record,
                                    1, &al) == PR_REC_DAX_NESTED_BOUNDARY);
        CHECK(f.calls == fail && al == 0xAA && !f.unexpected);
    }
    init(&f);
    io = io_for(&f);
    f.results[0] = 9;
    f.result_count = 1;
    f.fail_at = 6; /* The retry display boundary. */
    al = 0xAA;
    CHECK(pr_rec_res_0802_025d(&io, &name, &ext, 0, f.first_record,
                                1, &al) == PR_REC_DAX_NESTED_BOUNDARY);
    CHECK(strcmp(f.events, "ARCISD") == 0 && al == 0xAA);
    CHECK(pr_rec_res_0802_025d(NULL, &name, &ext, 0, f.first_record,
                                1, &al) == PR_REC_DAX_NESTED_ARGUMENT);
    CHECK(pr_rec_res_0802_025d(&io, &name, &ext, 0, f.first_record,
                                0, &al) == PR_REC_DAX_NESTED_ARGUMENT);
    io.retry_display = NULL;
    CHECK(pr_rec_res_0802_025d(&io, &name, &ext, 0, f.first_record,
                                1, &al) == PR_REC_DAX_NESTED_ARGUMENT);
    return 0;
}

int main(void) {
    if (test_success_and_nonzero_mode() || test_retry_and_bound() ||
        test_boundaries_and_arguments()) return 1;
    puts("resident nested DAX open model tests passed");
    return 0;
}
