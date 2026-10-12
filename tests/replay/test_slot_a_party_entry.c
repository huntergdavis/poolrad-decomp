#include "poolrad/formats/dos_savgam.h"
#include "poolrad/replay/slot_a_party_entry.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static int synthetic(void)
{
    static pr_slot_a_native_state world, before;
    pr_slot_a_party_entry out, unchanged;
    unsigned count;
    memset(&world, 0xA5, sizeof world);
    memset(&out, 0x5A, sizeof out);
    memcpy(&unchanged, &out, sizeof unchanged);
    CHECK(pr_slot_a_party_entry_begin(NULL, &out) == PR_SLOT_A_PARTY_ENTRY_ARGUMENT);
    CHECK(memcmp(&out, &unchanged, sizeof out) == 0);
    CHECK(pr_slot_a_party_entry_begin(&world, NULL) == PR_SLOT_A_PARTY_ENTRY_ARGUMENT);

    /* The source uses the full unsigned byte count, without a bound check.
     * Only the first slot is selected before the first external call. */
    for (count = 0u; count <= 0xFFu; ++count) {
        world.character_count = (uint8_t)count;
        world.name_slots[0] = (uint8_t)(count ^ 0x5Au);
        memcpy(&before, &world, sizeof before);
        CHECK(pr_slot_a_party_entry_begin(&world, &out) == PR_SLOT_A_PARTY_ENTRY_OK);
        CHECK(memcmp(&before, &world, sizeof before) == 0);
        CHECK(out.local_count == count && out.slot_offset == 0u);
        if (count == 0u) {
            CHECK(out.index_one_based == 0u && out.slot == NULL &&
                  out.next_pc == 0x19F7u);
        } else {
            CHECK(out.index_one_based == 1u && out.slot == world.name_slots &&
                  out.slot[0] == (uint8_t)(count ^ 0x5Au) &&
                  out.next_pc == 0x1965u);
        }
    }
    return 0;
}

static int private_save(const char *path)
{
    static pr_slot_a_native_state world;
    uint8_t data[PR_DOS_SAVGAM_SIZE];
    pr_slot_a_party_entry out;
    FILE *f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fread(data, 1u, sizeof data, f) == sizeof data);
    CHECK(fgetc(f) == EOF);
    CHECK(fclose(f) == 0);
    CHECK(pr_slot_a_native_restore(data, sizeof data, &world) == PR_SLOT_A_NATIVE_OK);
    CHECK(pr_slot_a_party_entry_begin(&world, &out) == PR_SLOT_A_PARTY_ENTRY_OK);
    CHECK(out.local_count == 6u && out.index_one_based == 1u &&
          out.slot == world.name_slots && out.slot_offset == 0u &&
          out.next_pc == 0x1965u);
    puts("saved party handoff: count=6, first slot selected, next=0x1965");
    return 0;
}

int main(int argc, char **argv)
{
    if (argc > 2 || synthetic() != 0) return 1;
    if (argc == 2 && private_save(argv[1]) != 0) return 1;
    puts("synthetic party-entry count sweep passed");
    return 0;
}
