#include "poolrad/recovered/resident_dax_file_open.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

typedef struct {
    pr_rec_dax_open_string name, extension, table;
    pr_rec_dax_open_string opened_name, opened_path, assigned_name;
    uint8_t selector, character, suffix_byte, mode, open_al;
    uint16_t header_from_read, table_offset;
    uint32_t seek_offset;
    char events[16];
    unsigned event_count, fail_at;
    bool mutate_name;
} fixture;

static void set_string(pr_rec_dax_open_string *value, const char *text)
{
    size_t n = strlen(text);
    assert(n <= 255u);
    value->length = (uint8_t)n;
    memcpy(value->bytes, text, n);
}

static void assert_text(const pr_rec_dax_open_string *value, const char *text)
{
    size_t n = strlen(text);
    assert(value->length == n);
    assert(memcmp(value->bytes, text, n) == 0);
}

static bool event_ok(fixture *f, char code)
{
    assert(f->event_count < sizeof(f->events));
    f->events[f->event_count++] = code;
    return f->event_count != f->fail_at;
}

static bool read_ds(void *context, uint16_t offset, uint8_t *value)
{
    fixture *f = context;
    char code;
    if (offset == 0x5376u) {
        code = 's';
        *value = f->selector;
    } else if (offset == 0x547du) {
        code = 'c';
        *value = f->character;
    } else {
        assert(offset == 0x557cu);
        code = 'm';
        *value = f->mode;
    }
    return event_ok(f, code);
}

static bool read_table(void *context, uint16_t offset,
                       pr_rec_dax_open_string *value)
{
    fixture *f = context;
    if (!event_ok(f, 't')) return false;
    f->table_offset = offset;
    *value = f->table;
    return true;
}

static bool read_code(void *context, uint16_t offset, uint8_t *value)
{
    fixture *f = context;
    assert(offset == 0x05ccu);
    if (!event_ok(f, 'l')) return false;
    *value = f->suffix_byte;
    return true;
}

static bool open_configured(void *context,
                            const pr_rec_dax_open_string *name_ext,
                            const pr_rec_dax_open_string *configured_path,
                            uint8_t mode, uint8_t *result_al)
{
    fixture *f = context;
    assert(mode == f->mode);
    if (!event_ok(f, 'o')) return false;
    f->opened_name = *name_ext;
    f->opened_path = *configured_path;
    *result_al = f->open_al;
    if (f->mutate_name) {
        set_string(&f->name, "SECOND");
        set_string(&f->extension, ".R");
    }
    return true;
}

static bool assign_second(void *context,
                          const pr_rec_dax_open_string *name_ext)
{
    fixture *f = context;
    if (!event_ok(f, 'a')) return false;
    f->assigned_name = *name_ext;
    return true;
}

static bool reset_second(void *context, uint16_t record_size)
{
    fixture *f = context;
    assert(record_size == 1u);
    return event_ok(f, 'r');
}

static bool io_result(void *context, uint16_t *result)
{
    fixture *f = context;
    if (!event_ok(f, 'i')) return false;
    *result = 0xbeefu;
    return true;
}

static bool read_second(void *context, uint16_t *header_word,
                        uint16_t requested_bytes, uint16_t *actual_bytes)
{
    fixture *f = context;
    assert(requested_bytes == 2u);
    if (!event_ok(f, 'b')) return false;
    *header_word = f->header_from_read;
    *actual_bytes = 2;
    return true;
}

static bool seek_first(void *context, uint32_t absolute_offset)
{
    fixture *f = context;
    if (!event_ok(f, 'k')) return false;
    f->seek_offset = absolute_offset;
    return true;
}

static pr_rec_dax_open_io make_io(fixture *f)
{
    pr_rec_dax_open_io io = {
        f, read_ds, read_table, read_code, open_configured,
        assign_second, reset_second, io_result, read_second, seek_first
    };
    return io;
}

static void prepare(fixture *f)
{
    memset(f, 0, sizeof(*f));
    set_string(&f->name, "NAME");
    set_string(&f->extension, ".X");
    set_string(&f->table, "CFG");
    f->selector = 7;
    f->character = '/';
    f->suffix_byte = 'Z';
    f->mode = 0x81u;
    f->open_al = 0x80u;
    f->header_from_read = 0xfffeu;
}

static void assert_events(const fixture *f, const char *expected)
{
    assert(f->event_count == strlen(expected));
    assert(memcmp(f->events, expected, f->event_count) == 0);
}

