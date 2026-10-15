#include "poolrad/recovered/overlay_u38_empty_319b.h"

#include <assert.h>
#include <stdlib.h>

#define RAM_SIZE PR_U38_101D_RAM_SIZE

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & (RAM_SIZE - 1u);
}

static void put16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & (RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static uint16_t get16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) &
                                     (RAM_SIZE - 1u)] << 8));
}

static void run(uint16_t ss, uint16_t sp, uint16_t bp, uint16_t flags)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_319b_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ax = 0xA5CDu;
    cpu.bx = 0x3456u;
    cpu.cx = 0x5678u;
    cpu.dx = 0x789Au;
    cpu.si = 0x9ABCu;
    cpu.di = 0xBCDEu;
    cpu.bp = bp;
    cpu.sp = sp;
    cpu.cs = 0xF000u;
    cpu.ip = 0x319Bu;
    cpu.ds = 0x3000u;
    cpu.es = 0x1111u;
    cpu.ss = ss;
    cpu.flags = flags;
    put16(ram, ss, sp, 0x1234u);
    put16(ram, ss, (uint16_t)(sp + 2u), 0xD000u);
    assert(pr_u38_empty_319b(ram, RAM_SIZE, &cpu) == PR_U38_319B_OK);
    assert(cpu.ip == 0x1234u && cpu.cs == 0xD000u);
    assert(cpu.sp == (uint16_t)(sp + 14u));
    assert(cpu.bp == bp && cpu.flags == flags);
    assert(cpu.ax == 0xA5CDu && cpu.bx == 0x3456u);
    assert(cpu.cx == 0x5678u && cpu.dx == 0x789Au);
    assert(cpu.si == 0x9ABCu && cpu.di == 0xBCDEu);
    assert(cpu.ds == 0x3000u && cpu.es == 0x1111u && cpu.ss == ss);
    assert(get16(ram, ss, (uint16_t)(sp - 2u)) == bp);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(RAM_SIZE, 1u);
    pr_u38_319b_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x319Bu;
    assert(pr_u38_empty_319b(NULL, RAM_SIZE, &cpu) == PR_U38_319B_ARGUMENT);
    assert(pr_u38_empty_319b(ram, RAM_SIZE - 1u, &cpu) ==
           PR_U38_319B_ARGUMENT);
    assert(pr_u38_empty_319b(ram, RAM_SIZE, NULL) == PR_U38_319B_ARGUMENT);
    cpu.ip = 0;
    assert(pr_u38_empty_319b(ram, RAM_SIZE, &cpu) == PR_U38_319B_ARGUMENT);
    free(ram);
    run(0xE000u, 0x8000u, 0x7777u, 0x72D3u);
    run(0xF123u, 0x0000u, 0xABCDu, 0x0002u);
    run(0xFFFEu, 0xFFFAu, 0x0123u, 0x0FD7u);
    run(0xE000u, 0xFFFEu, 0xBEEFu, 0x0202u);
    run(0xE000u, 0xFFFFu, 0x1234u, 0x0802u);
    return 0;
}
