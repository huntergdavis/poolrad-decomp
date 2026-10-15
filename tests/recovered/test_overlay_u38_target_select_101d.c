#include "poolrad/recovered/overlay_u38_target_select_101d.h"
#include "poolrad/recovered/resident_character_target_select.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

typedef struct {
    uint8_t *ram;
    uint16_t ds;
    uint16_t loaded_segment;
    int calls, abort;
} fixture;

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (RAM_SIZE - 1u);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = at(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & (RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static bool read_word(void *context, uint16_t seg, uint16_t off,
                      uint16_t *value)
{
    fixture *f = context;
    *value = get16(f->ram, seg, off);
    return true;
}

static bool read_byte(void *context, uint16_t seg, uint16_t off,
                      uint8_t *value)
{
    fixture *f = context;
    *value = f->ram[at(seg, off)];
    return true;
}

static bool read_global(void *context, uint16_t off, uint8_t *value)
{
    fixture *f = context;
    *value = f->ram[at(f->ds, off)];
    return true;
}

static bool write_word(void *context, uint16_t seg, uint16_t off,
                       uint16_t value)
{
    fixture *f = context;
    put16(f->ram, seg, off, value);
    return true;
}

static int select_target(void *context, uint8_t *ram, size_t size,
                         pr_u38_101d_cpu *cpu)
{
    fixture *f = context;
    pr_rec_target_select_io io = {f, read_word, read_byte, read_global,
                                  write_word};
    pr_rec_select_far_ptr output, character;
    uint16_t ax = 0;
    assert(size == RAM_SIZE && ram == f->ram);
    assert(cpu->cs == f->loaded_segment && cpu->ip == 0x2E8Bu);
    assert(get16(ram, cpu->ss, cpu->sp) == 0x104Au);
    assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == 0xF000u);
    output.offset = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
    output.segment = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
    character.offset = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u));
    character.segment = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 10u));
    assert(output.offset == (uint16_t)(cpu->bp - 8u) &&
           output.segment == cpu->ss);
    assert(character.offset == 0x2300u && character.segment == 0x4000u);
    ++f->calls;
    if (f->abort) return 0;
    assert(pr_rec_res_00ba_2e8b(&io, character, output, &ax) ==
           PR_REC_TARGET_SELECT_OK);
    cpu->ax = ax;
    cpu->bx = 0xBEEFu;
    cpu->flags = 0x0247u;
    return 1;
}

static void run(int mode, uint16_t loaded_segment)
{
    static const uint8_t flags[] = {0, 0x10, 0, 0x0A, 0x09, 0x88, 0x08, 0x10};
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_101d_cpu cpu = {0};
    fixture f = {ram, 0x3000u, loaded_segment, 0, mode == 7};
    uint16_t expected_off = 0, expected_seg = 0;
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bx = 0x3456u;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x101Du;
    cpu.ds = f.ds;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.cs, 0x1048u, loaded_segment);
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u), 0x2300u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 6u), 0x4000u);
    if (mode != 0) {
        put16(ram, 0x4000u, 0x23CCu, 0x2200u);
        put16(ram, 0x4000u, 0x23CEu, 0x5000u);
    }
    put16(ram, 0x4000u, 0x23F8u, 0x3300u);
    put16(ram, 0x4000u, 0x23FAu, 0x6000u);
    put16(ram, 0x4000u, 0x23FCu, 0x4400u);
    put16(ram, 0x4000u, 0x23FEu, 0x7000u);
    ram[at(0x5000u, 0x222Eu)] = 7u;
    ram[at(f.ds, 0x5600u)] = flags[mode];
    if (mode == 7) {
        assert(pr_u38_target_select_101d(ram, RAM_SIZE, &cpu,
                                          select_target, &f) ==
               PR_U38_101D_CALL_ABORT);
        assert(cpu.cs == loaded_segment && cpu.ip == 0x2E8Bu &&
               cpu.sp == 0x7FE8u);
    } else {
        if (mode == 4) { expected_off = 0x3300u; expected_seg = 0x6000u; }
        else if (mode == 5) { expected_off = 0x4400u; expected_seg = 0x7000u; }
        else if (mode != 0) { expected_off = 0x2200u; expected_seg = 0x5000u; }
        assert(pr_u38_target_select_101d(ram, RAM_SIZE, &cpu,
                                          select_target, &f) == PR_U38_101D_OK);
        assert(cpu.ax == expected_off && cpu.dx == expected_seg);
        assert(cpu.cs == 0xD000u && cpu.ip == 0x1234u && cpu.sp == 0x8008u);
        assert(cpu.bp == 0x7777u);
        assert(get16(ram, cpu.ss, 0x7FF6u) == expected_off);
        assert(get16(ram, cpu.ss, 0x7FF8u) == expected_seg);
        assert(get16(ram, cpu.ss, 0x7FFAu) == expected_off);
        assert(get16(ram, cpu.ss, 0x7FFCu) == expected_seg);
    }
    assert(f.calls == (mode == 0 ? 0 : 1));
    free(ram);
}

int main(void)
{
    const uint16_t loaded_segments[] = {0x00BAu, 0x3456u, 0xD123u};
    for (size_t i = 0; i < sizeof loaded_segments / sizeof loaded_segments[0];
         ++i)
        for (int mode = 0; mode < 8; ++mode) run(mode, loaded_segments[i]);
    return 0;
}
