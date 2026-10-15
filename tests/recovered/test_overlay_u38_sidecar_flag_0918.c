#include "poolrad/recovered/overlay_u38_sidecar_flag_0918.h"
#include "poolrad/recovered/resident_sidecar_lookup.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_0918_RAM_SIZE

typedef struct {
    uint8_t *ram;
    int calls;
    int abort;
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

static int read_word(void *context, uint16_t seg, uint16_t off,
                     uint16_t *value)
{
    fixture *f = context;
    *value = get16(f->ram, seg, off);
    return 1;
}

static int write_word(void *context, uint16_t seg, uint16_t off,
                      uint16_t value)
{
    fixture *f = context;
    put16(f->ram, seg, off, value);
    return 1;
}

static int read_byte(void *context, uint16_t seg, uint16_t off,
                     uint8_t *value)
{
    fixture *f = context;
    *value = f->ram[at(seg, off)];
    return 1;
}

static int lookup(void *context, uint8_t *ram, size_t size,
                  pr_u38_0918_cpu *cpu)
{
    fixture *f = context;
    pr_rec_lookup_io io = {f, read_word, write_word, read_byte, 4u};
    pr_rec_far_ptr cursor, character;
    uint16_t key;
    uint8_t found = 0xA5u;
    assert(size == RAM_SIZE && ram == f->ram);
    assert(cpu->ip == 0x092Eu);
    ++f->calls;
    if (f->abort) return 0;
    cursor.offset = get16(ram, cpu->ss, cpu->sp);
    cursor.segment = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    key = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 4u));
    character.offset = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 6u));
    character.segment = get16(ram, cpu->ss, (uint16_t)(cpu->sp + 8u));
    assert(cursor.offset == (uint16_t)(cpu->bp - 4u) &&
           cursor.segment == cpu->ss && (key & 255u) == 0x18u);
    assert(character.offset == 0x2300u && character.segment == 0x4000u);
    assert(pr_rec_res_00ba_2199(&io, cursor, key, character,
                                &found) == PR_REC_LOOKUP_OK);
    assert(found <= 1u);
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | found);
    return 1;
}

static void run(int mode)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_0918_cpu cpu = {0};
    fixture f = {ram, 0, mode == 4};
    pr_u38_0918_status status;
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bp = 0x7777u;
    cpu.sp = 0x8000u;
    cpu.cs = 0xF000u;
    cpu.ip = 0x0918u;
    cpu.ds = 0x3000u;
    cpu.ss = 0xE000u;
    cpu.flags = 0x0202u;
    put16(ram, cpu.ss, cpu.sp, 0x1234u);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), 0xD000u);
    put16(ram, cpu.ds, 0x5D92u, 0x2300u);
    put16(ram, cpu.ds, 0x5D94u, 0x4000u);
    put16(ram, 0x4000u, 0x237Fu, mode == 0 ? 0u : 0x2200u);
    put16(ram, 0x4000u, 0x2381u, mode == 0 ? 0u : 0x5000u);
    ram[at(0x5000u, 0x2200u)] = mode == 1 ? 0x18u : 0x55u;
    put16(ram, 0x5000u, 0x2205u, mode == 2 ? 0x2300u : 0u);
    put16(ram, 0x5000u, 0x2207u, mode == 2 ? 0x5000u : 0u);
    ram[at(0x5000u, 0x2300u)] = 0x18u;
    ram[at(cpu.ds, 0x681Eu)] = 0xA7u;
    ram[at(cpu.ds, 0x6822u)] = 3u;
    status = pr_u38_sidecar_flag_0918(ram, RAM_SIZE, &cpu, lookup, &f);
    assert(f.calls == 1);
    if (mode == 4) {
        assert(status == PR_U38_0918_CALL_ABORT && cpu.ip == 0x092Eu);
    } else {
        uint16_t expected_off = mode == 1 ? 0x2200u :
                                mode == 2 ? 0x2300u : 0u;
        uint16_t expected_seg = expected_off ? 0x5000u : 0u;
        assert(status == PR_U38_0918_OK);
        assert(cpu.ip == 0x1234u && cpu.cs == 0xD000u);
        assert(cpu.sp == 0x800Eu && cpu.bp == 0x7777u);
        assert((cpu.ax & 255u) == (mode == 1 || mode == 2 ? 1u : 0u));
        assert(get16(ram, cpu.ss, 0x7FFAu) == expected_off);
        assert(get16(ram, cpu.ss, 0x7FFCu) == expected_seg);
        assert(ram[at(cpu.ds, 0x681Eu)] ==
               (mode == 1 || mode == 2 ? 0xA7u : 1u));
        assert(ram[at(cpu.ds, 0x6822u)] == 0xFFu);
    }
    free(ram);
}

int main(void)
{
    for (int mode = 0; mode <= 4; ++mode) run(mode);
    return 0;
}
