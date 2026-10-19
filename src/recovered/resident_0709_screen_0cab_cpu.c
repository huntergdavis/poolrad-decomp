/* 0709 +0CAB: raw mode-dependent full-screen clear. Primitives reuse +0626. */
#include "poolrad/recovered/resident_0709_screen_0cab_cpu.h"
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
static void push(pr_bus *ram, pr_r0709_screen_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r0709_screen_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r0709_screen_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r0709_screen_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r0709_screen_cpu *c, uint16_t left, uint16_t right,
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


static int fill(pr_bus *b,pr_r0709_screen_cpu *c) {
    pr_system_leaf_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_system_fillchar_bus_16a8(b,&inner);
    memcpy(c,&inner,sizeof inner);return status;
}
static int rectangle(pr_bus *b,pr_r0709_screen_cpu *c,size_t limit,
                     pr_r0709_screen_cpu_diagnostic *d) {
    pr_r0709_rect_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_r0709_rect_fill_00b7_cpu(b,PR_BUS_RAM_SIZE,&inner,limit,&d->rect);
    memcpy(c,&inner,sizeof inner);return status;
}
pr_r0709_screen_cpu_status pr_r0709_screen_0cab_cpu(pr_bus *ram,size_t size,
    pr_r0709_screen_cpu *c,size_t rect_loopbacks,
    pr_r0709_screen_cpu_diagnostic *d)
{
    pr_r0709_screen_cpu_diagnostic fallback={-1,-1,{-1,-1}};uint16_t caller;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0xCABu || c->sp<0x32u || c->sp>0xFFFCu ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds))
        return PR_R0709_SCREEN_CPU_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_R0709_SCREEN_CPU_TRANSFER;
        switch(c->ip) {
        case 0x0CAB: push(ram,c,c->bp);c->ip=0x0CACu;break;
        case 0x0CAC: c->bp=c->sp;c->ip=0x0CAEu;break;
        case 0x0CAE: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(18974)))));c->ip=0x0CB1u;break;
        case 0x0CB1: (void)arithmetic(c,(uint8_t)c->ax,0x0000u,0u,8u,1u);c->ip=0x0CB3u;break;
        case 0x0CB3: if((c->flags&0x40u)==0){c->ip=0x0CE0u;}else c->ip=0x0CB5u;break;
        case 0x0CB5: c->di=0x0000u;c->ip=0x0CB8u;break;
        case 0x0CB8: c->ax=0xBA00u;c->ip=0x0CBBu;break;
        case 0x0CBB: push(ram,c,c->ax);c->ip=0x0CBCu;break;
        case 0x0CBC: push(ram,c,c->di);c->ip=0x0CBDu;break;
        case 0x0CBD: c->ax=0x1F40u;c->ip=0x0CC0u;break;
        case 0x0CC0: push(ram,c,c->ax);c->ip=0x0CC1u;break;
        case 0x0CC1: al(c,(uint8_t)(0x0000u));c->ip=0x0CC3u;break;
        case 0x0CC3: push(ram,c,c->ax);c->ip=0x0CC4u;break;
        case 0x0CC4: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0CC5u)!=0x16A8u)return PR_R0709_SCREEN_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0CC9u);c->cs=read_word(ram,c->cs,0x0CC7u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_SCREEN_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0CC9u)return PR_R0709_SCREEN_CPU_TRANSFER;break;
        case 0x0CC9: c->di=0x0000u;c->ip=0x0CCCu;break;
        case 0x0CCC: c->ax=0xB800u;c->ip=0x0CCFu;break;
        case 0x0CCF: push(ram,c,c->ax);c->ip=0x0CD0u;break;
        case 0x0CD0: push(ram,c,c->di);c->ip=0x0CD1u;break;
        case 0x0CD1: c->ax=0x1F40u;c->ip=0x0CD4u;break;
        case 0x0CD4: push(ram,c,c->ax);c->ip=0x0CD5u;break;
        case 0x0CD5: al(c,(uint8_t)(0x0000u));c->ip=0x0CD7u;break;
        case 0x0CD7: push(ram,c,c->ax);c->ip=0x0CD8u;break;
        case 0x0CD8: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0CD9u)!=0x16A8u)return PR_R0709_SCREEN_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0CDDu);c->cs=read_word(ram,c->cs,0x0CDBu);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_SCREEN_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0CDDu)return PR_R0709_SCREEN_CPU_TRANSFER;break;
        case 0x0CDD: c->ip=0x0D66u;break;
        case 0x0CE0: (void)arithmetic(c,(uint8_t)c->ax,0x0001u,0u,8u,1u);c->ip=0x0CE2u;break;
        case 0x0CE2: if((c->flags&0x40u)==0){c->ip=0x0D12u;}else c->ip=0x0CE4u;break;
        case 0x0CE4: al(c,(uint8_t)(0x0000u));c->ip=0x0CE6u;break;
        case 0x0CE6: push(ram,c,c->ax);c->ip=0x0CE7u;break;
        case 0x0CE7: al(c,(uint8_t)(0x0000u));c->ip=0x0CE9u;break;
        case 0x0CE9: push(ram,c,c->ax);c->ip=0x0CEAu;break;
        case 0x0CEA: al(c,(uint8_t)(0x0027u));c->ip=0x0CECu;break;
        case 0x0CEC: push(ram,c,c->ax);c->ip=0x0CEDu;break;
        case 0x0CED: al(c,(uint8_t)(0x0018u));c->ip=0x0CEFu;break;
        case 0x0CEF: push(ram,c,c->ax);c->ip=0x0CF0u;break;
        case 0x0CF0: al(c,(uint8_t)(0x0000u));c->ip=0x0CF2u;break;
        case 0x0CF2: push(ram,c,c->ax);c->ip=0x0CF3u;break;
        case 0x0CF3: al(c,(uint8_t)(0x0000u));c->ip=0x0CF5u;break;
        case 0x0CF5: push(ram,c,c->ax);c->ip=0x0CF6u;break;
        case 0x0CF6: push(ram,c,c->cs);c->ip=0x0CF7u;break;
        case 0x0CF7: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0xE8 || read_word(ram,c->cs,0x0CF8u)!=0xF3BDu)return PR_R0709_SCREEN_CPU_CODE;push(ram,c,0x0CFAu);c->ip=0x00B7u;d->callee=1;d->status=rectangle(ram,c,rect_loopbacks,d);if(d->status!=0)return PR_R0709_SCREEN_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0CFAu)return PR_R0709_SCREEN_CPU_TRANSFER;break;
        case 0x0CFA: al(c,(uint8_t)(0x0000u));c->ip=0x0CFCu;break;
        case 0x0CFC: push(ram,c,c->ax);c->ip=0x0CFDu;break;
        case 0x0CFD: al(c,(uint8_t)(0x0000u));c->ip=0x0CFFu;break;
        case 0x0CFF: push(ram,c,c->ax);c->ip=0x0D00u;break;
        case 0x0D00: al(c,(uint8_t)(0x0027u));c->ip=0x0D02u;break;
        case 0x0D02: push(ram,c,c->ax);c->ip=0x0D03u;break;
        case 0x0D03: al(c,(uint8_t)(0x0018u));c->ip=0x0D05u;break;
        case 0x0D05: push(ram,c,c->ax);c->ip=0x0D06u;break;
        case 0x0D06: al(c,(uint8_t)(0x0001u));c->ip=0x0D08u;break;
        case 0x0D08: push(ram,c,c->ax);c->ip=0x0D09u;break;
        case 0x0D09: al(c,(uint8_t)(0x0000u));c->ip=0x0D0Bu;break;
        case 0x0D0B: push(ram,c,c->ax);c->ip=0x0D0Cu;break;
        case 0x0D0C: push(ram,c,c->cs);c->ip=0x0D0Du;break;
        case 0x0D0D: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0xE8 || read_word(ram,c->cs,0x0D0Eu)!=0xF3A7u)return PR_R0709_SCREEN_CPU_CODE;push(ram,c,0x0D10u);c->ip=0x00B7u;d->callee=2;d->status=rectangle(ram,c,rect_loopbacks,d);if(d->status!=0)return PR_R0709_SCREEN_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0D10u)return PR_R0709_SCREEN_CPU_TRANSFER;break;
        case 0x0D10: c->ip=0x0D66u;break;
        case 0x0D12: (void)arithmetic(c,(uint8_t)c->ax,0x0002u,0u,8u,1u);c->ip=0x0D14u;break;
        case 0x0D14: if((c->flags&0x40u)==0){c->ip=0x0D66u;}else c->ip=0x0D16u;break;
        case 0x0D16: c->di=0x0000u;c->ip=0x0D19u;break;
        case 0x0D19: c->ax=0xB800u;c->ip=0x0D1Cu;break;
        case 0x0D1C: push(ram,c,c->ax);c->ip=0x0D1Du;break;
        case 0x0D1D: push(ram,c,c->di);c->ip=0x0D1Eu;break;
        case 0x0D1E: c->ax=0x1F40u;c->ip=0x0D21u;break;
        case 0x0D21: push(ram,c,c->ax);c->ip=0x0D22u;break;
        case 0x0D22: al(c,(uint8_t)(0x0000u));c->ip=0x0D24u;break;
        case 0x0D24: push(ram,c,c->ax);c->ip=0x0D25u;break;
        case 0x0D25: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0D26u)!=0x16A8u)return PR_R0709_SCREEN_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0D2Au);c->cs=read_word(ram,c->cs,0x0D28u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_SCREEN_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0D2Au)return PR_R0709_SCREEN_CPU_TRANSFER;break;
        case 0x0D2A: c->di=0x0000u;c->ip=0x0D2Du;break;
        case 0x0D2D: c->ax=0xBA00u;c->ip=0x0D30u;break;
        case 0x0D30: push(ram,c,c->ax);c->ip=0x0D31u;break;
        case 0x0D31: push(ram,c,c->di);c->ip=0x0D32u;break;
        case 0x0D32: c->ax=0x1F40u;c->ip=0x0D35u;break;
        case 0x0D35: push(ram,c,c->ax);c->ip=0x0D36u;break;
        case 0x0D36: al(c,(uint8_t)(0x0000u));c->ip=0x0D38u;break;
        case 0x0D38: push(ram,c,c->ax);c->ip=0x0D39u;break;
        case 0x0D39: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0D3Au)!=0x16A8u)return PR_R0709_SCREEN_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0D3Eu);c->cs=read_word(ram,c->cs,0x0D3Cu);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_SCREEN_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0D3Eu)return PR_R0709_SCREEN_CPU_TRANSFER;break;
        case 0x0D3E: c->di=0x0000u;c->ip=0x0D41u;break;
        case 0x0D41: c->ax=0xBC00u;c->ip=0x0D44u;break;
        case 0x0D44: push(ram,c,c->ax);c->ip=0x0D45u;break;
        case 0x0D45: push(ram,c,c->di);c->ip=0x0D46u;break;
        case 0x0D46: c->ax=0x1F40u;c->ip=0x0D49u;break;
        case 0x0D49: push(ram,c,c->ax);c->ip=0x0D4Au;break;
        case 0x0D4A: al(c,(uint8_t)(0x0000u));c->ip=0x0D4Cu;break;
        case 0x0D4C: push(ram,c,c->ax);c->ip=0x0D4Du;break;
        case 0x0D4D: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0D4Eu)!=0x16A8u)return PR_R0709_SCREEN_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0D52u);c->cs=read_word(ram,c->cs,0x0D50u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_SCREEN_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0D52u)return PR_R0709_SCREEN_CPU_TRANSFER;break;
        case 0x0D52: c->di=0x0000u;c->ip=0x0D55u;break;
        case 0x0D55: c->ax=0xBE00u;c->ip=0x0D58u;break;
        case 0x0D58: push(ram,c,c->ax);c->ip=0x0D59u;break;
        case 0x0D59: push(ram,c,c->di);c->ip=0x0D5Au;break;
        case 0x0D5A: c->ax=0x1F40u;c->ip=0x0D5Du;break;
        case 0x0D5D: push(ram,c,c->ax);c->ip=0x0D5Eu;break;
        case 0x0D5E: al(c,(uint8_t)(0x0000u));c->ip=0x0D60u;break;
        case 0x0D60: push(ram,c,c->ax);c->ip=0x0D61u;break;
        case 0x0D61: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x0D62u)!=0x16A8u)return PR_R0709_SCREEN_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0D66u);c->cs=read_word(ram,c->cs,0x0D64u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R0709_SCREEN_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0D66u)return PR_R0709_SCREEN_CPU_TRANSFER;break;
        case 0x0D66: c->bp=pop(ram,c);c->ip=0x0D67u;break;
        case 0x0D67: c->ip=pop(ram,c);c->cs=pop(ram,c);return PR_R0709_SCREEN_CPU_OK;
        default:return PR_R0709_SCREEN_CPU_TRANSFER;
        }
    }
}
