/* 00BA +17E3: raw fixed rectangle selector. Primitives reuse +0626. */
#include "poolrad/recovered/resident_00ba_clear_17e3_cpu.h"
#include "poolrad/recovered/system_fillchar_bus_16a8_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4u) + off) & MASK;
}
static uint16_t word_at(pr_bus *ram, size_t at)
{
    return (uint16_t)(pr_bus_read8(ram,at) | ((uint16_t)pr_bus_read8(ram,(at+1u)&MASK) << 8u));
}
static uint16_t read_word(pr_bus *ram, uint16_t seg, uint16_t off)
{
    return word_at(ram,physical(seg,off));
}
static void put_word(pr_bus *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at=physical(seg,off);
    pr_bus_write8(ram,at,(uint8_t)value);pr_bus_write8(ram,(at+1u)&MASK,(uint8_t)(value >> 8u));
}
static void push(pr_bus *ram, pr_r00ba_clear_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r00ba_clear_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r00ba_clear_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r00ba_clear_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r00ba_clear_cpu *c, uint16_t left, uint16_t right,
                           unsigned carry, unsigned bits, unsigned subtract)
{
    uint32_t mask=bits==8u ? 255u : 65535u;
    uint16_t sign=bits==8u ? 0x80u : 0x8000u;
    uint32_t wide=subtract ? (uint32_t)left-right-carry : (uint32_t)left+right+carry;
    uint16_t result=(uint16_t)(wide&mask);
    c->flags&=(uint16_t)~0x08D5u;
    if (subtract ? (uint32_t)left<(uint32_t)right+carry : wide>mask)c->flags|=1u;
    if ((left^right^result)&0x10u)c->flags|=0x10u;
    if ((subtract ? (left^right)&(left^result) : (uint16_t)~(left^right)&(left^result))&sign)
        c->flags|=0x800u;
    result_flags(c,result,sign);return result;
}


static int ordinary_segment(uint16_t s) {
    size_t start=(size_t)s<<4u;
    return start>0xAFFFFu || start+0xFFFFu<0xA0000u;
}


static int clear(pr_bus *b,pr_r00ba_clear_cpu *c,size_t limit,size_t rect_limit,
                     pr_r00ba_clear_cpu_diagnostic *d) {
    pr_r03f1_clear_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_r03f1_clear_0137_cpu(b,PR_BUS_RAM_SIZE,&inner,limit,rect_limit,&d->clear);
    memcpy(c,&inner,sizeof inner);return status;
}
pr_r00ba_clear_cpu_status pr_r00ba_clear_17e3_cpu(pr_bus *ram,size_t size,
    pr_r00ba_clear_cpu *c,size_t max_loopbacks,size_t rect_loopbacks,
    pr_r00ba_clear_cpu_diagnostic *d)
{
    pr_r00ba_clear_cpu_diagnostic fallback={-1,-1,{-1,-1,{-1,-1}}};uint16_t caller;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0x17E3u || c->sp<0x4Cu || c->sp>0xFFFCu ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds))
        return PR_R00BA_CLEAR_CPU_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_R00BA_CLEAR_CPU_TRANSFER;
        switch(c->ip) {
        case 0x17E3: push(ram,c,c->bp);c->ip=0x17E4u;break;
        case 0x17E4: c->bp=c->sp;c->ip=0x17E6u;break;
        case 0x17E6: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ds,(uint16_t)(18931))),0x0005u,0u,8u,1u);c->ip=0x17EBu;break;
        case 0x17EB: if((c->flags&0x40u)==0){c->ip=0x1800u;}else c->ip=0x17EDu;break;
        case 0x17ED: al(c,(uint8_t)(0x0017u));c->ip=0x17EFu;break;
        case 0x17EF: push(ram,c,c->ax);c->ip=0x17F0u;break;
        case 0x17F0: al(c,(uint8_t)(0x000Au));c->ip=0x17F2u;break;
        case 0x17F2: push(ram,c,c->ax);c->ip=0x17F3u;break;
        case 0x17F3: al(c,(uint8_t)(0x0026u));c->ip=0x17F5u;break;
        case 0x17F5: push(ram,c,c->ax);c->ip=0x17F6u;break;
        case 0x17F6: al(c,(uint8_t)(0x0015u));c->ip=0x17F8u;break;
        case 0x17F8: push(ram,c,c->ax);c->ip=0x17F9u;break;
        case 0x17F9: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x17FAu)!=0x0137u)return PR_R00BA_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x17FEu);c->cs=read_word(ram,c->cs,0x17FCu);c->ip=0x0137u;d->callee=0;d->status=clear(ram,c,max_loopbacks,rect_loopbacks,d);if(d->status!=0)return PR_R00BA_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x17FEu)return PR_R00BA_CLEAR_CPU_TRANSFER;break;
        case 0x17FE: c->ip=0x1811u;break;
        case 0x1800: al(c,(uint8_t)(0x0001u));c->ip=0x1802u;break;
        case 0x1802: push(ram,c,c->ax);c->ip=0x1803u;break;
        case 0x1803: al(c,(uint8_t)(0x0012u));c->ip=0x1805u;break;
        case 0x1805: push(ram,c,c->ax);c->ip=0x1806u;break;
        case 0x1806: al(c,(uint8_t)(0x0026u));c->ip=0x1808u;break;
        case 0x1808: push(ram,c,c->ax);c->ip=0x1809u;break;
        case 0x1809: al(c,(uint8_t)(0x0016u));c->ip=0x180Bu;break;
        case 0x180B: push(ram,c,c->ax);c->ip=0x180Cu;break;
        case 0x180C: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x180Du)!=0x0137u)return PR_R00BA_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x1811u);c->cs=read_word(ram,c->cs,0x180Fu);c->ip=0x0137u;d->callee=1;d->status=clear(ram,c,max_loopbacks,rect_loopbacks,d);if(d->status!=0)return PR_R00BA_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x1811u)return PR_R00BA_CLEAR_CPU_TRANSFER;break;
        case 0x1811: c->bp=pop(ram,c);c->ip=0x1812u;break;
        case 0x1812: c->ip=pop(ram,c);c->cs=pop(ram,c);return PR_R00BA_CLEAR_CPU_OK;
        default:return PR_R00BA_CLEAR_CPU_TRANSFER;
        }
    }
}