static void test_success_and_order(void)
{
    fixture f;
    pr_rec_dax_open_io io;
    uint16_t header_word = 0xaaaau;
    uint8_t al = 0x55u;
    prepare(&f);
    io = make_io(&f);
    assert(pr_rec_res_0802_05cd(&io, &f.name, &f.extension,
                                &header_word, &al) == PR_REC_DAX_OPEN_OK);
    assert_events(&f, "stclmoaribiki");
    assert(f.table_offset == 0x26e6u);
    assert_text(&f.opened_name, "NAME.X");
    assert_text(&f.opened_path, "CFG/Z");
    assert_text(&f.assigned_name, "NAME.X");
    assert(header_word == 0 && f.seek_offset == 0 && al == 1);
}

static void test_open_failure_and_second_name_read(void)
{
    fixture f;
    pr_rec_dax_open_io io;
    uint16_t header_word = 0xaaaau;
    uint8_t al = 0x55u;
    prepare(&f);
    f.open_al = 0;
    io = make_io(&f);
    assert(pr_rec_res_0802_05cd(&io, &f.name, &f.extension,
                                &header_word, &al) == PR_REC_DAX_OPEN_OK);
    assert_events(&f, "stclmo");
    assert(header_word == 0xaaaau && al == 0);

    prepare(&f);
    f.mutate_name = true;
    io = make_io(&f);
    al = 0;
    assert(pr_rec_res_0802_05cd(&io, &f.name, &f.extension,
                                &header_word, &al) == PR_REC_DAX_OPEN_OK);
    assert_text(&f.opened_name, "NAME.X");
    assert_text(&f.assigned_name, "SECOND.R");
    assert(al == 1);
}

static void test_copy_limit_and_append_saturation(void)
{
    fixture f;
    pr_rec_dax_open_io io;
    uint16_t header_word = 0;
    uint8_t al = 0;
    size_t i;
    prepare(&f);
    f.selector = 255u;
    f.name.length = 60;
    f.extension.length = 60;
    memset(f.name.bytes, 'N', 60);
    memset(f.extension.bytes, 'E', 60);
    f.table.length = 254;
    memset(f.table.bytes, 'T', 254);
    io = make_io(&f);
    assert(pr_rec_res_0802_05cd(&io, &f.name, &f.extension,
                                &header_word, &al) == PR_REC_DAX_OPEN_OK);
    assert(f.table_offset == 0x43f6u);
    assert(f.opened_name.length == 80);
    for (i = 0; i < 40; ++i) assert(f.opened_name.bytes[i] == 'N');
    for (i = 40; i < 80; ++i) assert(f.opened_name.bytes[i] == 'E');
    assert(f.opened_path.length == 255);
    assert(f.opened_path.bytes[254] == '/');
}

static void test_callback_failures_and_arguments(void)
{
    fixture f;
    pr_rec_dax_open_io io;
    uint16_t header_word;
    uint8_t al;
    unsigned n;
    for (n = 1; n <= 13; ++n) {
        prepare(&f);
        f.fail_at = n;
        io = make_io(&f);
        header_word = 0xcafeu;
        al = 0x55u;
        assert(pr_rec_res_0802_05cd(&io, &f.name, &f.extension,
                                    &header_word, &al) ==
               PR_REC_DAX_OPEN_BOUNDARY);
        assert(f.event_count == n && al == 0x55u);
        assert(header_word == (n <= 10 ? 0xcafeu :
                               n == 11 ? 0xfffeu : 0u));
    }
    prepare(&f);
    io = make_io(&f);
    header_word = 0;
    al = 0;
    assert(pr_rec_res_0802_05cd(NULL, &f.name, &f.extension,
                                &header_word, &al) ==
           PR_REC_DAX_OPEN_ARGUMENT);
    io.seek_first = NULL;
    assert(pr_rec_res_0802_05cd(&io, &f.name, &f.extension,
                                &header_word, &al) ==
           PR_REC_DAX_OPEN_ARGUMENT);
    assert(pr_rec_res_0802_05cd(&io, &f.name, &f.extension,
                                NULL, &al) == PR_REC_DAX_OPEN_ARGUMENT);
}

int main(void)
{
    test_success_and_order();
    test_open_failure_and_second_name_read();
    test_copy_limit_and_append_saturation();
    test_callback_failures_and_arguments();
    return 0;
}
