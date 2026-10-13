#include "poolrad/recovered/resident_loader_dos_read_029f.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                       \
    fprintf(stderr, "loader read +029F line %d: %s\n", __LINE__, #c); \
    return 1;                                                            \
} } while (0)

static uint8_t ram[PR_HANDLER_02E6_RAM_SIZE];
static uint8_t saved_ram[PR_HANDLER_02E6_RAM_SIZE];
static pr_loader_dos_read_029f_write large_journal[
    PR_LOADER_DOS_READ_029F_MAX_WRITES + 1u];

typedef struct {
    int mode;
    uint8_t payload[4];
    unsigned calls;
} fixture;

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (PR_HANDLER_02E6_RAM_SIZE - 1u);
}

static pr_handler_02e6_cpu request(uint16_t count)
{
    pr_handler_02e6_cpu cpu = {0};
    cpu.ax = 0x3F87u;
    cpu.bx = 0x0005u;
    cpu.cx = count;
    cpu.dx = 0u;
    cpu.si = 0x44BAu;
    cpu.di = 0x0824u;
    cpu.bp = 0x3794u;
    cpu.sp = 0x3770u;
    cpu.cs = 0x12C6u;
    cpu.ip = 0x029Fu;
    cpu.ds = 0x2CCDu;
    cpu.es = 0x08D0u;
    cpu.ss = 0x1D16u;
    cpu.flags = 0x7247u; /* synthetic incoming CF, cleared on success */
    return cpu;
}

static int read_service(void *context, const pr_handler_02e6_cpu *before,
                        const uint8_t *memory, size_t memory_size,
                        pr_handler_02e6_cpu *after,
                        pr_loader_dos_read_029f_write *writes,
                        size_t capacity, size_t *count)
{
    fixture *f = context;
    size_t i, bytes = f->mode == 1 ? 2u : 4u;
    ++f->calls;
    if (f->mode == 3 || before->bx != 5u ||
        memory_size != sizeof ram) return 1;
    *after = *before;
    after->ip = 0x02A1u;
    if (f->mode == 2) {
        after->ax = 0x0005u; /* synthetic DOS error */
        after->flags |= 1u;
        *count = 0u;
        return 0;
    }
    after->ax = (uint16_t)bytes;
    after->flags &= (uint16_t)~1u;
    if (f->mode == 4) after->ip = 0x02A2u;
    if (f->mode == 7) {
        *count = capacity + 1u;
        return 0;
    }
    if (f->mode == 8) {
        *count = PR_LOADER_DOS_READ_029F_MAX_WRITES + 1u;
        return 0;
    }
    if (capacity < bytes + (f->mode == 1 ? 0u : 2u)) return 1;
    for (i = 0; i < bytes; ++i) {
        size_t linear = at(before->ds, (uint16_t)(before->dx + i));
        writes[i] = (pr_loader_dos_read_029f_write){
            (uint32_t)linear, memory[linear], f->payload[i]};
    }
    if (f->mode != 1) {
        writes[bytes] = (pr_loader_dos_read_029f_write){
            0x0100u, memory[0x0100u], 1u};
        writes[bytes + 1u] = (pr_loader_dos_read_029f_write){
            0x0100u, 1u, 2u};
        *count = bytes + 2u;
    } else {
        *count = bytes;
    }
    if (f->mode == 5) writes[0].old_byte ^= 1u;
    if (f->mode == 6) writes[0].linear = 0x100000u;
    return 0;
}

