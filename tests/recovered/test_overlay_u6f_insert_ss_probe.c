#include "poolrad/recovered/overlay_u6f_insert_ss_probe.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

enum { DS = 0x14a0, SS = 0x1d16, BP = 0x37ae };

static uint8_t probe_ram[PR_U6F_INSERT_DOS_RAM_SIZE];
static uint8_t old_ram[PR_U6F_INSERT_DOS_RAM_SIZE];

static pr_u6f_insert_far far_at(uint16_t segment, uint16_t offset)
{
    pr_u6f_insert_far value = {offset, segment};
    return value;
}

static size_t address(pr_u6f_insert_far value, unsigned displacement)
{
    return (((size_t)value.segment << 4) +
            (uint16_t)(value.offset + displacement)) &
           (PR_U6F_INSERT_DOS_RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, pr_u6f_insert_far value,
                  unsigned displacement, uint16_t word)
{
    size_t a = address(value, displacement);
    ram[a] = (uint8_t)word;
    ram[(a + 1u) & (PR_U6F_INSERT_DOS_RAM_SIZE - 1u)] = (uint8_t)(word >> 8);
}

static uint8_t get8(const uint8_t *ram, pr_u6f_insert_far value,
                    unsigned displacement)
{
    return ram[address(value, displacement)];
}

static void seed(pr_u6f_insert_far incoming)
{
    pr_u6f_insert_far head = far_at(0x3000u, 0x0100u);
    pr_u6f_insert_far window = far_at(0x3a16u, 0u);
    memset(probe_ram, 0, sizeof probe_ram);
    put16(probe_ram, far_at(DS, 0x5d96u), 0u, head.offset);
    put16(probe_ram, far_at(DS, 0x5d96u), 2u, head.segment);
    put16(probe_ram, far_at(DS, 0x49d6u), 0u, window.offset);
    put16(probe_ram, far_at(DS, 0x49d6u), 2u, window.segment);
    /* Existing head slot zero and null +0x104 link; incoming link null. */
    put16(probe_ram, far_at(SS, BP), 6u, incoming.offset);
    put16(probe_ram, far_at(SS, BP), 8u, incoming.segment);
    memcpy(old_ram, probe_ram, sizeof old_ram);
}

static int test_ss_alias_changes_selection(void)
{
    pr_u6f_insert_far incoming = far_at(SS, 0x36e3u);
    pr_u6f_insert_dos_memory probe_memory = {probe_ram, sizeof probe_ram, DS};
    pr_u6f_insert_dos_memory old_memory = {old_ram, sizeof old_ram, DS};
    pr_u6f_ss_probe_report probe;
    pr_u6f_insert_dos_report old;
    pr_u6f_insert_far used0 = far_at(SS, BP - 12u);
    pr_u6f_insert_far used1 = far_at(SS, BP - 11u);

    seed(incoming);
    CHECK(address(incoming, 0xbfu) == address(used0, 0u));
    CHECK(address(used0, 0u) == 0x20902u);
    CHECK(pr_u6f_insert_ss_two_node_probe(&probe_memory, SS, BP, &probe) ==
          PR_U6F_SS_PROBE_OK);
    CHECK(pr_u6f_party_insert_1446_dos(&old_memory, incoming, &old) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(probe.first_marked == 1u && probe.incoming_marked == 1u);
    CHECK(probe.increments == 0u && probe.selected_slot == 0u);
    CHECK(old.selected_slot == 1u);
    CHECK(get8(probe_ram, incoming, 0xbfu) == 0u);
    CHECK(get8(old_ram, incoming, 0xbfu) == 1u);
    CHECK(get8(probe_ram, used0, 0u) == 0u);
    CHECK(get8(probe_ram, used1, 0u) == 1u);
    CHECK(get8(old_ram, used1, 0u) == 0u);
    return 0;
}

static int test_disjoint_selection_agrees(void)
{
    pr_u6f_insert_far incoming = far_at(0x449au, 0x0008u);
    pr_u6f_insert_dos_memory probe_memory = {probe_ram, sizeof probe_ram, DS};
    pr_u6f_insert_dos_memory old_memory = {old_ram, sizeof old_ram, DS};
    pr_u6f_ss_probe_report probe;
    pr_u6f_insert_dos_report old;

    seed(incoming);
    CHECK(address(incoming, 0xbfu) != address(far_at(SS, BP - 12u), 0u));
    CHECK(pr_u6f_insert_ss_two_node_probe(&probe_memory, SS, BP, &probe) ==
          PR_U6F_SS_PROBE_OK);
    CHECK(pr_u6f_party_insert_1446_dos(&old_memory, incoming, &old) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(probe.first_marked == 1u && probe.incoming_marked == 0u);
    CHECK(probe.increments == 1u && probe.selected_slot == 1u);
    CHECK(old.selected_slot == 1u);
    CHECK(get8(probe_ram, incoming, 0xbfu) == 1u);
    CHECK(get8(old_ram, incoming, 0xbfu) == 1u);
    return 0;
}

int main(void)
{
    if (test_ss_alias_changes_selection() || test_disjoint_selection_agrees())
        return 1;
    puts("u6F SS-frame two-node probe: alias and disjoint cases passed");
    return 0;
}
