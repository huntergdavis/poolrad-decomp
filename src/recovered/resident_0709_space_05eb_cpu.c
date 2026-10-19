/* 0709 +05EB: raw guarded space-glyph composition. */
#include "poolrad/recovered/resident_0709_space_05eb_cpu.h"
#include "poolrad/recovered/resident_0709_glyph_01df_cpu.h"
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
static void push(pr_bus *ram, pr_r0709_space_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r0709_space_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r0709_space_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r0709_space_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r0709_space_cpu *c, uint16_t left, uint16_t right,
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

static int glyph(pr_bus *b,pr_r0709_space_cpu *c,size_t limit,pr_r0709_space_cpu_diagnostic *d) {
    pr_r0709_glyph_cpu inner;pr_r0709_glyph_cpu_diagnostic sub;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_r0709_glyph_01df_cpu(b,PR_BUS_RAM_SIZE,&inner,limit,&sub);
    memcpy(c,&inner,sizeof inner);d->glyph_callee=sub.callee;d->glyph_status=sub.status;return status;
}
pr_r0709_space_cpu_status pr_r0709_space_05eb_cpu(pr_bus *ram,size_t size,
    pr_r0709_space_cpu *c,size_t max_loopbacks,pr_r0709_space_cpu_diagnostic *d)
{
    pr_r0709_space_cpu_diagnostic fallback={-1,-1,-1,-1};uint16_t caller;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0x5EBu || c->sp<0x2Cu || c->sp>0xFFF4u ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds))
        return PR_R0709_SPACE_CPU_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_R0709_SPACE_CPU_TRANSFER;
        switch(c->ip) {
        case 0x05EB: push(ram,c,c->bp);c->ip=0x05ECu;break;
        case 0x05EC: c->bp=c->sp;c->ip=0x05EEu;break;
        case 0x05EE: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12))),0x0000u,0u,8u,1u);c->ip=0x05F2u;break;
        case 0x05F2: if((c->flags&1u)!=0)c->ip=0x0606u;else c->ip=0x05F4u;break;
        case 0x05F4: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12))),0x0027u,0u,8u,1u);c->ip=0x05F8u;break;
        case 0x05F8: if((c->flags&0x41u)==0)c->ip=0x0606u;else c->ip=0x05FAu;break;
        case 0x05FA: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10))),0x0000u,0u,8u,1u);c->ip=0x05FEu;break;
        case 0x05FE: if((c->flags&1u)!=0)c->ip=0x0606u;else c->ip=0x0600u;break;
        case 0x0600: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10))),0x0018u,0u,8u,1u);c->ip=0x0604u;break;
        case 0x0604: if((c->flags&0x41u)!=0)c->ip=0x0608u;else c->ip=0x0606u;break;
        case 0x0606: c->ip=0x0622u;break;
        case 0x0608: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x060Bu;break;
        case 0x060B: push(ram,c,c->ax);c->ip=0x060Cu;break;
        case 0x060C: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x060Fu;break;
        case 0x060F: push(ram,c,c->ax);c->ip=0x0610u;break;
        case 0x0610: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x0613u;break;
        case 0x0613: push(ram,c,c->ax);c->ip=0x0614u;break;
        case 0x0614: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6)))));c->ip=0x0617u;break;
        case 0x0617: push(ram,c,c->ax);c->ip=0x0618u;break;
        case 0x0618: al(c,(uint8_t)(0x0020u));c->ip=0x061Au;break;
        case 0x061A: push(ram,c,c->ax);c->ip=0x061Bu;break;
        case 0x061B: al(c,(uint8_t)(0x0001u));c->ip=0x061Du;break;
        case 0x061D: push(ram,c,c->ax);c->ip=0x061Eu;break;
        case 0x061E: push(ram,c,c->cs);c->ip=0x061Fu;break;
        case 0x061F: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0xE8 || read_word(ram,c->cs,0x620u)!=0xFBBDu)return PR_R0709_SPACE_CPU_CODE;push(ram,c,0x0622u);c->ip=0x01DFu;d->callee=0;d->status=glyph(ram,c,max_loopbacks,d);if(d->status!=0)return PR_R0709_SPACE_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x622u)return PR_R0709_SPACE_CPU_TRANSFER;break;
        case 0x0622: c->bp=pop(ram,c);c->ip=0x0623u;break;
        case 0x0623: c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+8u);return PR_R0709_SPACE_CPU_OK;
        default:return PR_R0709_SPACE_CPU_TRANSFER;
        }
    }
}
