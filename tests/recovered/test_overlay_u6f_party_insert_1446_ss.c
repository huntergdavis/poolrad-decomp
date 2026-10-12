#include "poolrad/recovered/overlay_u6f_party_insert_1446_ss.h"
#include "poolrad/recovered/overlay_u6f_insert_ss_probe.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

enum { DS = 0x14a0, SS = 0x1d16, CS = 0x2a8e, BP = 0x37ae };
enum { SNAPSHOT_SIZE = 655360 };

static uint8_t ram[PR_U6F_INSERT_DOS_RAM_SIZE];
static uint8_t legacy_ram[PR_U6F_INSERT_DOS_RAM_SIZE];
static uint8_t probe_ram[PR_U6F_INSERT_DOS_RAM_SIZE];
static uint8_t native_after[SNAPSHOT_SIZE];

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

static void put16(uint8_t *view, pr_u6f_insert_far value,
                  unsigned displacement, uint16_t word)
{
    size_t a = address(value, displacement);
    view[a] = (uint8_t)word;
    view[(a + 1u) & (PR_U6F_INSERT_DOS_RAM_SIZE - 1u)] = (uint8_t)(word >> 8);
}

static uint16_t get16(const uint8_t *view, pr_u6f_insert_far value,
                      unsigned displacement)
{
    size_t a = address(value, displacement);
    return (uint16_t)((uint16_t)view[a] |
                      (uint16_t)((uint16_t)view[(a + 1u) &
                          (PR_U6F_INSERT_DOS_RAM_SIZE - 1u)] << 8));
}

static void put_far(uint8_t *view, pr_u6f_insert_far location,
                    pr_u6f_insert_far value)
{
    put16(view, location, 0u, value.offset);
    put16(view, location, 2u, value.segment);
}

static int far_equals(const uint8_t *view, pr_u6f_insert_far location,
                      pr_u6f_insert_far value)
{
    return get16(view, location, 0u) == value.offset &&
           get16(view, location, 2u) == value.segment;
}

static pr_u6f_insert_ss_frame frame(void)
{
    pr_u6f_insert_ss_frame value = {CS, SS, 0x37b0u, 0x3dcau};
    return value;
}

static void seed(pr_u6f_insert_far incoming)
{
    pr_u6f_insert_far head = far_at(0x3000u, 0x0100u);
    memset(ram, 0, sizeof ram);
    ram[address(far_at(CS, 0x1426u), 0u)] = 0xffu;
    put_far(ram, far_at(DS, 0x5d96u), head);
    put_far(ram, far_at(DS, 0x49d6u), far_at(0x3a16u, 0u));
    put16(ram, far_at(SS, 0x37b0u), 0u, 0x19e6u);
    put16(ram, far_at(SS, 0x37b2u), 0u, CS);
    put_far(ram, far_at(SS, BP + 6u), incoming);
    memcpy(legacy_ram, ram, sizeof ram);
    memcpy(probe_ram, ram, sizeof ram);
}

