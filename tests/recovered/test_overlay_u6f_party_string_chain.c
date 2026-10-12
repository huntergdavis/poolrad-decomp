#include "poolrad/recovered/overlay_u6f_party_string_chain.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static int synthetic(void)
{
    static pr_slot_a_native_state world, before;
    pr_slot_a_party_entry entry = {0};
    pr_u6f_party_string_frame frame, unchanged;
    const uint8_t literal[2] = {1u, '-'};
    const uint8_t digits[10] = {'0','1','2','3','4','5','6','7','8','9'};

    memset(&world, 0, sizeof world);
    memset(&frame, 0xa5, sizeof frame);
    CHECK(pr_u6f_party_string_first(NULL, &entry, literal, sizeof literal,
                                    digits, sizeof digits, &frame)
          == PR_U6F_PARTY_STRING_ARGUMENT);
    CHECK(pr_u6f_party_string_first(&world, NULL, literal, sizeof literal,
                                    digits, sizeof digits, &frame)
          == PR_U6F_PARTY_STRING_ARGUMENT);
    CHECK(pr_u6f_party_string_first(&world, &entry, literal, sizeof literal,
                                    digits, sizeof digits, NULL)
          == PR_U6F_PARTY_STRING_ARGUMENT);

    /* With zero transferred names, +0x1926 skips every string call. */
    CHECK(pr_slot_a_party_entry_begin(&world, &entry) == PR_SLOT_A_PARTY_ENTRY_OK);
    CHECK(pr_u6f_party_string_first(&world, &entry, NULL, 0u, NULL, 0u,
                                    &frame) == PR_U6F_PARTY_STRING_OK);
    CHECK(frame.next_pc == 0x19f7u && frame.after_0356[0] == 0u &&
          frame.after_0425[0] == 0u && frame.basename[0] == 0u);

    /* Reach the actual first-slot handoff, not a caller-invented name. */
    world.character_count = 1u;
    world.name_slots[0] = 11u;
    memcpy(world.name_slots + 1u, "a-b1cdefghi", 11u);
    memcpy(&before, &world, sizeof world);
    CHECK(pr_slot_a_party_entry_begin(&world, &entry) == PR_SLOT_A_PARTY_ENTRY_OK);
    CHECK(entry.slot == world.name_slots && entry.next_pc == 0x1965u);
    CHECK(pr_u6f_party_string_first(&world, &entry, literal, sizeof literal,
                                    digits, sizeof digits, &frame)
          == PR_U6F_PARTY_STRING_OK);
    CHECK(memcmp(&before, &world, sizeof world) == 0);
    CHECK(frame.literal_local[0] == 1u && frame.literal_local[1] == '-');
    CHECK(frame.after_0356[0] == 10u &&
          memcmp(frame.after_0356 + 1u, "ab1cdefghi", 10u) == 0);
    CHECK(frame.after_0425[0] == 8u &&
          memcmp(frame.after_0425 + 1u, "ABCDEFGH", 8u) == 0);
    CHECK(frame.basename[0] == 8u &&
          memcmp(frame.basename + 1u, "ABCDEFGH", 8u) == 0);
    CHECK(frame.next_pc == 0x1983u);

    /* The DOS count remains an unsigned byte: even 255 reaches slot one. */
    world.character_count = 255u;
    CHECK(pr_slot_a_party_entry_begin(&world, &entry) == PR_SLOT_A_PARTY_ENTRY_OK);
    CHECK(pr_u6f_party_string_first(&world, &entry, literal, sizeof literal,
                                    digits, sizeof digits, &frame)
          == PR_U6F_PARTY_STRING_OK);
    CHECK(frame.basename[0] == 8u && frame.next_pc == 0x1983u);

    /* Host validation rejects a stale handoff and malformed fixed locals. */
    memcpy(&unchanged, &frame, sizeof frame);
    entry.next_pc = 0x1974u;
    CHECK(pr_u6f_party_string_first(&world, &entry, literal, sizeof literal,
                                    digits, sizeof digits, &frame)
          == PR_U6F_PARTY_STRING_ENTRY);
    CHECK(memcmp(&frame, &unchanged, sizeof frame) == 0);
    entry.next_pc = 0x1965u;
    world.name_slots[0] = 41u;
    CHECK(pr_u6f_party_string_first(&world, &entry, literal, sizeof literal,
                                    digits, sizeof digits, &frame)
          == PR_U6F_PARTY_STRING_BOUNDS);
    world.name_slots[0] = 11u;
    CHECK(pr_u6f_party_string_first(&world, &entry, literal, 1u,
                                    digits, sizeof digits, &frame)
          == PR_U6F_PARTY_STRING_BOUNDS);
    CHECK(pr_u6f_party_string_first(&world, &entry, literal, sizeof literal,
                                    digits, 9u, &frame)
          == PR_U6F_PARTY_STRING_BOUNDS);
    CHECK(memcmp(&frame, &unchanged, sizeof frame) == 0);
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
                        const char *table_path, const char *output_path)
{
    static uint8_t save[0x3351];
    static pr_slot_a_native_state world;
    pr_slot_a_party_entry entry;
    pr_u6f_party_string_frame frame;
    uint8_t literal[2], table[10];
    FILE *f;
    CHECK(read_exact(save_path, save, sizeof save));
    CHECK(read_region(game_path, 0x1dfe6L + 0x15e7L, literal,
                      sizeof literal));
    CHECK(read_exact(table_path, table, sizeof table));
    CHECK(pr_slot_a_native_restore(save, sizeof save, &world) == PR_SLOT_A_NATIVE_OK);
    CHECK(pr_slot_a_party_entry_begin(&world, &entry) == PR_SLOT_A_PARTY_ENTRY_OK);
    CHECK(entry.local_count == 6u && entry.slot == world.name_slots &&
          entry.next_pc == 0x1965u && entry.slot[0] == 8u);
    CHECK(pr_u6f_party_string_first(&world, &entry, literal, sizeof literal,
                                    table, sizeof table, &frame)
          == PR_U6F_PARTY_STRING_OK);
    CHECK(frame.literal_local[0] == 1u && frame.after_0356[0] == 8u &&
          frame.after_0425[0] == 8u && frame.basename[0] == 8u &&
          frame.next_pc == 0x1983u);
    f = fopen(output_path, "wb");
    CHECK(f != NULL);
    CHECK(fwrite(frame.after_0425, 1u, (size_t)frame.after_0425[0] + 1u, f)
          == (size_t)frame.after_0425[0] + 1u);
    CHECK(fclose(f) == 0);
    puts("private first slot: count 6, 0356 length 8, 0425 length 8, cap-40 length 8");
    return 0;
}

int main(int argc, char **argv)
{
    if ((argc != 1 && argc != 5) || synthetic() != 0) return 1;
    if (argc == 5 && private_case(argv[1], argv[2], argv[3], argv[4]) != 0)
        return 1;
    puts("u6f party-string chain synthetic paths passed");
    return 0;
}
