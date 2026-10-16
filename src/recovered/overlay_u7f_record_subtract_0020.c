/* GAME.OVR unit 0x7F:+0020..+0039; docs/re/U7F_RECORD_SUBTRACT_0020.md. */
#include "poolrad/recovered/overlay_u7f_record_subtract_0020.h"
#define MASK (PR_U7F_0020_RAM_SIZE - 1u)
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
    ram[at]=(uint8_t)value;ram[(at+1u)&MASK]=(uint8_t)(value >> 8u);
}
static void load_record(const uint8_t *ram, pr_u7f_0020_cpu *cpu)
{
    size_t at=physical(cpu->ss,(uint16_t)(cpu->bp+8u));
    cpu->di=word_at(ram,at);cpu->es=word_at(ram,(at+2u)&MASK);
}
static void subtract_flags(pr_u7f_0020_cpu *cpu, uint16_t left, uint16_t right)
{
    uint8_t parity=(uint8_t)cpu->ax;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left<right) cpu->flags |= 1u;
    if ((parity&1u)==0u) cpu->flags |= 4u;
    if ((left^right^cpu->ax)&0x10u) cpu->flags |= 0x10u;
    if (cpu->ax==0u) cpu->flags |= 0x40u;
    if (cpu->ax&0x8000u) cpu->flags |= 0x80u;
    if ((left^right)&(left^cpu->ax)&0x8000u) cpu->flags |= 0x800u;
}

pr_u7f_0020_status pr_u7f_record_subtract_0020(
    uint8_t *ram, size_t ram_size, pr_u7f_0020_cpu *cpu)
{
    size_t at;
    uint16_t left,right;
    if (ram==NULL || cpu==NULL || ram_size!=PR_U7F_0020_RAM_SIZE)
        return PR_U7F_0020_ARGUMENT;
    cpu->sp=(uint16_t)(cpu->sp-2u);
    put(ram,cpu->ss,cpu->sp,cpu->bp);
    cpu->bp=cpu->sp;
    load_record(ram,cpu);
    left=word_at(ram,physical(cpu->es,(uint16_t)(cpu->di+0x102u)));
    right=word_at(ram,physical(cpu->ss,(uint16_t)(cpu->bp+6u)));
    cpu->ax=(uint16_t)(left-right);
    subtract_flags(cpu,left,right);
    load_record(ram,cpu);
    put(ram,cpu->es,(uint16_t)(cpu->di+0x102u),cpu->ax);
    cpu->bp=word_at(ram,physical(cpu->ss,cpu->sp));
    cpu->sp=(uint16_t)(cpu->sp+2u);
    at=physical(cpu->ss,cpu->sp);
    cpu->ip=word_at(ram,at);
    /* RETF pops two words: wrap SP between IP and CS, not within a word. */
    cpu->cs=word_at(ram,physical(cpu->ss,(uint16_t)(cpu->sp+2u)));
    cpu->sp=(uint16_t)(cpu->sp+10u);
    return PR_U7F_0020_OK;
}
