#include "poolrad/recovered/overlay_u6f_party_filename.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

typedef struct {
    char events[6];
    unsigned calls, fail_at;
    uint16_t io_word, reset_size, ds_offset, ds_value;
    uint8_t assigned_len, assigned_bytes[40];
} fixture;

static int event(fixture *f, char code)
{
    f->events[f->calls++] = code;
    f->events[f->calls] = '\0';
    return f->calls == f->fail_at;
}

static int assign_temp(void *context,
                       const pr_rec_dax_selector_02eb_string *name)
{
    fixture *f = context;
    f->assigned_len = name->length;
    memcpy(f->assigned_bytes, name->bytes, name->length);
    return event(f, 'A');
}

static int reset_temp(void *context, uint16_t size)
{
    fixture *f = context;
    f->reset_size = size;
    return event(f, 'R');
}

static int close_temp(void *context) { return event(context, 'C'); }

static int io_result(void *context, uint16_t *result)
{
    fixture *f = context;
    if (event(f, 'I')) return 1;
    *result = f->io_word;
    return 0;
}

static int store_ds_word(void *context, uint16_t offset, uint16_t value)
{
    fixture *f = context;
    f->ds_offset = offset;
    f->ds_value = value;
    return event(f, 'S');
}

static pr_rec_dax_selector_02eb_io callbacks(fixture *f)
{
    pr_rec_dax_selector_02eb_io io = {
        f, assign_temp, reset_temp, close_temp, io_result, store_ds_word
    };
    return io;
}

static int prior_from_name(const char *name,
                           pr_u6f_party_string_frame *prior)
{
    static pr_slot_a_native_state world;
    pr_slot_a_party_entry entry;
    const uint8_t literal[2] = {1u, '-'};
    const uint8_t digits[10] = {'0','1','2','3','4','5','6','7','8','9'};
    size_t n = strlen(name);
    CHECK(n <= 40u);
    memset(&world, 0, sizeof world);
    world.character_count = 1u;
    world.name_slots[0] = (uint8_t)n;
    memcpy(world.name_slots + 1u, name, n);
    CHECK(pr_slot_a_party_entry_begin(&world, &entry) == PR_SLOT_A_PARTY_ENTRY_OK);
    CHECK(pr_u6f_party_string_first(&world, &entry, literal, sizeof literal,
                                    digits, sizeof digits, prior)
          == PR_U6F_PARTY_STRING_OK);
    return 0;
}

static int synthetic(void)
{
    pr_u6f_party_string_frame prior;
    pr_u6f_party_filename_result out, unchanged;
    const uint8_t prefix[3] = {2u, 'X', '/'};
    const uint8_t suffix[3] = {2u, '.', 's'};
    const uint8_t empty[1] = {0u};
    uint8_t long_prefix[251] = {250u};
    fixture f = {0};
    pr_rec_dax_selector_02eb_io io = callbacks(&f);

    CHECK(prior_from_name("abc", &prior) == 0);
    CHECK(pr_u6f_party_filename_first(&prior, prefix, sizeof prefix,
                                      suffix, sizeof suffix, NULL, &out)
          == PR_U6F_PARTY_FILENAME_NEED_IO);
    CHECK(out.next_pc == 0x19a8u && out.filename[0] == 7u &&
          memcmp(out.filename + 1u, "X/ABC.s", 7u) == 0);
    CHECK(f.calls == 0u);

    CHECK(pr_u6f_party_filename_first(&prior, prefix, sizeof prefix,
                                      suffix, sizeof suffix, &io, &out)
          == PR_U6F_PARTY_FILENAME_OK);
    CHECK(out.next_pc == 0x19b1u && out.ax == 1u);
    CHECK(strcmp(f.events, "ARCIS") == 0 && f.assigned_len == 7u &&
          memcmp(f.assigned_bytes, "X/ABC.s", 7u) == 0);
    CHECK(f.reset_size == 0x80u && f.ds_offset == 0x49ceu && f.ds_value == 0u);

    memset(&f, 0, sizeof f);
    f.io_word = 0x0100u;
    CHECK(pr_u6f_party_filename_first(&prior, prefix, sizeof prefix,
                                      suffix, sizeof suffix, &io, &out)
          == PR_U6F_PARTY_FILENAME_OK);
    CHECK(out.next_pc == 0x19eau && out.ax == 0x0100u &&
          f.ds_value == 0x0100u && strcmp(f.events, "ARCIS") == 0);

    CHECK(prior_from_name("", &prior) == 0);
    memset(&f, 0, sizeof f);
    CHECK(pr_u6f_party_filename_first(&prior, empty, sizeof empty,
                                      empty, sizeof empty, &io, &out)
          == PR_U6F_PARTY_FILENAME_OK);
    CHECK(out.filename[0] == 0u && out.ax == 0u &&
          out.next_pc == 0x19eau && strcmp(f.events, "ARCIS") == 0);

    CHECK(prior_from_name("abc", &prior) == 0);
    memset(&f, 0, sizeof f);
    f.fail_at = 2u;
    CHECK(pr_u6f_party_filename_first(&prior, prefix, sizeof prefix,
                                      suffix, sizeof suffix, &io, &out)
          == PR_U6F_PARTY_FILENAME_IO_BOUNDARY);
    CHECK(strcmp(f.events, "AR") == 0 && out.next_pc == 0x19a8u &&
          out.filename[0] == 7u);

    /* Invalid prior and overlong local inputs are host bounds, with no
     * callback and no output mutation. */
    memcpy(&unchanged, &out, sizeof out);
    prior.next_pc = 0x1982u;
    CHECK(pr_u6f_party_filename_first(&prior, prefix, sizeof prefix,
                                      suffix, sizeof suffix, &io, &out)
          == PR_U6F_PARTY_FILENAME_PRIOR);
    prior.next_pc = 0x1983u;
    CHECK(pr_u6f_party_filename_first(&prior, long_prefix,
                                      sizeof long_prefix, suffix,
                                      sizeof suffix, &io, &out)
          == PR_U6F_PARTY_FILENAME_BOUNDS);
    CHECK(memcmp(&out, &unchanged, sizeof out) == 0 && f.calls == 2u);
    return 0;
}

