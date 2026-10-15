#include "poolrad/recovered/resident_time_caller_08ae_006b.h"

#include <stddef.h>

enum { ARITH_FLAGS = 0x08d5u, CARRY = 0x0001u };

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (size_t)segment * 16u + offset;
}

static uint16_t load_word(const uint8_t *ram, uint16_t segment,
                          uint16_t offset)
{
    size_t address = physical(segment, offset);
    return (uint16_t)((uint16_t)ram[address] |
                      ((uint16_t)ram[address + 1u] << 8));
}

static void store_word(uint8_t *ram, uint16_t segment, uint16_t offset,
                       uint16_t value)
{
    size_t address = physical(segment, offset);
    ram[address] = (uint8_t)value;
    ram[address + 1u] = (uint8_t)(value >> 8);
}

static uint16_t parity_flag(uint16_t value)
{
    uint8_t low = (uint8_t)value;
    low ^= (uint8_t)(low >> 4);
    low ^= (uint8_t)(low >> 2);
    low ^= (uint8_t)(low >> 1);
    return (low & 1u) == 0u ? 0x0004u : 0u;
}

static uint16_t subtract_flags(uint16_t old, uint16_t left, uint16_t right,
                               uint16_t borrow, uint16_t result)
{
    uint16_t flags = (uint16_t)(old & (uint16_t)~ARITH_FLAGS);
    if ((uint32_t)left < (uint32_t)right + borrow) flags |= CARRY;
    flags |= parity_flag(result);
    if (((left ^ right ^ result) & 0x0010u) != 0u) flags |= 0x0010u;
    if (result == 0u) flags |= 0x0040u;
    if ((result & 0x8000u) != 0u) flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        flags |= 0x0800u;
    return flags;
}

void pr_rec_res_08ae_006b_return(pr_rec_res_08ae_001f_cpu *cpu,
                                 const uint8_t ram[655360])
{
    uint16_t offset = load_word(ram, cpu->ss, cpu->sp);
    uint16_t segment = load_word(ram, cpu->ss, (uint16_t)(cpu->sp + 2u));
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    cpu->ip = offset;
    cpu->cs = segment;
}

void pr_rec_res_08ae_008d_to_00aa(pr_rec_res_08ae_001f_cpu *cpu,
                                  uint8_t ram[655360])
{
    uint16_t left, right, borrow;

    /* MOV [BP-8],AX; MOV [BP-6],DX. */
    store_word(ram, cpu->ss, (uint16_t)(cpu->bp - 8u), cpu->ax);
    store_word(ram, cpu->ss, (uint16_t)(cpu->bp - 6u), cpu->dx);
    cpu->ax = load_word(ram, cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->dx = 0u; /* XOR DX,DX clears the six arithmetic flags, then sets ZF/PF. */
    cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~ARITH_FLAGS) | 0x0044u);
    cpu->cx = cpu->ax;
    cpu->bx = cpu->dx;
    cpu->ax = load_word(ram, cpu->ss, (uint16_t)(cpu->bp - 4u));
    cpu->dx = load_word(ram, cpu->ss, (uint16_t)(cpu->bp - 2u));

    left = cpu->ax;
    right = load_word(ram, cpu->ss, (uint16_t)(cpu->bp - 8u));
    cpu->ax = (uint16_t)(left - right);
    cpu->flags = subtract_flags(cpu->flags, left, right, 0u, cpu->ax);
    borrow = (uint16_t)(cpu->flags & CARRY);

    left = cpu->dx;
    right = load_word(ram, cpu->ss, (uint16_t)(cpu->bp - 6u));
    cpu->dx = (uint16_t)(left - right - borrow);
    cpu->flags = subtract_flags(cpu->flags, left, right, borrow, cpu->dx);

    left = cpu->dx;
    right = cpu->bx;
    cpu->flags = subtract_flags(cpu->flags, left, right, 0u,
                                (uint16_t)(left - right));
    cpu->ip = 0x00aau;
}
