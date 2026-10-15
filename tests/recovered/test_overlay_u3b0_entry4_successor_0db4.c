#include "poolrad/recovered/overlay_u3b0_entry4_successor_0db4.h"

#include <assert.h>
#include <stdlib.h>

static size_t at(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) &
           (PR_U3B0_0DB4_RAM_SIZE - 1u);
}

static uint16_t get16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t a = at(segment, offset);
    return (uint16_t)(ram[a] |
                      ((uint16_t)ram[(a + 1u) &
                                      (PR_U3B0_0DB4_RAM_SIZE - 1u)] << 8));
}

static void put16(uint8_t *ram, uint16_t segment, uint16_t offset,
                  uint16_t value)
{
    size_t a = at(segment, offset);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & (PR_U3B0_0DB4_RAM_SIZE - 1u)] = (uint8_t)(value >> 8);
}

static void run(uint16_t bp, uint16_t source_offset,
                uint16_t source_segment)
{
    uint8_t *ram = calloc(PR_U3B0_0DB4_RAM_SIZE, 1u);
    pr_u3b0_0db4_cpu cpu = {0};
    uint16_t incoming_sp = (uint16_t)(bp - 0x018Eu);
    uint16_t outgoing_sp = (uint16_t)(incoming_sp - 10u);
    assert(ram != NULL);
    cpu.ax = 0xABCDu;
    cpu.bx = 0x1357u;
    cpu.di = 0x579Bu;
    cpu.bp = bp;
    cpu.sp = incoming_sp;
    cpu.cs = 0xF000u;
    cpu.ip = 0x0DB4u;
    cpu.ds = 0x6000u;
    cpu.es = 0x7000u;
    cpu.ss = 0x7000u;
    cpu.flags = 0x0AD7u;
    put16(ram, cpu.ss, (uint16_t)(bp + 0x26u), source_offset);
    put16(ram, cpu.ss, (uint16_t)(bp + 0x28u), source_segment);
    ram[at(cpu.ss, (uint16_t)(bp - 0x2Au))] = 5u;
    ram[at(cpu.ss, (uint16_t)(bp - 0x29u))] = 'H';
    assert(pr_u3b0_entry4_successor_0db4(ram, PR_U3B0_0DB4_RAM_SIZE,
                                           &cpu) == PR_U3B0_0DB4_OK);
    assert(cpu.ip == 0x0DC2u && cpu.cs == 0xF000u);
    assert(cpu.bp == bp && cpu.sp == outgoing_sp);
    assert(cpu.ax == 0x0028u && cpu.di == (uint16_t)(bp - 0x53u));
    assert(cpu.es == source_segment && cpu.ds == 0x6000u);
    assert(cpu.bx == 0x1357u && cpu.flags == 0x0AD7u);
    assert(ram[at(cpu.ss, (uint16_t)(bp - 0x2Au))] == 5u);
    assert(ram[at(cpu.ss, (uint16_t)(bp - 0x29u))] == 'H');
    assert(get16(ram, cpu.ss, outgoing_sp) == 0x0028u);
    assert(get16(ram, cpu.ss, (uint16_t)(outgoing_sp + 2u)) ==
           (uint16_t)(bp - 0x53u));
    assert(get16(ram, cpu.ss, (uint16_t)(outgoing_sp + 4u)) == cpu.ss);
    assert(get16(ram, cpu.ss, (uint16_t)(outgoing_sp + 6u)) == source_offset);
    assert(get16(ram, cpu.ss, (uint16_t)(outgoing_sp + 8u)) == source_segment);
    free(ram);
}

int main(void)
{
    uint8_t *ram = calloc(PR_U3B0_0DB4_RAM_SIZE, 1u);
    pr_u3b0_0db4_cpu cpu = {0};
    assert(ram != NULL);
    cpu.ip = 0x0DB4u;
    assert(pr_u3b0_entry4_successor_0db4(NULL, PR_U3B0_0DB4_RAM_SIZE,
                                           &cpu) == PR_U3B0_0DB4_ARGUMENT);
    assert(pr_u3b0_entry4_successor_0db4(ram, PR_U3B0_0DB4_RAM_SIZE - 1u,
                                           &cpu) == PR_U3B0_0DB4_ARGUMENT);
    cpu.ip = 0x0DB5u;
    assert(pr_u3b0_entry4_successor_0db4(ram, PR_U3B0_0DB4_RAM_SIZE,
                                           &cpu) == PR_U3B0_0DB4_ARGUMENT);
    free(ram);
    run(0x7FFEu, 0x1100u, 0x5000u);
    run(0x0100u, 0xFFFFu, 0x0000u);
    return 0;
}
