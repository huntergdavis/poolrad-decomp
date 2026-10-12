#include "poolrad/recovered/overlay_u6f_party_insert_1446_dos.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "%s:%d: %s\n", __FILE__, __LINE__, #x); return 1; \
} } while (0)

#define SNAPSHOT_SIZE 655360u

static uint8_t ram[PR_U6F_INSERT_DOS_RAM_SIZE];
static uint8_t expected[SNAPSHOT_SIZE];

static pr_u6f_insert_far far_at(uint16_t segment, uint16_t offset)
{
    pr_u6f_insert_far p = {offset, segment};
    return p;
}

static size_t address(pr_u6f_insert_far p, unsigned displacement)
{
    return (((size_t)p.segment << 4) +
            (uint16_t)(p.offset + displacement)) &
           (PR_U6F_INSERT_DOS_RAM_SIZE - 1u);
}

static void put8(pr_u6f_insert_far p, unsigned displacement, uint8_t value)
{
    ram[address(p, displacement)] = value;
}

static uint8_t get8(pr_u6f_insert_far p, unsigned displacement)
{
    return ram[address(p, displacement)];
}

static void put16(pr_u6f_insert_far p, unsigned displacement, uint16_t value)
{
    size_t a = address(p, displacement);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U6F_INSERT_DOS_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(pr_u6f_insert_far p, unsigned displacement)
{
    size_t a = address(p, displacement);
    return (uint16_t)((uint16_t)ram[a] |
                      (uint16_t)((uint16_t)ram[(a + 1u) &
                          (PR_U6F_INSERT_DOS_RAM_SIZE - 1u)] << 8));
}

static void putfar(uint16_t ds, uint16_t offset, pr_u6f_insert_far value)
{
    pr_u6f_insert_far base = far_at(ds, offset);
    put16(base, 0u, value.offset);
    put16(base, 2u, value.segment);
}

static int far_is(uint16_t ds, uint16_t offset, pr_u6f_insert_far value)
{
    pr_u6f_insert_far base = far_at(ds, offset);
    return get16(base, 0u) == value.offset &&
           get16(base, 2u) == value.segment;
}

static pr_u6f_insert_dos_memory memory_at(uint16_t ds)
{
    pr_u6f_insert_dos_memory memory = {ram, sizeof ram, ds};
    return memory;
}

static int test_empty_and_u9b(void)
{
    pr_u6f_insert_dos_memory memory = memory_at(0x1000u);
    pr_u6f_insert_far incoming = far_at(0x2000u, 0x0100u);
    pr_u6f_insert_far window = far_at(0x3000u, 0u);
    pr_u6f_insert_dos_report report;
    memset(ram, 0, sizeof ram);
    putfar(memory.ds, 0x49d6u, window);
    put16(window, 0x67cu, 0xffffu);
    put8(incoming, 0x12u, 0x11u);
    put8(incoming, 0xb2u, 1u);
    put8(incoming, 0xb3u, 2u);
    put8(incoming, 0xb4u, 3u);
    CHECK(pr_u6f_party_insert_1446_dos(&memory, incoming, &report) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(far_is(memory.ds, 0x5d96u, incoming));
    CHECK(far_is(memory.ds, 0x5d92u, incoming));
    CHECK(get8(incoming, 0xbfu) == 0u && report.selected_slot == 0u);
    CHECK(report.append_nodes == 0u && report.mark_nodes == 1u);
    CHECK(report.window_word == 0u && get16(window, 0x67cu) == 0u);
    CHECK(get8(incoming, 0xb2u) == 3u && get8(incoming, 0xb3u) == 4u &&
          get8(incoming, 0xb4u) == 4u);
    CHECK(report.callee.increments == 5u && report.callee.steps_taken == 0x1fu);
    return 0;
}

static int test_linked_and_occupied(void)
{
    pr_u6f_insert_dos_memory memory = memory_at(0x1000u);
    pr_u6f_insert_far first = far_at(0x2000u, 0x0100u);
    pr_u6f_insert_far second = far_at(0x2000u, 0x0300u);
    pr_u6f_insert_far incoming = far_at(0x2000u, 0x0500u);
    pr_u6f_insert_far window = far_at(0x3000u, 0u);
    pr_u6f_insert_dos_report report;
    unsigned n;
    memset(ram, 0, sizeof ram);
    putfar(memory.ds, 0x5d96u, first);
    putfar(memory.ds, 0x5d92u, far_at(0x2000u, 0x0700u));
    putfar(memory.ds, 0x49d6u, window);
    put16(first, 0x104u, second.offset);
    put16(first, 0x106u, second.segment);
    put8(first, 0xbfu, 0u);
    put8(second, 0xbfu, 2u);
    put16(window, 0x67cu, 3u);
    CHECK(pr_u6f_party_insert_1446_dos(&memory, incoming, &report) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(get16(second, 0x104u) == incoming.offset &&
          get16(second, 0x106u) == incoming.segment);
    CHECK(far_is(memory.ds, 0x5d92u, incoming));
    CHECK(report.append_nodes == 2u && report.mark_nodes == 3u);
    CHECK(report.selected_slot == 1u && get8(incoming, 0xbfu) == 1u);
    CHECK(get16(window, 0x67cu) == 4u);

    /* Eight occupied indices drive the source loop to the sentinel 8. */
    memset(ram, 0, sizeof ram);
    putfar(memory.ds, 0x49d6u, window);
    for (n = 0u; n < 8u; ++n) {
        pr_u6f_insert_far node = far_at(0x2000u, (uint16_t)(0x100u + n * 0x200u));
        put8(node, 0xbfu, (uint8_t)n);
        if (n == 0u)
            putfar(memory.ds, 0x5d96u, node);
        if (n < 7u) {
            pr_u6f_insert_far next = far_at(0x2000u,
                                              (uint16_t)(node.offset + 0x200u));
            put16(node, 0x104u, next.offset);
            put16(node, 0x106u, next.segment);
        }
    }
    incoming = far_at(0x2000u, 0x1200u);
    CHECK(pr_u6f_party_insert_1446_dos(&memory, incoming, &report) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(report.append_nodes == 8u && report.mark_nodes == 9u);
    CHECK(report.selected_slot == 8u && get8(incoming, 0xbfu) == 8u);
    return 0;
}

static int test_first_write_aliases_head(void)
{
    pr_u6f_insert_dos_memory memory = memory_at(0x1000u);
    pr_u6f_insert_far incoming = far_at(memory.ds, 0x5cd7u);
    pr_u6f_insert_far first = far_at(0u, 0x00ffu);
    pr_u6f_insert_far window = far_at(0x3000u, 0u);
    pr_u6f_insert_dos_report report;
    memset(ram, 0, sizeof ram);
    putfar(memory.ds, 0x49d6u, window);
    /* incoming +0xBF is the low byte of DS:5D96. Its first FF store turns
     * an initially null head into 0000:00FF before the branch reads it. */
    CHECK(address(incoming, 0xbfu) == address(far_at(memory.ds, 0x5d96u), 0u));
    put8(first, 0xbfu, 0u);
    CHECK(pr_u6f_party_insert_1446_dos(&memory, incoming, &report) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(report.append_nodes == 1u && report.mark_nodes == 2u);
    CHECK(get16(first, 0x104u) == incoming.offset &&
          get16(first, 0x106u) == incoming.segment);
    CHECK(report.selected_slot == 1u && get8(incoming, 0xbfu) == 1u);
    CHECK(far_is(memory.ds, 0x5d96u, far_at(0u, 1u)));
    CHECK(far_is(memory.ds, 0x5d92u, incoming));
    return 0;
}

static int test_long_finite_list(void)
{
    pr_u6f_insert_dos_memory memory = memory_at(0x1000u);
    pr_u6f_insert_far incoming = far_at(0x2400u, 0x4900u);
    pr_u6f_insert_dos_report report;
    unsigned i;
    memset(ram, 0, sizeof ram);
    putfar(memory.ds, 0x49d6u, far_at(0x3000u, 0u));
    for (i = 0u; i < 64u; ++i) {
        pr_u6f_insert_far node = far_at(0x2400u,
                                          (uint16_t)(0x100u + i * 0x120u));
        put8(node, 0xbfu, 0xffu);
        if (i == 0u) putfar(memory.ds, 0x5d96u, node);
        if (i + 1u < 64u) {
            pr_u6f_insert_far next = far_at(0x2400u,
                                              (uint16_t)(node.offset + 0x120u));
            put16(node, 0x104u, next.offset);
            put16(node, 0x106u, next.segment);
        }
    }
    CHECK(pr_u6f_party_insert_1446_dos(&memory, incoming, &report) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(report.append_nodes == 64u && report.mark_nodes == 65u);
    CHECK(report.selected_slot == 0u && get8(incoming, 0xbfu) == 0u);
    return 0;
}

static int test_window_aliases_u9b_counters(void)
{
    pr_u6f_insert_dos_memory memory = memory_at(0x1000u);
    pr_u6f_insert_far incoming = far_at(0x2000u, 0x0200u);
    pr_u6f_insert_far window = far_at(0x1f00u, 0x0c36u);
    pr_u6f_insert_dos_report report;
    memset(ram, 0, sizeof ram);
    CHECK(address(window, 0x67cu) == address(incoming, 0xb2u));
    putfar(memory.ds, 0x49d6u, window);
    put8(incoming, 0x12u, 0x11u);
    put8(incoming, 0xb2u, 0xffu);
    put8(incoming, 0xb3u, 1u);
    put8(incoming, 0xb4u, 1u);
    CHECK(pr_u6f_party_insert_1446_dos(&memory, incoming, &report) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(report.window_word == 0x0200u);
    CHECK(get8(incoming, 0xb2u) == 0u && get8(incoming, 0xb3u) == 4u &&
          get8(incoming, 0xb4u) == 2u);
    CHECK(report.callee.increments == 3u && report.callee.steps_taken == 0x1cu);
    CHECK(get16(window, 0x67cu) == 0x0400u);
    return 0;
}

static int test_physical_word_crosses_20bit_end(void)
{
    pr_u6f_insert_dos_memory memory = memory_at(0x1000u);
    pr_u6f_insert_far incoming = far_at(0x2000u, 0x0100u);
    pr_u6f_insert_far window = far_at(0xffffu, 0xf993u);
    pr_u6f_insert_dos_report report;
    memset(ram, 0, sizeof ram);
    CHECK(address(window, 0x67cu) == 0xfffffu);
    putfar(memory.ds, 0x49d6u, window);
    ram[0xfffffu] = 0xffu;
    ram[0] = 0x12u;
    ram[0xffff0u] = 0x34u;
    CHECK(pr_u6f_party_insert_1446_dos(&memory, incoming, &report) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(report.window_word == 0x1300u);
    CHECK(ram[0xfffffu] == 0u && ram[0] == 0x13u && ram[0xffff0u] == 0x34u);
    return 0;
}

static int test_invalid_host_view_stops_before_write(void)
{
    pr_u6f_insert_dos_memory memory = memory_at(0x1000u);
    pr_u6f_insert_far incoming = far_at(0x2000u, 0x0100u);
    pr_u6f_insert_dos_report report;
    memset(ram, 0, sizeof ram);
    put8(incoming, 0xbfu, 0x23u);
    memory.ram_size = sizeof ram - 1u;
    CHECK(pr_u6f_party_insert_1446_dos(&memory, incoming, &report) ==
          PR_U6F_INSERT_DOS_ARGUMENT);
    CHECK(get8(incoming, 0xbfu) == 0x23u);
    return 0;
}

static int read_snapshot(const char *path, uint8_t *bytes)
{
    FILE *stream = fopen(path, "rb");
    int trailing;
    if (!stream) return 1;
    if (fread(bytes, 1u, SNAPSHOT_SIZE, stream) != SNAPSHOT_SIZE) {
        fclose(stream);
        return 1;
    }
    trailing = fgetc(stream);
    if (fclose(stream) != 0) return 1;
    return trailing != EOF;
}

static int test_private_saved_case(const char *pre, const char *post)
{
    pr_u6f_insert_dos_memory memory = memory_at(0x14a0u);
    pr_u6f_insert_far incoming = far_at(0x449au, 8u);
    pr_u6f_insert_dos_report report;
    size_t record = address(incoming, 0u);
    size_t globals = address(far_at(memory.ds, 0x5d92u), 0u);
    size_t window = address(far_at(0x3a16u, 0u), 0x67cu);
    memset(ram, 0, sizeof ram);
    CHECK(read_snapshot(pre, ram) == 0 && read_snapshot(post, expected) == 0);
    CHECK(pr_u6f_party_insert_1446_dos(&memory, incoming, &report) ==
          PR_U6F_INSERT_DOS_OK);
    CHECK(report.append_nodes == 0u && report.mark_nodes == 1u &&
          report.selected_slot == 0u && report.window_word == 1u);
    CHECK(memcmp(ram + record, expected + record, 285u) == 0);
    CHECK(memcmp(ram + globals, expected + globals, 8u) == 0);
    CHECK(memcmp(ram + window, expected + window, 2u) == 0);
    return 0;
}

int main(int argc, char **argv)
{
    if (test_empty_and_u9b() || test_linked_and_occupied() ||
        test_first_write_aliases_head() || test_long_finite_list() ||
        test_window_aliases_u9b_counters() ||
        test_physical_word_crosses_20bit_end() ||
        test_invalid_host_view_stops_before_write()) return 1;
    if (argc == 3 && test_private_saved_case(argv[1], argv[2])) return 1;
    if (argc != 1 && argc != 3) return 2;
    puts("unit 6F party insertion DOS-memory tests passed");
    return 0;
}
