#include "poolrad/recovered/resident_loader_composed_028f.h"

#include <stdio.h>
#include <string.h>

#define CHECK(c) do { if (!(c)) {                                         \
    fprintf(stderr, "loader composed +028F line %d: %s\n", __LINE__, #c); \
    return 1;                                                              \
} } while (0)

static uint8_t ram[PR_HANDLER_02E6_RAM_SIZE];
static uint8_t saved_ram[PR_HANDLER_02E6_RAM_SIZE];
static pr_loader_seek_028f_write seek_journal[2];
static pr_loader_dos_read_029f_write read_journal[6];

typedef struct {
    unsigned seek_calls, read_calls;
    unsigned read_count;
    int seek_bad, read_bad, read_error;
} fixture;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_HANDLER_02E6_RAM_SIZE - 1u);
}

static void set_word(uint16_t segment, uint16_t offset, uint16_t value)
{
    ram[physical(segment, offset)] = (uint8_t)value;
    ram[physical(segment, (uint16_t)(offset + 1u))] =
        (uint8_t)(value >> 8);
}

static pr_handler_02e6_cpu request(void)
{
    pr_handler_02e6_cpu cpu = {0};
    cpu.ax = 0x4200u;
    cpu.bx = 5u;
    cpu.cx = 0x0002u;
    cpu.dx = 0x0100u;
    cpu.si = 0x44BAu;
    cpu.di = 0x0824u;
    cpu.bp = 0x3794u;
    cpu.sp = 0x3770u;
    cpu.cs = 0x12C6u;
    cpu.ip = 0x028Fu;
    cpu.ds = 0x14A0u;
    cpu.es = 0x08D0u;
    cpu.ss = 0x1D16u;
    cpu.flags = 0x7202u;
    return cpu;
}

static void setup(pr_handler_02e6_cpu *cpu)
{
    memset(ram, 0x7Cu, sizeof ram);
    *cpu = request();
    set_word(cpu->es, 0x0008u, 4u);
    set_word(cpu->es, 0x000Au, 0u);
    set_word(cpu->es, 0x0010u, 0x2CCDu);
    set_word(cpu->ss, cpu->sp, cpu->ds);
    set_word(cpu->ss, (uint16_t)(cpu->sp + 2u), 0x1234u);
    set_word(cpu->ss, (uint16_t)(cpu->sp + 4u), 0x5678u);
    set_word(cpu->ss, (uint16_t)(cpu->sp + 6u), cpu->es);
}

static int seek_event(void *context, const pr_handler_02e6_cpu *before,
                      const uint8_t *memory, size_t memory_size,
                      pr_handler_02e6_cpu *after,
                      pr_loader_seek_028f_write *writes, size_t capacity,
                      size_t *count)
{
    fixture *f = context;
    ++f->seek_calls;
    if (before->ax != 0x4200u || before->ip != 0x028Fu ||
        memory_size != sizeof ram || capacity < 1u) return 1;
    *after = *before;
    after->ax = 0x0020u; /* synthetic host seek result */
    after->dx = 0u;
    after->ip = 0x0291u;
    after->flags &= (uint16_t)~1u;
    writes[0] = (pr_loader_seek_028f_write){
        0x0120u, memory[0x0120u], 0x61u};
    if (f->seek_bad) writes[0].old_byte ^= 1u;
    *count = 1u;
    return 0;
}

static int read_event(void *context, const pr_handler_02e6_cpu *before,
                      const uint8_t *memory, size_t memory_size,
                      pr_handler_02e6_cpu *after,
                      pr_loader_dos_read_029f_write *writes, size_t capacity,
                      size_t *count)
{
    fixture *f = context;
    unsigned i;
    ++f->read_calls;
    if (before->ax != 0x3F20u || before->cx != 4u ||
        before->ds != 0x2CCDu || before->dx != 0u ||
        memory_size != sizeof ram || capacity < 6u) return 1;
    *after = *before;
    after->ip = 0x02A1u;
    if (f->read_error) {
        after->ax = 5u;
        after->flags |= 1u;
        *count = 0u;
        return 0;
    }
    after->ax = (uint16_t)f->read_count;
    after->flags &= (uint16_t)~1u;
    for (i = 0; i < f->read_count; ++i) {
        size_t address = physical(before->ds, (uint16_t)(before->dx + i));
        writes[i] = (pr_loader_dos_read_029f_write){
            (uint32_t)address, memory[address], (uint8_t)(0x20u + i)};
    }
    writes[f->read_count] = (pr_loader_dos_read_029f_write){
        0x0130u, memory[0x0130u], 0x42u};
    if (f->read_bad) writes[0].old_byte ^= 1u;
    *count = f->read_count + 1u;
    return 0;
}

