/* GAME.OVR unit 0x1C:+2B14..+2B32, complete 31-byte body. */
#include "poolrad/recovered/overlay_u1c_clear_box_2b14.h"

#define RAM_MASK (PR_U1C_2B14_RAM_SIZE - 1u)
#define INC_FLAGS 0x08D4u /* OF SF ZF AF PF; CF is unchanged */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint16_t read16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(ram[a] | ((uint16_t)ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    ram[physical(seg, off)] = value;
}

static void write16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    ram[a] = (uint8_t)value;
    ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(uint8_t *ram, pr_u1c_2b14_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static void set_al(pr_u1c_2b14_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void inc_word_flags(pr_u1c_2b14_cpu *cpu, uint16_t before,
                           uint16_t result)
{
    uint8_t bits = (uint8_t)result;
    cpu->flags &= (uint16_t)~INC_FLAGS;
    bits ^= (uint8_t)(bits >> 4);
    bits ^= (uint8_t)(bits >> 2);
    bits ^= (uint8_t)(bits >> 1);
    if (!(bits & 1u)) cpu->flags |= 0x0004u;
    if ((before & 0xFu) == 0xFu) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if (before == 0x7FFFu) cpu->flags |= 0x0800u;
}

pr_u1c_2b14_status pr_u1c_clear_box_2b14(
    uint8_t *ram, size_t ram_size, pr_u1c_2b14_cpu *cpu,
    pr_u1c_2b14_display display, void *context)
{
    uint16_t pc;
    if (ram == NULL || cpu == NULL || ram_size < PR_U1C_2B14_RAM_SIZE)
        return PR_U1C_2B14_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    pc = read16(ram, cpu->ds, 0x49EDu);
    write16(ram, cpu->ds, 0x49EDu, (uint16_t)(pc + 1u));
    inc_word_flags(cpu, pc, (uint16_t)(pc + 1u));
    set_al(cpu, 1u);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, 1u);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x0Fu);
    push16(ram, cpu, cpu->ax);
    set_al(cpu, 0x0Fu);
    push16(ram, cpu, cpu->ax);
    cpu->ip = 0x2B27u;
    if (display == NULL || !display(context, ram, ram_size, cpu))
        return PR_U1C_2B14_CALL_ABORT;
    cpu->sp = (uint16_t)(cpu->sp + 8u);
    cpu->ip = 0x2B2Cu;
    write8(ram, cpu->ds, 0x84DEu, 1u);
    cpu->bp = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->ip = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    cpu->cs = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return PR_U1C_2B14_OK;
}

/* Raw bus entry. The legacy callback implementation above is unchanged. */
#include <string.h>
static int ordinary_segment(uint16_t segment)
{
    size_t a=(size_t)segment<<4u;
    return a>0xAFFFFu || a+0xFFFFu<0xA0000u;
}
static uint16_t bus_word(pr_bus *bus,uint16_t seg,uint16_t off)
{
    size_t a=physical(seg,off);
    return (uint16_t)(pr_bus_read8(bus,a)|
        ((uint16_t)pr_bus_read8(bus,(a+1u)&RAM_MASK)<<8u));
}
static uint16_t raw_pop(pr_bus *bus,pr_u1c_2b14_cpu *cpu)
{
    uint16_t value=read16(bus->ram,cpu->ss,cpu->sp);
    cpu->sp=(uint16_t)(cpu->sp+2u);return value;
}
pr_u1c_2b14_cpu_status pr_u1c_clear_box_2b14_cpu(pr_bus *bus,size_t size,
    pr_u1c_2b14_cpu *cpu,size_t max_loopbacks,size_t rect_loopbacks,
    pr_u1c_2b14_cpu_diagnostic *d)
{
    pr_u1c_2b14_cpu_diagnostic local={-1,{-1,-1,{-1,-1}}};
    pr_r0709_clear_cpu inner;
    uint16_t caller,target,pc,result;
    _Static_assert(sizeof inner==sizeof *cpu,"CPU layout");
    if(d==NULL)d=&local;
    *d=local;
    if(bus==NULL || bus->ram==NULL || bus->ega==NULL || cpu==NULL ||
       size!=PR_U1C_2B14_RAM_SIZE || cpu->ip!=0x2B14u ||
       cpu->sp<0x004Cu || cpu->sp>0xFFFCu ||
       !ordinary_segment(cpu->ds) || !ordinary_segment(cpu->ss))
        return PR_U1C_2B14_CPU_ARGUMENT;
    caller=cpu->cs;
    /* DS/SS are ordinary RAM throughout this entry: these helpers perform
     * the same contiguous physical accesses as the bus, without MMIO. */
    push16(bus->ram,cpu,cpu->bp);cpu->bp=cpu->sp;
    pc=read16(bus->ram,cpu->ds,0x49EDu);result=(uint16_t)(pc+1u);
    write16(bus->ram,cpu->ds,0x49EDu,result);inc_word_flags(cpu,pc,result);
    set_al(cpu,1u);push16(bus->ram,cpu,cpu->ax);
    set_al(cpu,1u);push16(bus->ram,cpu,cpu->ax);
    set_al(cpu,15u);push16(bus->ram,cpu,cpu->ax);
    set_al(cpu,15u);push16(bus->ram,cpu,cpu->ax);
    cpu->ip=0x2B27u;
    if(pr_bus_read8(bus,physical(caller,0x2B27u))!=0x9Au ||
       bus_word(bus,caller,0x2B28u)!=0x0DD3u)
        return PR_U1C_2B14_CPU_CODE;
    target=bus_word(bus,caller,0x2B2Au);
    push16(bus->ram,cpu,caller);push16(bus->ram,cpu,0x2B2Cu);
    cpu->cs=target;cpu->ip=0x0DD3u;
    memcpy(&inner,cpu,sizeof inner);
    d->clear_status=pr_r0709_clear_0dd3_cpu(bus,size,&inner,
        max_loopbacks,rect_loopbacks,&d->clear);
    memcpy(cpu,&inner,sizeof inner);
    if(d->clear_status!=PR_R0709_CLEAR_CPU_OK)return PR_U1C_2B14_CPU_CALLEE;
    if(cpu->cs!=caller || cpu->ip!=0x2B2Cu)return PR_U1C_2B14_CPU_TRANSFER;
    write8(bus->ram,cpu->ds,0x84DEu,1u);
    cpu->bp=raw_pop(bus,cpu);cpu->ip=raw_pop(bus,cpu);cpu->cs=raw_pop(bus,cpu);
    return PR_U1C_2B14_CPU_OK;
}
