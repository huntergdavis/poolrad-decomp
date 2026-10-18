/* 0709 +01DF raw glyph renderer. Original operand order through pr_bus.
 * Memory/flag primitives reuse the accepted +00B7 caller.
 * MUL, shifts and IDIV result flags are dead before any returned/stopped
 * state, OUT, conditional transfer, or carry-dependent operation. */
#include "poolrad/recovered/resident_0709_glyph_01df_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
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
static void push(pr_bus *ram, pr_r0709_glyph_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r0709_glyph_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r0709_glyph_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r0709_glyph_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r0709_glyph_cpu *c, uint16_t left, uint16_t right,
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


static uint16_t logic(pr_r0709_glyph_cpu *c,uint16_t value,unsigned bits) {
    c->flags&=(uint16_t)~0x08D5u;result_flags(c,value,bits==8u?0x80u:0x8000u);return value;
}
static uint16_t increment(pr_r0709_glyph_cpu *c,uint16_t value,unsigned bits) {
    uint16_t cf=c->flags&1u;value=arithmetic(c,value,1u,0u,bits,0u);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return value;
}
static int ordinary_segment(uint16_t s) {
    size_t start=(size_t)s<<4u;
    return start>0xAFFFFu || start+0xFFFFu<0xA0000u;
}
static int upcase(pr_bus *b,pr_r0709_glyph_cpu *c) {
    pr_system_leaf_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_system_upcase_182a(b->ram,PR_BUS_RAM_SIZE,&inner);
    memcpy(c,&inner,sizeof inner);return status;
}
pr_r0709_glyph_cpu_status pr_r0709_glyph_01df_cpu(pr_bus *ram,size_t size,
    pr_r0709_glyph_cpu *c,size_t max_loopbacks,pr_r0709_glyph_cpu_diagnostic *d)
{
    pr_r0709_glyph_cpu_diagnostic fallback={-1,-1};uint16_t caller;size_t loops=0;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0x1DFu || c->sp<0x1Au || c->sp>0xFFF0u ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds))
        return PR_R0709_GLYPH_CPU_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_R0709_GLYPH_CPU_TRANSFER;
        switch(c->ip) {
        case 0x01DF: push(ram,c,c->bp);c->ip=0x01E0u;break;
        case 0x01E0: c->bp=c->sp;c->ip=0x01E2u;break;
        case 0x01E2: c->sp=arithmetic(c,c->sp,0x0012u,0u,16u,1u);c->ip=0x01E5u;break;
        case 0x01E5: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+16))),0x0027u,0u,8u,1u);c->ip=0x01E9u;break;
        case 0x01E9: if((c->flags&0x41u)==0){c->ip=0x01FAu;}else c->ip=0x01EBu;break;
        case 0x01EB: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+14))),0x0018u,0u,8u,1u);c->ip=0x01EFu;break;
        case 0x01EF: if((c->flags&0x41u)==0){c->ip=0x01FAu;}else c->ip=0x01F1u;break;
        case 0x01F1: c->ax=read_word(ram,c->ds,(uint16_t)(24096));c->ip=0x01F4u;break;
        case 0x01F4: c->ax=logic(c,(uint16_t)(c->ax|read_word(ram,c->ds,(uint16_t)(24098))),16u);c->ip=0x01F8u;break;
        case 0x01F8: if((c->flags&0x40u)==0){c->ip=0x01FDu;}else c->ip=0x01FAu;break;
        case 0x01FA: if(1){c->ip=0x05E5u;}else c->ip=0x01FDu;break;
        case 0x01FD: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6))),0x0000u,0u,8u,1u);c->ip=0x0201u;break;
        case 0x0201: if((c->flags&0x40u)!=0){c->ip=0x021Au;}else c->ip=0x0203u;break;
        case 0x0203: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x0206u;break;
        case 0x0206: push(ram,c,c->ax);c->ip=0x0207u;break;
        case 0x0207: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x208u)!=0x182Au)return PR_R0709_GLYPH_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x020Cu);c->cs=read_word(ram,c->cs,0x20Au);c->ip=0x182Au;d->callee=0;d->status=upcase(ram,c);if(d->status!=0)return PR_R0709_GLYPH_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x020Cu)return PR_R0709_GLYPH_CPU_TRANSFER;break;
        case 0x020C: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x020Eu;break;
        case 0x020E: c->dx=(c->ax&0x8000u)?0xFFFFu:0;c->ip=0x020Fu;break;
        case 0x020F: c->cx=0x0040u;c->ip=0x0212u;break;
        case 0x0212: /* XOR AH,AH; CWD; MOV CX,64 guarantee dividend 0..255. */ c->dx=(uint16_t)(c->ax%64u);c->ax=(uint16_t)(c->ax/64u);c->ip=0x0214u;break;
        case 0x0214: {uint16_t value=c->dx;c->dx=c->ax;c->ax=value;}c->ip=0x0215u;break;
        case 0x0215: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-2)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x0218u;break;
        case 0x0218: if(1){c->ip=0x0220u;}else c->ip=0x021Au;break;
        case 0x021A: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x021Du;break;
        case 0x021D: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-2)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x0220u;break;
        case 0x0220: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x0223u;break;
        case 0x0223: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-17)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x0226u;break;
        case 0x0226: al(c,(uint8_t)(0x0001u));c->ip=0x0228u;break;
        case 0x0228: (void)arithmetic(c,(uint8_t)(c->ax>>0u),pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-17))),0u,8u,1u);c->ip=0x022Bu;break;
        case 0x022B: if((c->flags&0x41u)!=0){c->ip=0x0230u;}else c->ip=0x022Du;break;
        case 0x022D: if(1){c->ip=0x05E5u;}else c->ip=0x0230u;break;
        case 0x0230: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-1)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x0233u;break;
        case 0x0233: if(1){c->ip=0x0238u;}else c->ip=0x0235u;break;
        case 0x0235: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-1)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-1))),8u)));c->ip=0x0238u;break;
        case 0x0238: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(18974)))));c->ip=0x023Bu;break;
        case 0x023B: (void)arithmetic(c,(uint8_t)(c->ax>>0u),0x0000u,0u,8u,1u);c->ip=0x023Du;break;
        case 0x023D: if((c->flags&0x40u)!=0){c->ip=0x0242u;}else c->ip=0x023Fu;break;
        case 0x023F: if(1){c->ip=0x0387u;}else c->ip=0x0242u;break;
        case 0x0242: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0245u;break;
        case 0x0245: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0247u;break;
        case 0x0247: c->di=c->ax;c->ip=0x0249u;break;
        case 0x0249: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(c->di+9685)))));c->ip=0x024Du;break;
        case 0x024D: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+12)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x0250u;break;
        case 0x0250: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+14)))));c->ip=0x0253u;break;
        case 0x0253: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0255u;break;
        case 0x0255: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0257u;break;
        case 0x0257: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0259u;break;
        case 0x0259: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x025Cu;break;
        case 0x025C: c->ax=logic(c,(uint16_t)(c->ax^c->ax),16u);c->ip=0x025Eu;break;
        case 0x025E: put_word(ram,c->ss,(uint16_t)(c->bp+-6),c->ax);c->ip=0x0261u;break;
        case 0x0261: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-15)),(uint8_t)(0x0000u));c->ip=0x0265u;break;
        case 0x0265: if(1){c->ip=0x026Au;}else c->ip=0x0267u;break;
        case 0x0267: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-15)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-15))),8u)));c->ip=0x026Au;break;
        case 0x026A: c->ax=logic(c,(uint16_t)(c->ax^c->ax),16u);c->ip=0x026Cu;break;
        case 0x026C: put_word(ram,c->ss,(uint16_t)(c->bp+-10),c->ax);c->ip=0x026Fu;break;
        case 0x026F: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-13)),(uint8_t)(0x0080u));c->ip=0x0273u;break;
        case 0x0273: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-12)),(uint8_t)(0x0000u));c->ip=0x0277u;break;
        case 0x0277: if(1){c->ip=0x027Cu;}else c->ip=0x0279u;break;
        case 0x0279: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-12)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12))),8u)));c->ip=0x027Cu;break;
        case 0x027C: c->dx=read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ip=0x027Fu;break;
        case 0x027F: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-2)))));c->ip=0x0282u;break;
        case 0x0282: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0284u;break;
        case 0x0284: c->cx=(uint16_t)((c->cx&0xFF00u)|((uint16_t)(uint8_t)(0x0003u)<<0u));c->ip=0x0286u;break;
        case 0x0286: c->ax=(uint16_t)(c->ax<<(uint8_t)(c->cx>>0u));c->ip=0x0288u;break;
        case 0x0288: {size_t at=physical(c->ds,(uint16_t)(24096));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x028Cu;break;
        case 0x028C: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x028Eu;break;
        case 0x028E: c->di=arithmetic(c,c->di,c->dx,0u,16u,0u);c->ip=0x0290u;break;
        case 0x0290: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->es,(uint16_t)(c->di+0)))));c->ip=0x0293u;break;
        case 0x0293: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)&pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-13)))),8u)));c->ip=0x0296u;break;
        case 0x0296: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)|(uint8_t)(c->ax>>0u)),8u)));c->ip=0x0298u;break;
        case 0x0298: if((c->flags&0x41u)!=0){c->ip=0x02BCu;}else c->ip=0x029Au;break;
        case 0x029A: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x029Du;break;
        case 0x029D: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x029Fu;break;
        case 0x029F: c->di=c->ax;c->ip=0x02A1u;break;
        case 0x02A1: c->di=(uint16_t)(c->di<<0x0001u);c->ip=0x02A3u;break;
        case 0x02A3: c->dx=read_word(ram,c->ds,(uint16_t)(c->di+9677));c->ip=0x02A7u;break;
        case 0x02A7: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12)))));c->ip=0x02AAu;break;
        case 0x02AA: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x02ACu;break;
        case 0x02AC: c->di=c->ax;c->ip=0x02AEu;break;
        case 0x02AE: c->di=(uint16_t)(c->di<<0x0001u);c->ip=0x02B0u;break;
        case 0x02B0: c->ax=read_word(ram,c->ds,(uint16_t)(c->di+9701));c->ip=0x02B4u;break;
        case 0x02B4: c->ax=logic(c,(uint16_t)(c->ax&c->dx),16u);c->ip=0x02B6u;break;
        case 0x02B6: c->ax=logic(c,(uint16_t)(c->ax|read_word(ram,c->ss,(uint16_t)(c->bp+-10))),16u);c->ip=0x02B9u;break;
        case 0x02B9: put_word(ram,c->ss,(uint16_t)(c->bp+-10),c->ax);c->ip=0x02BCu;break;
        case 0x02BC: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-13)))));c->ip=0x02BFu;break;
        case 0x02BF: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x02C1u;break;
        case 0x02C1: c->ax=(uint16_t)(c->ax>>0x0001u);c->ip=0x02C3u;break;
        case 0x02C3: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-13)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x02C6u;break;
        case 0x02C6: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12))),0x0007u,0u,8u,1u);c->ip=0x02CAu;break;
        case 0x02CA: if((c->flags&0x40u)==0){if(loops==max_loopbacks)return PR_R0709_GLYPH_CPU_LIMIT;++loops;c->ip=0x0279u;}else c->ip=0x02CCu;break;
        case 0x02CC: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ip=0x02CFu;break;
        case 0x02CF: {uint16_t value=(uint8_t)(c->ax>>8u);c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)((uint8_t)(c->ax>>0u))<<8u));al(c,(uint8_t)(value));}c->ip=0x02D1u;break;
        case 0x02D1: c->bx=c->ax;c->ip=0x02D3u;break;
        case 0x02D3: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+16)))));c->ip=0x02D6u;break;
        case 0x02D6: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x02D8u;break;
        case 0x02D8: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x02DAu;break;
        case 0x02DA: c->cx=c->ax;c->ip=0x02DCu;break;
        case 0x02DC: c->ax=0x0050u;c->ip=0x02DFu;break;
        case 0x02DF: {uint32_t value=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ax=(uint16_t)value;c->dx=(uint16_t)(value>>16u);}c->ip=0x02E2u;break;
        case 0x02E2: c->di=c->ax;c->ip=0x02E4u;break;
        case 0x02E4: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x02E6u;break;
        case 0x02E6: c->si=0xB800u;c->ip=0x02E9u;break;
        case 0x02E9: c->es=c->si;c->ip=0x02EBu;break;
        case 0x02EB: put_word(ram,c->es,(uint16_t)(c->di+0),c->bx);c->ip=0x02EEu;break;
        case 0x02EE: put_word(ram,c->ss,(uint16_t)(c->bp+-6),increment(c,read_word(ram,c->ss,(uint16_t)(c->bp+-6)),16u));c->ip=0x02F1u;break;
        case 0x02F1: c->ax=logic(c,(uint16_t)(c->ax^c->ax),16u);c->ip=0x02F3u;break;
        case 0x02F3: put_word(ram,c->ss,(uint16_t)(c->bp+-10),c->ax);c->ip=0x02F6u;break;
        case 0x02F6: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-13)),(uint8_t)(0x0080u));c->ip=0x02FAu;break;
        case 0x02FA: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-12)),(uint8_t)(0x0000u));c->ip=0x02FEu;break;
        case 0x02FE: if(1){c->ip=0x0303u;}else c->ip=0x0300u;break;
        case 0x0300: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-12)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12))),8u)));c->ip=0x0303u;break;
        case 0x0303: c->dx=read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ip=0x0306u;break;
        case 0x0306: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-2)))));c->ip=0x0309u;break;
        case 0x0309: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x030Bu;break;
        case 0x030B: c->cx=(uint16_t)((c->cx&0xFF00u)|((uint16_t)(uint8_t)(0x0003u)<<0u));c->ip=0x030Du;break;
        case 0x030D: c->ax=(uint16_t)(c->ax<<(uint8_t)(c->cx>>0u));c->ip=0x030Fu;break;
        case 0x030F: {size_t at=physical(c->ds,(uint16_t)(24096));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x0313u;break;
        case 0x0313: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x0315u;break;
        case 0x0315: c->di=arithmetic(c,c->di,c->dx,0u,16u,0u);c->ip=0x0317u;break;
        case 0x0317: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->es,(uint16_t)(c->di+0)))));c->ip=0x031Au;break;
        case 0x031A: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)&pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-13)))),8u)));c->ip=0x031Du;break;
        case 0x031D: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)|(uint8_t)(c->ax>>0u)),8u)));c->ip=0x031Fu;break;
        case 0x031F: if((c->flags&0x41u)!=0){c->ip=0x0343u;}else c->ip=0x0321u;break;
        case 0x0321: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0324u;break;
        case 0x0324: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0326u;break;
        case 0x0326: c->di=c->ax;c->ip=0x0328u;break;
        case 0x0328: c->di=(uint16_t)(c->di<<0x0001u);c->ip=0x032Au;break;
        case 0x032A: c->dx=read_word(ram,c->ds,(uint16_t)(c->di+9677));c->ip=0x032Eu;break;
        case 0x032E: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12)))));c->ip=0x0331u;break;
        case 0x0331: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0333u;break;
        case 0x0333: c->di=c->ax;c->ip=0x0335u;break;
        case 0x0335: c->di=(uint16_t)(c->di<<0x0001u);c->ip=0x0337u;break;
        case 0x0337: c->ax=read_word(ram,c->ds,(uint16_t)(c->di+9701));c->ip=0x033Bu;break;
        case 0x033B: c->ax=logic(c,(uint16_t)(c->ax&c->dx),16u);c->ip=0x033Du;break;
        case 0x033D: c->ax=logic(c,(uint16_t)(c->ax|read_word(ram,c->ss,(uint16_t)(c->bp+-10))),16u);c->ip=0x0340u;break;
        case 0x0340: put_word(ram,c->ss,(uint16_t)(c->bp+-10),c->ax);c->ip=0x0343u;break;
        case 0x0343: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-13)))));c->ip=0x0346u;break;
        case 0x0346: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0348u;break;
        case 0x0348: c->ax=(uint16_t)(c->ax>>0x0001u);c->ip=0x034Au;break;
        case 0x034A: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-13)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x034Du;break;
        case 0x034D: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12))),0x0007u,0u,8u,1u);c->ip=0x0351u;break;
        case 0x0351: if((c->flags&0x40u)==0){if(loops==max_loopbacks)return PR_R0709_GLYPH_CPU_LIMIT;++loops;c->ip=0x0300u;}else c->ip=0x0353u;break;
        case 0x0353: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ip=0x0356u;break;
        case 0x0356: {uint16_t value=(uint8_t)(c->ax>>8u);c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)((uint8_t)(c->ax>>0u))<<8u));al(c,(uint8_t)(value));}c->ip=0x0358u;break;
        case 0x0358: c->bx=c->ax;c->ip=0x035Au;break;
        case 0x035A: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+16)))));c->ip=0x035Du;break;
        case 0x035D: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x035Fu;break;
        case 0x035F: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0361u;break;
        case 0x0361: c->cx=c->ax;c->ip=0x0363u;break;
        case 0x0363: c->ax=0x0050u;c->ip=0x0366u;break;
        case 0x0366: {uint32_t value=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ax=(uint16_t)value;c->dx=(uint16_t)(value>>16u);}c->ip=0x0369u;break;
        case 0x0369: c->di=c->ax;c->ip=0x036Bu;break;
        case 0x036B: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x036Du;break;
        case 0x036D: c->si=0xBA00u;c->ip=0x0370u;break;
        case 0x0370: c->es=c->si;c->ip=0x0372u;break;
        case 0x0372: put_word(ram,c->es,(uint16_t)(c->di+0),c->bx);c->ip=0x0375u;break;
        case 0x0375: put_word(ram,c->ss,(uint16_t)(c->bp+-6),increment(c,read_word(ram,c->ss,(uint16_t)(c->bp+-6)),16u));c->ip=0x0378u;break;
        case 0x0378: put_word(ram,c->ss,(uint16_t)(c->bp+-4),increment(c,read_word(ram,c->ss,(uint16_t)(c->bp+-4)),16u));c->ip=0x037Bu;break;
        case 0x037B: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-15))),0x0003u,0u,8u,1u);c->ip=0x037Fu;break;
        case 0x037F: if((c->flags&0x40u)!=0){c->ip=0x0384u;}else c->ip=0x0381u;break;
        case 0x0381: if(1){if(loops==max_loopbacks)return PR_R0709_GLYPH_CPU_LIMIT;++loops;c->ip=0x0267u;}else c->ip=0x0384u;break;
        case 0x0384: if(1){c->ip=0x05D7u;}else c->ip=0x0387u;break;
        case 0x0387: (void)arithmetic(c,(uint8_t)(c->ax>>0u),0x0001u,0u,8u,1u);c->ip=0x0389u;break;
        case 0x0389: if((c->flags&0x40u)!=0){c->ip=0x038Eu;}else c->ip=0x038Bu;break;
        case 0x038B: if(1){c->ip=0x0474u;}else c->ip=0x038Eu;break;
        case 0x038E: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+16)))));c->ip=0x0391u;break;
        case 0x0391: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0393u;break;
        case 0x0393: c->bx=c->ax;c->ip=0x0395u;break;
        case 0x0395: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+14)))));c->ip=0x0398u;break;
        case 0x0398: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x039Au;break;
        case 0x039A: c->cx=(uint16_t)((c->cx&0xFF00u)|((uint16_t)(uint8_t)(0x0003u)<<0u));c->ip=0x039Cu;break;
        case 0x039C: c->ax=(uint16_t)(c->ax<<(uint8_t)(c->cx>>0u));c->ip=0x039Eu;break;
        case 0x039E: c->dx=0x0028u;c->ip=0x03A1u;break;
        case 0x03A1: {uint32_t value=(uint32_t)c->ax*c->dx;c->ax=(uint16_t)value;c->dx=(uint16_t)(value>>16u);}c->ip=0x03A3u;break;
        case 0x03A3: c->ax=arithmetic(c,c->ax,c->bx,0u,16u,0u);c->ip=0x03A5u;break;
        case 0x03A5: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x03A8u;break;
        case 0x03A8: al(c,(uint8_t)(0x0002u));c->ip=0x03AAu;break;
        case 0x03AA: c->dx=0x03C4u;c->ip=0x03ADu;break;
        case 0x03AD: if(pr_bus_out(ram,c->dx,(uint8_t)c->ax,1u,c->cs,c->ip))return PR_R0709_GLYPH_CPU_PORT;c->ip=0x03AEu;break;
        case 0x03AE: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-15)),(uint8_t)(0x0000u));c->ip=0x03B2u;break;
        case 0x03B2: if(1){c->ip=0x03B7u;}else c->ip=0x03B4u;break;
        case 0x03B4: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-15)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-15))),8u)));c->ip=0x03B7u;break;
        case 0x03B7: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-12)),(uint8_t)(0x0000u));c->ip=0x03BBu;break;
        case 0x03BB: if(1){c->ip=0x03C0u;}else c->ip=0x03BDu;break;
        case 0x03BD: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-12)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12))),8u)));c->ip=0x03C0u;break;
        case 0x03C0: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12)))));c->ip=0x03C3u;break;
        case 0x03C3: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x03C5u;break;
        case 0x03C5: c->dx=c->ax;c->ip=0x03C7u;break;
        case 0x03C7: c->ax=0x0003u;c->ip=0x03CAu;break;
        case 0x03CA: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,1u);c->ip=0x03CCu;break;
        case 0x03CC: c->di=c->ax;c->ip=0x03CEu;break;
        case 0x03CE: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(c->di+9673)))));c->ip=0x03D2u;break;
        case 0x03D2: c->dx=0x03C5u;c->ip=0x03D5u;break;
        case 0x03D5: if(pr_bus_out(ram,c->dx,(uint8_t)c->ax,1u,c->cs,c->ip))return PR_R0709_GLYPH_CPU_PORT;c->ip=0x03D6u;break;
        case 0x03D6: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12)))));c->ip=0x03D9u;break;
        case 0x03D9: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x03DBu;break;
        case 0x03DB: c->dx=c->ax;c->ip=0x03DDu;break;
        case 0x03DD: c->ax=0x0003u;c->ip=0x03E0u;break;
        case 0x03E0: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,1u);c->ip=0x03E2u;break;
        case 0x03E2: c->di=c->ax;c->ip=0x03E4u;break;
        case 0x03E4: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(c->di+9673)))));c->ip=0x03E8u;break;
        case 0x03E8: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)&pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))),8u)));c->ip=0x03EBu;break;
        case 0x03EB: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)|(uint8_t)(c->ax>>0u)),8u)));c->ip=0x03EDu;break;
        case 0x03ED: if((c->flags&0x41u)!=0){c->ip=0x043Du;}else c->ip=0x03EFu;break;
        case 0x03EF: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-15)))));c->ip=0x03F2u;break;
        case 0x03F2: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x03F4u;break;
        case 0x03F4: c->dx=c->ax;c->ip=0x03F6u;break;
        case 0x03F6: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-2)))));c->ip=0x03F9u;break;
        case 0x03F9: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x03FBu;break;
        case 0x03FB: c->cx=(uint16_t)((c->cx&0xFF00u)|((uint16_t)(uint8_t)(0x0003u)<<0u));c->ip=0x03FDu;break;
        case 0x03FD: c->ax=(uint16_t)(c->ax<<(uint8_t)(c->cx>>0u));c->ip=0x03FFu;break;
        case 0x03FF: {size_t at=physical(c->ds,(uint16_t)(24096));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x0403u;break;
        case 0x0403: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x0405u;break;
        case 0x0405: c->di=arithmetic(c,c->di,c->dx,0u,16u,0u);c->ip=0x0407u;break;
        case 0x0407: c->dx=(uint16_t)((c->dx&0xFF00u)|((uint16_t)(uint8_t)(pr_bus_read8(ram,physical(c->es,(uint16_t)(c->di+0))))<<0u));c->ip=0x040Au;break;
        case 0x040A: c->ax=read_word(ram,c->ds,(uint16_t)(18966));c->ip=0x040Du;break;
        case 0x040D: push(ram,c,c->ax);c->ip=0x040Eu;break;
        case 0x040E: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x0411u;break;
        case 0x0411: c->es=pop(ram,c);c->ip=0x0412u;break;
        case 0x0412: pr_bus_write8(ram,physical(c->es,(uint16_t)(c->di+0)),(uint8_t)((uint8_t)(c->dx>>0u)));c->ip=0x0415u;break;
        case 0x0415: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-15)))));c->ip=0x0418u;break;
        case 0x0418: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x041Au;break;
        case 0x041A: c->dx=c->ax;c->ip=0x041Cu;break;
        case 0x041C: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-2)))));c->ip=0x041Fu;break;
        case 0x041F: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0421u;break;
        case 0x0421: c->cx=(uint16_t)((c->cx&0xFF00u)|((uint16_t)(uint8_t)(0x0003u)<<0u));c->ip=0x0423u;break;
        case 0x0423: c->ax=(uint16_t)(c->ax<<(uint8_t)(c->cx>>0u));c->ip=0x0425u;break;
        case 0x0425: {size_t at=physical(c->ds,(uint16_t)(24096));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x0429u;break;
        case 0x0429: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x042Bu;break;
        case 0x042B: c->di=arithmetic(c,c->di,c->dx,0u,16u,0u);c->ip=0x042Du;break;
        case 0x042D: c->dx=(uint16_t)((c->dx&0xFF00u)|((uint16_t)(uint8_t)(pr_bus_read8(ram,physical(c->es,(uint16_t)(c->di+0))))<<0u));c->ip=0x0430u;break;
        case 0x0430: c->ax=read_word(ram,c->ds,(uint16_t)(18968));c->ip=0x0433u;break;
        case 0x0433: push(ram,c,c->ax);c->ip=0x0434u;break;
        case 0x0434: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x0437u;break;
        case 0x0437: c->es=pop(ram,c);c->ip=0x0438u;break;
        case 0x0438: pr_bus_write8(ram,physical(c->es,(uint16_t)(c->di+0)),(uint8_t)((uint8_t)(c->dx>>0u)));c->ip=0x043Bu;break;
        case 0x043B: if(1){c->ip=0x0455u;}else c->ip=0x043Du;break;
        case 0x043D: c->ax=read_word(ram,c->ds,(uint16_t)(18966));c->ip=0x0440u;break;
        case 0x0440: push(ram,c,c->ax);c->ip=0x0441u;break;
        case 0x0441: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x0444u;break;
        case 0x0444: c->es=pop(ram,c);c->ip=0x0445u;break;
        case 0x0445: pr_bus_write8(ram,physical(c->es,(uint16_t)(c->di+0)),(uint8_t)(0x0000u));c->ip=0x0449u;break;
        case 0x0449: c->ax=read_word(ram,c->ds,(uint16_t)(18968));c->ip=0x044Cu;break;
        case 0x044C: push(ram,c,c->ax);c->ip=0x044Du;break;
        case 0x044D: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x0450u;break;
        case 0x0450: c->es=pop(ram,c);c->ip=0x0451u;break;
        case 0x0451: pr_bus_write8(ram,physical(c->es,(uint16_t)(c->di+0)),(uint8_t)(0x0000u));c->ip=0x0455u;break;
        case 0x0455: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12))),0x0003u,0u,8u,1u);c->ip=0x0459u;break;
        case 0x0459: if((c->flags&0x40u)!=0){c->ip=0x045Eu;}else c->ip=0x045Bu;break;
        case 0x045B: if(1){if(loops==max_loopbacks)return PR_R0709_GLYPH_CPU_LIMIT;++loops;c->ip=0x03BDu;}else c->ip=0x045Eu;break;
        case 0x045E: put_word(ram,c->ss,(uint16_t)(c->bp+-4),arithmetic(c,read_word(ram,c->ss,(uint16_t)(c->bp+-4)),0x0028u,0u,16u,0u));c->ip=0x0462u;break;
        case 0x0462: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-15))),0x0007u,0u,8u,1u);c->ip=0x0466u;break;
        case 0x0466: if((c->flags&0x40u)!=0){c->ip=0x046Bu;}else c->ip=0x0468u;break;
        case 0x0468: if(1){if(loops==max_loopbacks)return PR_R0709_GLYPH_CPU_LIMIT;++loops;c->ip=0x03B4u;}else c->ip=0x046Bu;break;
        case 0x046B: al(c,(uint8_t)(0x000Fu));c->ip=0x046Du;break;
        case 0x046D: c->dx=0x03C5u;c->ip=0x0470u;break;
        case 0x0470: if(pr_bus_out(ram,c->dx,(uint8_t)c->ax,1u,c->cs,c->ip))return PR_R0709_GLYPH_CPU_PORT;c->ip=0x0471u;break;
        case 0x0471: if(1){c->ip=0x05D7u;}else c->ip=0x0474u;break;
        case 0x0474: (void)arithmetic(c,(uint8_t)(c->ax>>0u),0x0002u,0u,8u,1u);c->ip=0x0476u;break;
        case 0x0476: if((c->flags&0x40u)!=0){c->ip=0x047Bu;}else c->ip=0x0478u;break;
        case 0x0478: if(1){c->ip=0x05D7u;}else c->ip=0x047Bu;break;
        case 0x047B: c->ax=logic(c,(uint16_t)(c->ax^c->ax),16u);c->ip=0x047Du;break;
        case 0x047D: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x0480u;break;
        case 0x0480: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+14)))));c->ip=0x0483u;break;
        case 0x0483: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0485u;break;
        case 0x0485: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0487u;break;
        case 0x0487: put_word(ram,c->ss,(uint16_t)(c->bp+-6),c->ax);c->ip=0x048Au;break;
        case 0x048A: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+16)))));c->ip=0x048Du;break;
        case 0x048D: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x048Fu;break;
        case 0x048F: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0491u;break;
        case 0x0491: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0493u;break;
        case 0x0493: put_word(ram,c->ss,(uint16_t)(c->bp+-8),c->ax);c->ip=0x0496u;break;
        case 0x0496: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-15)),(uint8_t)(0x0000u));c->ip=0x049Au;break;
        case 0x049A: if(1){c->ip=0x049Fu;}else c->ip=0x049Cu;break;
        case 0x049C: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-15)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-15))),8u)));c->ip=0x049Fu;break;
        case 0x049F: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-14)),(uint8_t)(0x0000u));c->ip=0x04A3u;break;
        case 0x04A3: if(1){c->ip=0x04A8u;}else c->ip=0x04A5u;break;
        case 0x04A5: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-14)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-14))),8u)));c->ip=0x04A8u;break;
        case 0x04A8: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-13)),(uint8_t)(0x0080u));c->ip=0x04ACu;break;
        case 0x04AC: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-12)),(uint8_t)(0x0000u));c->ip=0x04B0u;break;
        case 0x04B0: if(1){c->ip=0x04B5u;}else c->ip=0x04B2u;break;
        case 0x04B2: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-12)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12))),8u)));c->ip=0x04B5u;break;
        case 0x04B5: c->dx=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x04B8u;break;
        case 0x04B8: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-2)))));c->ip=0x04BBu;break;
        case 0x04BB: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x04BDu;break;
        case 0x04BD: c->cx=(uint16_t)((c->cx&0xFF00u)|((uint16_t)(uint8_t)(0x0003u)<<0u));c->ip=0x04BFu;break;
        case 0x04BF: c->ax=(uint16_t)(c->ax<<(uint8_t)(c->cx>>0u));c->ip=0x04C1u;break;
        case 0x04C1: {size_t at=physical(c->ds,(uint16_t)(24096));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x04C5u;break;
        case 0x04C5: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x04C7u;break;
        case 0x04C7: c->di=arithmetic(c,c->di,c->dx,0u,16u,0u);c->ip=0x04C9u;break;
        case 0x04C9: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->es,(uint16_t)(c->di+0)))));c->ip=0x04CCu;break;
        case 0x04CC: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)&pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-13)))),8u)));c->ip=0x04CFu;break;
        case 0x04CF: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)|(uint8_t)(c->ax>>0u)),8u)));c->ip=0x04D1u;break;
        case 0x04D1: if((c->flags&0x41u)!=0){c->ip=0x04E2u;}else c->ip=0x04D3u;break;
        case 0x04D3: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x04D6u;break;
        case 0x04D6: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x04D8u;break;
        case 0x04D8: c->cx=0x0004u;c->ip=0x04DBu;break;
        case 0x04DB: c->ax=(uint16_t)(c->ax<<(uint8_t)(c->cx>>0u));c->ip=0x04DDu;break;
        case 0x04DD: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-11)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x04E0u;break;
        case 0x04E0: if(1){c->ip=0x04E6u;}else c->ip=0x04E2u;break;
        case 0x04E2: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-11)),(uint8_t)(0x0000u));c->ip=0x04E6u;break;
        case 0x04E6: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-13)))));c->ip=0x04E9u;break;
        case 0x04E9: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x04EBu;break;
        case 0x04EB: c->ax=(uint16_t)(c->ax>>0x0001u);c->ip=0x04EDu;break;
        case 0x04ED: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-13)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x04F0u;break;
        case 0x04F0: c->dx=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x04F3u;break;
        case 0x04F3: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-2)))));c->ip=0x04F6u;break;
        case 0x04F6: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x04F8u;break;
        case 0x04F8: c->cx=(uint16_t)((c->cx&0xFF00u)|((uint16_t)(uint8_t)(0x0003u)<<0u));c->ip=0x04FAu;break;
        case 0x04FA: c->ax=(uint16_t)(c->ax<<(uint8_t)(c->cx>>0u));c->ip=0x04FCu;break;
        case 0x04FC: {size_t at=physical(c->ds,(uint16_t)(24096));uint16_t off=word_at(ram,at),segment=word_at(ram,(at+2u)&MASK);c->di=off;c->es=segment;}c->ip=0x0500u;break;
        case 0x0500: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x0502u;break;
        case 0x0502: c->di=arithmetic(c,c->di,c->dx,0u,16u,0u);c->ip=0x0504u;break;
        case 0x0504: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->es,(uint16_t)(c->di+0)))));c->ip=0x0507u;break;
        case 0x0507: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)&pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-13)))),8u)));c->ip=0x050Au;break;
        case 0x050A: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)|(uint8_t)(c->ax>>0u)),8u)));c->ip=0x050Cu;break;
        case 0x050C: if((c->flags&0x41u)!=0){c->ip=0x0517u;}else c->ip=0x050Eu;break;
        case 0x050E: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-11)))));c->ip=0x0511u;break;
        case 0x0511: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>0u)|pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))),8u)));c->ip=0x0514u;break;
        case 0x0514: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-11)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x0517u;break;
        case 0x0517: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-13)))));c->ip=0x051Au;break;
        case 0x051A: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x051Cu;break;
        case 0x051C: c->ax=(uint16_t)(c->ax>>0x0001u);c->ip=0x051Eu;break;
        case 0x051E: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-13)),(uint8_t)((uint8_t)(c->ax>>0u)));c->ip=0x0521u;break;
        case 0x0521: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-14)))));c->ip=0x0524u;break;
        case 0x0524: (void)arithmetic(c,(uint8_t)(c->ax>>0u),0x0000u,0u,8u,1u);c->ip=0x0526u;break;
        case 0x0526: if((c->flags&0x40u)==0){c->ip=0x0549u;}else c->ip=0x0528u;break;
        case 0x0528: c->bx=(uint16_t)((c->bx&0xFF00u)|((uint16_t)(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-11))))<<0u));c->ip=0x052Bu;break;
        case 0x052B: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12)))));c->ip=0x052Eu;break;
        case 0x052E: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0530u;break;
        case 0x0530: c->ax=arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-8)),0u,16u,0u);c->ip=0x0533u;break;
        case 0x0533: c->cx=c->ax;c->ip=0x0535u;break;
        case 0x0535: c->ax=0x00A0u;c->ip=0x0538u;break;
        case 0x0538: {uint32_t value=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ax=(uint16_t)value;c->dx=(uint16_t)(value>>16u);}c->ip=0x053Bu;break;
        case 0x053B: c->di=c->ax;c->ip=0x053Du;break;
        case 0x053D: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x053Fu;break;
        case 0x053F: c->si=0xB800u;c->ip=0x0542u;break;
        case 0x0542: c->es=c->si;c->ip=0x0544u;break;
        case 0x0544: pr_bus_write8(ram,physical(c->es,(uint16_t)(c->di+0)),(uint8_t)((uint8_t)(c->bx>>0u)));c->ip=0x0547u;break;
        case 0x0547: if(1){c->ip=0x05B6u;}else c->ip=0x0549u;break;
        case 0x0549: (void)arithmetic(c,(uint8_t)(c->ax>>0u),0x0001u,0u,8u,1u);c->ip=0x054Bu;break;
        case 0x054B: if((c->flags&0x40u)==0){c->ip=0x056Eu;}else c->ip=0x054Du;break;
        case 0x054D: c->bx=(uint16_t)((c->bx&0xFF00u)|((uint16_t)(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-11))))<<0u));c->ip=0x0550u;break;
        case 0x0550: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12)))));c->ip=0x0553u;break;
        case 0x0553: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x0555u;break;
        case 0x0555: c->ax=arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-8)),0u,16u,0u);c->ip=0x0558u;break;
        case 0x0558: c->cx=c->ax;c->ip=0x055Au;break;
        case 0x055A: c->ax=0x00A0u;c->ip=0x055Du;break;
        case 0x055D: {uint32_t value=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ax=(uint16_t)value;c->dx=(uint16_t)(value>>16u);}c->ip=0x0560u;break;
        case 0x0560: c->di=c->ax;c->ip=0x0562u;break;
        case 0x0562: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0564u;break;
        case 0x0564: c->si=0xBA00u;c->ip=0x0567u;break;
        case 0x0567: c->es=c->si;c->ip=0x0569u;break;
        case 0x0569: pr_bus_write8(ram,physical(c->es,(uint16_t)(c->di+0)),(uint8_t)((uint8_t)(c->bx>>0u)));c->ip=0x056Cu;break;
        case 0x056C: if(1){c->ip=0x05B6u;}else c->ip=0x056Eu;break;
        case 0x056E: (void)arithmetic(c,(uint8_t)(c->ax>>0u),0x0002u,0u,8u,1u);c->ip=0x0570u;break;
        case 0x0570: if((c->flags&0x40u)==0){c->ip=0x0593u;}else c->ip=0x0572u;break;
        case 0x0572: c->bx=(uint16_t)((c->bx&0xFF00u)|((uint16_t)(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-11))))<<0u));c->ip=0x0575u;break;
        case 0x0575: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12)))));c->ip=0x0578u;break;
        case 0x0578: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x057Au;break;
        case 0x057A: c->ax=arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-8)),0u,16u,0u);c->ip=0x057Du;break;
        case 0x057D: c->cx=c->ax;c->ip=0x057Fu;break;
        case 0x057F: c->ax=0x00A0u;c->ip=0x0582u;break;
        case 0x0582: {uint32_t value=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ax=(uint16_t)value;c->dx=(uint16_t)(value>>16u);}c->ip=0x0585u;break;
        case 0x0585: c->di=c->ax;c->ip=0x0587u;break;
        case 0x0587: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0589u;break;
        case 0x0589: c->si=0xBC00u;c->ip=0x058Cu;break;
        case 0x058C: c->es=c->si;c->ip=0x058Eu;break;
        case 0x058E: pr_bus_write8(ram,physical(c->es,(uint16_t)(c->di+0)),(uint8_t)((uint8_t)(c->bx>>0u)));c->ip=0x0591u;break;
        case 0x0591: if(1){c->ip=0x05B6u;}else c->ip=0x0593u;break;
        case 0x0593: (void)arithmetic(c,(uint8_t)(c->ax>>0u),0x0003u,0u,8u,1u);c->ip=0x0595u;break;
        case 0x0595: if((c->flags&0x40u)==0){c->ip=0x05B6u;}else c->ip=0x0597u;break;
        case 0x0597: c->bx=(uint16_t)((c->bx&0xFF00u)|((uint16_t)(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-11))))<<0u));c->ip=0x059Au;break;
        case 0x059A: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12)))));c->ip=0x059Du;break;
        case 0x059D: c->ax=(uint16_t)((c->ax&0x00FFu)|((uint16_t)(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u))<<8u));c->ip=0x059Fu;break;
        case 0x059F: c->ax=arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-8)),0u,16u,0u);c->ip=0x05A2u;break;
        case 0x05A2: c->cx=c->ax;c->ip=0x05A4u;break;
        case 0x05A4: c->ax=0x00A0u;c->ip=0x05A7u;break;
        case 0x05A7: {uint32_t value=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ax=(uint16_t)value;c->dx=(uint16_t)(value>>16u);}c->ip=0x05AAu;break;
        case 0x05AA: c->di=c->ax;c->ip=0x05ACu;break;
        case 0x05AC: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x05AEu;break;
        case 0x05AE: c->si=0xBE00u;c->ip=0x05B1u;break;
        case 0x05B1: c->es=c->si;c->ip=0x05B3u;break;
        case 0x05B3: pr_bus_write8(ram,physical(c->es,(uint16_t)(c->di+0)),(uint8_t)((uint8_t)(c->bx>>0u)));c->ip=0x05B6u;break;
        case 0x05B6: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-12))),0x0003u,0u,8u,1u);c->ip=0x05BAu;break;
        case 0x05BA: if((c->flags&0x40u)!=0){c->ip=0x05BFu;}else c->ip=0x05BCu;break;
        case 0x05BC: if(1){if(loops==max_loopbacks)return PR_R0709_GLYPH_CPU_LIMIT;++loops;c->ip=0x04B2u;}else c->ip=0x05BFu;break;
        case 0x05BF: put_word(ram,c->ss,(uint16_t)(c->bp+-4),increment(c,read_word(ram,c->ss,(uint16_t)(c->bp+-4)),16u));c->ip=0x05C2u;break;
        case 0x05C2: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-14))),0x0003u,0u,8u,1u);c->ip=0x05C6u;break;
        case 0x05C6: if((c->flags&0x40u)!=0){c->ip=0x05CBu;}else c->ip=0x05C8u;break;
        case 0x05C8: if(1){if(loops==max_loopbacks)return PR_R0709_GLYPH_CPU_LIMIT;++loops;c->ip=0x04A5u;}else c->ip=0x05CBu;break;
        case 0x05CB: put_word(ram,c->ss,(uint16_t)(c->bp+-6),increment(c,read_word(ram,c->ss,(uint16_t)(c->bp+-6)),16u));c->ip=0x05CEu;break;
        case 0x05CE: (void)arithmetic(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-15))),0x0001u,0u,8u,1u);c->ip=0x05D2u;break;
        case 0x05D2: if((c->flags&0x40u)!=0){c->ip=0x05D7u;}else c->ip=0x05D4u;break;
        case 0x05D4: if(1){if(loops==max_loopbacks)return PR_R0709_GLYPH_CPU_LIMIT;++loops;c->ip=0x049Cu;}else c->ip=0x05D7u;break;
        case 0x05D7: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+16)),(uint8_t)(increment(c,pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+16))),8u)));c->ip=0x05DAu;break;
        case 0x05DA: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-1)))));c->ip=0x05DDu;break;
        case 0x05DD: (void)arithmetic(c,(uint8_t)(c->ax>>0u),pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-17))),0u,8u,1u);c->ip=0x05E0u;break;
        case 0x05E0: if((c->flags&0x40u)!=0){c->ip=0x05E5u;}else c->ip=0x05E2u;break;
        case 0x05E2: if(1){if(loops==max_loopbacks)return PR_R0709_GLYPH_CPU_LIMIT;++loops;c->ip=0x0235u;}else c->ip=0x05E5u;break;
        case 0x05E5: c->sp=c->bp;c->ip=0x05E7u;break;
        case 0x05E7: c->bp=pop(ram,c);c->ip=0x05E8u;break;
        case 0x05E8: c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+12u);return PR_R0709_GLYPH_CPU_OK;
        default:return PR_R0709_GLYPH_CPU_TRANSFER;
        }
    }
}
