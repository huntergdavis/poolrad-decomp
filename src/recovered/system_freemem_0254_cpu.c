/* START.EXE SYSTEM FreeMem 0AF8:0254 with near core 03D3, merge 0432 and rounding 0491 (raw CPU);
 * docs/re/SYSTEM_FREEMEM_0254.md. */
#include "poolrad/recovered/system_freemem_0254_cpu.h"

#include <string.h>

#define MASK (PR_SYSTEM_FREEMEM_RAM_SIZE - 1u)
#define CF 0x0001u
#define ZF 0x0040u

typedef struct {
    uint8_t *ram;
    pr_system_freemem_cpu *cpu;
    uint16_t code_cs;
    unsigned steps, max_steps;
    pr_system_freemem_status stop;
} machine;

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const machine *m, uint16_t segment, uint16_t offset)
{
    return read16_at(m->ram, physical(segment, offset));
}

static void write16(machine *m, uint16_t segment, uint16_t offset, uint16_t value)
{
    size_t at = physical(segment, offset);
    m->ram[at] = (uint8_t)value;
    m->ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static void push16(machine *m, uint16_t value)
{
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    write16(m, m->cpu->ss, m->cpu->sp, value);
}

static uint16_t pop16(machine *m)
{
    uint16_t value = read16(m, m->cpu->ss, m->cpu->sp);
    m->cpu->sp = (uint16_t)(m->cpu->sp + 2u);
    return value;
}

/* LES DI,segment:[offset], the pair read physically contiguous. */
static void les(machine *m, uint16_t segment, uint16_t offset)
{
    size_t at = physical(segment, offset);
    m->cpu->di = read16_at(m->ram, at);
    m->cpu->es = read16_at(m->ram, (at + 2u) & MASK);
}

static int even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* OR, XOR or AND: CF = OF = AF = 0, ZF/SF/PF from the result whose sign bit is `sign`; the other flags are kept. */
static void logic_flags(pr_system_freemem_cpu *cpu, uint16_t result, uint16_t sign)
{
    cpu->flags &= (uint16_t)~0x08D5u;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= ZF;
    if (result & sign) cpu->flags |= 0x0080u;
}

/* CMP left,right in `bits` bits: the six arithmetic flags. */
static void cmp_flags(pr_system_freemem_cpu *cpu, uint32_t left, uint32_t right, unsigned bits)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u, result = (left - right) & mask;
    cpu->flags &= (uint16_t)~0x08D5u;
    if (left < right) cpu->flags |= CF;
    if (even_parity((uint8_t)result)) cpu->flags |= 0x0004u;
    if ((left ^ right ^ result) & 0x10u) cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= ZF;
    if (result & sign) cpu->flags |= 0x0080u;
    if ((left ^ right) & (left ^ result) & sign) cpu->flags |= 0x0800u;
}

/* A near RET (n = 0) back to `expected`; anything else stops with TRANSFER. */
static int near_ret(machine *m, uint16_t expected)
{
    m->cpu->ip = pop16(m);
    if (m->cpu->ip != expected) {
        m->stop = PR_SYSTEM_FREEMEM_TRANSFER;
        return 0;
    }
    return 1;
}