static int test_alias_and_transient_stack(void)
{
    pr_u6f_insert_far incoming = far_at(SS, 0x36e3u);
    pr_u6f_insert_dos_memory memory = {ram, sizeof ram, DS};
    pr_u6f_insert_dos_memory legacy = {legacy_ram, sizeof legacy_ram, DS};
    pr_u6f_insert_dos_memory probe_memory = {probe_ram, sizeof probe_ram, DS};
    pr_u6f_insert_ss_frame call = frame();
    pr_u6f_insert_ss_report result;
    pr_u6f_insert_dos_report old;
    pr_u6f_ss_probe_report probe;

    seed(incoming);
    CHECK(address(incoming, 0xbfu) == address(far_at(SS, BP - 12u), 0u));
    CHECK(pr_u6f_party_insert_1446_ss(&memory, &call, &result) ==
          PR_U6F_INSERT_SS_AT_U9B_TRANSFER);
    CHECK(pr_u6f_insert_ss_two_node_probe(&probe_memory, SS, BP, &probe) ==
          PR_U6F_SS_PROBE_OK);
    CHECK(pr_u6f_party_insert_1446_dos(&legacy, incoming, &old) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(result.callee_bp == BP && result.transfer_sp == BP - 0x18u);
    CHECK(result.append_nodes == 1u && result.mark_nodes == 2u);
    CHECK(result.selected_slot == probe.selected_slot && result.selected_slot == 0u);
    CHECK(old.selected_slot == 1u);
    CHECK(ram[address(incoming, 0xbfu)] == 0u);
    CHECK(ram[address(far_at(SS, BP - 11u), 0u)] == 1u);
    CHECK(legacy_ram[address(incoming, 0xbfu)] == 1u);
    /* Transient helper/final-call stack words alias incoming +B2..+B4. */
    CHECK(ram[address(incoming, 0xb2u)] == 0x14u);
    CHECK(ram[address(incoming, 0xb3u)] == 0x79u);
    CHECK(ram[address(incoming, 0xb4u)] == 0x15u);
    CHECK(legacy_ram[address(incoming, 0xb2u)] == 0u);
    return 0;
}

static int test_disjoint_and_long_finite_list(void)
{
    pr_u6f_insert_far incoming = far_at(0x449au, 0x0008u);
    pr_u6f_insert_dos_memory memory = {ram, sizeof ram, DS};
    pr_u6f_insert_dos_memory legacy = {legacy_ram, sizeof legacy_ram, DS};
    pr_u6f_insert_ss_frame call = frame();
    pr_u6f_insert_ss_report result;
    pr_u6f_insert_dos_report old;
    unsigned i;

    seed(incoming);
    CHECK(pr_u6f_party_insert_1446_ss(&memory, &call, &result) ==
          PR_U6F_INSERT_SS_AT_U9B_TRANSFER);
    CHECK(pr_u6f_party_insert_1446_dos(&legacy, incoming, &old) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(result.selected_slot == 1u && old.selected_slot == 1u);
    CHECK(result.window_word == 1u && old.window_word == 1u);
    CHECK(far_equals(ram, far_at(DS, 0x5d92u), incoming));

    incoming = far_at(0x9000u, 0x0100u);
    seed(incoming);
    for (i = 0u; i < 300u; ++i) {
        pr_u6f_insert_far node = far_at((uint16_t)(0x4000u + 0x20u * i),
                                        0x0100u);
        pr_u6f_insert_far next = far_at((uint16_t)(node.segment + 0x20u),
                                        node.offset);
        ram[address(node, 0xbfu)] = 0xffu;
        if (i == 0u)
            put_far(ram, far_at(DS, 0x5d96u), node);
        if (i + 1u < 300u)
            put_far(ram, far_at(node.segment, node.offset + 0x104u), next);
    }
    CHECK(pr_u6f_party_insert_1446_ss(&memory, &call, &result) ==
          PR_U6F_INSERT_SS_AT_U9B_TRANSFER);
    CHECK(result.append_nodes == 300u && result.mark_nodes == 301u);
    CHECK(result.selected_slot == 0u);
    return 0;
}

static int load_file(const char *path, uint8_t *destination)
{
    FILE *stream = fopen(path, "rb");
    int extra;
    if (!stream)
        return 0;
    if (fread(destination, 1u, SNAPSHOT_SIZE, stream) != SNAPSHOT_SIZE) {
        fclose(stream);
        return 0;
    }
    extra = fgetc(stream);
    fclose(stream);
    return extra == EOF;
}

static int test_saved(const char *before, const char *after,
                      pr_u6f_insert_far incoming,
                      pr_u6f_insert_far expected_head,
                      uint16_t expected_window, uint8_t expected_slot,
                      size_t expected_append, size_t expected_mark)
{
    pr_u6f_insert_dos_memory memory = {ram, sizeof ram, DS};
    pr_u6f_insert_ss_frame call = frame();
    pr_u6f_insert_ss_report result;
    pr_u6f_insert_far head = far_at(DS, 0x5d96u);
    pr_u6f_insert_far active = far_at(DS, 0x5d92u);
    pr_u6f_insert_far window = far_at(0x3a16u, 0x067cu);

    memset(ram, 0, sizeof ram);
    CHECK(load_file(before, ram) && load_file(after, native_after));
    CHECK(get16(ram, far_at(SS, 0x37b2u), 0u) == CS);
    CHECK(far_equals(ram, far_at(SS, BP + 6u), incoming));
    /* The capture precedes the near CALL; model its one return-IP push. */
    put16(ram, far_at(SS, 0x37b0u), 0u, 0x19e6u);
    CHECK(pr_u6f_party_insert_1446_ss(&memory, &call, &result) ==
          PR_U6F_INSERT_SS_AT_U9B_TRANSFER);
    CHECK(result.callee_bp == BP && result.selected_slot == expected_slot);
    CHECK(result.append_nodes == expected_append &&
          result.mark_nodes == expected_mark);
    CHECK(far_equals(ram, head, expected_head) &&
          far_equals(native_after, head, expected_head));
    CHECK(far_equals(ram, active, incoming) &&
          far_equals(native_after, active, incoming));
    CHECK(get16(ram, window, 0u) == expected_window &&
          get16(native_after, window, 0u) == expected_window);
    CHECK(ram[address(incoming, 0xbfu)] == expected_slot &&
          native_after[address(incoming, 0xbfu)] == expected_slot);
    if (expected_append != 0u) {
        pr_u6f_insert_far link = far_at(expected_head.segment,
                                        (uint16_t)(expected_head.offset + 0x104u));
        CHECK(far_equals(ram, link, incoming) &&
              far_equals(native_after, link, incoming));
    }
    return 0;
}

int main(int argc, char **argv)
{
    if (test_alias_and_transient_stack() ||
        test_disjoint_and_long_finite_list())
        return 1;
    if (argc != 1 && argc != 3 && argc != 5)
        return 2;
    if (argc >= 3 && test_saved(argv[1], argv[2],
                                far_at(0x449au, 0x0008u),
                                far_at(0x449au, 0x0008u),
                                1u, 0u, 0u, 1u))
        return 1;
    if (argc == 5 && test_saved(argv[3], argv[4],
                                far_at(0x44bbu, 0x0008u),
                                far_at(0x449au, 0x0008u),
                                2u, 1u, 1u, 2u))
        return 1;
    puts("u6F SS-memory frame tests passed");
    return 0;
}