static pr_loader_composed_028f_services services(fixture *f)
{
    pr_loader_composed_028f_services s = {0};
    s.seek = seek_event;
    s.seek_context = f;
    s.seek_journal = seek_journal;
    s.seek_capacity = 2u;
    s.read = read_event;
    s.read_context = f;
    s.read_journal = read_journal;
    s.read_capacity = 6u;
    return s;
}

static int complete_and_resume(void)
{
    pr_handler_02e6_cpu cpu;
    fixture f = {0};
    pr_loader_composed_028f_services s = services(&f);
    size_t base = physical(0x2CCDu, 0u);
    setup(&cpu);
    f.read_count = 4u;
    memcpy(saved_ram, ram, sizeof ram);

    s.seek = NULL;
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_NEED_SEEK);
    CHECK(cpu.ip == 0x028Fu && f.seek_calls == 0u && f.read_calls == 0u);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);

    s.seek = seek_event;
    s.read = NULL;
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_NEED_FIRST_READ);
    CHECK(cpu.ip == 0x029Fu && cpu.ax == 0x3F20u && cpu.cx == 4u);
    CHECK(f.seek_calls == 1u && f.read_calls == 0u && ram[0x0120u] == 0x61u);

    s.read = read_event;
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_AT_CALLER);
    CHECK(f.seek_calls == 1u && f.read_calls == 1u);
    CHECK(cpu.cs == 0x5678u && cpu.ip == 0x1234u && cpu.sp == 0x3778u);
    CHECK(cpu.ds == 0x14A0u && cpu.ax == 0u && cpu.cx == 0u);
    CHECK(cpu.flags == 0x7246u && cpu.bx == 5u);
    CHECK(ram[base] == 0x20u && ram[base + 3u] == 0x23u);
    CHECK(ram[0x0130u] == 0x42u);
    return 0;
}

static int branch_exits_and_bad_events(void)
{
    pr_handler_02e6_cpu cpu, before;
    fixture f = {0};
    pr_loader_composed_028f_services s = services(&f);

    setup(&cpu);
    before = cpu;
    f.seek_bad = 1;
    memcpy(saved_ram, ram, sizeof ram);
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_BAD_SEEK_EVENT);
    CHECK(memcmp(&cpu, &before, sizeof cpu) == 0);
    CHECK(memcmp(ram, saved_ram, sizeof ram) == 0);

    setup(&cpu);
    f.seek_bad = 0;
    f.read_bad = 1;
    f.read_count = 4u;
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_BAD_READ_EVENT);
    CHECK(cpu.ip == 0x029Fu && ram[0x0120u] == 0x61u);

    setup(&cpu);
    f.read_bad = 0;
    f.read_count = 2u;
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_AT_CALLER);
    CHECK(cpu.ax == 0x0064u && cpu.cs == 0x5678u && cpu.ip == 0x1234u);

    setup(&cpu);
    f.read_error = 1;
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_AT_CALLER);
    CHECK(cpu.ax == 5u && (cpu.flags & 1u) != 0u);

    setup(&cpu);
    f.read_error = 0;
    f.read_count = 4u;
    set_word(cpu.es, 0x000Au, 2u);
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_NEED_SECOND_READ);
    CHECK(cpu.cs == 0x12C6u && cpu.ip == 0x02C0u && cpu.sp == 0x3770u);
    CHECK(cpu.ax == 0x3FCEu && cpu.cx == 2u && cpu.ds == 0x2CCEu);
    return 0;
}

static int epilogue_wrap_and_arguments(void)
{
    pr_handler_02e6_cpu cpu;
    pr_loader_composed_028f_services s = {0};
    setup(&cpu);
    cpu.ip = 0x02E1u;
    cpu.sp = 0xFFFCu;
    set_word(cpu.ss, 0xFFFCu, 0xABCDu);
    set_word(cpu.ss, 0xFFFEu, 0x4321u);
    set_word(cpu.ss, 0x0000u, 0x8765u);
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_AT_CALLER);
    CHECK(cpu.ds == 0xABCDu && cpu.cs == 0x8765u && cpu.ip == 0x4321u);
    CHECK(cpu.sp == 0x0004u);

    setup(&cpu);
    CHECK(pr_resident_loader_composed_028f(NULL, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_ARGUMENT);
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram - 1u, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_ARGUMENT);
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, NULL, &s) ==
          PR_LOADER_COMPOSED_028F_ARGUMENT);
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, NULL) ==
          PR_LOADER_COMPOSED_028F_ARGUMENT);
    cpu.ip = 0x02C0u;
    CHECK(pr_resident_loader_composed_028f(ram, sizeof ram, &cpu, &s) ==
          PR_LOADER_COMPOSED_028F_ARGUMENT);
    return 0;
}

int main(void)
{
    if (complete_and_resume() || branch_exits_and_bad_events() ||
        epilogue_wrap_and_arguments()) return 1;
    puts("resident loader composed seek/read/return synthetic paths: passed");
    return 0;
}
