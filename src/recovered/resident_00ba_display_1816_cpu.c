/* 00BA +1816: raw node display composition. Primitives reuse +0626. */
#include "poolrad/recovered/resident_00ba_display_1816_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
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
static void push(pr_bus *ram, pr_r00ba_display_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r00ba_display_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r00ba_display_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r00ba_display_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r00ba_display_cpu *c, uint16_t left, uint16_t right,
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


static uint16_t logic(pr_r00ba_display_cpu *c,uint16_t value,unsigned bits) {
    c->flags&=(uint16_t)~0x08D5u;result_flags(c,value,bits==8u?0x80u:0x8000u);return value;
}
static int ordinary_segment(uint16_t s) {
    size_t start=(size_t)s<<4u;
    return start>0xAFFFFu || start+0xFFFFu<0xA0000u;
}


static int strload(pr_bus *b,pr_r00ba_display_cpu *c) {
    pr_system_leaf_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_system_strload_0b0b(b->ram,PR_BUS_RAM_SIZE,&inner);
    memcpy(c,&inner,sizeof inner);return status;
}
static int printer(pr_bus *b,pr_r00ba_display_cpu *c,size_t limit,size_t glyph_limit,
                   pr_r00ba_display_cpu_diagnostic *d) {
    pr_r0709_printer_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);
    status=pr_r0709_printer_0626_cpu(b,PR_BUS_RAM_SIZE,&inner,limit,glyph_limit,&d->printer);
    memcpy(c,&inner,sizeof inner);return status;
}
pr_r00ba_display_cpu_status pr_r00ba_display_1816_cpu(pr_bus *ram,size_t size,
    pr_r00ba_display_cpu *c,size_t printer_loopbacks,size_t glyph_loopbacks,
    pr_r00ba_display_cpu_diagnostic *d)
{
    pr_r00ba_display_cpu_diagnostic fallback={-1,-1,{-1,-1,-1,-1}};uint16_t caller;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0x1816u || c->sp<0x144u || c->sp>0xFFF2u ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds) || !ordinary_segment(c->cs) ||
       !ordinary_segment(read_word(ram,c->ss,(uint16_t)(c->sp+12u))) ||
       pr_bus_read8(ram,physical(c->cs,0x1813u))!=2u)
        return PR_R00BA_DISPLAY_CPU_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_R00BA_DISPLAY_CPU_TRANSFER;
        switch(c->ip) {
        case 0x1816: push(ram,c,c->bp);c->ip=0x1817u;break;
        case 0x1817: c->bp=c->sp;c->ip=0x1819u;break;
        case 0x1819: c->sp=arithmetic(c,c->sp,0x0004u,0u,16u,1u);c->ip=0x181Cu;break;
        case 0x181C: {size_t at=physical(c->ss,(uint16_t)(c->bp+12));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x181Fu;break;
        case 0x181F: (void)arithmetic(c,pr_bus_read8(ram,physical(c->es,(uint16_t)(c->di+269))),0x0000u,0u,8u,1u);c->ip=0x1825u;break;
        case 0x1825: if((c->flags&0x40u)==0)c->ip=0x182Du;else c->ip=0x1827u;break;
        case 0x1827: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-1)),(uint8_t)(0x000Cu));c->ip=0x182Bu;break;
        case 0x182B: c->ip=0x1842u;break;
        case 0x182D: {size_t at=physical(c->ss,(uint16_t)(c->bp+12));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x1830u;break;
        case 0x1830: (void)arithmetic(c,pr_bus_read8(ram,physical(c->es,(uint16_t)(c->di+270))),0x0001u,0u,8u,1u);c->ip=0x1836u;break;
        case 0x1836: if((c->flags&0x40u)==0)c->ip=0x183Eu;else c->ip=0x1838u;break;
        case 0x1838: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-1)),(uint8_t)(0x000Eu));c->ip=0x183Cu;break;
        case 0x183C: c->ip=0x1842u;break;
        case 0x183E: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-1)),(uint8_t)(0x000Bu));c->ip=0x1842u;break;
        case 0x1842: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x1845u;break;
        case 0x1845: push(ram,c,c->ax);c->ip=0x1846u;break;
        case 0x1846: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x1849u;break;
        case 0x1849: push(ram,c,c->ax);c->ip=0x184Au;break;
        case 0x184A: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-1)))));c->ip=0x184Du;break;
        case 0x184D: push(ram,c,c->ax);c->ip=0x184Eu;break;
        case 0x184E: {size_t at=physical(c->ss,(uint16_t)(c->bp+12));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x1851u;break;
        case 0x1851: push(ram,c,c->es);c->ip=0x1852u;break;
        case 0x1852: push(ram,c,c->di);c->ip=0x1853u;break;
        case 0x1853: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x1854u)!=0x0626u)return PR_R00BA_DISPLAY_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x1858u);c->cs=read_word(ram,c->cs,0x1856u);c->ip=0x0626u;d->callee=0;d->status=printer(ram,c,printer_loopbacks,glyph_loopbacks,d);if(d->status!=0)return PR_R00BA_DISPLAY_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x1858u)return PR_R00BA_DISPLAY_CPU_TRANSFER;break;
        case 0x1858: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6))),0x0000u,0u,8u,1u);c->ip=0x185Cu;break;
        case 0x185C: if((c->flags&0x40u)!=0)c->ip=0x188Cu;else c->ip=0x185Eu;break;
        case 0x185E: {size_t at=physical(c->ss,(uint16_t)(c->bp+12));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x1861u;break;
        case 0x1861: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->es,(uint16_t)(c->di+0)))));c->ip=0x1864u;break;
        case 0x1864: c->ax=(uint16_t)((c->ax&255u)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x1866u;break;
        case 0x1866: c->dx=c->ax;c->ip=0x1868u;break;
        case 0x1868: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x186Bu;break;
        case 0x186B: c->ax=(uint16_t)((c->ax&255u)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x186Du;break;
        case 0x186D: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,0u);c->ip=0x186Fu;break;
        case 0x186F: push(ram,c,c->ax);c->ip=0x1870u;break;
        case 0x1870: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x1873u;break;
        case 0x1873: push(ram,c,c->ax);c->ip=0x1874u;break;
        case 0x1874: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-1)))));c->ip=0x1877u;break;
        case 0x1877: push(ram,c,c->ax);c->ip=0x1878u;break;
        case 0x1878: c->di=(uint16_t)(c->bp+-4);c->ip=0x187Bu;break;
        case 0x187B: push(ram,c,c->ss);c->ip=0x187Cu;break;
        case 0x187C: push(ram,c,c->di);c->ip=0x187Du;break;
        case 0x187D: c->di=0x1813u;c->ip=0x1880u;break;
        case 0x1880: push(ram,c,c->cs);c->ip=0x1881u;break;
        case 0x1881: push(ram,c,c->di);c->ip=0x1882u;break;
        case 0x1882: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x1883u)!=0x0B0Bu)return PR_R00BA_DISPLAY_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x1887u);c->cs=read_word(ram,c->cs,0x1885u);c->ip=0x0B0Bu;d->callee=1;d->status=strload(ram,c);if(d->status!=0)return PR_R00BA_DISPLAY_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x1887u)return PR_R00BA_DISPLAY_CPU_TRANSFER;break;
        case 0x1887: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x1888u)!=0x0626u)return PR_R00BA_DISPLAY_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x188Cu);c->cs=read_word(ram,c->cs,0x188Au);c->ip=0x0626u;d->callee=2;d->status=printer(ram,c,printer_loopbacks,glyph_loopbacks,d);if(d->status!=0)return PR_R00BA_DISPLAY_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x188Cu)return PR_R00BA_DISPLAY_CPU_TRANSFER;break;
        case 0x188C: c->sp=c->bp;c->ip=0x188Eu;break;
        case 0x188E: c->bp=pop(ram,c);c->ip=0x188Fu;break;
        case 0x188F: c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+10u);return PR_R00BA_DISPLAY_CPU_OK;
        default:return PR_R00BA_DISPLAY_CPU_TRANSFER;
        }
    }
}
