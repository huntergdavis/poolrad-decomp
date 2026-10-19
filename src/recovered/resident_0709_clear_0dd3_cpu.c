/* 0709 +0DD3: raw mode-dependent rectangle clear. Primitives reuse +0626. */
#include "poolrad/recovered/resident_0709_clear_0dd3_cpu.h"
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
static void push(pr_bus *ram, pr_r0709_clear_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r0709_clear_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r0709_clear_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r0709_clear_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r0709_clear_cpu *c, uint16_t left, uint16_t right,
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


static uint16_t increment(pr_r0709_clear_cpu *c,uint16_t value,unsigned bits) {
    uint16_t cf=c->flags&1u;value=arithmetic(c,value,1u,0u,bits,0u);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return value;
}
static int ordinary_segment(uint16_t s) {
    size_t start=(size_t)s<<4u;
    return start>0xAFFFFu || start+0xFFFFu<0xA0000u;
}


static int fill(pr_bus *b,pr_r0709_clear_cpu *c) {
    pr_system_leaf_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_system_fillchar_bus_16a8(b,&inner);
    memcpy(c,&inner,sizeof inner);return status;
}
static int rectangle(pr_bus *b,pr_r0709_clear_cpu *c,size_t limit,
                     pr_r0709_clear_cpu_diagnostic *d) {
    pr_r0709_rect_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_r0709_rect_fill_00b7_cpu(b,PR_BUS_RAM_SIZE,&inner,limit,&d->rect);
    memcpy(c,&inner,sizeof inner);return status;
}
pr_r0709_clear_cpu_status pr_r0709_clear_0dd3_cpu(pr_bus *ram,size_t size,
    pr_r0709_clear_cpu *c,size_t max_loopbacks,size_t rect_loopbacks,
    pr_r0709_clear_cpu_diagnostic *d)
{
    pr_r0709_clear_cpu_diagnostic fallback={-1,-1,{-1,-1}};uint16_t caller;size_t loops=0;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0xDD3u || c->sp<0x3Eu || c->sp>0xFFF4u ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds))
        return PR_R0709_CLEAR_CPU_ARGUMENT;
    caller=c->cs;
    /* MUL/SHL result flags are overwritten by ADD/CMP before any observable
     * call, OUT, branch, budget stop or return. Their register effects remain. */
    for(;;) {
        if(c->cs!=caller)return PR_R0709_CLEAR_CPU_TRANSFER;
        switch(c->ip) {
        case 0x0DD3: push(ram,c,c->bp);c->ip=0x0DD4u;break;
        case 0x0DD4: c->bp=c->sp;c->ip=0x0DD6u;break;
        case 0x0DD6: c->sp=arithmetic(c,c->sp,0x000Cu,0u,16u,1u);c->ip=0x0DD9u;break;
        case 0x0DD9: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(18974)))));c->ip=0x0DDCu;break;
        case 0x0DDC: (void)arithmetic(c,(uint8_t)c->ax,0x0000u,0u,8u,1u);c->ip=0x0DDEu;break;
        case 0x0DDE: if((c->flags&0x40u)!=0){c->ip=0x0DE3u;}else c->ip=0x0DE0u;break;
        case 0x0DE0: c->ip=0x0E73u;break;
        case 0x0DE3: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x0DE6u;break;
        case 0x0DE6: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0DE7u;break;
        case 0x0DE7: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0DE9u;break;
        case 0x0DE9: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0DEBu;break;
        case 0x0DEB: put_word(ram,c->ss,(uint16_t)(c->bp+-2),c->ax);c->ip=0x0DEEu;break;
        case 0x0DEE: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6)))));c->ip=0x0DF1u;break;
        case 0x0DF1: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0DF2u;break;
        case 0x0DF2: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0DF4u;break;
        case 0x0DF4: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0DF6u;break;
        case 0x0DF6: c->ax=arithmetic(c,c->ax,0x0003u,0u,16u,0u);c->ip=0x0DF9u;break;
        case 0x0DF9: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x0DFCu;break;
        case 0x0DFC: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0DFFu;break;
        case 0x0DFF: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0E00u;break;
        case 0x0E00: c->dx=c->ax;c->ip=0x0E02u;break;
        case 0x0E02: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x0E05u;break;
        case 0x0E05: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0E06u;break;
        case 0x0E06: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,1u);c->ip=0x0E08u;break;
        case 0x0E08: c->ax=increment(c,c->ax,16u);c->ip=0x0E09u;break;
        case 0x0E09: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0E0Bu;break;
        case 0x0E0B: put_word(ram,c->ss,(uint16_t)(c->bp+-8),c->ax);c->ip=0x0E0Eu;break;
        case 0x0E0E: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x0E11u;break;
        case 0x0E11: put_word(ram,c->ss,(uint16_t)(c->bp+-12),c->ax);c->ip=0x0E14u;break;
        case 0x0E14: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-2));c->ip=0x0E17u;break;
        case 0x0E17: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-12)),0u,16u,1u);c->ip=0x0E1Au;break;
        case 0x0E1A: if((c->flags&0x41u)==0){c->ip=0x0E70u;}else c->ip=0x0E1Cu;break;
        case 0x0E1C: put_word(ram,c->ss,(uint16_t)(c->bp+-10),c->ax);c->ip=0x0E1Fu;break;
        case 0x0E1F: c->ip=0x0E24u;break;
        case 0x0E21: put_word(ram,c->ss,(uint16_t)(c->bp+-10),increment(c,read_word(ram,c->ss,(uint16_t)(c->bp+-10)),16u));c->ip=0x0E24u;break;
        case 0x0E24: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0E27u;break;
        case 0x0E27: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0E28u;break;
        case 0x0E28: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0E2Au;break;
        case 0x0E2A: c->cx=c->ax;c->ip=0x0E2Cu;break;
        case 0x0E2C: c->ax=0x0050u;c->ip=0x0E2Fu;break;
        case 0x0E2F: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x0E32u;break;
        case 0x0E32: c->di=c->ax;c->ip=0x0E34u;break;
        case 0x0E34: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0E36u;break;
        case 0x0E36: c->ax=0xB800u;c->ip=0x0E39u;break;
        case 0x0E39: push(ram,c,c->ax);c->ip=0x0E3Au;break;
        case 0x0E3A: push(ram,c,c->di);c->ip=0x0E3Bu;break;
        case 0x0E3B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x0E3Eu;break;
        case 0x0E3E: al(c,(uint8_t)(0x0000u));c->ip=0x0E40u;break;
        case 0x0E40: push(ram,c,c->ax);c->ip=0x0E41u;break;
        case 0x0E41: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0E42u)!=0x16A8u)return PR_R0709_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0E46u);c->cs=read_word(ram,c->cs,0x0E44u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0E46u)return PR_R0709_CLEAR_CPU_TRANSFER;break;
        case 0x0E46: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0E49u;break;
        case 0x0E49: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0E4Au;break;
        case 0x0E4A: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0E4Cu;break;
        case 0x0E4C: c->cx=c->ax;c->ip=0x0E4Eu;break;
        case 0x0E4E: c->ax=0x0050u;c->ip=0x0E51u;break;
        case 0x0E51: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x0E54u;break;
        case 0x0E54: c->di=c->ax;c->ip=0x0E56u;break;
        case 0x0E56: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0E58u;break;
        case 0x0E58: c->ax=0xBA00u;c->ip=0x0E5Bu;break;
        case 0x0E5B: push(ram,c,c->ax);c->ip=0x0E5Cu;break;
        case 0x0E5C: push(ram,c,c->di);c->ip=0x0E5Du;break;
        case 0x0E5D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x0E60u;break;
        case 0x0E60: al(c,(uint8_t)(0x0000u));c->ip=0x0E62u;break;
        case 0x0E62: push(ram,c,c->ax);c->ip=0x0E63u;break;
        case 0x0E63: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0E64u)!=0x16A8u)return PR_R0709_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0E68u);c->cs=read_word(ram,c->cs,0x0E66u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0E68u)return PR_R0709_CLEAR_CPU_TRANSFER;break;
        case 0x0E68: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ip=0x0E6Bu;break;
        case 0x0E6B: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-12)),0u,16u,1u);c->ip=0x0E6Eu;break;
        case 0x0E6E: if((c->flags&0x40u)==0){if(loops==max_loopbacks)return PR_R0709_CLEAR_CPU_LIMIT;++loops;c->ip=0x0E21u;}else c->ip=0x0E70u;break;
        case 0x0E70: c->ip=0x0F7Cu;break;
        case 0x0E73: (void)arithmetic(c,(uint8_t)c->ax,0x0001u,0u,8u,1u);c->ip=0x0E75u;break;
        case 0x0E75: if((c->flags&0x40u)==0){c->ip=0x0EAEu;}else c->ip=0x0E77u;break;
        case 0x0E77: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0E7Au;break;
        case 0x0E7A: push(ram,c,c->ax);c->ip=0x0E7Bu;break;
        case 0x0E7B: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x0E7Eu;break;
        case 0x0E7E: push(ram,c,c->ax);c->ip=0x0E7Fu;break;
        case 0x0E7F: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x0E82u;break;
        case 0x0E82: push(ram,c,c->ax);c->ip=0x0E83u;break;
        case 0x0E83: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6)))));c->ip=0x0E86u;break;
        case 0x0E86: push(ram,c,c->ax);c->ip=0x0E87u;break;
        case 0x0E87: al(c,(uint8_t)(0x0000u));c->ip=0x0E89u;break;
        case 0x0E89: push(ram,c,c->ax);c->ip=0x0E8Au;break;
        case 0x0E8A: al(c,(uint8_t)(0x0000u));c->ip=0x0E8Cu;break;
        case 0x0E8C: push(ram,c,c->ax);c->ip=0x0E8Du;break;
        case 0x0E8D: push(ram,c,c->cs);c->ip=0x0E8Eu;break;
        case 0x0E8E: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0xE8 || read_word(ram,c->cs,0x0E8Fu)!=0xF226u)return PR_R0709_CLEAR_CPU_CODE;push(ram,c,0x0E91u);c->ip=0x00B7u;d->callee=1;d->status=rectangle(ram,c,rect_loopbacks,d);if(d->status!=0)return PR_R0709_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0E91u)return PR_R0709_CLEAR_CPU_TRANSFER;break;
        case 0x0E91: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0E94u;break;
        case 0x0E94: push(ram,c,c->ax);c->ip=0x0E95u;break;
        case 0x0E95: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x0E98u;break;
        case 0x0E98: push(ram,c,c->ax);c->ip=0x0E99u;break;
        case 0x0E99: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x0E9Cu;break;
        case 0x0E9C: push(ram,c,c->ax);c->ip=0x0E9Du;break;
        case 0x0E9D: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6)))));c->ip=0x0EA0u;break;
        case 0x0EA0: push(ram,c,c->ax);c->ip=0x0EA1u;break;
        case 0x0EA1: al(c,(uint8_t)(0x0001u));c->ip=0x0EA3u;break;
        case 0x0EA3: push(ram,c,c->ax);c->ip=0x0EA4u;break;
        case 0x0EA4: al(c,(uint8_t)(0x0000u));c->ip=0x0EA6u;break;
        case 0x0EA6: push(ram,c,c->ax);c->ip=0x0EA7u;break;
        case 0x0EA7: push(ram,c,c->cs);c->ip=0x0EA8u;break;
        case 0x0EA8: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0xE8 || read_word(ram,c->cs,0x0EA9u)!=0xF20Cu)return PR_R0709_CLEAR_CPU_CODE;push(ram,c,0x0EABu);c->ip=0x00B7u;d->callee=2;d->status=rectangle(ram,c,rect_loopbacks,d);if(d->status!=0)return PR_R0709_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0EABu)return PR_R0709_CLEAR_CPU_TRANSFER;break;
        case 0x0EAB: c->ip=0x0F7Cu;break;
        case 0x0EAE: (void)arithmetic(c,(uint8_t)c->ax,0x0002u,0u,8u,1u);c->ip=0x0EB0u;break;
        case 0x0EB0: if((c->flags&0x40u)!=0){c->ip=0x0EB5u;}else c->ip=0x0EB2u;break;
        case 0x0EB2: c->ip=0x0F7Cu;break;
        case 0x0EB5: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x0EB8u;break;
        case 0x0EB8: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0EB9u;break;
        case 0x0EB9: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0EBBu;break;
        case 0x0EBB: put_word(ram,c->ss,(uint16_t)(c->bp+-2),c->ax);c->ip=0x0EBEu;break;
        case 0x0EBE: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6)))));c->ip=0x0EC1u;break;
        case 0x0EC1: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0EC2u;break;
        case 0x0EC2: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0EC4u;break;
        case 0x0EC4: c->ax=increment(c,c->ax,16u);c->ip=0x0EC5u;break;
        case 0x0EC5: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x0EC8u;break;
        case 0x0EC8: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0ECBu;break;
        case 0x0ECB: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0ECCu;break;
        case 0x0ECC: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0ECEu;break;
        case 0x0ECE: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0ED0u;break;
        case 0x0ED0: put_word(ram,c->ss,(uint16_t)(c->bp+-6),c->ax);c->ip=0x0ED3u;break;
        case 0x0ED3: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0ED6u;break;
        case 0x0ED6: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0ED7u;break;
        case 0x0ED7: c->dx=c->ax;c->ip=0x0ED9u;break;
        case 0x0ED9: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x0EDCu;break;
        case 0x0EDC: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0EDDu;break;
        case 0x0EDD: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,1u);c->ip=0x0EDFu;break;
        case 0x0EDF: c->ax=increment(c,c->ax,16u);c->ip=0x0EE0u;break;
        case 0x0EE0: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0EE2u;break;
        case 0x0EE2: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0EE4u;break;
        case 0x0EE4: put_word(ram,c->ss,(uint16_t)(c->bp+-8),c->ax);c->ip=0x0EE7u;break;
        case 0x0EE7: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x0EEAu;break;
        case 0x0EEA: put_word(ram,c->ss,(uint16_t)(c->bp+-12),c->ax);c->ip=0x0EEDu;break;
        case 0x0EED: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-2));c->ip=0x0EF0u;break;
        case 0x0EF0: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-12)),0u,16u,1u);c->ip=0x0EF3u;break;
        case 0x0EF3: if((c->flags&0x41u)!=0){c->ip=0x0EF8u;}else c->ip=0x0EF5u;break;
        case 0x0EF5: c->ip=0x0F7Cu;break;
        case 0x0EF8: put_word(ram,c->ss,(uint16_t)(c->bp+-10),c->ax);c->ip=0x0EFBu;break;
        case 0x0EFB: c->ip=0x0F00u;break;
        case 0x0EFD: put_word(ram,c->ss,(uint16_t)(c->bp+-10),increment(c,read_word(ram,c->ss,(uint16_t)(c->bp+-10)),16u));c->ip=0x0F00u;break;
        case 0x0F00: c->cx=read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ip=0x0F03u;break;
        case 0x0F03: c->ax=0x00A0u;c->ip=0x0F06u;break;
        case 0x0F06: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x0F09u;break;
        case 0x0F09: c->di=c->ax;c->ip=0x0F0Bu;break;
        case 0x0F0B: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0F0Du;break;
        case 0x0F0D: c->ax=0xB800u;c->ip=0x0F10u;break;
        case 0x0F10: push(ram,c,c->ax);c->ip=0x0F11u;break;
        case 0x0F11: push(ram,c,c->di);c->ip=0x0F12u;break;
        case 0x0F12: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x0F15u;break;
        case 0x0F15: al(c,(uint8_t)(0x0000u));c->ip=0x0F17u;break;
        case 0x0F17: push(ram,c,c->ax);c->ip=0x0F18u;break;
        case 0x0F18: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0F19u)!=0x16A8u)return PR_R0709_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0F1Du);c->cs=read_word(ram,c->cs,0x0F1Bu);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0F1Du)return PR_R0709_CLEAR_CPU_TRANSFER;break;
        case 0x0F1D: c->cx=read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ip=0x0F20u;break;
        case 0x0F20: c->ax=0x00A0u;c->ip=0x0F23u;break;
        case 0x0F23: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x0F26u;break;
        case 0x0F26: c->di=c->ax;c->ip=0x0F28u;break;
        case 0x0F28: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0F2Au;break;
        case 0x0F2A: c->ax=0xBA00u;c->ip=0x0F2Du;break;
        case 0x0F2D: push(ram,c,c->ax);c->ip=0x0F2Eu;break;
        case 0x0F2E: push(ram,c,c->di);c->ip=0x0F2Fu;break;
        case 0x0F2F: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x0F32u;break;
        case 0x0F32: al(c,(uint8_t)(0x0000u));c->ip=0x0F34u;break;
        case 0x0F34: push(ram,c,c->ax);c->ip=0x0F35u;break;
        case 0x0F35: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0F36u)!=0x16A8u)return PR_R0709_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0F3Au);c->cs=read_word(ram,c->cs,0x0F38u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0F3Au)return PR_R0709_CLEAR_CPU_TRANSFER;break;
        case 0x0F3A: c->cx=read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ip=0x0F3Du;break;
        case 0x0F3D: c->ax=0x00A0u;c->ip=0x0F40u;break;
        case 0x0F40: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x0F43u;break;
        case 0x0F43: c->di=c->ax;c->ip=0x0F45u;break;
        case 0x0F45: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0F47u;break;
        case 0x0F47: c->ax=0xBC00u;c->ip=0x0F4Au;break;
        case 0x0F4A: push(ram,c,c->ax);c->ip=0x0F4Bu;break;
        case 0x0F4B: push(ram,c,c->di);c->ip=0x0F4Cu;break;
        case 0x0F4C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x0F4Fu;break;
        case 0x0F4F: al(c,(uint8_t)(0x0000u));c->ip=0x0F51u;break;
        case 0x0F51: push(ram,c,c->ax);c->ip=0x0F52u;break;
        case 0x0F52: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0F53u)!=0x16A8u)return PR_R0709_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0F57u);c->cs=read_word(ram,c->cs,0x0F55u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0F57u)return PR_R0709_CLEAR_CPU_TRANSFER;break;
        case 0x0F57: c->cx=read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ip=0x0F5Au;break;
        case 0x0F5A: c->ax=0x00A0u;c->ip=0x0F5Du;break;
        case 0x0F5D: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x0F60u;break;
        case 0x0F60: c->di=c->ax;c->ip=0x0F62u;break;
        case 0x0F62: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0F64u;break;
        case 0x0F64: c->ax=0xBE00u;c->ip=0x0F67u;break;
        case 0x0F67: push(ram,c,c->ax);c->ip=0x0F68u;break;
        case 0x0F68: push(ram,c,c->di);c->ip=0x0F69u;break;
        case 0x0F69: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x0F6Cu;break;
        case 0x0F6C: al(c,(uint8_t)(0x0000u));c->ip=0x0F6Eu;break;
        case 0x0F6E: push(ram,c,c->ax);c->ip=0x0F6Fu;break;
        case 0x0F6F: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0F70u)!=0x16A8u)return PR_R0709_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0F74u);c->cs=read_word(ram,c->cs,0x0F72u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0F74u)return PR_R0709_CLEAR_CPU_TRANSFER;break;
        case 0x0F74: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ip=0x0F77u;break;
        case 0x0F77: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-12)),0u,16u,1u);c->ip=0x0F7Au;break;
        case 0x0F7A: if((c->flags&0x40u)==0){if(loops==max_loopbacks)return PR_R0709_CLEAR_CPU_LIMIT;++loops;c->ip=0x0EFDu;}else c->ip=0x0F7Cu;break;
        case 0x0F7C: c->sp=c->bp;c->ip=0x0F7Eu;break;
        case 0x0F7E: c->bp=pop(ram,c);c->ip=0x0F7Fu;break;
        case 0x0F7F: c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+8u);return PR_R0709_CLEAR_CPU_OK;
        default:return PR_R0709_CLEAR_CPU_TRANSFER;
        }
    }
}
