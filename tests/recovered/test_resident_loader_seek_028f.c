#include "poolrad/recovered/resident_loader_seek_028f.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                      \
    fprintf(stderr, "loader seek +028F line %d: %s\n", __LINE__, #c); \
    return 1;                                                           \
} } while (0)

static uint8_t ram[PR_HANDLER_02E6_RAM_SIZE];
static uint8_t saved_ram[PR_HANDLER_02E6_RAM_SIZE];

typedef struct {
    int mode;
    uint16_t handle;
    unsigned calls;
} fixture;

static pr_handler_02e6_cpu request(uint16_t handle, uint32_t offset)
{
    pr_handler_02e6_cpu cpu = {0};
    cpu.ax = 0x4200u;
    cpu.bx = handle;
    cpu.cx = (uint16_t)(offset >> 16);
    cpu.dx = (uint16_t)offset;
    cpu.cs = 0x12C6u;
    cpu.ip = 0x028Fu;
    cpu.ss = 0x1D16u;
    cpu.sp = 0x3770u;
    cpu.ds = 0x14A0u;
    cpu.es = 0x08D0u;
    cpu.flags = 0x7283u;
    return cpu;
}

static int seek_service(void *context, const pr_handler_02e6_cpu *before,
                        const uint8_t *memory, size_t memory_size,
                        pr_handler_02e6_cpu *after,
                        pr_loader_seek_028f_write *writes, size_t capacity,
                        size_t *count)
{
    fixture *f = context;
    uint32_t offset = ((uint32_t)before->cx << 16) | before->dx;
    ++f->calls;
    if (f->mode == 1 || before->bx != f->handle || memory_size != sizeof ram)
        return 1;
    *after = *before;
    after->ip = 0x0291u;
    if (f->mode == 2) {
        after->ax = 0x0006u; /* synthetic invalid-handle DOS error */
        after->flags |= 0x0001u;
        *count = 0u;
        return 0;
    }
    /* Absolute seek returns the new 32-bit position in DX:AX. */
    after->ax = (uint16_t)offset;
    after->dx = (uint16_t)(offset >> 16);
    after->flags &= (uint16_t)~0x0001u;
    if (f->mode == 3) after->ip = 0x0292u;
    if (f->mode == 6) {
        *count = capacity + 1u;
        return 0;
    }
    if (capacity < 2u) return 1;
    writes[0] = (pr_loader_seek_028f_write){0x0100u, memory[0x0100u],
                                               (uint8_t)offset};
    writes[1] = (pr_loader_seek_028f_write){0x0100u, (uint8_t)offset,
                                               (uint8_t)(offset >> 8)};
    if (f->mode == 4) writes[1].old_byte ^= 1u;
    if (f->mode == 5) writes[1].linear = 0x100000u;
    *count = 2u;
    return 0;
}

static int success_and_error(void)
{
    pr_loader_seek_028f_write writes[2];
    pr_handler_02e6_cpu cpu, before;
    fixture f = {0, 0x0005u, 0u};

    memset(ram, 0x47, sizeof ram);
    cpu = request(0x0005u, 0x12345678u);
    before = cpu;
    CHECK(pr_resident_loader_seek_028f(ram, sizeof ram, &cpu,
          seek_service, &f, writes, 2u) == PR_LOADER_SEEK_028F_POST_SERVICE);
    CHECK(f.calls == 1u && cpu.ip == 0x0291u);
    CHECK(cpu.ax == 0x5678u && cpu.dx == 0x1234u);
    CHECK(cpu.flags == (uint16_t)(before.flags & ~1u));
    CHECK(cpu.bx == before.bx && cpu.cx == before.cx);
    CHECK(ram[0x0100u] == 0x56u);

    cpu = request(0x0005u, 0xABCDEF01u);
    CHECK(pr_resident_loader_seek_028f(ram, sizeof ram, &cpu,
          seek_service, &f, writes, 2u) == PR_LOADER_SEEK_028F_POST_SERVICE);
    CHECK(cpu.ax == 0xEF01u && cpu.dx == 0xABCDu);
    CHECK(ram[0x0100u] == 0xEFu);

    f.mode = 2;
    cpu = request(0x0005u, 0x0002AA87u);
    CHECK(pr_resident_loader_seek_028f(ram, sizeof ram, &cpu,
          seek_service, &f, writes, 2u) == PR_LOADER_SEEK_028F_POST_SERVICE);
    CHECK(cpu.ax == 0x0006u && (cpu.flags & 1u) != 0u);
    CHECK(cpu.ip == 0x0291u && ram[0x0100u] == 0xEFu);
    return 0;
}

static int blocked_and_invalid(void)
{
    pr_loader_seek_028f_write writes[2];
    pr_handler_02e6_cpu cpu, before;
    fixture f = {0, 0x0005u, 0u};
    int mode;

    memset(ram, 0x29, sizeof ram);
    cpu = request(0x0005u, 0x0002AA87u);
    before = cpu;
    memcpy(saved_ram, ram, sizeof ram);
    CHECK(pr_resident_loader_seek_028f(ram, sizeof ram, &cpu,
          NULL, &f, writes, 2u) ==
          PR_LOADER_SEEK_028F_BLOCKED_DOS_SERVICE);
    CHECK(f.calls == 0u);
    f.mode = 1;
    CHECK(pr_resident_loader_seek_028f(ram, sizeof ram, &cpu,
          seek_service, &f, writes, 2u) ==
          PR_LOADER_SEEK_028F_BLOCKED_DOS_SERVICE);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);

    for (mode = 3; mode <= 6; ++mode) {
        f.mode = mode;
        CHECK(pr_resident_loader_seek_028f(ram, sizeof ram, &cpu,
              seek_service, &f, writes, 2u) == PR_LOADER_SEEK_028F_BAD_EVENT);
        CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
        CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);
    }

    f.mode = 0;
    cpu.ip = 0x0291u;
    CHECK(pr_resident_loader_seek_028f(ram, sizeof ram, &cpu,
          seek_service, &f, writes, 2u) == PR_LOADER_SEEK_028F_ARGUMENT);
    cpu = before;
    CHECK(pr_resident_loader_seek_028f(ram, sizeof ram - 1u, &cpu,
          seek_service, &f, writes, 2u) == PR_LOADER_SEEK_028F_ARGUMENT);
    CHECK(pr_resident_loader_seek_028f(ram, sizeof ram, &cpu,
          seek_service, &f, NULL, 2u) == PR_LOADER_SEEK_028F_ARGUMENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);
    return 0;
}

int main(void)
{
    if (success_and_error() || blocked_and_invalid()) return 1;
    puts("resident loader +028F synthetic service events: passed");
    return 0;
}
