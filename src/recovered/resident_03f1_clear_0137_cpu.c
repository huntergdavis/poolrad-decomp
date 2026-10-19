/* 03F1 +0137: raw mode-dependent rectangle clear. Primitives reuse +0626. */
#include "poolrad/recovered/resident_03f1_clear_0137_cpu.h"
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
static void push(pr_bus *ram, pr_r03f1_clear_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r03f1_clear_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r03f1_clear_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r03f1_clear_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r03f1_clear_cpu *c, uint16_t left, uint16_t right,
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


static uint16_t increment(pr_r03f1_clear_cpu *c,uint16_t value,unsigned bits) {
    uint16_t cf=c->flags&1u;value=arithmetic(c,value,1u,0u,bits,0u);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return value;
}
static int ordinary_segment(uint16_t s) {
    size_t start=(size_t)s<<4u;
    return start>0xAFFFFu || start+0xFFFFu<0xA0000u;
}


static int fill(pr_bus *b,pr_r03f1_clear_cpu *c) {
    pr_system_leaf_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_system_fillchar_bus_16a8(b,&inner);
    memcpy(c,&inner,sizeof inner);return status;
}
static int rectangle(pr_bus *b,pr_r03f1_clear_cpu *c,size_t limit,
                     pr_r03f1_clear_cpu_diagnostic *d) {
    pr_r0709_rect_cpu inner;int status;
    _Static_assert(sizeof inner==sizeof *c,"CPU layout");
    memcpy(&inner,c,sizeof inner);status=pr_r0709_rect_fill_00b7_cpu(b,PR_BUS_RAM_SIZE,&inner,limit,&d->rect);
    memcpy(c,&inner,sizeof inner);return status;
}
pr_r03f1_clear_cpu_status pr_r03f1_clear_0137_cpu(pr_bus *ram,size_t size,
    pr_r03f1_clear_cpu *c,size_t max_loopbacks,size_t rect_loopbacks,
    pr_r03f1_clear_cpu_diagnostic *d)
{
    pr_r03f1_clear_cpu_diagnostic fallback={-1,-1,{-1,-1}};uint16_t caller;size_t loops=0;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0x137u || c->sp<0x3Eu || c->sp>0xFFF4u ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds))
        return PR_R03F1_CLEAR_CPU_ARGUMENT;
    caller=c->cs;
    /* MUL/SHL result flags are overwritten by ADD/CMP before any observable
     * call, OUT, branch, budget stop or return. Their register effects remain. */
    for(;;) {
        if(c->cs!=caller)return PR_R03F1_CLEAR_CPU_TRANSFER;
        switch(c->ip) {
        case 0x0137: push(ram,c,c->bp);c->ip=0x0138u;break;
        case 0x0138: c->bp=c->sp;c->ip=0x013Au;break;
        case 0x013A: c->sp=arithmetic(c,c->sp,0x000Cu,0u,16u,1u);c->ip=0x013Du;break;
        case 0x013D: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(18974)))));c->ip=0x0140u;break;
        case 0x0140: (void)arithmetic(c,(uint8_t)c->ax,0x0000u,0u,8u,1u);c->ip=0x0142u;break;
        case 0x0142: if((c->flags&0x40u)!=0){c->ip=0x0147u;}else c->ip=0x0144u;break;
        case 0x0144: c->ip=0x01D7u;break;
        case 0x0147: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x014Au;break;
        case 0x014A: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x014Bu;break;
        case 0x014B: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x014Du;break;
        case 0x014D: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x014Fu;break;
        case 0x014F: put_word(ram,c->ss,(uint16_t)(c->bp+-2),c->ax);c->ip=0x0152u;break;
        case 0x0152: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6)))));c->ip=0x0155u;break;
        case 0x0155: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0156u;break;
        case 0x0156: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0158u;break;
        case 0x0158: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x015Au;break;
        case 0x015A: c->ax=arithmetic(c,c->ax,0x0003u,0u,16u,0u);c->ip=0x015Du;break;
        case 0x015D: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x0160u;break;
        case 0x0160: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0163u;break;
        case 0x0163: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0164u;break;
        case 0x0164: c->dx=c->ax;c->ip=0x0166u;break;
        case 0x0166: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x0169u;break;
        case 0x0169: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x016Au;break;
        case 0x016A: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,1u);c->ip=0x016Cu;break;
        case 0x016C: c->ax=increment(c,c->ax,16u);c->ip=0x016Du;break;
        case 0x016D: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x016Fu;break;
        case 0x016F: put_word(ram,c->ss,(uint16_t)(c->bp+-8),c->ax);c->ip=0x0172u;break;
        case 0x0172: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x0175u;break;
        case 0x0175: put_word(ram,c->ss,(uint16_t)(c->bp+-12),c->ax);c->ip=0x0178u;break;
        case 0x0178: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-2));c->ip=0x017Bu;break;
        case 0x017B: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-12)),0u,16u,1u);c->ip=0x017Eu;break;
        case 0x017E: if((c->flags&0x41u)==0){c->ip=0x01D4u;}else c->ip=0x0180u;break;
        case 0x0180: put_word(ram,c->ss,(uint16_t)(c->bp+-10),c->ax);c->ip=0x0183u;break;
        case 0x0183: c->ip=0x0188u;break;
        case 0x0185: put_word(ram,c->ss,(uint16_t)(c->bp+-10),increment(c,read_word(ram,c->ss,(uint16_t)(c->bp+-10)),16u));c->ip=0x0188u;break;
        case 0x0188: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x018Bu;break;
        case 0x018B: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x018Cu;break;
        case 0x018C: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x018Eu;break;
        case 0x018E: c->cx=c->ax;c->ip=0x0190u;break;
        case 0x0190: c->ax=0x0050u;c->ip=0x0193u;break;
        case 0x0193: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x0196u;break;
        case 0x0196: c->di=c->ax;c->ip=0x0198u;break;
        case 0x0198: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x019Au;break;
        case 0x019A: c->ax=0xB800u;c->ip=0x019Du;break;
        case 0x019D: push(ram,c,c->ax);c->ip=0x019Eu;break;
        case 0x019E: push(ram,c,c->di);c->ip=0x019Fu;break;
        case 0x019F: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x01A2u;break;
        case 0x01A2: al(c,(uint8_t)(0x0000u));c->ip=0x01A4u;break;
        case 0x01A4: push(ram,c,c->ax);c->ip=0x01A5u;break;
        case 0x01A5: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x01A6u)!=0x16A8u)return PR_R03F1_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x01AAu);c->cs=read_word(ram,c->cs,0x01A8u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R03F1_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x01AAu)return PR_R03F1_CLEAR_CPU_TRANSFER;break;
        case 0x01AA: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x01ADu;break;
        case 0x01AD: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x01AEu;break;
        case 0x01AE: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x01B0u;break;
        case 0x01B0: c->cx=c->ax;c->ip=0x01B2u;break;
        case 0x01B2: c->ax=0x0050u;c->ip=0x01B5u;break;
        case 0x01B5: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x01B8u;break;
        case 0x01B8: c->di=c->ax;c->ip=0x01BAu;break;
        case 0x01BA: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x01BCu;break;
        case 0x01BC: c->ax=0xBA00u;c->ip=0x01BFu;break;
        case 0x01BF: push(ram,c,c->ax);c->ip=0x01C0u;break;
        case 0x01C0: push(ram,c,c->di);c->ip=0x01C1u;break;
        case 0x01C1: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x01C4u;break;
        case 0x01C4: al(c,(uint8_t)(0x0000u));c->ip=0x01C6u;break;
        case 0x01C6: push(ram,c,c->ax);c->ip=0x01C7u;break;
        case 0x01C7: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x01C8u)!=0x16A8u)return PR_R03F1_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x01CCu);c->cs=read_word(ram,c->cs,0x01CAu);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R03F1_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x01CCu)return PR_R03F1_CLEAR_CPU_TRANSFER;break;
        case 0x01CC: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ip=0x01CFu;break;
        case 0x01CF: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-12)),0u,16u,1u);c->ip=0x01D2u;break;
        case 0x01D2: if((c->flags&0x40u)==0){if(loops==max_loopbacks)return PR_R03F1_CLEAR_CPU_LIMIT;++loops;c->ip=0x0185u;}else c->ip=0x01D4u;break;
        case 0x01D4: c->ip=0x02E2u;break;
        case 0x01D7: (void)arithmetic(c,(uint8_t)c->ax,0x0001u,0u,8u,1u);c->ip=0x01D9u;break;
        case 0x01D9: if((c->flags&0x40u)==0){c->ip=0x0214u;}else c->ip=0x01DBu;break;
        case 0x01DB: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x01DEu;break;
        case 0x01DE: push(ram,c,c->ax);c->ip=0x01DFu;break;
        case 0x01DF: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x01E2u;break;
        case 0x01E2: push(ram,c,c->ax);c->ip=0x01E3u;break;
        case 0x01E3: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x01E6u;break;
        case 0x01E6: push(ram,c,c->ax);c->ip=0x01E7u;break;
        case 0x01E7: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6)))));c->ip=0x01EAu;break;
        case 0x01EA: push(ram,c,c->ax);c->ip=0x01EBu;break;
        case 0x01EB: al(c,(uint8_t)(0x0000u));c->ip=0x01EDu;break;
        case 0x01ED: push(ram,c,c->ax);c->ip=0x01EEu;break;
        case 0x01EE: al(c,(uint8_t)(0x0000u));c->ip=0x01F0u;break;
        case 0x01F0: push(ram,c,c->ax);c->ip=0x01F1u;break;
        case 0x01F1: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x01F2u)!=0x00B7u)return PR_R03F1_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x01F6u);c->cs=read_word(ram,c->cs,0x01F4u);c->ip=0x00B7u;d->callee=1;d->status=rectangle(ram,c,rect_loopbacks,d);if(d->status!=0)return PR_R03F1_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x01F6u)return PR_R03F1_CLEAR_CPU_TRANSFER;break;
        case 0x01F6: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x01F9u;break;
        case 0x01F9: push(ram,c,c->ax);c->ip=0x01FAu;break;
        case 0x01FA: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x01FDu;break;
        case 0x01FD: push(ram,c,c->ax);c->ip=0x01FEu;break;
        case 0x01FE: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x0201u;break;
        case 0x0201: push(ram,c,c->ax);c->ip=0x0202u;break;
        case 0x0202: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6)))));c->ip=0x0205u;break;
        case 0x0205: push(ram,c,c->ax);c->ip=0x0206u;break;
        case 0x0206: al(c,(uint8_t)(0x0001u));c->ip=0x0208u;break;
        case 0x0208: push(ram,c,c->ax);c->ip=0x0209u;break;
        case 0x0209: al(c,(uint8_t)(0x0000u));c->ip=0x020Bu;break;
        case 0x020B: push(ram,c,c->ax);c->ip=0x020Cu;break;
        case 0x020C: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x020Du)!=0x00B7u)return PR_R03F1_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0211u);c->cs=read_word(ram,c->cs,0x020Fu);c->ip=0x00B7u;d->callee=2;d->status=rectangle(ram,c,rect_loopbacks,d);if(d->status!=0)return PR_R03F1_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0211u)return PR_R03F1_CLEAR_CPU_TRANSFER;break;
        case 0x0211: c->ip=0x02E2u;break;
        case 0x0214: (void)arithmetic(c,(uint8_t)c->ax,0x0002u,0u,8u,1u);c->ip=0x0216u;break;
        case 0x0216: if((c->flags&0x40u)!=0){c->ip=0x021Bu;}else c->ip=0x0218u;break;
        case 0x0218: c->ip=0x02E2u;break;
        case 0x021B: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x021Eu;break;
        case 0x021E: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x021Fu;break;
        case 0x021F: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0221u;break;
        case 0x0221: put_word(ram,c->ss,(uint16_t)(c->bp+-2),c->ax);c->ip=0x0224u;break;
        case 0x0224: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6)))));c->ip=0x0227u;break;
        case 0x0227: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0228u;break;
        case 0x0228: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x022Au;break;
        case 0x022A: c->ax=increment(c,c->ax,16u);c->ip=0x022Bu;break;
        case 0x022B: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x022Eu;break;
        case 0x022E: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0231u;break;
        case 0x0231: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0232u;break;
        case 0x0232: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0234u;break;
        case 0x0234: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0236u;break;
        case 0x0236: put_word(ram,c->ss,(uint16_t)(c->bp+-6),c->ax);c->ip=0x0239u;break;
        case 0x0239: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x023Cu;break;
        case 0x023C: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x023Du;break;
        case 0x023D: c->dx=c->ax;c->ip=0x023Fu;break;
        case 0x023F: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x0242u;break;
        case 0x0242: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0243u;break;
        case 0x0243: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,1u);c->ip=0x0245u;break;
        case 0x0245: c->ax=increment(c,c->ax,16u);c->ip=0x0246u;break;
        case 0x0246: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x0248u;break;
        case 0x0248: c->ax=(uint16_t)(c->ax<<0x0001u);c->ip=0x024Au;break;
        case 0x024A: put_word(ram,c->ss,(uint16_t)(c->bp+-8),c->ax);c->ip=0x024Du;break;
        case 0x024D: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x0250u;break;
        case 0x0250: put_word(ram,c->ss,(uint16_t)(c->bp+-12),c->ax);c->ip=0x0253u;break;
        case 0x0253: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-2));c->ip=0x0256u;break;
        case 0x0256: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-12)),0u,16u,1u);c->ip=0x0259u;break;
        case 0x0259: if((c->flags&0x41u)!=0){c->ip=0x025Eu;}else c->ip=0x025Bu;break;
        case 0x025B: c->ip=0x02E2u;break;
        case 0x025E: put_word(ram,c->ss,(uint16_t)(c->bp+-10),c->ax);c->ip=0x0261u;break;
        case 0x0261: c->ip=0x0266u;break;
        case 0x0263: put_word(ram,c->ss,(uint16_t)(c->bp+-10),increment(c,read_word(ram,c->ss,(uint16_t)(c->bp+-10)),16u));c->ip=0x0266u;break;
        case 0x0266: c->cx=read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ip=0x0269u;break;
        case 0x0269: c->ax=0x00A0u;c->ip=0x026Cu;break;
        case 0x026C: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x026Fu;break;
        case 0x026F: c->di=c->ax;c->ip=0x0271u;break;
        case 0x0271: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0273u;break;
        case 0x0273: c->ax=0xB800u;c->ip=0x0276u;break;
        case 0x0276: push(ram,c,c->ax);c->ip=0x0277u;break;
        case 0x0277: push(ram,c,c->di);c->ip=0x0278u;break;
        case 0x0278: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x027Bu;break;
        case 0x027B: al(c,(uint8_t)(0x0000u));c->ip=0x027Du;break;
        case 0x027D: push(ram,c,c->ax);c->ip=0x027Eu;break;
        case 0x027E: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x027Fu)!=0x16A8u)return PR_R03F1_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0283u);c->cs=read_word(ram,c->cs,0x0281u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R03F1_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0283u)return PR_R03F1_CLEAR_CPU_TRANSFER;break;
        case 0x0283: c->cx=read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ip=0x0286u;break;
        case 0x0286: c->ax=0x00A0u;c->ip=0x0289u;break;
        case 0x0289: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x028Cu;break;
        case 0x028C: c->di=c->ax;c->ip=0x028Eu;break;
        case 0x028E: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x0290u;break;
        case 0x0290: c->ax=0xBA00u;c->ip=0x0293u;break;
        case 0x0293: push(ram,c,c->ax);c->ip=0x0294u;break;
        case 0x0294: push(ram,c,c->di);c->ip=0x0295u;break;
        case 0x0295: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x0298u;break;
        case 0x0298: al(c,(uint8_t)(0x0000u));c->ip=0x029Au;break;
        case 0x029A: push(ram,c,c->ax);c->ip=0x029Bu;break;
        case 0x029B: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x029Cu)!=0x16A8u)return PR_R03F1_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x02A0u);c->cs=read_word(ram,c->cs,0x029Eu);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R03F1_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x02A0u)return PR_R03F1_CLEAR_CPU_TRANSFER;break;
        case 0x02A0: c->cx=read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ip=0x02A3u;break;
        case 0x02A3: c->ax=0x00A0u;c->ip=0x02A6u;break;
        case 0x02A6: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x02A9u;break;
        case 0x02A9: c->di=c->ax;c->ip=0x02ABu;break;
        case 0x02AB: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x02ADu;break;
        case 0x02AD: c->ax=0xBC00u;c->ip=0x02B0u;break;
        case 0x02B0: push(ram,c,c->ax);c->ip=0x02B1u;break;
        case 0x02B1: push(ram,c,c->di);c->ip=0x02B2u;break;
        case 0x02B2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x02B5u;break;
        case 0x02B5: al(c,(uint8_t)(0x0000u));c->ip=0x02B7u;break;
        case 0x02B7: push(ram,c,c->ax);c->ip=0x02B8u;break;
        case 0x02B8: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x02B9u)!=0x16A8u)return PR_R03F1_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x02BDu);c->cs=read_word(ram,c->cs,0x02BBu);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R03F1_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x02BDu)return PR_R03F1_CLEAR_CPU_TRANSFER;break;
        case 0x02BD: c->cx=read_word(ram,c->ss,(uint16_t)(c->bp+-6));c->ip=0x02C0u;break;
        case 0x02C0: c->ax=0x00A0u;c->ip=0x02C3u;break;
        case 0x02C3: {uint32_t v=(uint32_t)c->ax*read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x02C6u;break;
        case 0x02C6: c->di=c->ax;c->ip=0x02C8u;break;
        case 0x02C8: c->di=arithmetic(c,c->di,c->cx,0u,16u,0u);c->ip=0x02CAu;break;
        case 0x02CA: c->ax=0xBE00u;c->ip=0x02CDu;break;
        case 0x02CD: push(ram,c,c->ax);c->ip=0x02CEu;break;
        case 0x02CE: push(ram,c,c->di);c->ip=0x02CFu;break;
        case 0x02CF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-8)));c->ip=0x02D2u;break;
        case 0x02D2: al(c,(uint8_t)(0x0000u));c->ip=0x02D4u;break;
        case 0x02D4: push(ram,c,c->ax);c->ip=0x02D5u;break;
        case 0x02D5: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x02D6u)!=0x16A8u)return PR_R03F1_CLEAR_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x02DAu);c->cs=read_word(ram,c->cs,0x02D8u);c->ip=0x16A8u;d->callee=0;d->status=fill(ram,c);if(d->status!=0)return PR_R03F1_CLEAR_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x02DAu)return PR_R03F1_CLEAR_CPU_TRANSFER;break;
        case 0x02DA: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-10));c->ip=0x02DDu;break;
        case 0x02DD: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-12)),0u,16u,1u);c->ip=0x02E0u;break;
        case 0x02E0: if((c->flags&0x40u)==0){if(loops==max_loopbacks)return PR_R03F1_CLEAR_CPU_LIMIT;++loops;c->ip=0x0263u;}else c->ip=0x02E2u;break;
        case 0x02E2: c->sp=c->bp;c->ip=0x02E4u;break;
        case 0x02E4: c->bp=pop(ram,c);c->ip=0x02E5u;break;
        case 0x02E5: c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+8u);return PR_R03F1_CLEAR_CPU_OK;
        default:return PR_R03F1_CLEAR_CPU_TRANSFER;
        }
    }
}
