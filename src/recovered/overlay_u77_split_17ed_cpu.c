/* U77 +17ED, composed original far-call frames; legacy API is separate. */
#include "poolrad/recovered/overlay_u77_split_17ed_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include <string.h>
#define RAM_MASK (PR_U77_17ED_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t read8(pr_bus *ram, uint16_t seg, uint16_t off)
{
    return pr_bus_read8(ram,physical(seg,off));
}

static uint16_t read16_at(pr_bus *ram, size_t address)
{
    return (uint16_t)(pr_bus_read8(ram,address) |
                      ((uint16_t)pr_bus_read8(ram,(address + 1u) & RAM_MASK) << 8));
}

static uint16_t read16(pr_bus *ram, uint16_t seg, uint16_t off)
{
    return read16_at(ram, physical(seg, off));
}

static void write8(pr_bus *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    pr_bus_write8(ram,physical(seg,off),value);
}

static void write16(pr_bus *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    pr_bus_write8(ram,a,(uint8_t)value);
    pr_bus_write8(ram,(a+1u)&RAM_MASK,(uint8_t)(value>>8));
}

static void push16(pr_bus *ram, pr_u77_17ed_cpu *cpu, uint16_t value)
{
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    write16(ram, cpu->ss, cpu->sp, value);
}

static uint16_t pop16(pr_bus *ram, pr_u77_17ed_cpu *cpu)
{
    uint16_t value = read16(ram, cpu->ss, cpu->sp);
    cpu->sp = (uint16_t)(cpu->sp + 2u);
    return value;
}

