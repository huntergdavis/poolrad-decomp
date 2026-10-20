#include "poolrad/recovered/resident_08ae_byte_181a_cpu.h"
#define MASK (PR_R08AE_181A_RAM_SIZE - 1u)
static size_t physical(uint16_t seg, uint16_t off)
{ return (((size_t)seg << 4) + off) & MASK; }
static uint16_t word(const uint8_t *ram, size_t at)
{ return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8)); }
static void push(uint8_t *ram, pr_r08ae_181a_cpu *c, uint16_t value)
{
    size_t at;
    c->sp = (uint16_t)(c->sp - 2u); at = physical(c->ss, c->sp);
    ram[at] = (uint8_t)value; ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}
static uint16_t pop(const uint8_t *ram, pr_r08ae_181a_cpu *c)
{
    uint16_t value = word(ram, physical(c->ss, c->sp));
    c->sp = (uint16_t)(c->sp + 2u); return value;
}
pr_r08ae_181a_status pr_r08ae_byte_181a_cpu(
    uint8_t *ram, size_t size, pr_r08ae_181a_cpu *c)
{
    size_t at;
    if (!ram || !c || size != PR_R08AE_181A_RAM_SIZE || c->ip != 0x181Au ||
        c->sp < 6u || c->sp > 0xFFFAu) return PR_R08AE_181A_ARGUMENT;
    push(ram, c, c->bp); c->bp = c->sp;
    push(ram, c, c->ax); push(ram, c, c->ds);
    c->ax = word(ram, physical(c->cs, 0x1820u)); c->ds = c->ax;
    c->ax = word(ram, physical(c->ss, (uint16_t)(c->bp + 6u)));
    ram[physical(c->ds, 0x020Du)] = (uint8_t)c->ax;
    c->ds = pop(ram, c); c->ax = pop(ram, c); c->bp = pop(ram, c);
    at = physical(c->ss, c->sp);
    c->ip = word(ram, at); c->cs = word(ram, (at + 2u) & MASK);
    c->sp = (uint16_t)(c->sp + 6u);
    return PR_R08AE_181A_OK;
}
