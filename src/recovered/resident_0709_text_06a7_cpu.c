/* 0709 +06A7: raw parent-frame text loop. Primitives reuse +01DF. */
#include "poolrad/recovered/resident_0709_text_06a7_cpu.h"
#include "poolrad/recovered/resident_0709_glyph_01df_cpu.h"
#include "poolrad/recovered/resident_a40_delay_029c_cpu.h"
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
static void push(pr_bus *ram, pr_r0709_text_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r0709_text_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r0709_text_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r0709_text_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r0709_text_cpu *c, uint16_t left, uint16_t right,
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


static uint16_t logic(pr_r0709_text_cpu *c,uint16_t value,unsigned bits) {
    c->flags&=(uint16_t)~0x08D5u;result_flags(c,value,bits==8u?0x80u:0x8000u);return value;
}
static uint16_t increment(pr_r0709_text_cpu *c,uint16_t value,unsigned bits) {
    uint16_t cf=c->flags&1u;value=arithmetic(c,value,1u,0u,bits,0u);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return value;
}
static int ordinary_segment(uint16_t s) {
    size_t start=(size_t)s<<4u;
    return start>0xAFFFFu || start+0xFFFFu<0xA0000u;
}

static int delay(pr_bus *b,pr_r0709_text_cpu *c,uint64_t limit) {
    pr_a40_delay_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_a40_delay_029c_cpu(b->ram,PR_BUS_RAM_SIZE,&inner,limit);
    memcpy(c,&inner,sizeof inner);return status;
}
static int glyph(pr_bus *b,pr_r0709_text_cpu *c,size_t limit,pr_r0709_text_cpu_diagnostic *d) {
    pr_r0709_glyph_cpu inner;pr_r0709_glyph_cpu_diagnostic sub;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_r0709_glyph_01df_cpu(b,PR_BUS_RAM_SIZE,&inner,limit,&sub);
    memcpy(c,&inner,sizeof inner);d->glyph_callee=sub.callee;d->glyph_status=sub.status;return status;
}
pr_r0709_text_cpu_status pr_r0709_text_06a7_cpu(pr_bus *ram,size_t size,
    pr_r0709_text_cpu *c,size_t max_loopbacks,size_t glyph_loopbacks,uint64_t delay_loopbacks,
    pr_r0709_text_cpu_diagnostic *d)
{
    pr_r0709_text_cpu_diagnostic fallback={-1,-1,-1,-1};uint16_t caller;size_t loops=0;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0x6A7u || c->sp<0x2Cu || c->sp>0xFFFAu ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds))
        return PR_R0709_TEXT_CPU_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_R0709_TEXT_CPU_TRANSFER;
        switch(c->ip) {
        case 0x06A7: push(ram,c,c->bp);c->ip=0x06A8u;break;
        case 0x06A8: c->bp=c->sp;c->ip=0x06AAu;break;
        case 0x06AA: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));c->ip=0x06ADu;break;
        case 0x06AD: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->di+-259)))));c->ip=0x06B2u;break;
        case 0x06B2: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));c->ip=0x06B5u;break;
        case 0x06B5: (void)arithmetic(c,(uint8_t)c->ax,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->di+-260))),0u,8u,1u);c->ip=0x06BAu;break;
        case 0x06BA: if((c->flags&0x41u)==0){c->ip=0x0710u;}else c->ip=0x06BCu;break;
        case 0x06BC: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(24100)))));c->ip=0x06BFu;break;
        case 0x06BF: push(ram,c,c->ax);c->ip=0x06C0u;break;
        case 0x06C0: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(24101)))));c->ip=0x06C3u;break;
        case 0x06C3: push(ram,c,c->ax);c->ip=0x06C4u;break;
        case 0x06C4: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));c->ip=0x06C7u;break;
        case 0x06C7: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->di+12)))));c->ip=0x06CBu;break;
        case 0x06CB: push(ram,c,c->ax);c->ip=0x06CCu;break;
        case 0x06CC: al(c,(uint8_t)(0x0001u));c->ip=0x06CEu;break;
        case 0x06CE: push(ram,c,c->ax);c->ip=0x06CFu;break;
        case 0x06CF: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));c->ip=0x06D2u;break;
        case 0x06D2: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->di+-259)))));c->ip=0x06D7u;break;
        case 0x06D7: c->ax=(uint16_t)((c->ax&255u)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x06D9u;break;
        case 0x06D9: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));c->ip=0x06DCu;break;
        case 0x06DC: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x06DEu;break;
        case 0x06DE: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->di+-256)))));c->ip=0x06E3u;break;
        case 0x06E3: push(ram,c,c->ax);c->ip=0x06E4u;break;
        case 0x06E4: al(c,(uint8_t)(0x0001u));c->ip=0x06E6u;break;
        case 0x06E6: push(ram,c,c->ax);c->ip=0x06E7u;break;
        case 0x06E7: push(ram,c,c->cs);c->ip=0x06E8u;break;
        case 0x06E8: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0xE8 || read_word(ram,c->cs,0x6E9u)!=0xFAF4u)return PR_R0709_TEXT_CPU_CODE;push(ram,c,0x06EBu);c->ip=0x01DFu;d->callee=0;d->status=glyph(ram,c,glyph_loopbacks,d);if(d->status!=0)return PR_R0709_TEXT_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x06EBu)return PR_R0709_TEXT_CPU_TRANSFER;break;
        case 0x06EB: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ds,(uint16_t)(18944))),0x0000u,0u,8u,1u);c->ip=0x06F0u;break;
        case 0x06F0: if((c->flags&0x40u)!=0){c->ip=0x0702u;}else c->ip=0x06F2u;break;
        case 0x06F2: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(18914)))));c->ip=0x06F5u;break;
        case 0x06F5: c->ax=(uint16_t)((c->ax&255u)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x06F7u;break;
        case 0x06F7: c->dx=0x000Au;c->ip=0x06FAu;break;
        case 0x06FA: {uint32_t v=(uint32_t)c->ax*c->dx;c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);{uint16_t w=(uint16_t)(c->ax^c->dx);w^=(uint16_t)(w>>8u);w^=(uint16_t)(w>>4u);w^=(uint16_t)(w>>2u);w^=(uint16_t)(w>>1u);/* pinned MULW: SF/AF kept, ZF from AX alone, PF = even parity of DX:AX, CF=OF=(DX!=0) */c->flags=(uint16_t)((c->flags&~0x0845u)|(c->dx?0x0801u:0u)|(c->ax==0u?0x0040u:0u)|((w&1u)?0u:0x0004u));}}c->ip=0x06FCu;break;
        case 0x06FC: push(ram,c,c->ax);c->ip=0x06FDu;break;
        case 0x06FD: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x6FEu)!=0x029Cu)return PR_R0709_TEXT_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0702u);c->cs=read_word(ram,c->cs,0x700u);c->ip=0x029Cu;d->callee=1;d->status=delay(ram,c,delay_loopbacks);if(d->status!=0)return PR_R0709_TEXT_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0702u)return PR_R0709_TEXT_CPU_TRANSFER;break;
        case 0x0702: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));c->ip=0x0705u;break;
        case 0x0705: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->di+-259)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->di+-259))),8u)));c->ip=0x070Au;break;
        case 0x070A: pr_bus_write8(ram,physical(c->ds,(uint16_t)(24100)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ds,(uint16_t)(24100))),8u)));c->ip=0x070Eu;break;
        case 0x070E: if(1){if(loops==max_loopbacks)return PR_R0709_TEXT_CPU_LIMIT;++loops;c->ip=0x06AAu;}else c->ip=0x0710u;break;
        case 0x0710: c->bp=pop(ram,c);c->ip=0x0711u;break;
        case 0x0711: c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+2u);return PR_R0709_TEXT_CPU_OK;
        default:return PR_R0709_TEXT_CPU_TRANSFER;
        }
    }
}