/* CALL 0491 from03F9; intermediate flags are overwritten by AND AX,8. */
static int round_up(machine *m)
{
    pr_system_freemem_cpu *c=m->cpu;uint32_t sum=(uint32_t)c->ax+7u;
    push16(m,0x03FC);c->ax=(uint16_t)(sum&8u);c->dx=(uint16_t)(sum>>4);
    logic_flags(c,c->ax,0x8000u);return near_ret(m,0x03FC);
}
static void movsw(machine *m)
{
    pr_system_freemem_cpu *c=m->cpu;
    uint16_t value=read16(m,c->ds,c->si);
    write16(m,c->es,c->di,value);c->si=(uint16_t)(c->si+2u);c->di=(uint16_t)(c->di+2u);
}
static void lodsw(machine *m)
{
    m->cpu->ax=read16(m,m->cpu->ds,m->cpu->si);m->cpu->si=(uint16_t)(m->cpu->si+2u);
}
static void stosw(machine *m)
{
    write16(m,m->cpu->es,m->cpu->di,m->cpu->ax);m->cpu->di=(uint16_t)(m->cpu->di+2u);
}
static void exchange_ax_dx(pr_system_freemem_cpu *c)
{
    uint16_t temp=c->ax;c->ax=c->dx;c->dx=temp;
}
/* +0432: link the block BX:CX to ES:DI, merging adjacency in source order. */
static int merge(machine *m,uint16_t expected)
{
    pr_system_freemem_cpu *c=m->cpu;uint16_t value;
    push16(m,expected);c->ax=c->di;c->dx=c->es;c->di=c->cx;c->es=c->bx;
    c->cx=(uint16_t)(c->cx+read16(m,c->es,(uint16_t)(c->di+4u)));
    c->bx=(uint16_t)(c->bx+read16(m,c->es,(uint16_t)(c->di+6u)));
    if((c->cx&255u)>=16u)c->bx=(uint16_t)(c->bx+1u);
    c->cx=(uint16_t)((c->cx&0xFF00u)|(c->cx&15u));logic_flags(c,(uint8_t)c->cx,0x80u);
    cmp_flags(c,c->bx,c->dx,16);
    if(c->bx!=c->dx)goto link;
    cmp_flags(c,c->cx,c->ax,16);
    if(c->cx!=c->ax)goto link;
    value=read16(m,c->ds,0x4406);cmp_flags(c,c->dx,value,16);
    if(c->dx!=value)goto join;
    value=read16(m,c->ds,0x4404);cmp_flags(c,c->ax,value,16);
    if(c->ax!=value)goto join;
    write16(m,c->ds,0x4404,c->di);write16(m,c->ds,0x4406,c->es);
    return near_ret(m,expected);
join:
    push16(m,c->ds);push16(m,c->di);c->si=c->ax;c->ds=c->dx;c->flags&=(uint16_t)~0x400u;
    movsw(m);movsw(m);lodsw(m);exchange_ax_dx(c);lodsw(m);exchange_ax_dx(c);
    c->ax=(uint16_t)(c->ax+read16(m,c->es,c->di));
    c->dx=(uint16_t)(c->dx+read16(m,c->es,(uint16_t)(c->di+2u)));
    if((c->ax&255u)>=16u)c->dx=(uint16_t)(c->dx+1u);
    c->ax=(uint16_t)((c->ax&0xFF00u)|(c->ax&15u));logic_flags(c,(uint8_t)c->ax,0x80u);
    stosw(m);exchange_ax_dx(c);stosw(m);c->di=pop16(m);c->ds=pop16(m);
    return near_ret(m,expected);
link:
    write16(m,c->es,c->di,c->ax);write16(m,c->es,(uint16_t)(c->di+2u),c->dx);
    return near_ret(m,expected);
}
static int core(machine *m)
{
    pr_system_freemem_cpu *c=m->cpu;uint16_t value;
    push16(m,0x0262);logic_flags(c,c->ax,0x8000u);
    if(c->ax==0)return near_ret(m,0x0262);
    value=(uint16_t)(c->cx&0xFFF7u);logic_flags(c,value,0x8000u);
    if(value!=0)goto invalid;
    value=read16(m,c->ds,0x4402);cmp_flags(c,c->bx,value,16);
    if(c->bx<value)goto invalid;
    if(c->bx==value) { value=read16(m,c->ds,0x4400);cmp_flags(c,c->cx,value,16);if(c->cx<value)goto invalid; }
    value=read16(m,c->ds,0x4406);cmp_flags(c,c->bx,value,16);
    if(c->bx>value)goto invalid;
    if(c->bx==value) { value=read16(m,c->ds,0x4404);cmp_flags(c,c->cx,value,16);if(c->cx>=value)goto invalid; }
    if(!round_up(m))return 0;
    c->di=c->cx;c->es=c->bx;write16(m,c->es,(uint16_t)(c->di+4u),c->ax);write16(m,c->es,(uint16_t)(c->di+6u),c->dx);
    c->di=0x440C;c->ax=c->ds;c->es=c->ax;
    for(;;) {
        c->ax=c->di;c->dx=c->es;les(m,c->es,c->di);c->si=c->es;
        cmp_flags(c,c->bx,c->si,16);
        if(c->bx<c->si)break;
        if(c->bx==c->si) { cmp_flags(c,c->cx,c->di,16);if(c->cx<c->di)break;if(c->cx==c->di)goto invalid; }
        if(m->steps==m->max_steps) { c->ip=0x040F;m->stop=PR_SYSTEM_FREEMEM_LIMIT;return 0; }
        ++m->steps;
    }
    push16(m,c->dx);push16(m,c->ax);
    if(!merge(m,0x0429))return 0;
    c->cx=pop16(m);c->bx=pop16(m);
    if(!merge(m,0x042E))return 0;
    c->flags&=(uint16_t)~CF;return near_ret(m,0x0262);
invalid:
    c->flags|=CF;return near_ret(m,0x0262);
}
pr_system_freemem_status pr_system_freemem_0254(uint8_t *ram,size_t size,pr_system_freemem_cpu *c,unsigned max_steps)
{
    machine m;size_t at;
    if(ram==NULL || c==NULL || size!=PR_SYSTEM_FREEMEM_RAM_SIZE || c->ip!=0x0254 || c->sp<0x20 || c->sp>0xFFF6)
        return PR_SYSTEM_FREEMEM_ARGUMENT;
    m=(machine){ram,c,c->cs,0,max_steps,PR_SYSTEM_FREEMEM_RETURNED};
    push16(&m,c->bp);c->bp=c->sp;c->ax=read16(&m,c->ss,(uint16_t)(c->bp+6u));
    at=physical(c->ss,(uint16_t)(c->bp+8u));c->cx=read16_at(ram,at);c->es=read16_at(ram,(at+2u)&MASK);c->bx=c->es;
    if(!core(&m))return m.stop;
    c->bp=pop16(&m);
    if(c->flags&CF) { c->ax=204;c->ip=0x00E2;return PR_SYSTEM_FREEMEM_RUNERROR; }
    at=physical(c->ss,c->sp);c->ip=read16_at(ram,at);c->cs=read16_at(ram,(at+2u)&MASK);
    c->sp=(uint16_t)(c->sp+10u);return PR_SYSTEM_FREEMEM_RETURNED;
}
