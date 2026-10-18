/* 0709 +0626: raw string printer composition. Primitives reuse +01DF. */
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
#include "poolrad/recovered/resident_0709_glyph_01df_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
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
static void push(pr_bus *ram, pr_r0709_printer_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r0709_printer_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r0709_printer_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r0709_printer_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r0709_printer_cpu *c, uint16_t left, uint16_t right,
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


static uint16_t logic(pr_r0709_printer_cpu *c,uint16_t value,unsigned bits) {
    c->flags&=(uint16_t)~0x08D5u;result_flags(c,value,bits==8u?0x80u:0x8000u);return value;
}
static uint16_t increment(pr_r0709_printer_cpu *c,uint16_t value,unsigned bits) {
    uint16_t cf=c->flags&1u;value=arithmetic(c,value,1u,0u,bits,0u);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return value;
}
static int ordinary_segment(uint16_t s) {
    size_t start=(size_t)s<<4u;
    return start>0xAFFFFu || start+0xFFFFu<0xA0000u;
}

static int strstore(pr_bus *b,pr_r0709_printer_cpu *c) {
    pr_system_0b25_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_system_strstore_0b25(b->ram,PR_BUS_RAM_SIZE,&inner);
    memcpy(c,&inner,sizeof inner);return status;
}
static int glyph(pr_bus *b,pr_r0709_printer_cpu *c,size_t limit,pr_r0709_printer_cpu_diagnostic *d) {
    pr_r0709_glyph_cpu inner;pr_r0709_glyph_cpu_diagnostic sub;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_r0709_glyph_01df_cpu(b,PR_BUS_RAM_SIZE,&inner,limit,&sub);
    memcpy(c,&inner,sizeof inner);d->glyph_callee=sub.callee;d->glyph_status=sub.status;return status;
}
pr_r0709_printer_cpu_status pr_r0709_printer_0626_cpu(pr_bus *ram,size_t size,
    pr_r0709_printer_cpu *c,size_t max_loopbacks,size_t glyph_loopbacks,
    pr_r0709_printer_cpu_diagnostic *d)
{
    pr_r0709_printer_cpu_diagnostic fallback={-1,-1,-1,-1};uint16_t caller;size_t loops=0;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0x626u || c->sp<0x130u || c->sp>0xFFF2u ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds) ||
       !ordinary_segment(read_word(ram,c->ss,(uint16_t)(c->sp+6u))))
        return PR_R0709_PRINTER_CPU_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_R0709_PRINTER_CPU_TRANSFER;
        switch(c->ip) {
        case 0x0626: push(ram,c,c->bp);c->ip=0x0627u;break;
        case 0x0627: c->bp=c->sp;c->ip=0x0629u;break;
        case 0x0629: c->sp=arithmetic(c,c->sp,0x0104u,0u,16u,1u);c->ip=0x062Du;break;
        case 0x062D: {size_t at=physical(c->ss,(uint16_t)(c->bp+6));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x0630u;break;
        case 0x0630: push(ram,c,c->es);c->ip=0x0631u;break;
        case 0x0631: push(ram,c,c->di);c->ip=0x0632u;break;
        case 0x0632: c->di=(uint16_t)(c->bp+-256);c->ip=0x0636u;break;
        case 0x0636: push(ram,c,c->ss);c->ip=0x0637u;break;
        case 0x0637: push(ram,c,c->di);c->ip=0x0638u;break;
        case 0x0638: c->ax=0x00FFu;c->ip=0x063Bu;break;
        case 0x063B: push(ram,c,c->ax);c->ip=0x063Cu;break;
        case 0x063C: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x63Du)!=0x0B25u)return PR_R0709_PRINTER_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0641u);c->cs=read_word(ram,c->cs,0x63Fu);c->ip=0x0B25u;d->callee=0;d->status=strstore(ram,c);if(d->status!=0)return PR_R0709_PRINTER_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0641u)return PR_R0709_PRINTER_CPU_TRANSFER;break;
        case 0x0641: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+14))),0x0027u,0u,8u,1u);c->ip=0x0645u;break;
        case 0x0645: if((c->flags&0x41u)==0){c->ip=0x064Du;}else c->ip=0x0647u;break;
        case 0x0647: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12))),0x0027u,0u,8u,1u);c->ip=0x064Bu;break;
        case 0x064B: if((c->flags&0x41u)!=0){c->ip=0x064Fu;}else c->ip=0x064Du;break;
        case 0x064D: if(1){c->ip=0x06A1u;}else c->ip=0x064Fu;break;
        case 0x064F: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-256)))));c->ip=0x0653u;break;
        case 0x0653: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-257)),(uint8_t)((uint8_t)c->ax));c->ip=0x0657u;break;
        case 0x0657: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-257)))));c->ip=0x065Bu;break;
        case 0x065B: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-259)),(uint8_t)((uint8_t)c->ax));c->ip=0x065Fu;break;
        case 0x065F: al(c,(uint8_t)(0x0001u));c->ip=0x0661u;break;
        case 0x0661: (void)arithmetic(c,(uint8_t)c->ax,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-259))),0u,8u,1u);c->ip=0x0665u;break;
        case 0x0665: if((c->flags&0x41u)==0){c->ip=0x06A1u;}else c->ip=0x0667u;break;
        case 0x0667: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-258)),(uint8_t)((uint8_t)c->ax));c->ip=0x066Bu;break;
        case 0x066B: if(1){c->ip=0x0671u;}else c->ip=0x066Du;break;
        case 0x066D: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-258)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-258))),8u)));c->ip=0x0671u;break;
        case 0x0671: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+14)))));c->ip=0x0674u;break;
        case 0x0674: push(ram,c,c->ax);c->ip=0x0675u;break;
        case 0x0675: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0678u;break;
        case 0x0678: push(ram,c,c->ax);c->ip=0x0679u;break;
        case 0x0679: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x067Cu;break;
        case 0x067C: push(ram,c,c->ax);c->ip=0x067Du;break;
        case 0x067D: al(c,(uint8_t)(0x0001u));c->ip=0x067Fu;break;
        case 0x067F: push(ram,c,c->ax);c->ip=0x0680u;break;
        case 0x0680: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-258)))));c->ip=0x0684u;break;
        case 0x0684: c->ax=(uint16_t)((c->ax&255u)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0686u;break;
        case 0x0686: c->di=c->ax;c->ip=0x0688u;break;
        case 0x0688: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+c->di+-256)))));c->ip=0x068Cu;break;
        case 0x068C: push(ram,c,c->ax);c->ip=0x068Du;break;
        case 0x068D: al(c,(uint8_t)(0x0001u));c->ip=0x068Fu;break;
        case 0x068F: push(ram,c,c->ax);c->ip=0x0690u;break;
        case 0x0690: push(ram,c,c->cs);c->ip=0x0691u;break;
        case 0x0691: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0xE8 || read_word(ram,c->cs,0x692u)!=0xFB4Bu)return PR_R0709_PRINTER_CPU_CODE;push(ram,c,0x0694u);c->ip=0x01DFu;d->callee=1;d->status=glyph(ram,c,glyph_loopbacks,d);if(d->status!=0)return PR_R0709_PRINTER_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0694u)return PR_R0709_PRINTER_CPU_TRANSFER;break;
        case 0x0694: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+14)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+14))),8u)));c->ip=0x0697u;break;
        case 0x0697: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-258)))));c->ip=0x069Bu;break;
        case 0x069B: (void)arithmetic(c,(uint8_t)c->ax,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-259))),0u,8u,1u);c->ip=0x069Fu;break;
        case 0x069F: if((c->flags&0x40u)==0){if(loops==max_loopbacks)return PR_R0709_PRINTER_CPU_LIMIT;++loops;c->ip=0x066Du;}else c->ip=0x06A1u;break;
        case 0x06A1: c->sp=c->bp;c->ip=0x06A3u;break;
        case 0x06A3: c->bp=pop(ram,c);c->ip=0x06A4u;break;
        case 0x06A4: c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+10u);return PR_R0709_PRINTER_CPU_OK;
        default:return PR_R0709_PRINTER_CPU_TRANSFER;
        }
    }
}
