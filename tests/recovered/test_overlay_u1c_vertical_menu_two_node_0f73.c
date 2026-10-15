#include "poolrad/recovered/overlay_u1c_vertical_menu_two_node_0f73.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U1C_0F08_RAM_SIZE

typedef struct {
    int count;
    int abort_second_copy;
} fixture;

static size_t at(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = at(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = at(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & (RAM_SIZE - 1u)] << 8));
}

static int call(void *context, uint8_t *ram, size_t size,
                pr_u1c_0f08_cpu *cpu, pr_u1c_0f73_call event)
{
    static const pr_u1c_0f73_call sequence[] = {
        PR_U1C_0F73_NODE, PR_U1C_0F73_DRAW, PR_U1C_0F73_COPY,
        PR_U1C_0F73_COPY, PR_U1C_0F73_MENU, PR_U1C_0F73_STORE,
        PR_U1C_0F73_RELEASE, PR_U1C_0F73_CLEANUP
    };
    fixture *f = context;
    assert(size == RAM_SIZE && f->count < 8);
    assert(event == sequence[f->count]);
    ++f->count;
    if (event == PR_U1C_0F73_NODE) {
        assert(get16(ram, cpu->ss, cpu->sp) == (uint16_t)(cpu->bp - 0x106u));
        put16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x106u), 0x2200u);
        put16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x104u), 0x4000u);
        put16(ram, 0x4000u, 0x222Au, 0x2300u);
        put16(ram, 0x4000u, 0x222Cu, 0x4000u);
    } else if (event == PR_U1C_0F73_COPY) {
        uint16_t expected = f->count == 3 ? 0x2200u : 0x2300u;
        assert(cpu->ip == 0x0FD4u);
        assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 2u)) == expected);
        if (f->abort_second_copy && f->count == 4) return 0;
    } else if (event == PR_U1C_0F73_MENU) {
        assert(get16(ram, cpu->ss, (uint16_t)(cpu->sp + 14u)) == 0x2200u);
        put16(ram, cpu->ss, (uint16_t)(cpu->bp - 0x10Eu), 3u);
        ram[at(cpu->ss, (uint16_t)(cpu->bp - 0x111u))] = 0x42u;
    } else if (event == PR_U1C_0F73_STORE) {
        assert(get16(ram, cpu->ss, cpu->sp) == 3u);
    }
    return 1;
}

static void run(int abort_second_copy)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u1c_0f08_cpu cpu = {0};
    fixture f = {0, abort_second_copy};
    pr_u1c_0f73_status status;
    assert(ram != NULL);
    cpu.bp = 0x4000u;
    cpu.sp = 0x3EE8u;
    cpu.ip = 0x0F73u;
    cpu.ss = 0xE000u;
    cpu.ds = 0x3000u;
    cpu.cs = 0xF000u;
    cpu.ax = 0xA500u;
    put16(ram, cpu.ss, cpu.sp, 0x3EFAu);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 2u), cpu.ss);
    put16(ram, cpu.ss, (uint16_t)(cpu.sp + 4u), 1u);
    put16(ram, cpu.ss, cpu.bp, 0x3456u);
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 2u), 0x0712u);
    put16(ram, cpu.ss, (uint16_t)(cpu.bp + 4u), 0xD000u);
    status = pr_u1c_vertical_menu_two_node_0f73(ram, RAM_SIZE,
                                                  &cpu, call, &f);
    if (abort_second_copy) {
        assert(status == PR_U1C_0F73_CALL_ABORT && f.count == 4);
        assert(cpu.ip == 0x0FD4u && cpu.cs == 0xF000u);
    } else {
        assert(status == PR_U1C_0F73_OK && f.count == 8);
        assert(cpu.ip == 0x0712u && cpu.cs == 0xD000u);
        assert(cpu.bp == 0x3456u && cpu.sp == 0x4006u);
        assert(ram[at(0x4000u, 0x2229u)] == 0u);
        assert(ram[at(0x4000u, 0x2329u)] == 0u);
    }
    free(ram);
}

int main(void)
{
    run(0);
    run(1);
    return 0;
}
