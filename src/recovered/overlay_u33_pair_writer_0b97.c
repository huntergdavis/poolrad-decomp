/* GAME.OVR unit 0x33:+0B97..+0BDD; docs/re/U33_PAIR_WRITER_0B97.md. */
#include "poolrad/recovered/overlay_u33_pair_writer_0b97.h"
#define MASK (PR_U33_0B97_RAM_SIZE - 1u)
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}
static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at+1u)&MASK] << 8u));
}
static void put(uint8_t *ram, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at=physical(segment,offset);
    ram[at]=(uint8_t)value;
    ram[(at+1u)&MASK]=(uint8_t)(value >> 8u);
}
static uint16_t signed_byte(uint8_t value)
{
    return value < 0x80u ? value : (uint16_t)(0xFF00u | value);
}
static int cell(uint8_t *ram, pr_u33_0b97_cpu *cpu, unsigned right)
{
    uint32_t product,sum;
    size_t at;
    cpu->ax=signed_byte(ram[physical(cpu->ss,(uint16_t)(cpu->bp+8u))]);
    if (right) cpu->ax=(uint16_t)(cpu->ax+1u);
    cpu->cx=cpu->ax;
    cpu->ax=signed_byte(ram[physical(cpu->ss,(uint16_t)(cpu->bp+10u))]);
    cpu->dx=50u;
    product=(uint32_t)cpu->ax * cpu->dx;
    cpu->ax=(uint16_t)product;
    cpu->dx=(uint16_t)(product >> 16u);
    at=physical(cpu->ds,0x6716u);
    cpu->di=word_at(ram,at);
    cpu->es=word_at(ram,(at+2u)&MASK);
    cpu->di=(uint16_t)(cpu->di+cpu->ax);
    sum=(uint32_t)cpu->di+cpu->cx;
    cpu->di=(uint16_t)sum;
    ram[physical(cpu->es,(uint16_t)(cpu->di+7u))]=(uint8_t)(0x34u+right);
    return sum > 0xFFFFu;
}
static void increment_flags(pr_u33_0b97_cpu *cpu, uint8_t before, uint8_t result, int carry)
{
    uint8_t parity=result;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (carry) cpu->flags |= 1u;
    if ((parity & 1u)==0u) cpu->flags |= 4u;
    if ((before & 0xFu)==0xFu) cpu->flags |= 0x10u;
    if (result==0u) cpu->flags |= 0x40u;
    if (result & 0x80u) cpu->flags |= 0x80u;
    if (before==0x7Fu) cpu->flags |= 0x800u;
}

pr_u33_0b97_status pr_u33_pair_writer_0b97(
    uint8_t *ram, size_t ram_size, pr_u33_0b97_cpu *cpu)
{
    size_t at;
    uint8_t before,result;
    int carry;
    if (ram==NULL || cpu==NULL || ram_size!=PR_U33_0B97_RAM_SIZE)
        return PR_U33_0B97_ARGUMENT;
    cpu->sp=(uint16_t)(cpu->sp-2u);
    put(ram,cpu->ss,cpu->sp,cpu->bp);
    cpu->bp=cpu->sp;
    (void)cell(ram,cpu,0u);
    carry=cell(ram,cpu,1u);
    cpu->di=word_at(ram,physical(cpu->ss,(uint16_t)(cpu->bp+6u)));
    at=physical(cpu->ss,(uint16_t)(cpu->di-2u));
    before=ram[at];result=(uint8_t)(before+1u);ram[at]=result;
    increment_flags(cpu,before,result,carry);
    cpu->bp=word_at(ram,physical(cpu->ss,cpu->sp));
    cpu->sp=(uint16_t)(cpu->sp+2u);
    at=physical(cpu->ss,cpu->sp);
    cpu->ip=word_at(ram,at);
    cpu->cs=word_at(ram,(at+2u)&MASK);
    cpu->sp=(uint16_t)(cpu->sp+10u);
    return PR_U33_0B97_OK;
}
