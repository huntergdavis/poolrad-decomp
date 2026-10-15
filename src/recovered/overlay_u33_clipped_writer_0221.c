/* GAME.OVR unit 0x33:+0221..+02B9; docs/re/U33_CLIPPED_WRITER_0221.md. */
#include "poolrad/recovered/overlay_u33_clipped_writer_0221.h"
#define MASK (PR_U33_0221_RAM_SIZE - 1u)
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
static uint8_t frame_byte(const uint8_t *ram, const pr_u33_0221_cpu *cpu, int displacement)
{
    return ram[physical(cpu->ss,(uint16_t)(cpu->bp+displacement))];
}
static uint16_t signed_byte(uint8_t value)
{
    return value < 0x80u ? value : (uint16_t)(0xFF00u | value);
}
static void arithmetic_flags(pr_u33_0221_cpu *cpu, uint16_t left,
                             uint16_t right, int subtract, int byte)
{
    uint32_t sum=(uint32_t)left+right;
    uint16_t result=subtract ? (uint16_t)(left-right) : (uint16_t)sum;
    uint16_t sign=byte ? 0x80u : 0x8000u;
    uint8_t parity;
    if (byte) result &= 0xFFu;
    parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);
    parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    cpu->flags &= (uint16_t)~0x08D5u;
    if (subtract ? left<right : sum>0xFFFFu) cpu->flags |= 1u;
    if ((parity & 1u)==0u) cpu->flags |= 4u;
    if ((left^right^result)&0x10u) cpu->flags |= 0x10u;
    if (result==0u) cpu->flags |= 0x40u;
    if (result&sign) cpu->flags |= 0x80u;
    if ((subtract ? (left^right)&(left^result) : ~(left^right)&(left^result))&sign)
        cpu->flags |= 0x800u;
}
static int clipped(pr_u33_0221_cpu *cpu, uint8_t coordinate, uint8_t upper)
{
    arithmetic_flags(cpu,coordinate,0u,1,1);
    if (coordinate&0x80u) return 1;
    arithmetic_flags(cpu,coordinate,upper,1,1);
    return coordinate>upper;
}

pr_u33_0221_status pr_u33_clipped_writer_0221(
    uint8_t *ram, size_t ram_size, pr_u33_0221_cpu *cpu)
{
    size_t at;
    uint16_t left;
    if (ram==NULL || cpu==NULL || ram_size!=PR_U33_0221_RAM_SIZE)
        return PR_U33_0221_ARGUMENT;
    cpu->sp=(uint16_t)(cpu->sp-2u);
    put(ram,cpu->ss,cpu->sp,cpu->bp);
    cpu->bp=cpu->sp;cpu->sp=(uint16_t)(cpu->sp-2u);
    cpu->cx=frame_byte(ram,cpu,12);
    cpu->di=word_at(ram,physical(cpu->ss,(uint16_t)(cpu->bp+6u)));
    cpu->si=signed_byte(ram[physical(cpu->ss,(uint16_t)(cpu->di-2u))]);
    cpu->dx=(uint16_t)(5u*cpu->si);
    cpu->di=word_at(ram,physical(cpu->ss,(uint16_t)(cpu->bp+6u)));
    cpu->si=(uint16_t)(2u*signed_byte(ram[physical(cpu->ss,(uint16_t)(cpu->di-1u))]));
    cpu->ax=(uint16_t)(3u*cpu->si+cpu->dx+21u+cpu->cx);
    ram[physical(cpu->ss,(uint16_t)(cpu->bp-1u))]=(uint8_t)cpu->ax;
    cpu->dx=frame_byte(ram,cpu,10);
    cpu->di=word_at(ram,physical(cpu->ss,(uint16_t)(cpu->bp+6u)));
    cpu->si=signed_byte(ram[physical(cpu->ss,(uint16_t)(cpu->di-2u))]);
    cpu->ax=(uint16_t)(5u*cpu->si+10u+cpu->dx);
    ram[physical(cpu->ss,(uint16_t)(cpu->bp-2u))]=(uint8_t)cpu->ax;
    if (!clipped(cpu,frame_byte(ram,cpu,-1),49u) &&
        !clipped(cpu,frame_byte(ram,cpu,-2),24u)) {
        cpu->ax=(uint16_t)(frame_byte(ram,cpu,8)+1u);
        cpu->bx=(uint16_t)((cpu->bx&0xFF00u)|(cpu->ax&0xFFu));
        cpu->cx=signed_byte(frame_byte(ram,cpu,-1));
        cpu->ax=(uint16_t)(50u*signed_byte(frame_byte(ram,cpu,-2)));
        cpu->dx=0u; /* Bounds above guarantee a nonnegative row <=24. */
        at=physical(cpu->ds,0x6716u);
        cpu->di=word_at(ram,at);cpu->es=word_at(ram,(at+2u)&MASK);
        cpu->di=(uint16_t)(cpu->di+cpu->ax);
        left=cpu->di;
        cpu->di=(uint16_t)(cpu->di+cpu->cx);
        arithmetic_flags(cpu,left,cpu->cx,0,0);
        ram[physical(cpu->es,(uint16_t)(cpu->di+7u))]=(uint8_t)cpu->bx;
    }
    cpu->sp=cpu->bp;
    cpu->bp=word_at(ram,physical(cpu->ss,cpu->sp));
    cpu->sp=(uint16_t)(cpu->sp+2u);
    at=physical(cpu->ss,cpu->sp);
    cpu->ip=word_at(ram,at);cpu->cs=word_at(ram,(at+2u)&MASK);
    cpu->sp=(uint16_t)(cpu->sp+12u);
    return PR_U33_0221_OK;
}
