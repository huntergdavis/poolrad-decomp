#include "poolrad/replay/slot_a_saved_copy_1965.h"

#include <stdio.h>
#include <string.h>

#define RAM_SIZE 655360u
#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #x); \
    return 1; \
} } while (0)

static uint8_t ram[RAM_SIZE], expected[RAM_SIZE];

static int same_cpu(const pr_slot_a_directory_cpu *a,
                    const pr_slot_a_directory_cpu *b)
{
    return a->ax == b->ax && a->bx == b->bx && a->cx == b->cx &&
           a->dx == b->dx && a->si == b->si && a->di == b->di &&
           a->bp == b->bp && a->sp == b->sp && a->cs == b->cs &&
           a->ds == b->ds && a->es == b->es && a->ss == b->ss &&
           a->fs == b->fs && a->gs == b->gs && a->ip == b->ip &&
           a->flags == b->flags;
}

static void put_word(size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[at + 1u] = (uint8_t)(value >> 8);
}

static int synthetic(void)
{
    pr_slot_a_directory_state state = {0};
    pr_slot_a_directory_cpu before;
    pr_slot_a_saved_copy_report report;
    unsigned steps;
    state.ram = ram;
    state.ram_size = RAM_SIZE;
    state.cpu.cs = 0x2A8Eu;
    state.cpu.ip = 0x1965u;
    state.cpu.ss = 0x100u;
    state.cpu.sp = 0x100u;
    state.cpu.ds = 0xABCDu;
    state.cpu.ax = 0xBEEF;
    state.cpu.flags = 0xFFFFu;
    /* Source and destination pointers above the future return frame. */
    put_word(0x1100u, 0x20u);
    put_word(0x1102u, 0x200u);
    put_word(0x1104u, 0x30u);
    put_word(0x1106u, 0x300u);
    ram[0x2020u] = 3u;
    memcpy(ram + 0x2021u, "ABC", 3u);
    for (steps = 0u; steps < 13u; ++steps)
        CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
              PR_SLOT_A_SAVED_COPY_STEPPED);
    CHECK(state.cpu.cs == 0x2A8Eu && state.cpu.ip == 0x196Au);
    CHECK(state.cpu.sp == 0x104u && state.cpu.ds == 0xABCDu);
    CHECK(state.cpu.ax == 0xBE03u && state.cpu.cx == 0u);
    CHECK(state.cpu.si == 0x24u && state.cpu.di == 0x34u);
    CHECK((state.cpu.flags & 0x0400u) == 0u);
    CHECK((state.cpu.flags & 0x08D5u) == 0x0044u);
    CHECK(ram[0x3030u] == 3u && memcmp(ram + 0x3031u, "ABC", 3u) == 0);
    before = state.cpu;
    memcpy(expected, ram, sizeof ram);
    CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
          PR_SLOT_A_SAVED_COPY_BLOCKED_0356_ABI);
    CHECK(same_cpu(&state.cpu, &before));
    CHECK(memcmp(ram, expected, sizeof ram) == 0);
    CHECK(report.dependency_valid == 1u && report.caller_unit == 0x6Fu);
    CHECK(report.call_pc == 0x196Au && report.callee_segment == 0x0802u);
    CHECK(report.callee_pc == 0x0356u && report.return_pc == 0x196Fu);

    /* REP MOVSB reads each source byte after preceding forward writes. */
    state.cpu.cs = 0x131Cu;
    state.cpu.ip = 0x0B1Eu;
    state.cpu.ds = state.cpu.es = 0u;
    state.cpu.si = 0x100u;
    state.cpu.di = 0x101u;
    state.cpu.cx = 3u;
    ram[0x100u] = 0x41u;
    ram[0x101u] = 0x42u;
    ram[0x102u] = 0x43u;
    CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
          PR_SLOT_A_SAVED_COPY_STEPPED);
    CHECK(ram[0x101u] == 0x41u && ram[0x102u] == 0x41u && ram[0x103u] == 0x41u);
    state.cpu.ip = 0x0B1Eu;
    state.cpu.si = 0xFFFFu;
    state.cpu.cx = 2u;
    before = state.cpu;
    memcpy(expected, ram, sizeof ram);
    CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
          PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY);
    CHECK(same_cpu(&state.cpu, &before) && memcmp(ram, expected, sizeof ram) == 0);
    CHECK(report.dependency_valid == 0u); /* stale dependency invalidated */
    state.cpu.cx = 0u;
    state.cpu.ds = state.cpu.es = 0xFFFFu;
    CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
          PR_SLOT_A_SAVED_COPY_STEPPED); /* zero count touches no source/dest */

    state.cpu.cs = 0x2A8Eu;
    state.cpu.ip = 0x1965u;
    state.cpu.sp = 2u;
    before = state.cpu;
    CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
          PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY);
    CHECK(same_cpu(&state.cpu, &before) && memcmp(ram, expected, sizeof ram) == 0);
    state.cpu.cs = 0x131Cu;
    state.cpu.ip = 0x0B1Eu;
    state.cpu.ds = state.cpu.es = 0u;
    state.cpu.si = 0x100u;
    state.cpu.di = 0x104u;
    state.cpu.cx = 2u;
    state.ram_size = 0x105u; /* source fits, destination fails before any write */
    before = state.cpu;
    CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
          PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY);
    CHECK(same_cpu(&state.cpu, &before) && memcmp(ram, expected, sizeof ram) == 0);
    state.ram_size = RAM_SIZE;
    state.cpu.flags |= 0x0400u;
    before = state.cpu;
    CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
          PR_SLOT_A_SAVED_COPY_BLOCKED_PC); /* unreachable reverse-copy input */
    CHECK(same_cpu(&state.cpu, &before) && memcmp(ram, expected, sizeof ram) == 0);
    state.cpu.ip = 0x0B22u;
    state.cpu.ss = 0xFFFFu;
    before = state.cpu;
    CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
          PR_SLOT_A_SAVED_COPY_BLOCKED_MEMORY);
    CHECK(same_cpu(&state.cpu, &before) && memcmp(ram, expected, sizeof ram) == 0);
    CHECK(pr_slot_a_saved_copy_1965_step(NULL, &report) == PR_SLOT_A_SAVED_COPY_ARGUMENT);
    CHECK(pr_slot_a_saved_copy_1965_step(&state, NULL) == PR_SLOT_A_SAVED_COPY_ARGUMENT);
    return 0;
}

