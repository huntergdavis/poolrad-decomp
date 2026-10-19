/* Complete U38 +27D9; docs/re/U38_ROLL_EFFECT_27D9_CPU.md.
 * Memory/flags and live-stub composition follow accepted U38 +2DE4. */
#include "poolrad/recovered/overlay_u38_roll_effect_27d9_cpu.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"
#include "poolrad/recovered/overlay_u9e_list_append_0e52_cpu.h"
#include <string.h>
#define MASK (PR_U38_27D9_RAM_SIZE-1u)
#define ARITH 0x08D5u
_Static_assert(sizeof(pr_u38_27d9_cpu)==sizeof(pr_u9e_entry8_compose_cpu),"dice CPU layout");
_Static_assert(sizeof(pr_u38_27d9_cpu)==sizeof(pr_u9e_0e52_cpu),"append CPU layout");
static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & MASK;
}

static uint8_t read8(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return ram[physical(segment, offset)];
}

static uint16_t read16_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t read16(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    return read16_at(ram, physical(segment, offset));
}

static void push16(uint8_t *ram, pr_u38_27d9_cpu *cpu, uint16_t value)
{
    size_t at;
    cpu->sp = (uint16_t)(cpu->sp - 2u);
    at = physical(cpu->ss, cpu->sp);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static unsigned even_parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

static void logic8(pr_u38_27d9_cpu *cpu, uint8_t result)
{
    cpu->flags &= (uint16_t)~ARITH;
    if (even_parity(result)) cpu->flags |= 0x0004u;
    if (result == 0u) cpu->flags |= 0x0040u;
    if (result & 0x80u) cpu->flags |= 0x0080u;
}

static void set_al(pr_u38_27d9_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

static pr_u38_27d9_status call(uint8_t *ram,size_t size,pr_u38_27d9_cpu *c,
    const pr_u38_27d9_limits *l,pr_u38_27d9_diagnostic *d,uint16_t site,unsigned append)
{
    uint16_t caller=c->cs,next=(uint16_t)(site+5u),offset=append?0x52u:0x48u,target=append?0x0E52u:0x0DE3u,stub;
    c->ip=site;
    if(read8(ram,caller,site)!=0x9Au || read16(ram,caller,(uint16_t)(site+1u))!=offset)return PR_U38_27D9_CODE;
    stub=read16(ram,caller,(uint16_t)(site+3u));
    if(read8(ram,stub,offset)!=0xEAu || read16(ram,stub,(uint16_t)(offset+1u))!=target)return PR_U38_27D9_CODE;
    push16(ram,c,caller);push16(ram,c,next);c->cs=read16(ram,stub,(uint16_t)(offset+3u));c->ip=target;
    d->callee=append?PR_U38_27D9_APPEND:PR_U38_27D9_ROLL;d->getmem_status=-1;
    if(append) {
        pr_u9e_0e52_cpu inner;memcpy(&inner,c,sizeof inner);
        d->status=(int)pr_u9e_list_append_0e52_cpu(ram,size,&inner,l->max_heap_steps,l->max_links,&d->getmem_status);
        memcpy(c,&inner,sizeof inner);
    } else {
        pr_u9e_entry8_compose_cpu inner;memcpy(&inner,c,sizeof inner);
        d->status=(int)pr_u9e_entry8_fullstate_compose_0de3(ram,size,&inner,l->max_system_calls);
        memcpy(c,&inner,sizeof inner);
    }
    if(d->status!=0)return PR_U38_27D9_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U38_27D9_TRANSFER;
    return PR_U38_27D9_OK;
}
pr_u38_27d9_status pr_u38_roll_effect_27d9_cpu(uint8_t *ram,size_t size,pr_u38_27d9_cpu *c,
    const pr_u38_27d9_limits *l,pr_u38_27d9_diagnostic *d)
{
    pr_u38_27d9_diagnostic local={PR_U38_27D9_NO_CALLEE,-1,-1};pr_u38_27d9_status status;size_t frame;
    if(d==NULL)d=&local;
    *d=local;
    if(ram==NULL || c==NULL || l==NULL || size!=PR_U38_27D9_RAM_SIZE || c->ip!=0x27D9u || c->sp<0x40u || c->sp>0xFFF2u)
        return PR_U38_27D9_ARGUMENT;
    push16(ram,c,c->bp);c->bp=c->sp;                       /* +27D9 PUSH BP; MOV BP,SP */
    set_al(c,(uint8_t)(read8(ram,c->ds,0x6819u)&1u));
    logic8(c,(uint8_t)c->ax);                             /* AND AL,1; OR AL,AL */
    if ((c->flags&0x40u)!=0u) {
        set_al(c,(uint8_t)(read8(ram,c->ds,0x6819u)&0x10u));
        logic8(c,(uint8_t)c->ax);                         /* AND AL,10h; OR AL,AL */
        if ((c->flags&0x40u)!=0u) {
            push16(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xEu)));
            push16(ram,c,read16(ram,c->ss,(uint16_t)(c->bp+0xCu)));
            set_al(c,0x66);push16(ram,c,c->ax);
            set_al(c,3);push16(ram,c,c->ax);set_al(c,6);push16(ram,c,c->ax);
            status=call(ram,size,c,l,d,0x27FD,0);if(status!=0)return status;
            c->ax&=0x00FFu;logic8(c,0);                   /* +2802 XOR AH,AH */
            push16(ram,c,c->ax);set_al(c,0xFF);push16(ram,c,c->ax);
            set_al(c,1);push16(ram,c,c->ax);
            status=call(ram,size,c,l,d,0x280B,1);if(status!=0)return status;
        }
    }
    c->bp=read16(ram,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u); /* +2810 POP BP */
    frame=physical(c->ss,c->sp);c->ip=read16_at(ram,frame);c->cs=read16_at(ram,(frame+2u)&MASK);
    c->sp=(uint16_t)(c->sp+14u);return PR_U38_27D9_OK;        /* +2811 RETF0Ah */
}
