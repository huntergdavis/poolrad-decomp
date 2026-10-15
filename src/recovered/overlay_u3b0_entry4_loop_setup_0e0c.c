/* DOS v1.3 GAME.OVR U3B0 entry4 +0E0C..+0E2F, 36 original bytes. */
#include "poolrad/recovered/overlay_u3b0_entry4_loop_setup_0e0c.h"

#define MASK (PR_U3B0_0DEA_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static void write16(uint8_t *ram, uint16_t segment, uint16_t offset,
                    uint16_t value)
{
    size_t at = physical(segment, offset);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t signed_byte(uint8_t value)
{
    return (value & 0x80u) != 0u ? (uint16_t)(0xFF00u | value) : value;
}

pr_u3b0_0e0c_status pr_u3b0_entry4_loop_setup_0e0c(
    uint8_t *ram, size_t ram_size, pr_u3b0_0dea_cpu *cpu)
{
    uint16_t bp;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3B0_0DEA_RAM_SIZE ||
        cpu->ip != 0x0E0Cu)
        return PR_U3B0_0E0C_ARGUMENT;

    bp = cpu->bp;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(bp + 0x1Cu)));
    cpu->ax = signed_byte((uint8_t)cpu->ax); /* CWDE */
    cpu->dx = cpu->ax;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         read8(ram, cpu->ss, (uint16_t)(bp + 0x18u)));
    cpu->ax = signed_byte((uint8_t)cpu->ax); /* CWDE */
    cpu->ax = (uint16_t)(cpu->ax - cpu->dx + 1u);
    write16(ram, cpu->ss, (uint16_t)(bp - 0x59u), cpu->ax);

    cpu->ax = read16(ram, cpu->ss, (uint16_t)(bp + 0x14u));
    cpu->dx = read16(ram, cpu->ss, (uint16_t)(bp + 0x16u));
    write16(ram, cpu->ss, (uint16_t)(bp - 0x89u), cpu->ax);
    write16(ram, cpu->ss, (uint16_t)(bp - 0x87u), cpu->dx);
    cpu->ax = 0u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x0044u);
    write16(ram, cpu->ss, (uint16_t)(bp - 0x8Bu), cpu->ax);
    cpu->ip = 0x0E30u;
    return PR_U3B0_0E0C_OK;
}