static int read_exact(const char *path, uint8_t *out, size_t size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fread(out, 1u, size, f) != size || fgetc(f) != EOF) {
        fclose(f);
        return 0;
    }
    return fclose(f) == 0;
}

static int read_region(const char *path, long offset, uint8_t *out, size_t size)
{
    FILE *f = fopen(path, "rb");
    if (!f) return 0;
    if (fseek(f, offset, SEEK_SET) != 0 || fread(out, 1u, size, f) != size) {
        fclose(f);
        return 0;
    }
    return fclose(f) == 0;
}

static int private_case(const char *save_path, const char *game_path,
                        const char *table_path, const char *prefix_path,
                        const char *output_path)
{
    static uint8_t save[0x3351];
    static pr_slot_a_native_state world;
    pr_slot_a_party_entry entry;
    pr_u6f_party_string_frame prior;
    pr_u6f_party_filename_result out;
    uint8_t literal[2], suffix[5], table[10], prefix[12];
    FILE *f;
    CHECK(read_exact(save_path, save, sizeof save));
    CHECK(read_region(game_path, 0x1dfe6L + 0x15e7L, literal,
                      sizeof literal));
    CHECK(read_region(game_path, 0x1dfe6L + 0x15e9L, suffix,
                      sizeof suffix));
    CHECK(read_exact(table_path, table, sizeof table));
    CHECK(read_exact(prefix_path, prefix, sizeof prefix));
    CHECK(pr_slot_a_native_restore(save, sizeof save, &world) == PR_SLOT_A_NATIVE_OK);
    CHECK(pr_slot_a_party_entry_begin(&world, &entry) == PR_SLOT_A_PARTY_ENTRY_OK);
    CHECK(pr_u6f_party_string_first(&world, &entry, literal, sizeof literal,
                                    table, sizeof table, &prior)
          == PR_U6F_PARTY_STRING_OK);
    CHECK(entry.local_count == 6u && prior.basename[0] == 8u &&
          prefix[0] == 11u && suffix[0] == 4u);
    CHECK(pr_u6f_party_filename_first(&prior, prefix, sizeof prefix,
                                      suffix, sizeof suffix, NULL, &out)
          == PR_U6F_PARTY_FILENAME_NEED_IO);
    CHECK(out.filename[0] == 23u && out.next_pc == 0x19a8u);
    f = fopen(output_path, "wb");
    CHECK(f != NULL);
    CHECK(fwrite(out.filename, 1u, (size_t)out.filename[0] + 1u, f)
          == (size_t)out.filename[0] + 1u);
    CHECK(fclose(f) == 0);
    puts("private first filename: components 11+8+4, Pascal length 23; file probe pending");
    return 0;
}

int main(int argc, char **argv)
{
    if ((argc != 1 && argc != 6) || synthetic() != 0) return 1;
    if (argc == 6 && private_case(argv[1], argv[2], argv[3], argv[4],
                                  argv[5]) != 0)
        return 1;
    puts("u6f party filename synthetic paths passed");
    return 0;
}
