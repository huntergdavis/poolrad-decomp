/* Complete original U3BD:+031F..+034F; docs/re/U3BD_IN_GRID_031F.md. */
#include "poolrad/recovered/overlay_u3bd_in_grid_031f.h"

#define RAM_MASK (PR_U3BD_031F_RAM_SIZE - 1u)
#define CMP_FLAGS 0x08D5u

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & RAM_MASK;
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & RAM_MASK] << 8));
}

static void put_word(uint8_t *ram, size_t at, uint16_t value)
{
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static uint16_t parity_flag(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u ? 0x0004u : 0u;
}

static void compare_byte(pr_u3bd_031f_cpu *cpu, uint8_t left,
                         uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    uint16_t flags = parity_flag(result);
    if (left < right) flags |= 0x0001u;
    if (((left ^ right ^ result) & 0x10u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x80u) != 0u) flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        flags |= 0x0800u;
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~CMP_FLAGS) | flags);
}

pr_u3bd_031f_status pr_u3bd_in_grid_031f(
    uint8_t *ram, size_t ram_size, pr_u3bd_031f_cpu *cpu)
{
    uint8_t x, y, value;
    if (ram == NULL || cpu == NULL || ram_size < PR_U3BD_031F_RAM_SIZE)
        return PR_U3BD_031F_ARGUMENT;

    cpu->sp = (uint16_t)(cpu->sp - 2u);  /* +031F PUSH BP */
    put_word(ram, physical(cpu->ss, cpu->sp), cpu->bp);
    cpu->bp = cpu->sp;                   /* +0320 MOV BP,SP */
    cpu->sp = (uint16_t)(cpu->sp - 2u);  /* +0322 SUB SP,2 */

    x = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 8u))];
    compare_byte(cpu, x, 15u);          /* +0325 CMP x,15; JG +033D */
    if (x <= 0x7Fu && x > 15u) goto store_zero;
    x = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 8u))];
    compare_byte(cpu, x, 0u);           /* +032B CMP x,0; JL +033D */
    if (x >= 0x80u) goto store_zero;
    y = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))];
    compare_byte(cpu, y, 15u);          /* +0331 CMP y,15; JG +033D */
    if (y <= 0x7Fu && y > 15u) goto store_zero;
    y = ram[physical(cpu->ss, (uint16_t)(cpu->bp + 6u))];
    compare_byte(cpu, y, 0u);           /* +0337 CMP y,0; JGE +0343 */
    if (y >= 0x80u) goto store_zero;
    value = 1u;
    goto store_result;

store_zero:
    value = 0u;
store_result:
    ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))] = value;
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) |
                         ram[physical(cpu->ss, (uint16_t)(cpu->bp - 1u))]);
    cpu->sp = cpu->bp;
    cpu->bp = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->cs = word_at(ram, physical(cpu->ss, cpu->sp));
    cpu->sp = (uint16_t)(cpu->sp + 6u); /* return CS and two argument words */
    return PR_U3BD_031F_OK;
}