static int read_ram(const char *root, const char *label, uint8_t *out)
{
    char path[1024];
    FILE *f;
    int ok, length = snprintf(path, sizeof path, "%s/ram-%s.bin", root, label);
    if (length < 0 || (size_t)length >= sizeof path) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    ok = fread(out, 1u, RAM_SIZE, f) == RAM_SIZE && fgetc(f) == EOF;
    return fclose(f) == 0 && ok;
}

static int next_call(const char *root, const pr_slot_a_directory_state *state,
                     const pr_slot_a_saved_copy_report *report)
{
    char path[1024];
    uint8_t original[5];
    const uint8_t *loaded;
    unsigned offset, segment;
    size_t at = ((size_t)state->cpu.cs << 4) + state->cpu.ip;
    FILE *f;
    int length = snprintf(path, sizeof path, "%s/POOLRAD/GAME.OVR", root);
    CHECK(length >= 0 && (size_t)length < sizeof path);
    f = fopen(path, "rb");
    CHECK(f != NULL);
    CHECK(fseek(f, 0x1DFE6L + report->call_pc, SEEK_SET) == 0);
    CHECK(fread(original, 1u, sizeof original, f) == sizeof original);
    CHECK(fclose(f) == 0);
    offset = (unsigned)original[1] | ((unsigned)original[2] << 8);
    segment = (unsigned)original[3] | ((unsigned)original[4] << 8);
    CHECK(original[0] == 0x9Au && offset == report->callee_pc);
    CHECK(segment == report->callee_segment);
    CHECK(report->return_pc == report->call_pc + 5u);
    CHECK(at <= RAM_SIZE - sizeof original);
    loaded = state->ram + at;
    CHECK(loaded[0] == original[0] && loaded[1] == original[1] &&
          loaded[2] == original[2]);
    CHECK(((unsigned)loaded[3] | ((unsigned)loaded[4] << 8)) == segment + 0x0824u);
    return 0;
}