static int complete_short_and_error(void)
{
    pr_loader_dos_read_029f_write journal[6];
    fixture f = {0, {0x21u, 0x43u, 0x65u, 0x87u}, 0u};
    pr_handler_02e6_cpu cpu;
    size_t base = at(0x2CCDu, 0u);

    memset(ram, 0x29, sizeof ram);
    cpu = request(4u);
    CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram, &cpu,
          read_service, &f, journal, 6u) ==
          PR_LOADER_DOS_READ_029F_POST_SERVICE);
    CHECK(f.calls == 1u && cpu.ip == 0x02A1u);
    CHECK(cpu.ax == 4u && (cpu.flags & 1u) == 0u);
    CHECK(memcmp(ram + base, f.payload, 4u) == 0 && ram[0x0100u] == 2u);

    f.mode = 1;
    cpu = request(4u);
    memset(ram + base, 0x29, 4u);
    CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram, &cpu,
          read_service, &f, journal, 6u) ==
          PR_LOADER_DOS_READ_029F_POST_SERVICE);
    CHECK(cpu.ax == 2u && ram[base] == 0x21u && ram[base + 1u] == 0x43u);
    CHECK(ram[base + 2u] == 0x29u && ram[base + 3u] == 0x29u);

    f.mode = 2;
    cpu = request(4u);
    CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram, &cpu,
          read_service, &f, journal, 6u) ==
          PR_LOADER_DOS_READ_029F_POST_SERVICE);
    CHECK(cpu.ax == 5u && (cpu.flags & 1u) != 0u);
    CHECK(cpu.ip == 0x02A1u && ram[base] == 0x21u);
    return 0;
}

static int blocked_and_bad_events(void)
{
    pr_loader_dos_read_029f_write journal[6];
    fixture f = {0, {1u, 2u, 3u, 4u}, 0u};
    pr_handler_02e6_cpu cpu = request(4u), before = cpu;
    int mode;

    memset(ram, 0x38, sizeof ram);
    memcpy(saved_ram, ram, sizeof ram);
    CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram, &cpu,
          NULL, &f, journal, 6u) ==
          PR_LOADER_DOS_READ_029F_BLOCKED_DOS_SERVICE);
    CHECK(f.calls == 0u);
    f.mode = 3;
    CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram, &cpu,
          read_service, &f, journal, 6u) ==
          PR_LOADER_DOS_READ_029F_BLOCKED_DOS_SERVICE);
    for (mode = 4; mode <= 7; ++mode) {
        f.mode = mode;
        CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram, &cpu,
              read_service, &f, journal, 6u) ==
              PR_LOADER_DOS_READ_029F_BAD_EVENT);
        CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
        CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);
    }
    f.mode = 8;
    CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram, &cpu,
          read_service, &f, large_journal,
          PR_LOADER_DOS_READ_029F_MAX_WRITES + 1u) ==
          PR_LOADER_DOS_READ_029F_BAD_EVENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);
    return 0;
}

static int arguments(void)
{
    pr_loader_dos_read_029f_write journal[6];
    fixture f = {0, {1u, 2u, 3u, 4u}, 0u};
    pr_handler_02e6_cpu cpu = request(4u), before = cpu;
    memset(ram, 0x5Au, sizeof ram);
    memcpy(saved_ram, ram, sizeof ram);
    CHECK(pr_resident_loader_dos_read_029f(NULL, sizeof ram, &cpu,
          read_service, &f, journal, 6u) == PR_LOADER_DOS_READ_029F_ARGUMENT);
    CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram - 1u, &cpu,
          read_service, &f, journal, 6u) == PR_LOADER_DOS_READ_029F_ARGUMENT);
    CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram, NULL,
          read_service, &f, journal, 6u) == PR_LOADER_DOS_READ_029F_ARGUMENT);
    CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram, &cpu,
          read_service, &f, NULL, 6u) == PR_LOADER_DOS_READ_029F_ARGUMENT);
    cpu.ip = 0x02A1u;
    CHECK(pr_resident_loader_dos_read_029f(ram, sizeof ram, &cpu,
          read_service, &f, journal, 6u) == PR_LOADER_DOS_READ_029F_ARGUMENT);
    cpu = before;
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0 && f.calls == 0u);
    return 0;
}

int main(void)
{
    if (complete_short_and_error() || blocked_and_bad_events() || arguments())
        return 1;
    puts("resident loader +029F synthetic read events: passed");
    return 0;
}
