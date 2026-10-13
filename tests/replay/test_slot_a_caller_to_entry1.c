#include "poolrad/replay/slot_a_caller_to_entry1.h"

#include <stdio.h>
#include <string.h>

#define CHECK(expr) do { if (!(expr)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #expr); \
    return 1; \
} } while (0)
#define RAM_SIZE 655360u
#define DS_SEG 0x14A0u

static uint8_t and_ram[RAM_SIZE], search_ram[RAM_SIZE];

static size_t physical(unsigned seg, unsigned off)
{
    return ((size_t)seg << 4) + off;
}

static uint16_t word_at(const uint8_t *ram, unsigned seg, unsigned off)
{
    size_t p = physical(seg, off);
    return (uint16_t)((uint16_t)ram[p] | ((uint16_t)ram[p + 1u] << 8));
}

static int load_ram(const char *path, uint8_t *ram)
{
    FILE *file = fopen(path, "rb");
    int complete;
    if (!file) return 0;
    complete = fread(ram, 1u, RAM_SIZE, file) == RAM_SIZE &&
               fgetc(file) == EOF;
    if (fclose(file) != 0) return 0;
    return complete;
}

static int test_synthetic(void)
{
    uint8_t window0[0x200u];
    pr_slot_a_caller_to_entry1_state st = {0u, 0u, 15u, 4u,
                                             window0, sizeof window0};
    pr_slot_a_caller_to_entry1_report report = {0u, 0u};
    memset(window0, 0xA5, sizeof window0);
    CHECK(pr_slot_a_caller_to_entry1(NULL, &report) == PR_SLOT_A_CALLER_ARGUMENT);
    CHECK(pr_slot_a_caller_to_entry1(&st, NULL) == PR_SLOT_A_CALLER_ARGUMENT);
    CHECK(pr_slot_a_caller_to_entry1(&st, &report) == PR_SLOT_A_CALLER_ENTRY1);
    CHECK(report.next_pc == 0x348Du && report.window0_words_written == 2u);
    CHECK(window0[0x1E0u] == 15u && window0[0x1E1u] == 0u);
    CHECK(window0[0x1E2u] == 4u && window0[0x1E3u] == 0u);
    CHECK(window0[0x1DFu] == 0xA5u && window0[0x1E4u] == 0xA5u);

    st.flag_442f = 1u;
    st.window0 = NULL;
    CHECK(pr_slot_a_caller_to_entry1(&st, &report) == PR_SLOT_A_CALLER_31B4);
    CHECK(report.next_pc == 0x3462u && report.window0_words_written == 0u);
    st.flag_442f = 0u;
    st.flag_49ff = 1u;
    CHECK(pr_slot_a_caller_to_entry1(&st, &report) == PR_SLOT_A_CALLER_TAIL);
    CHECK(report.next_pc == 0x34F5u && report.window0_words_written == 0u);

    st.flag_49ff = 0u;
    st.window0 = window0;
    st.party_x_6aad = 0x90u;
    st.party_y_6aae = 0x80u;
    st.window0_size = 0x1E1u;
    memset(window0, 0xA5, sizeof window0);
    CHECK(pr_slot_a_caller_to_entry1(&st, &report) == PR_SLOT_A_CALLER_ARGUMENT);
    CHECK(report.next_pc == 0x347Bu && report.window0_words_written == 0u);
    CHECK(window0[0x1E0u] == 0xA5u);
    st.window0_size = 0x1E2u;
    CHECK(pr_slot_a_caller_to_entry1(&st, &report) == PR_SLOT_A_CALLER_ARGUMENT);
    CHECK(report.next_pc == 0x3488u && report.window0_words_written == 1u);
    CHECK(window0[0x1E0u] == 0x90u && window0[0x1E1u] == 0xFFu);
    CHECK(window0[0x1E2u] == 0xA5u);
    st.window0_size = 0x1E4u;
    CHECK(pr_slot_a_caller_to_entry1(&st, &report) == PR_SLOT_A_CALLER_ENTRY1);
    CHECK(window0[0x1E2u] == 0x80u && window0[0x1E3u] == 0xFFu);
    return 0;
}

static int test_native_projection(const char *and_path, const char *search_path)
{
    uint8_t window0[0x800u];
    uint16_t off, seg;
    pr_slot_a_caller_to_entry1_state st;
    pr_slot_a_caller_to_entry1_report report;
    CHECK(load_ram(and_path, and_ram));
    CHECK(load_ram(search_path, search_ram));
    off = word_at(and_ram, DS_SEG, 0x49D2u);
    seg = word_at(and_ram, DS_SEG, 0x49D4u);
    CHECK(off == 0u && seg == 0x3996u);
    CHECK(word_at(search_ram, DS_SEG, 0x49D2u) == off);
    CHECK(word_at(search_ram, DS_SEG, 0x49D4u) == seg);
    CHECK(physical(seg, off) + sizeof window0 <= RAM_SIZE);
    memcpy(window0, and_ram + physical(seg, off), sizeof window0);
    st = (pr_slot_a_caller_to_entry1_state){
        and_ram[physical(DS_SEG, 0x442Fu)],
        and_ram[physical(DS_SEG, 0x49FFu)],
        and_ram[physical(DS_SEG, 0x6AADu)],
        and_ram[physical(DS_SEG, 0x6AAEu)],
        window0, sizeof window0};
    CHECK(st.flag_442f == 0u && st.flag_49ff == 0u);
    CHECK(st.party_x_6aad == 15u && st.party_y_6aae == 4u);
    CHECK(word_at(and_ram, seg, off + 0x1E0u) == 1u);
    CHECK(word_at(and_ram, seg, off + 0x1E2u) == 4u);
    CHECK(pr_slot_a_caller_to_entry1(&st, &report) == PR_SLOT_A_CALLER_ENTRY1);
    CHECK(report.next_pc == 0x348Du && report.window0_words_written == 2u);
    CHECK(window0[0x1E0u] == 15u && window0[0x1E1u] == 0u);
    CHECK(window0[0x1E2u] == 4u && window0[0x1E3u] == 0u);
    CHECK(word_at(search_ram, seg, off + 0x1E0u) == 15u);
    CHECK(word_at(search_ram, seg, off + 0x1E2u) == 4u);
    CHECK(search_ram[physical(DS_SEG, 0x6AADu)] == 14u);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 1 && argc != 3) return 2;
    if (test_synthetic()) return 1;
    if (argc == 3 && test_native_projection(argv[1], argv[2])) return 1;
    puts("slot-A caller gate to entry 1 boundary passed");
    return 0;
}