static int native(const char *root, const char *transcript_name,
                  unsigned expected_stops)
{
    char path[1024], line[512], label[16];
    unsigned v[16], stops = 0u, steps = 0u, made, i;
    pr_slot_a_directory_state state = {0};
    pr_slot_a_directory_cpu target, before;
    pr_slot_a_saved_copy_report report;
    FILE *f;
    int length = snprintf(path, sizeof path, "%s/%s", root, transcript_name);
    CHECK(length >= 0 && (size_t)length < sizeof path);
    f = fopen(path, "r");
    CHECK(f != NULL);
    state.ram = ram;
    state.ram_size = RAM_SIZE;
    while (fgets(line, sizeof line, f)) {
        CHECK(sscanf(line, "%15s %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x %x",
              label, &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7],
              &v[8], &v[9], &v[10], &v[11], &v[12], &v[13], &v[14], &v[15]) == 17);
        for (i = 0u; i < 16u; ++i) CHECK(v[i] <= 0xFFFFu);
        target = (pr_slot_a_directory_cpu){
            .cs = (uint16_t)v[0], .ip = (uint16_t)v[1],
            .ds = (uint16_t)v[2], .es = (uint16_t)v[3],
            .fs = (uint16_t)v[4], .gs = (uint16_t)v[5],
            .ss = (uint16_t)v[6], .sp = (uint16_t)v[7],
            .bp = (uint16_t)v[8], .ax = (uint16_t)v[9],
            .bx = (uint16_t)v[10], .cx = (uint16_t)v[11],
            .dx = (uint16_t)v[12], .si = (uint16_t)v[13],
            .di = (uint16_t)v[14], .flags = (uint16_t)v[15]};
        if (stops == 0u) {
            CHECK(strcmp(label, "1965") == 0);
            CHECK(read_ram(root, label, ram));
            state.cpu = target;
        } else {
            for (made = 0u; made < 32u &&
                 (state.cpu.cs != target.cs || state.cpu.ip != target.ip); ++made) {
                CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
                      PR_SLOT_A_SAVED_COPY_STEPPED);
                ++steps;
            }
            CHECK(made < 32u);
            CHECK(same_cpu(&state.cpu, &target));
            CHECK(read_ram(root, label, expected));
            CHECK(memcmp(ram, expected, sizeof ram) == 0);
        }
        ++stops;
    }
    CHECK(!ferror(f) && fclose(f) == 0);
    CHECK(stops == expected_stops && steps == 13u);
    CHECK(state.cpu.cs == 0x2A8Eu && state.cpu.ip == 0x196Au);
    before = state.cpu;
    memcpy(expected, ram, sizeof ram);
    CHECK(pr_slot_a_saved_copy_1965_step(&state, &report) ==
          PR_SLOT_A_SAVED_COPY_BLOCKED_0356_ABI);
    CHECK(same_cpu(&state.cpu, &before));
    CHECK(memcmp(ram, expected, sizeof ram) == 0);
    CHECK(report.dependency_valid && report.call_pc == 0x196Au);
    CHECK(next_call(root, &state, &report) == 0);
    printf("native: %u captured stops, %u instructions, all 16 CPU fields and 655360 RAM bytes match; next 0356 ABI blocked unchanged\n",
           stops, steps);
    return 0;
}

int main(int argc, char **argv)
{
    if (argc != 1 && argc != 3) return 2;
    if (synthetic()) return 1;
    if (argc == 3 &&
        (native(argv[1], "ev-transcript-1965-first.txt", 5u) ||
         native(argv[2], "ev-transcript-1965.txt", 7u))) return 1;
    puts("saved-A +1965 native call/copy/return and typed blocker passed");
    return 0;
}
