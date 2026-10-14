#include "poolrad/replay/slot_a_caller_to_entry1.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RAM_SIZE 655360u
#define DS_SEG 0x14A0u

static uint8_t before[RAM_SIZE];
static uint8_t after[RAM_SIZE];

static size_t physical(unsigned segment, unsigned offset)
{
    return ((size_t)segment << 4) + offset;
}

static unsigned word_at(const uint8_t *ram, unsigned segment, unsigned offset)
{
    size_t at = physical(segment, offset);
    return (unsigned)ram[at] | ((unsigned)ram[at + 1u] << 8);
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

int main(int argc, char **argv)
{
    pr_slot_a_caller_to_entry1_state state;
    pr_slot_a_caller_to_entry1_report report;
    unsigned window_segment, window_offset;
    size_t window_at, i;

    if (argc != 3 || !load_ram(argv[1], before) ||
        !load_ram(argv[2], after)) {
        fputs("usage: test_slot_a_post_exit_native BEFORE_345B AFTER_348D\n",
              stderr);
        return 2;
    }
    window_offset = word_at(before, DS_SEG, 0x49D2u);
    window_segment = word_at(before, DS_SEG, 0x49D4u);
    window_at = physical(window_segment, window_offset);
    if (window_at >= RAM_SIZE) return 1;
    state = (pr_slot_a_caller_to_entry1_state){
        before[physical(DS_SEG, 0x442Fu)],
        before[physical(DS_SEG, 0x49FFu)],
        before[physical(DS_SEG, 0x6AADu)],
        before[physical(DS_SEG, 0x6AAEu)],
        before + window_at,
        RAM_SIZE - window_at
    };
    if (pr_slot_a_caller_to_entry1(&state, &report) !=
            PR_SLOT_A_CALLER_ENTRY1 ||
        report.next_pc != 0x348Du || report.window0_words_written != 2u) {
        fputs("caller model did not reach entry 1\n", stderr);
        return 1;
    }
    if (memcmp(before, after, RAM_SIZE) != 0) {
        for (i = 0u; i < RAM_SIZE; ++i) {
            if (before[i] != after[i]) {
                fprintf(stderr, "full RAM mismatch at physical 0x%zx\n", i);
                break;
            }
        }
        return 1;
    }
    puts("direct native +345B to +348D full-RAM C parity passed");
    return 0;
}