static uint8_t local8(pr_bus *ram, const pr_u77_17ed_cpu *cpu,
                      uint16_t distance)
{
    return read8(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void set_local8(pr_bus *ram, const pr_u77_17ed_cpu *cpu,
                       uint16_t distance, uint8_t value)
{
    write8(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}

static uint16_t local16(pr_bus *ram, const pr_u77_17ed_cpu *cpu,
                        uint16_t distance)
{
    return read16(ram, cpu->ss, (uint16_t)(cpu->bp - distance));
}

static void set_local16(pr_bus *ram, const pr_u77_17ed_cpu *cpu,
                        uint16_t distance, uint16_t value)
{
    write16(ram, cpu->ss, (uint16_t)(cpu->bp - distance), value);
}

static void les_far_argument(pr_bus *ram, pr_u77_17ed_cpu *cpu)
{
    size_t a = physical(cpu->ss, (uint16_t)(cpu->bp + 6u));
    cpu->di = read16_at(ram, a);
    cpu->es = read16_at(ram, (a + 2u) & RAM_MASK);
}

static void les_local(pr_bus *ram, pr_u77_17ed_cpu *cpu)
{
    cpu->di = local16(ram, cpu, 4u);
    cpu->es = local16(ram, cpu, 2u);
}

static void set_al(pr_u77_17ed_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static void parity(pr_u77_17ed_cpu *cpu, uint8_t result)
{
    result ^= (uint8_t)(result >> 4);
    result ^= (uint8_t)(result >> 2);
    result ^= (uint8_t)(result >> 1);
    if (!(result & 1u)) cpu->flags |= 0x0004u;
}

static void cmp8(pr_u77_17ed_cpu *cpu, uint8_t left, uint8_t right)
{
    uint8_t result = (uint8_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    parity(cpu, result);
    if (((left ^ right ^ result) & 0x10u) != 0u)
        cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x80u) != 0u)
        cpu->flags |= 0x0800u;
}

static void sub16(pr_u77_17ed_cpu *cpu, uint16_t right)
{
    uint16_t left = cpu->ax;
    uint16_t result = (uint16_t)(left - right);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (left < right) cpu->flags |= 0x0001u;
    parity(cpu, (uint8_t)result);
    if (((left ^ right ^ result) & 0x0010u) != 0u)
        cpu->flags |= 0x0010u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x8000u) cpu->flags |= 0x0080u;
    if (((left ^ right) & (left ^ result) & 0x8000u) != 0u)
        cpu->flags |= 0x0800u;
    cpu->ax = result;
}

static int ordinary(uint16_t seg)
{
    size_t a=(size_t)seg<<4u;
    return a>0xAFFFFu || a+0xFFFFu<0xA0000u;
}
static pr_u77_17ed_cpu_status far_call(pr_bus *ram,pr_u77_17ed_cpu *cpu,
    uint16_t site,uint16_t target,int which,const pr_u77_17ed_limits *l,
    pr_u77_17ed_diagnostic *d)
{
    uint16_t caller=cpu->cs,next=(uint16_t)(site+5u),segment;
    cpu->ip=site;
    if(read8(ram,caller,site)!=0x9Au || read16(ram,caller,(uint16_t)(site+1u))!=target)
        return PR_U77_17ED_CPU_CODE;
    segment=read16(ram,caller,(uint16_t)(site+3u));
    push16(ram,cpu,caller);push16(ram,cpu,next);
    cpu->cs=segment;cpu->ip=target;d->callee=which;
    if(which==0) {
        pr_system_getmem_cpu inner;
        _Static_assert(sizeof inner==sizeof *cpu,"CPU layout");
        memcpy(&inner,cpu,sizeof inner);
        d->status=pr_system_getmem_023f(ram->ram,PR_BUS_RAM_SIZE,&inner,l->heap_steps);
        memcpy(cpu,&inner,sizeof inner);
    } else if(which==1 || which==2) {
        pr_system_leaf_cpu inner;
        _Static_assert(sizeof inner==sizeof *cpu,"CPU layout");
        memcpy(&inner,cpu,sizeof inner);
        d->status=which==1 ? pr_system_move_1685(ram->ram,PR_BUS_RAM_SIZE,&inner) :
                            pr_system_strload_0b0b(ram->ram,PR_BUS_RAM_SIZE,&inner);
        memcpy(cpu,&inner,sizeof inner);
    } else {
        pr_r00ba_1693_cpu inner;
        _Static_assert(sizeof inner==sizeof *cpu,"CPU layout");
        memcpy(&inner,cpu,sizeof inner);
        d->status=pr_r00ba_display_1693_cpu(ram,PR_BUS_RAM_SIZE,&inner,
            l->clear_loopbacks,l->rect_loopbacks,l->printer_loopbacks,
            l->glyph_loopbacks,l->delay_loopbacks,&d->display);
        memcpy(cpu,&inner,sizeof inner);
    }
    if(d->status!=0)return PR_U77_17ED_CPU_CALLEE;
    if(cpu->cs!=caller || cpu->ip!=next)return PR_U77_17ED_CPU_TRANSFER;
    return PR_U77_17ED_CPU_OK;
}
pr_u77_17ed_cpu_status pr_u77_split_17ed_cpu(pr_bus *ram,size_t size,
    pr_u77_17ed_cpu *cpu,const pr_u77_17ed_limits *l,pr_u77_17ed_diagnostic *d)
{
    pr_u77_17ed_diagnostic fallback={-1,-1,{-1,-1,-1,{-1,-1,{-1,-1}},{-1,-1,-1,-1}}};
    pr_u77_17ed_cpu_status status;
    uint16_t half;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || cpu==NULL || l==NULL ||
       size!=PR_BUS_RAM_SIZE || cpu->ip!=0x17EDu || cpu->sp<0x018Cu || cpu->sp>0xFFF8u ||
       !ordinary(cpu->cs) || !ordinary(cpu->ds) || !ordinary(cpu->ss))
        return PR_U77_17ED_CPU_ARGUMENT;
    push16(ram, cpu, cpu->bp);
    cpu->bp = cpu->sp;
    cpu->sp = (uint16_t)(cpu->sp - 24u);
    les_far_argument(ram, cpu);
    set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u)));
    cpu->ax &= 0x00FFu;
    cpu->ax >>= 1;
    half = cpu->ax;
    set_local8(ram, cpu, 5u, (uint8_t)half);
    cmp8(cpu, local8(ram, cpu, 5u), 0u);

    if (half != 0u) {
        set_al(cpu, local8(ram, cpu, 5u));
        cpu->ax &= 0x00FFu;
        cpu->dx = cpu->ax;
        les_far_argument(ram, cpu);
        set_al(cpu, read8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u)));
        cpu->ax &= 0x00FFu;
        sub16(cpu, cpu->dx);
        set_local8(ram, cpu, 6u, (uint8_t)cpu->ax);

        cpu->ax = 0x003Fu;
        push16(ram, cpu, cpu->ax);
        status=far_call(ram,cpu,0x1820u,0x023Fu,0,l,d);
        if(status!=PR_U77_17ED_CPU_OK)return status;
        set_local16(ram, cpu, 4u, cpu->ax);
        set_local16(ram, cpu, 2u, cpu->dx);

        les_far_argument(ram, cpu);
        push16(ram, cpu, cpu->es);
        push16(ram, cpu, cpu->di);
        les_local(ram, cpu);
        push16(ram, cpu, cpu->es);
        push16(ram, cpu, cpu->di);
        cpu->ax = 0x003Fu;
        push16(ram, cpu, cpu->ax);
        status=far_call(ram,cpu,0x1839u,0x1685u,1,l,d);
        if(status!=PR_U77_17ED_CPU_OK)return status;

        set_al(cpu, local8(ram, cpu, 5u));
        les_local(ram, cpu);
        write8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u), (uint8_t)cpu->ax);
        les_local(ram, cpu);
        write8(ram, cpu->es, (uint16_t)(cpu->di + 0x34u), 0u);
        les_far_argument(ram, cpu);
        cpu->ax = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au));
        cpu->dx = read16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu));
        les_local(ram, cpu);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au), cpu->ax);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu), cpu->dx);
        set_al(cpu, local8(ram, cpu, 6u));
        les_far_argument(ram, cpu);
        write8(ram, cpu->es, (uint16_t)(cpu->di + 0x39u), (uint8_t)cpu->ax);
        cpu->ax = local16(ram, cpu, 4u);
        cpu->dx = local16(ram, cpu, 2u);
        les_far_argument(ram, cpu);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Au), cpu->ax);
        write16(ram, cpu->es, (uint16_t)(cpu->di + 0x2Cu), cpu->dx);
    } else {
        cpu->di = (uint16_t)(cpu->bp - 0x17u);
        push16(ram, cpu, cpu->ss);
        push16(ram, cpu, cpu->di);
        cpu->di = 0x17DCu;
        push16(ram, cpu, cpu->cs);
        push16(ram, cpu, cpu->di);
        status=far_call(ram,cpu,0x188Du,0x0B0Bu,2,l,d);
        if(status!=PR_U77_17ED_CPU_OK)return status;
        status=far_call(ram,cpu,0x1892u,0x1693u,3,l,d);
        if(status!=PR_U77_17ED_CPU_OK)return status;
    }

    cpu->sp = cpu->bp;
    cpu->bp = pop16(ram, cpu);
    cpu->ip = pop16(ram, cpu);
    cpu->cs = pop16(ram, cpu);
    cpu->sp = (uint16_t)(cpu->sp + 4u);
    return PR_U77_17ED_CPU_OK;
}
