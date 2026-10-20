/* Raw transient message caller; see docs/re/RES_00BA_DISPLAY_1693_CPU.md. */
#include "poolrad/recovered/resident_00ba_display_1693_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#include "poolrad/recovered/resident_0709_delay_wrapper_0dbe_cpu.h"
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
static void push(pr_bus *ram, pr_r00ba_1693_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r00ba_1693_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r00ba_1693_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r00ba_1693_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r00ba_1693_cpu *c, uint16_t left, uint16_t right,
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


static int invoke(pr_bus *b,pr_r00ba_1693_cpu *c,int which,
    size_t clear_limit,size_t rect_limit,size_t printer_limit,size_t glyph_limit,
    uint64_t delay_limit,pr_r00ba_1693_diagnostic *d)
{
    int status;
    if(which==0) {
        pr_system_0b25_cpu inner;
        _Static_assert(sizeof inner==sizeof *c,"CPU layout");
        memcpy(&inner,c,sizeof inner);status=pr_system_strstore_0b25(b->ram,PR_BUS_RAM_SIZE,&inner);
        memcpy(c,&inner,sizeof inner);
    } else if(which==1 || which==4) {
        pr_r03f1_clear_cpu inner;
        _Static_assert(sizeof inner==sizeof *c,"CPU layout");
        memcpy(&inner,c,sizeof inner);
        status=pr_r03f1_clear_0137_cpu(b,PR_BUS_RAM_SIZE,&inner,clear_limit,rect_limit,&d->clear);
        memcpy(c,&inner,sizeof inner);
    } else if(which==2) {
        pr_r0709_printer_cpu inner;
        _Static_assert(sizeof inner==sizeof *c,"CPU layout");
        memcpy(&inner,c,sizeof inner);
        status=pr_r0709_printer_0626_cpu(b,PR_BUS_RAM_SIZE,&inner,printer_limit,glyph_limit,&d->printer);
        memcpy(c,&inner,sizeof inner);
    } else {
        pr_r0709_delay_cpu inner;
        _Static_assert(sizeof inner==sizeof *c,"CPU layout");
        memcpy(&inner,c,sizeof inner);
        status=pr_r0709_delay_wrapper_0dbe_cpu(b->ram,PR_BUS_RAM_SIZE,&inner,delay_limit,&d->delay_status);
        memcpy(c,&inner,sizeof inner);
    }
    return status;
}
pr_r00ba_1693_status pr_r00ba_display_1693_cpu(pr_bus *ram,size_t size,
    pr_r00ba_1693_cpu *c,size_t clear_limit,size_t rect_limit,size_t printer_limit,
    size_t glyph_limit,uint64_t delay_limit,pr_r00ba_1693_diagnostic *d)
{
    pr_r00ba_1693_diagnostic fallback={-1,-1,-1,{-1,-1,{-1,-1}},{-1,-1,-1,-1}};
    uint16_t caller;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0x1693u || c->sp<0x16Au || c->sp>0xFFF8u ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds) || !ordinary_segment(c->cs) ||
       !ordinary_segment(read_word(ram,c->ss,(uint16_t)(c->sp+6u))))
        return PR_R00BA_1693_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_R00BA_1693_TRANSFER;
        switch(c->ip) {
        case 0x1693: push(ram,c,c->bp);c->ip=0x1694u;break;
        case 0x1694: c->bp=c->sp;c->ip=0x1696u;break;
        case 0x1696: c->sp=arithmetic(c,c->sp,0x2Au,0u,16u,1u);c->ip=0x1699u;break;
        case 0x1699: {size_t at=physical(c->ss,(uint16_t)(c->bp+6u));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);};c->ip=0x169Cu;break;
        case 0x169C: push(ram,c,c->es);c->ip=0x169Du;break;
        case 0x169D: push(ram,c,c->di);c->ip=0x169Eu;break;
        case 0x169E: c->di=(uint16_t)(c->bp-41u);c->ip=0x16A1u;break;
        case 0x16A1: push(ram,c,c->ss);c->ip=0x16A2u;break;
        case 0x16A2: push(ram,c,c->di);c->ip=0x16A3u;break;
        case 0x16A3: c->ax=40u;c->ip=0x16A6u;break;
        case 0x16A6: push(ram,c,c->ax);c->ip=0x16A7u;break;
        case 0x16A7: {
            uint16_t segment;
            if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9Au ||
               read_word(ram,c->cs,0x16A8u)!=0x0B25u)return PR_R00BA_1693_CODE;
            push(ram,c,c->cs);push(ram,c,0x16ACu);
            segment=read_word(ram,c->cs,0x16AAu);c->cs=segment;c->ip=0x0B25u;
            d->callee=0;d->status=invoke(ram,c,0,clear_limit,rect_limit,printer_limit,glyph_limit,delay_limit,d);
            if(d->status!=0)return PR_R00BA_1693_CALLEE;
            if(c->cs!=caller || c->ip!=0x16ACu)return PR_R00BA_1693_TRANSFER;
            break;
        }
        case 0x16AC: al(c,0u);c->ip=0x16AEu;break;
        case 0x16AE: push(ram,c,c->ax);c->ip=0x16AFu;break;
        case 0x16AF: al(c,24u);c->ip=0x16B1u;break;
        case 0x16B1: push(ram,c,c->ax);c->ip=0x16B2u;break;
        case 0x16B2: al(c,39u);c->ip=0x16B4u;break;
        case 0x16B4: push(ram,c,c->ax);c->ip=0x16B5u;break;
        case 0x16B5: al(c,24u);c->ip=0x16B7u;break;
        case 0x16B7: push(ram,c,c->ax);c->ip=0x16B8u;break;
        case 0x16B8: {
            uint16_t segment;
            if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9Au ||
               read_word(ram,c->cs,0x16B9u)!=0x0137u)return PR_R00BA_1693_CODE;
            push(ram,c,c->cs);push(ram,c,0x16BDu);
            segment=read_word(ram,c->cs,0x16BBu);c->cs=segment;c->ip=0x0137u;
            d->callee=1;d->status=invoke(ram,c,1,clear_limit,rect_limit,printer_limit,glyph_limit,delay_limit,d);
            if(d->status!=0)return PR_R00BA_1693_CALLEE;
            if(c->cs!=caller || c->ip!=0x16BDu)return PR_R00BA_1693_TRANSFER;
            break;
        }
        case 0x16BD: al(c,0u);c->ip=0x16BFu;break;
        case 0x16BF: push(ram,c,c->ax);c->ip=0x16C0u;break;
        case 0x16C0: al(c,24u);c->ip=0x16C2u;break;
        case 0x16C2: push(ram,c,c->ax);c->ip=0x16C3u;break;
        case 0x16C3: al(c,10u);c->ip=0x16C5u;break;
        case 0x16C5: push(ram,c,c->ax);c->ip=0x16C6u;break;
        case 0x16C6: c->di=(uint16_t)(c->bp-41u);c->ip=0x16C9u;break;
        case 0x16C9: push(ram,c,c->ss);c->ip=0x16CAu;break;
        case 0x16CA: push(ram,c,c->di);c->ip=0x16CBu;break;
        case 0x16CB: {
            uint16_t segment;
            if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9Au ||
               read_word(ram,c->cs,0x16CCu)!=0x0626u)return PR_R00BA_1693_CODE;
            push(ram,c,c->cs);push(ram,c,0x16D0u);
            segment=read_word(ram,c->cs,0x16CEu);c->cs=segment;c->ip=0x0626u;
            d->callee=2;d->status=invoke(ram,c,2,clear_limit,rect_limit,printer_limit,glyph_limit,delay_limit,d);
            if(d->status!=0)return PR_R00BA_1693_CALLEE;
            if(c->cs!=caller || c->ip!=0x16D0u)return PR_R00BA_1693_TRANSFER;
            break;
        }
        case 0x16D0: {
            uint16_t segment;
            if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9Au ||
               read_word(ram,c->cs,0x16D1u)!=0x0DBEu)return PR_R00BA_1693_CODE;
            push(ram,c,c->cs);push(ram,c,0x16D5u);
            segment=read_word(ram,c->cs,0x16D3u);c->cs=segment;c->ip=0x0DBEu;
            d->callee=3;d->status=invoke(ram,c,3,clear_limit,rect_limit,printer_limit,glyph_limit,delay_limit,d);
            if(d->status!=0)return PR_R00BA_1693_CALLEE;
            if(c->cs!=caller || c->ip!=0x16D5u)return PR_R00BA_1693_TRANSFER;
            break;
        }
        case 0x16D5: al(c,0u);c->ip=0x16D7u;break;
        case 0x16D7: push(ram,c,c->ax);c->ip=0x16D8u;break;
        case 0x16D8: al(c,24u);c->ip=0x16DAu;break;
        case 0x16DA: push(ram,c,c->ax);c->ip=0x16DBu;break;
        case 0x16DB: al(c,39u);c->ip=0x16DDu;break;
        case 0x16DD: push(ram,c,c->ax);c->ip=0x16DEu;break;
        case 0x16DE: al(c,24u);c->ip=0x16E0u;break;
        case 0x16E0: push(ram,c,c->ax);c->ip=0x16E1u;break;
        case 0x16E1: {
            uint16_t segment;
            if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9Au ||
               read_word(ram,c->cs,0x16E2u)!=0x0137u)return PR_R00BA_1693_CODE;
            push(ram,c,c->cs);push(ram,c,0x16E6u);
            segment=read_word(ram,c->cs,0x16E4u);c->cs=segment;c->ip=0x0137u;
            d->callee=4;d->status=invoke(ram,c,4,clear_limit,rect_limit,printer_limit,glyph_limit,delay_limit,d);
            if(d->status!=0)return PR_R00BA_1693_CALLEE;
            if(c->cs!=caller || c->ip!=0x16E6u)return PR_R00BA_1693_TRANSFER;
            break;
        }
        case 0x16E6: c->sp=c->bp;c->ip=0x16E8u;break;
        case 0x16E8: c->bp=pop(ram,c);c->ip=0x16E9u;break;
        case 0x16E9: c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+4u);return PR_R00BA_1693_OK;
        default:return PR_R00BA_1693_TRANSFER;
        }
    }
}
