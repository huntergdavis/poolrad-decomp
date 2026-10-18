/* U9E +02FA: grouped sidecar dispatch with the inherited pending boundary.
 * Primitives and callee contract: overlay_u9e_sidecar_0160_cpu.c. */
#include "poolrad/recovered/overlay_u9e_sidecar_groups_02fa_cpu.h"
#define MASK (PR_U9E_0160_RAM_SIZE - 1u)
static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4u) + off) & MASK;
}
static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at+1u)&MASK] << 8u));
}
static uint16_t read_word(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return word_at(ram,physical(seg,off));
}
static void put_word(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at=physical(seg,off);
    ram[at]=(uint8_t)value;ram[(at+1u)&MASK]=(uint8_t)(value >> 8u);
}
static void push(uint8_t *ram, pr_u9e_0160_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u9e_0160_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u9e_0160_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_u9e_0160_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u9e_0160_cpu *c, uint16_t left, uint16_t right,
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
static void retf(const uint8_t *ram, pr_u9e_0160_cpu *c, unsigned cleanup)
{
    c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+cleanup);
}



pr_u9e_0160_status pr_u9e_sidecar_groups_02fa_cpu(uint8_t *ram,size_t size,
    pr_u9e_0160_cpu *c,const pr_u9e_0160_layout *layout,
    const pr_u9e_0160_limits *limits,pr_u9e_0160_diagnostic *diagnostic)
{
    pr_u9e_0160_diagnostic fallback={-1,-1,0,0};
    pr_u9e_0160_status status;uint16_t caller;
    if(diagnostic==NULL)diagnostic=&fallback;
    *diagnostic=fallback;
    if(ram==NULL || c==NULL || layout==NULL || limits==NULL ||
       size!=PR_U9E_0160_RAM_SIZE || c->ip!=0x02FAu ||
       c->sp<0x0196u || c->sp>0xFFF6u ||
       limits->chain_iterations==0 || limits->line_iterations==0 || limits->sort_loopbacks==0)
        return PR_U9E_0160_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_U9E_0160_TRANSFER;
        switch(c->ip) {
        case 0x02FA: push(ram,c,c->bp);c->ip=0x02FBu;break;
        case 0x02FB: c->bp=c->sp;c->ip=0x02FDu;break;
        case 0x02FD: c->sp=arithmetic(c,c->sp,0x0004u,0u,16u,1u);c->ip=0x0300u;break;
        case 0x0300: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+10))]));c->ip=0x0303u;break;
        case 0x0303: (void)arithmetic(c,(uint8_t)c->ax,0x0000u,0u,8u,1u);c->ip=0x0305u;break;
        case 0x0305: c->ip=((c->flags&0x40u)==0u)?0x0317u:0x0307u;break;
        case 0x0307: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x030Au;break;
        case 0x030A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x030Du;break;
        case 0x030D: al(c,(uint8_t)(0x007Eu));c->ip=0x030Fu;break;
        case 0x030F: push(ram,c,c->ax);c->ip=0x0310u;break;
        case 0x0310: push(ram,c,c->cs);c->ip=0x0311u;break;
        case 0x0311: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0312u)!=0xFE4Cu)return PR_U9E_0160_CODE;
            push(ram,c,0x0314u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0314u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0314: c->ip=0x0AB5u;break;
        case 0x0317: (void)arithmetic(c,(uint8_t)c->ax,0x0001u,0u,8u,1u);c->ip=0x0319u;break;
        case 0x0319: c->ip=((c->flags&0x40u)==0u)?0x0352u:0x031Bu;break;
        case 0x031B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x031Eu;break;
        case 0x031E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0321u;break;
        case 0x0321: al(c,(uint8_t)(0x003Fu));c->ip=0x0323u;break;
        case 0x0323: push(ram,c,c->ax);c->ip=0x0324u;break;
        case 0x0324: push(ram,c,c->cs);c->ip=0x0325u;break;
        case 0x0325: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0326u)!=0xFE38u)return PR_U9E_0160_CODE;
            push(ram,c,0x0328u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0328u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0328: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x032Bu;break;
        case 0x032B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x032Eu;break;
        case 0x032E: al(c,(uint8_t)(0x0025u));c->ip=0x0330u;break;
        case 0x0330: push(ram,c,c->ax);c->ip=0x0331u;break;
        case 0x0331: push(ram,c,c->cs);c->ip=0x0332u;break;
        case 0x0332: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0333u)!=0xFE2Bu)return PR_U9E_0160_CODE;
            push(ram,c,0x0335u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0335u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0335: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0338u;break;
        case 0x0338: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x033Bu;break;
        case 0x033B: al(c,(uint8_t)(0x0019u));c->ip=0x033Du;break;
        case 0x033D: push(ram,c,c->ax);c->ip=0x033Eu;break;
        case 0x033E: push(ram,c,c->cs);c->ip=0x033Fu;break;
        case 0x033F: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0340u)!=0xFE1Eu)return PR_U9E_0160_CODE;
            push(ram,c,0x0342u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0342u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0342: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0345u;break;
        case 0x0345: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0348u;break;
        case 0x0348: al(c,(uint8_t)(0x0047u));c->ip=0x034Au;break;
        case 0x034A: push(ram,c,c->ax);c->ip=0x034Bu;break;
        case 0x034B: push(ram,c,c->cs);c->ip=0x034Cu;break;
        case 0x034C: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x034Du)!=0xFE11u)return PR_U9E_0160_CODE;
            push(ram,c,0x034Fu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x034Fu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x034F: c->ip=0x0AB5u;break;
        case 0x0352: (void)arithmetic(c,(uint8_t)c->ax,0x0002u,0u,8u,1u);c->ip=0x0354u;break;
        case 0x0354: c->ip=((c->flags&0x40u)==0u)?0x03CEu:0x0356u;break;
        case 0x0356: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0359u;break;
        case 0x0359: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x035Cu;break;
        case 0x035C: al(c,(uint8_t)(0x0055u));c->ip=0x035Eu;break;
        case 0x035E: push(ram,c,c->ax);c->ip=0x035Fu;break;
        case 0x035F: push(ram,c,c->cs);c->ip=0x0360u;break;
        case 0x0360: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0361u)!=0xFDFDu)return PR_U9E_0160_CODE;
            push(ram,c,0x0363u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0363u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0363: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0366u;break;
        case 0x0366: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0369u;break;
        case 0x0369: al(c,(uint8_t)(0x0056u));c->ip=0x036Bu;break;
        case 0x036B: push(ram,c,c->ax);c->ip=0x036Cu;break;
        case 0x036C: push(ram,c,c->cs);c->ip=0x036Du;break;
        case 0x036D: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x036Eu)!=0xFDF0u)return PR_U9E_0160_CODE;
            push(ram,c,0x0370u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0370u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0370: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0373u;break;
        case 0x0373: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0376u;break;
        case 0x0376: al(c,(uint8_t)(0x0057u));c->ip=0x0378u;break;
        case 0x0378: push(ram,c,c->ax);c->ip=0x0379u;break;
        case 0x0379: push(ram,c,c->cs);c->ip=0x037Au;break;
        case 0x037A: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x037Bu)!=0xFDE3u)return PR_U9E_0160_CODE;
            push(ram,c,0x037Du);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x037Du)return PR_U9E_0160_TRANSFER;
            break;
        case 0x037D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0380u;break;
        case 0x0380: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0383u;break;
        case 0x0383: al(c,(uint8_t)(0x0044u));c->ip=0x0385u;break;
        case 0x0385: push(ram,c,c->ax);c->ip=0x0386u;break;
        case 0x0386: push(ram,c,c->cs);c->ip=0x0387u;break;
        case 0x0387: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0388u)!=0xFDD6u)return PR_U9E_0160_CODE;
            push(ram,c,0x038Au);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x038Au)return PR_U9E_0160_TRANSFER;
            break;
        case 0x038A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x038Du;break;
        case 0x038D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0390u;break;
        case 0x0390: al(c,(uint8_t)(0x004Fu));c->ip=0x0392u;break;
        case 0x0392: push(ram,c,c->ax);c->ip=0x0393u;break;
        case 0x0393: push(ram,c,c->cs);c->ip=0x0394u;break;
        case 0x0394: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0395u)!=0xFDC9u)return PR_U9E_0160_CODE;
            push(ram,c,0x0397u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0397u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0397: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x039Au;break;
        case 0x039A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x039Du;break;
        case 0x039D: al(c,(uint8_t)(0x0050u));c->ip=0x039Fu;break;
        case 0x039F: push(ram,c,c->ax);c->ip=0x03A0u;break;
        case 0x03A0: push(ram,c,c->cs);c->ip=0x03A1u;break;
        case 0x03A1: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x03A2u)!=0xFDBCu)return PR_U9E_0160_CODE;
            push(ram,c,0x03A4u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x03A4u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x03A4: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x03A7u;break;
        case 0x03A7: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x03AAu;break;
        case 0x03AA: al(c,(uint8_t)(0x004Cu));c->ip=0x03ACu;break;
        case 0x03AC: push(ram,c,c->ax);c->ip=0x03ADu;break;
        case 0x03AD: push(ram,c,c->cs);c->ip=0x03AEu;break;
        case 0x03AE: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x03AFu)!=0xFDAFu)return PR_U9E_0160_CODE;
            push(ram,c,0x03B1u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x03B1u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x03B1: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x03B4u;break;
        case 0x03B4: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x03B7u;break;
        case 0x03B7: al(c,(uint8_t)(0x0049u));c->ip=0x03B9u;break;
        case 0x03B9: push(ram,c,c->ax);c->ip=0x03BAu;break;
        case 0x03BA: push(ram,c,c->cs);c->ip=0x03BBu;break;
        case 0x03BB: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x03BCu)!=0xFDA2u)return PR_U9E_0160_CODE;
            push(ram,c,0x03BEu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x03BEu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x03BE: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x03C1u;break;
        case 0x03C1: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x03C4u;break;
        case 0x03C4: al(c,(uint8_t)(0x004Du));c->ip=0x03C6u;break;
        case 0x03C6: push(ram,c,c->ax);c->ip=0x03C7u;break;
        case 0x03C7: push(ram,c,c->cs);c->ip=0x03C8u;break;
        case 0x03C8: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x03C9u)!=0xFD95u)return PR_U9E_0160_CODE;
            push(ram,c,0x03CBu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x03CBu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x03CB: c->ip=0x0AB5u;break;
        case 0x03CE: (void)arithmetic(c,(uint8_t)c->ax,0x0003u,0u,8u,1u);c->ip=0x03D0u;break;
        case 0x03D0: c->ip=((c->flags&0x40u)!=0u)?0x03D5u:0x03D2u;break;
        case 0x03D2: c->ip=0x0467u;break;
        case 0x03D5: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x03D8u;break;
        case 0x03D8: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x03DBu;break;
        case 0x03DB: al(c,(uint8_t)(0x0040u));c->ip=0x03DDu;break;
        case 0x03DD: push(ram,c,c->ax);c->ip=0x03DEu;break;
        case 0x03DE: push(ram,c,c->cs);c->ip=0x03DFu;break;
        case 0x03DF: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x03E0u)!=0xFD7Eu)return PR_U9E_0160_CODE;
            push(ram,c,0x03E2u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x03E2u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x03E2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x03E5u;break;
        case 0x03E5: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x03E8u;break;
        case 0x03E8: al(c,(uint8_t)(0x0041u));c->ip=0x03EAu;break;
        case 0x03EA: push(ram,c,c->ax);c->ip=0x03EBu;break;
        case 0x03EB: push(ram,c,c->cs);c->ip=0x03ECu;break;
        case 0x03EC: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x03EDu)!=0xFD71u)return PR_U9E_0160_CODE;
            push(ram,c,0x03EFu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x03EFu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x03EF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x03F2u;break;
        case 0x03F2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x03F5u;break;
        case 0x03F5: al(c,(uint8_t)(0x0042u));c->ip=0x03F7u;break;
        case 0x03F7: push(ram,c,c->ax);c->ip=0x03F8u;break;
        case 0x03F8: push(ram,c,c->cs);c->ip=0x03F9u;break;
        case 0x03F9: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x03FAu)!=0xFD64u)return PR_U9E_0160_CODE;
            push(ram,c,0x03FCu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x03FCu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x03FC: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x03FFu;break;
        case 0x03FF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0402u;break;
        case 0x0402: al(c,(uint8_t)(0x0043u));c->ip=0x0404u;break;
        case 0x0404: push(ram,c,c->ax);c->ip=0x0405u;break;
        case 0x0405: push(ram,c,c->cs);c->ip=0x0406u;break;
        case 0x0406: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0407u)!=0xFD57u)return PR_U9E_0160_CODE;
            push(ram,c,0x0409u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0409u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0409: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x040Cu;break;
        case 0x040C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x040Fu;break;
        case 0x040F: al(c,(uint8_t)(0x0044u));c->ip=0x0411u;break;
        case 0x0411: push(ram,c,c->ax);c->ip=0x0412u;break;
        case 0x0412: push(ram,c,c->cs);c->ip=0x0413u;break;
        case 0x0413: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0414u)!=0xFD4Au)return PR_U9E_0160_CODE;
            push(ram,c,0x0416u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0416u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0416: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0419u;break;
        case 0x0419: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x041Cu;break;
        case 0x041C: al(c,(uint8_t)(0x0045u));c->ip=0x041Eu;break;
        case 0x041E: push(ram,c,c->ax);c->ip=0x041Fu;break;
        case 0x041F: push(ram,c,c->cs);c->ip=0x0420u;break;
        case 0x0420: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0421u)!=0xFD3Du)return PR_U9E_0160_CODE;
            push(ram,c,0x0423u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0423u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0423: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0426u;break;
        case 0x0426: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0429u;break;
        case 0x0429: al(c,(uint8_t)(0x0046u));c->ip=0x042Bu;break;
        case 0x042B: push(ram,c,c->ax);c->ip=0x042Cu;break;
        case 0x042C: push(ram,c,c->cs);c->ip=0x042Du;break;
        case 0x042D: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x042Eu)!=0xFD30u)return PR_U9E_0160_CODE;
            push(ram,c,0x0430u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0430u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0430: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0433u;break;
        case 0x0433: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0436u;break;
        case 0x0436: al(c,(uint8_t)(0x004Fu));c->ip=0x0438u;break;
        case 0x0438: push(ram,c,c->ax);c->ip=0x0439u;break;
        case 0x0439: push(ram,c,c->cs);c->ip=0x043Au;break;
        case 0x043A: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x043Bu)!=0xFD23u)return PR_U9E_0160_CODE;
            push(ram,c,0x043Du);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x043Du)return PR_U9E_0160_TRANSFER;
            break;
        case 0x043D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0440u;break;
        case 0x0440: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0443u;break;
        case 0x0443: al(c,(uint8_t)(0x0055u));c->ip=0x0445u;break;
        case 0x0445: push(ram,c,c->ax);c->ip=0x0446u;break;
        case 0x0446: push(ram,c,c->cs);c->ip=0x0447u;break;
        case 0x0447: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0448u)!=0xFD16u)return PR_U9E_0160_CODE;
            push(ram,c,0x044Au);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x044Au)return PR_U9E_0160_TRANSFER;
            break;
        case 0x044A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x044Du;break;
        case 0x044D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0450u;break;
        case 0x0450: al(c,(uint8_t)(0x0056u));c->ip=0x0452u;break;
        case 0x0452: push(ram,c,c->ax);c->ip=0x0453u;break;
        case 0x0453: push(ram,c,c->cs);c->ip=0x0454u;break;
        case 0x0454: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0455u)!=0xFD09u)return PR_U9E_0160_CODE;
            push(ram,c,0x0457u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0457u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0457: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x045Au;break;
        case 0x045A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x045Du;break;
        case 0x045D: al(c,(uint8_t)(0x0057u));c->ip=0x045Fu;break;
        case 0x045F: push(ram,c,c->ax);c->ip=0x0460u;break;
        case 0x0460: push(ram,c,c->cs);c->ip=0x0461u;break;
        case 0x0461: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0462u)!=0xFCFCu)return PR_U9E_0160_CODE;
            push(ram,c,0x0464u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0464u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0464: c->ip=0x0AB5u;break;
        case 0x0467: (void)arithmetic(c,(uint8_t)c->ax,0x0004u,0u,8u,1u);c->ip=0x0469u;break;
        case 0x0469: c->ip=((c->flags&0x40u)==0u)?0x0495u:0x046Bu;break;
        case 0x046B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x046Eu;break;
        case 0x046E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0471u;break;
        case 0x0471: al(c,(uint8_t)(0x001Du));c->ip=0x0473u;break;
        case 0x0473: push(ram,c,c->ax);c->ip=0x0474u;break;
        case 0x0474: push(ram,c,c->cs);c->ip=0x0475u;break;
        case 0x0475: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0476u)!=0xFCE8u)return PR_U9E_0160_CODE;
            push(ram,c,0x0478u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0478u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0478: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x047Bu;break;
        case 0x047B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x047Eu;break;
        case 0x047E: al(c,(uint8_t)(0x0003u));c->ip=0x0480u;break;
        case 0x0480: push(ram,c,c->ax);c->ip=0x0481u;break;
        case 0x0481: push(ram,c,c->cs);c->ip=0x0482u;break;
        case 0x0482: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0483u)!=0xFCDBu)return PR_U9E_0160_CODE;
            push(ram,c,0x0485u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0485u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0485: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0488u;break;
        case 0x0488: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x048Bu;break;
        case 0x048B: al(c,(uint8_t)(0x0006u));c->ip=0x048Du;break;
        case 0x048D: push(ram,c,c->ax);c->ip=0x048Eu;break;
        case 0x048E: push(ram,c,c->cs);c->ip=0x048Fu;break;
        case 0x048F: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0490u)!=0xFCCEu)return PR_U9E_0160_CODE;
            push(ram,c,0x0492u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0492u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0492: c->ip=0x0AB5u;break;
        case 0x0495: (void)arithmetic(c,(uint8_t)c->ax,0x0005u,0u,8u,1u);c->ip=0x0497u;break;
        case 0x0497: c->ip=((c->flags&0x40u)!=0u)?0x049Cu:0x0499u;break;
        case 0x0499: c->ip=0x0555u;break;
        case 0x049C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x049Fu;break;
        case 0x049F: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x04A2u;break;
        case 0x04A2: al(c,(uint8_t)(0x001Cu));c->ip=0x04A4u;break;
        case 0x04A4: push(ram,c,c->ax);c->ip=0x04A5u;break;
        case 0x04A5: push(ram,c,c->cs);c->ip=0x04A6u;break;
        case 0x04A6: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x04A7u)!=0xFCB7u)return PR_U9E_0160_CODE;
            push(ram,c,0x04A9u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x04A9u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x04A9: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x04ACu;break;
        case 0x04AC: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x04AFu;break;
        case 0x04AF: al(c,(uint8_t)(0x0029u));c->ip=0x04B1u;break;
        case 0x04B1: push(ram,c,c->ax);c->ip=0x04B2u;break;
        case 0x04B2: push(ram,c,c->cs);c->ip=0x04B3u;break;
        case 0x04B3: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x04B4u)!=0xFCAAu)return PR_U9E_0160_CODE;
            push(ram,c,0x04B6u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x04B6u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x04B6: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x04B9u;break;
        case 0x04B9: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x04BCu;break;
        case 0x04BC: al(c,(uint8_t)(0x0068u));c->ip=0x04BEu;break;
        case 0x04BE: push(ram,c,c->ax);c->ip=0x04BFu;break;
        case 0x04BF: push(ram,c,c->cs);c->ip=0x04C0u;break;
        case 0x04C0: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x04C1u)!=0xFC9Du)return PR_U9E_0160_CODE;
            push(ram,c,0x04C3u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x04C3u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x04C3: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x04C6u;break;
        case 0x04C6: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x04C9u;break;
        case 0x04C9: al(c,(uint8_t)(0x0078u));c->ip=0x04CBu;break;
        case 0x04CB: push(ram,c,c->ax);c->ip=0x04CCu;break;
        case 0x04CC: push(ram,c,c->cs);c->ip=0x04CDu;break;
        case 0x04CD: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x04CEu)!=0xFC90u)return PR_U9E_0160_CODE;
            push(ram,c,0x04D0u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x04D0u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x04D0: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x04D3u;break;
        case 0x04D3: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x04D6u;break;
        case 0x04D6: al(c,(uint8_t)(0x0065u));c->ip=0x04D8u;break;
        case 0x04D8: push(ram,c,c->ax);c->ip=0x04D9u;break;
        case 0x04D9: push(ram,c,c->cs);c->ip=0x04DAu;break;
        case 0x04DA: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x04DBu)!=0xFC83u)return PR_U9E_0160_CODE;
            push(ram,c,0x04DDu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x04DDu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x04DD: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x04E0u;break;
        case 0x04E0: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x04E3u;break;
        case 0x04E3: al(c,(uint8_t)(0x0073u));c->ip=0x04E5u;break;
        case 0x04E5: push(ram,c,c->ax);c->ip=0x04E6u;break;
        case 0x04E6: push(ram,c,c->cs);c->ip=0x04E7u;break;
        case 0x04E7: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x04E8u)!=0xFC76u)return PR_U9E_0160_CODE;
            push(ram,c,0x04EAu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x04EAu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x04EA: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x04EDu;break;
        case 0x04ED: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x04F0u;break;
        case 0x04F0: al(c,(uint8_t)(0x0074u));c->ip=0x04F2u;break;
        case 0x04F2: push(ram,c,c->ax);c->ip=0x04F3u;break;
        case 0x04F3: push(ram,c,c->cs);c->ip=0x04F4u;break;
        case 0x04F4: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x04F5u)!=0xFC69u)return PR_U9E_0160_CODE;
            push(ram,c,0x04F7u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x04F7u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x04F7: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x04FAu;break;
        case 0x04FA: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x04FDu;break;
        case 0x04FD: al(c,(uint8_t)(0x0077u));c->ip=0x04FFu;break;
        case 0x04FF: push(ram,c,c->ax);c->ip=0x0500u;break;
        case 0x0500: push(ram,c,c->cs);c->ip=0x0501u;break;
        case 0x0501: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0502u)!=0xFC5Cu)return PR_U9E_0160_CODE;
            push(ram,c,0x0504u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0504u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0504: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0507u;break;
        case 0x0507: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x050Au;break;
        case 0x050A: al(c,(uint8_t)(0x007Bu));c->ip=0x050Cu;break;
        case 0x050C: push(ram,c,c->ax);c->ip=0x050Du;break;
        case 0x050D: push(ram,c,c->cs);c->ip=0x050Eu;break;
        case 0x050E: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x050Fu)!=0xFC4Fu)return PR_U9E_0160_CODE;
            push(ram,c,0x0511u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0511u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0511: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0514u;break;
        case 0x0514: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0517u;break;
        case 0x0517: al(c,(uint8_t)(0x0060u));c->ip=0x0519u;break;
        case 0x0519: push(ram,c,c->ax);c->ip=0x051Au;break;
        case 0x051A: push(ram,c,c->cs);c->ip=0x051Bu;break;
        case 0x051B: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x051Cu)!=0xFC42u)return PR_U9E_0160_CODE;
            push(ram,c,0x051Eu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x051Eu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x051E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0521u;break;
        case 0x0521: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0524u;break;
        case 0x0524: al(c,(uint8_t)(0x005Eu));c->ip=0x0526u;break;
        case 0x0526: push(ram,c,c->ax);c->ip=0x0527u;break;
        case 0x0527: push(ram,c,c->cs);c->ip=0x0528u;break;
        case 0x0528: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0529u)!=0xFC35u)return PR_U9E_0160_CODE;
            push(ram,c,0x052Bu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x052Bu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x052B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x052Eu;break;
        case 0x052E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0531u;break;
        case 0x0531: al(c,(uint8_t)(0x003Cu));c->ip=0x0533u;break;
        case 0x0533: push(ram,c,c->ax);c->ip=0x0534u;break;
        case 0x0534: push(ram,c,c->cs);c->ip=0x0535u;break;
        case 0x0535: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0536u)!=0xFC28u)return PR_U9E_0160_CODE;
            push(ram,c,0x0538u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0538u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0538: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x053Bu;break;
        case 0x053B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x053Eu;break;
        case 0x053E: al(c,(uint8_t)(0x007Au));c->ip=0x0540u;break;
        case 0x0540: push(ram,c,c->ax);c->ip=0x0541u;break;
        case 0x0541: push(ram,c,c->cs);c->ip=0x0542u;break;
        case 0x0542: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0543u)!=0xFC1Bu)return PR_U9E_0160_CODE;
            push(ram,c,0x0545u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0545u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0545: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0548u;break;
        case 0x0548: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x054Bu;break;
        case 0x054B: al(c,(uint8_t)(0x0075u));c->ip=0x054Du;break;
        case 0x054D: push(ram,c,c->ax);c->ip=0x054Eu;break;
        case 0x054E: push(ram,c,c->cs);c->ip=0x054Fu;break;
        case 0x054F: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0550u)!=0xFC0Eu)return PR_U9E_0160_CODE;
            push(ram,c,0x0552u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0552u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0552: c->ip=0x0AB5u;break;
        case 0x0555: (void)arithmetic(c,(uint8_t)c->ax,0x0006u,0u,8u,1u);c->ip=0x0557u;break;
        case 0x0557: c->ip=((c->flags&0x40u)!=0u)?0x055Cu:0x0559u;break;
        case 0x0559: c->ip=0x062Fu;break;
        case 0x055C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x055Fu;break;
        case 0x055F: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0562u;break;
        case 0x0562: al(c,(uint8_t)(0x0071u));c->ip=0x0564u;break;
        case 0x0564: push(ram,c,c->ax);c->ip=0x0565u;break;
        case 0x0565: push(ram,c,c->cs);c->ip=0x0566u;break;
        case 0x0566: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0567u)!=0xFBF7u)return PR_U9E_0160_CODE;
            push(ram,c,0x0569u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0569u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0569: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x056Cu;break;
        case 0x056C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x056Fu;break;
        case 0x056F: al(c,(uint8_t)(0x003Du));c->ip=0x0571u;break;
        case 0x0571: push(ram,c,c->ax);c->ip=0x0572u;break;
        case 0x0572: push(ram,c,c->cs);c->ip=0x0573u;break;
        case 0x0573: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0574u)!=0xFBEAu)return PR_U9E_0160_CODE;
            push(ram,c,0x0576u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0576u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0576: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0579u;break;
        case 0x0579: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x057Cu;break;
        case 0x057C: al(c,(uint8_t)(0x007Au));c->ip=0x057Eu;break;
        case 0x057E: push(ram,c,c->ax);c->ip=0x057Fu;break;
        case 0x057F: push(ram,c,c->cs);c->ip=0x0580u;break;
        case 0x0580: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0581u)!=0xFBDDu)return PR_U9E_0160_CODE;
            push(ram,c,0x0583u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0583u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0583: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0586u;break;
        case 0x0586: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0589u;break;
        case 0x0589: al(c,(uint8_t)(0x003Cu));c->ip=0x058Bu;break;
        case 0x058B: push(ram,c,c->ax);c->ip=0x058Cu;break;
        case 0x058C: push(ram,c,c->cs);c->ip=0x058Du;break;
        case 0x058D: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x058Eu)!=0xFBD0u)return PR_U9E_0160_CODE;
            push(ram,c,0x0590u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0590u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0590: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0593u;break;
        case 0x0593: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0596u;break;
        case 0x0596: al(c,(uint8_t)(0x005Bu));c->ip=0x0598u;break;
        case 0x0598: push(ram,c,c->ax);c->ip=0x0599u;break;
        case 0x0599: push(ram,c,c->cs);c->ip=0x059Au;break;
        case 0x059A: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x059Bu)!=0xFBC3u)return PR_U9E_0160_CODE;
            push(ram,c,0x059Du);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x059Du)return PR_U9E_0160_TRANSFER;
            break;
        case 0x059D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x05A0u;break;
        case 0x05A0: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x05A3u;break;
        case 0x05A3: al(c,(uint8_t)(0x000Au));c->ip=0x05A5u;break;
        case 0x05A5: push(ram,c,c->ax);c->ip=0x05A6u;break;
        case 0x05A6: push(ram,c,c->cs);c->ip=0x05A7u;break;
        case 0x05A7: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x05A8u)!=0xFBB6u)return PR_U9E_0160_CODE;
            push(ram,c,0x05AAu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x05AAu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x05AA: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x05ADu;break;
        case 0x05AD: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x05B0u;break;
        case 0x05B0: al(c,(uint8_t)(0x0014u));c->ip=0x05B2u;break;
        case 0x05B2: push(ram,c,c->ax);c->ip=0x05B3u;break;
        case 0x05B3: push(ram,c,c->cs);c->ip=0x05B4u;break;
        case 0x05B4: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x05B5u)!=0xFBA9u)return PR_U9E_0160_CODE;
            push(ram,c,0x05B7u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x05B7u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x05B7: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x05BAu;break;
        case 0x05BA: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x05BDu;break;
        case 0x05BD: al(c,(uint8_t)(0x0069u));c->ip=0x05BFu;break;
        case 0x05BF: push(ram,c,c->ax);c->ip=0x05C0u;break;
        case 0x05C0: push(ram,c,c->cs);c->ip=0x05C1u;break;
        case 0x05C1: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x05C2u)!=0xFB9Cu)return PR_U9E_0160_CODE;
            push(ram,c,0x05C4u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x05C4u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x05C4: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x05C7u;break;
        case 0x05C7: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x05CAu;break;
        case 0x05CA: al(c,(uint8_t)(0x006Au));c->ip=0x05CCu;break;
        case 0x05CC: push(ram,c,c->ax);c->ip=0x05CDu;break;
        case 0x05CD: push(ram,c,c->cs);c->ip=0x05CEu;break;
        case 0x05CE: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x05CFu)!=0xFB8Fu)return PR_U9E_0160_CODE;
            push(ram,c,0x05D1u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x05D1u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x05D1: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x05D4u;break;
        case 0x05D4: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x05D7u;break;
        case 0x05D7: al(c,(uint8_t)(0x0070u));c->ip=0x05D9u;break;
        case 0x05D9: push(ram,c,c->ax);c->ip=0x05DAu;break;
        case 0x05DA: push(ram,c,c->cs);c->ip=0x05DBu;break;
        case 0x05DB: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x05DCu)!=0xFB82u)return PR_U9E_0160_CODE;
            push(ram,c,0x05DEu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x05DEu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x05DE: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x05E1u;break;
        case 0x05E1: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x05E4u;break;
        case 0x05E4: al(c,(uint8_t)(0x0072u));c->ip=0x05E6u;break;
        case 0x05E6: push(ram,c,c->ax);c->ip=0x05E7u;break;
        case 0x05E7: push(ram,c,c->cs);c->ip=0x05E8u;break;
        case 0x05E8: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x05E9u)!=0xFB75u)return PR_U9E_0160_CODE;
            push(ram,c,0x05EBu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x05EBu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x05EB: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x05EEu;break;
        case 0x05EE: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x05F1u;break;
        case 0x05F1: al(c,(uint8_t)(0x0076u));c->ip=0x05F3u;break;
        case 0x05F3: push(ram,c,c->ax);c->ip=0x05F4u;break;
        case 0x05F4: push(ram,c,c->cs);c->ip=0x05F5u;break;
        case 0x05F5: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x05F6u)!=0xFB68u)return PR_U9E_0160_CODE;
            push(ram,c,0x05F8u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x05F8u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x05F8: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x05FBu;break;
        case 0x05FB: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x05FEu;break;
        case 0x05FE: al(c,(uint8_t)(0x0011u));c->ip=0x0600u;break;
        case 0x0600: push(ram,c,c->ax);c->ip=0x0601u;break;
        case 0x0601: push(ram,c,c->cs);c->ip=0x0602u;break;
        case 0x0602: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0603u)!=0xFB5Bu)return PR_U9E_0160_CODE;
            push(ram,c,0x0605u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0605u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0605: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0608u;break;
        case 0x0608: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x060Bu;break;
        case 0x060B: al(c,(uint8_t)(0x005Du));c->ip=0x060Du;break;
        case 0x060D: push(ram,c,c->ax);c->ip=0x060Eu;break;
        case 0x060E: push(ram,c,c->cs);c->ip=0x060Fu;break;
        case 0x060F: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0610u)!=0xFB4Eu)return PR_U9E_0160_CODE;
            push(ram,c,0x0612u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0612u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0612: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0615u;break;
        case 0x0615: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0618u;break;
        case 0x0618: al(c,(uint8_t)(0x0065u));c->ip=0x061Au;break;
        case 0x061A: push(ram,c,c->ax);c->ip=0x061Bu;break;
        case 0x061B: push(ram,c,c->cs);c->ip=0x061Cu;break;
        case 0x061C: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x061Du)!=0xFB41u)return PR_U9E_0160_CODE;
            push(ram,c,0x061Fu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x061Fu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x061F: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0622u;break;
        case 0x0622: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0625u;break;
        case 0x0625: al(c,(uint8_t)(0x001Cu));c->ip=0x0627u;break;
        case 0x0627: push(ram,c,c->ax);c->ip=0x0628u;break;
        case 0x0628: push(ram,c,c->cs);c->ip=0x0629u;break;
        case 0x0629: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x062Au)!=0xFB34u)return PR_U9E_0160_CODE;
            push(ram,c,0x062Cu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x062Cu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x062C: c->ip=0x0AB5u;break;
        case 0x062F: (void)arithmetic(c,(uint8_t)c->ax,0x0007u,0u,8u,1u);c->ip=0x0631u;break;
        case 0x0631: c->ip=((c->flags&0x40u)==0u)?0x066Au:0x0633u;break;
        case 0x0633: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0636u;break;
        case 0x0636: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0639u;break;
        case 0x0639: al(c,(uint8_t)(0x0033u));c->ip=0x063Bu;break;
        case 0x063B: push(ram,c,c->ax);c->ip=0x063Cu;break;
        case 0x063C: push(ram,c,c->cs);c->ip=0x063Du;break;
        case 0x063D: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x063Eu)!=0xFB20u)return PR_U9E_0160_CODE;
            push(ram,c,0x0640u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0640u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0640: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0643u;break;
        case 0x0643: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0646u;break;
        case 0x0646: al(c,(uint8_t)(0x0034u));c->ip=0x0648u;break;
        case 0x0648: push(ram,c,c->ax);c->ip=0x0649u;break;
        case 0x0649: push(ram,c,c->cs);c->ip=0x064Au;break;
        case 0x064A: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x064Bu)!=0xFB13u)return PR_U9E_0160_CODE;
            push(ram,c,0x064Du);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x064Du)return PR_U9E_0160_TRANSFER;
            break;
        case 0x064D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0650u;break;
        case 0x0650: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0653u;break;
        case 0x0653: al(c,(uint8_t)(0x0035u));c->ip=0x0655u;break;
        case 0x0655: push(ram,c,c->ax);c->ip=0x0656u;break;
        case 0x0656: push(ram,c,c->cs);c->ip=0x0657u;break;
        case 0x0657: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0658u)!=0xFB06u)return PR_U9E_0160_CODE;
            push(ram,c,0x065Au);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x065Au)return PR_U9E_0160_TRANSFER;
            break;
        case 0x065A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x065Du;break;
        case 0x065D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0660u;break;
        case 0x0660: al(c,(uint8_t)(0x001Fu));c->ip=0x0662u;break;
        case 0x0662: push(ram,c,c->ax);c->ip=0x0663u;break;
        case 0x0663: push(ram,c,c->cs);c->ip=0x0664u;break;
        case 0x0664: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0665u)!=0xFAF9u)return PR_U9E_0160_CODE;
            push(ram,c,0x0667u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0667u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0667: c->ip=0x0AB5u;break;
        case 0x066A: (void)arithmetic(c,(uint8_t)c->ax,0x0008u,0u,8u,1u);c->ip=0x066Cu;break;
        case 0x066C: c->ip=((c->flags&0x40u)==0u)?0x06BFu:0x066Eu;break;
        case 0x066E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0671u;break;
        case 0x0671: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0674u;break;
        case 0x0674: al(c,(uint8_t)(0x0063u));c->ip=0x0676u;break;
        case 0x0676: push(ram,c,c->ax);c->ip=0x0677u;break;
        case 0x0677: push(ram,c,c->cs);c->ip=0x0678u;break;
        case 0x0678: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0679u)!=0xFAE5u)return PR_U9E_0160_CODE;
            push(ram,c,0x067Bu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x067Bu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x067B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x067Eu;break;
        case 0x067E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0681u;break;
        case 0x0681: al(c,(uint8_t)(0x0051u));c->ip=0x0683u;break;
        case 0x0683: push(ram,c,c->ax);c->ip=0x0684u;break;
        case 0x0684: push(ram,c,c->cs);c->ip=0x0685u;break;
        case 0x0685: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0686u)!=0xFAD8u)return PR_U9E_0160_CODE;
            push(ram,c,0x0688u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0688u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0688: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x068Bu;break;
        case 0x068B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x068Eu;break;
        case 0x068E: al(c,(uint8_t)(0x0052u));c->ip=0x0690u;break;
        case 0x0690: push(ram,c,c->ax);c->ip=0x0691u;break;
        case 0x0691: push(ram,c,c->cs);c->ip=0x0692u;break;
        case 0x0692: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0693u)!=0xFACBu)return PR_U9E_0160_CODE;
            push(ram,c,0x0695u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0695u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0695: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0698u;break;
        case 0x0698: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x069Bu;break;
        case 0x069B: al(c,(uint8_t)(0x0059u));c->ip=0x069Du;break;
        case 0x069D: push(ram,c,c->ax);c->ip=0x069Eu;break;
        case 0x069E: push(ram,c,c->cs);c->ip=0x069Fu;break;
        case 0x069F: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x06A0u)!=0xFABEu)return PR_U9E_0160_CODE;
            push(ram,c,0x06A2u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x06A2u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x06A2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x06A5u;break;
        case 0x06A5: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x06A8u;break;
        case 0x06A8: al(c,(uint8_t)(0x0048u));c->ip=0x06AAu;break;
        case 0x06AA: push(ram,c,c->ax);c->ip=0x06ABu;break;
        case 0x06AB: push(ram,c,c->cs);c->ip=0x06ACu;break;
        case 0x06AC: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x06ADu)!=0xFAB1u)return PR_U9E_0160_CODE;
            push(ram,c,0x06AFu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x06AFu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x06AF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x06B2u;break;
        case 0x06B2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x06B5u;break;
        case 0x06B5: al(c,(uint8_t)(0x0038u));c->ip=0x06B7u;break;
        case 0x06B7: push(ram,c,c->ax);c->ip=0x06B8u;break;
        case 0x06B8: push(ram,c,c->cs);c->ip=0x06B9u;break;
        case 0x06B9: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x06BAu)!=0xFAA4u)return PR_U9E_0160_CODE;
            push(ram,c,0x06BCu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x06BCu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x06BC: c->ip=0x0AB5u;break;
        case 0x06BF: (void)arithmetic(c,(uint8_t)c->ax,0x0009u,0u,8u,1u);c->ip=0x06C1u;break;
        case 0x06C1: c->ip=((c->flags&0x40u)!=0u)?0x06C6u:0x06C3u;break;
        case 0x06C3: c->ip=0x074Bu;break;
        case 0x06C6: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x06C9u;break;
        case 0x06C9: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x06CCu;break;
        case 0x06CC: al(c,(uint8_t)(0x0069u));c->ip=0x06CEu;break;
        case 0x06CE: push(ram,c,c->ax);c->ip=0x06CFu;break;
        case 0x06CF: push(ram,c,c->cs);c->ip=0x06D0u;break;
        case 0x06D0: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x06D1u)!=0xFA8Du)return PR_U9E_0160_CODE;
            push(ram,c,0x06D3u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x06D3u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x06D3: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x06D6u;break;
        case 0x06D6: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x06D9u;break;
        case 0x06D9: al(c,(uint8_t)(0x006Au));c->ip=0x06DBu;break;
        case 0x06DB: push(ram,c,c->ax);c->ip=0x06DCu;break;
        case 0x06DC: push(ram,c,c->cs);c->ip=0x06DDu;break;
        case 0x06DD: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x06DEu)!=0xFA80u)return PR_U9E_0160_CODE;
            push(ram,c,0x06E0u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x06E0u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x06E0: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x06E3u;break;
        case 0x06E3: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x06E6u;break;
        case 0x06E6: al(c,(uint8_t)(0x006Bu));c->ip=0x06E8u;break;
        case 0x06E8: push(ram,c,c->ax);c->ip=0x06E9u;break;
        case 0x06E9: push(ram,c,c->cs);c->ip=0x06EAu;break;
        case 0x06EA: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x06EBu)!=0xFA73u)return PR_U9E_0160_CODE;
            push(ram,c,0x06EDu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x06EDu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x06ED: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x06F0u;break;
        case 0x06F0: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x06F3u;break;
        case 0x06F3: al(c,(uint8_t)(0x006Cu));c->ip=0x06F5u;break;
        case 0x06F5: push(ram,c,c->ax);c->ip=0x06F6u;break;
        case 0x06F6: push(ram,c,c->cs);c->ip=0x06F7u;break;
        case 0x06F7: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x06F8u)!=0xFA66u)return PR_U9E_0160_CODE;
            push(ram,c,0x06FAu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x06FAu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x06FA: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x06FDu;break;
        case 0x06FD: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0700u;break;
        case 0x0700: al(c,(uint8_t)(0x006Du));c->ip=0x0702u;break;
        case 0x0702: push(ram,c,c->ax);c->ip=0x0703u;break;
        case 0x0703: push(ram,c,c->cs);c->ip=0x0704u;break;
        case 0x0704: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0705u)!=0xFA59u)return PR_U9E_0160_CODE;
            push(ram,c,0x0707u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0707u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0707: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x070Au;break;
        case 0x070A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x070Du;break;
        case 0x070D: al(c,(uint8_t)(0x006Eu));c->ip=0x070Fu;break;
        case 0x070F: push(ram,c,c->ax);c->ip=0x0710u;break;
        case 0x0710: push(ram,c,c->cs);c->ip=0x0711u;break;
        case 0x0711: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0712u)!=0xFA4Cu)return PR_U9E_0160_CODE;
            push(ram,c,0x0714u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0714u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0714: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0717u;break;
        case 0x0717: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x071Au;break;
        case 0x071A: al(c,(uint8_t)(0x006Fu));c->ip=0x071Cu;break;
        case 0x071C: push(ram,c,c->ax);c->ip=0x071Du;break;
        case 0x071D: push(ram,c,c->cs);c->ip=0x071Eu;break;
        case 0x071E: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x071Fu)!=0xFA3Fu)return PR_U9E_0160_CODE;
            push(ram,c,0x0721u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0721u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0721: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0724u;break;
        case 0x0724: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0727u;break;
        case 0x0727: al(c,(uint8_t)(0x0070u));c->ip=0x0729u;break;
        case 0x0729: push(ram,c,c->ax);c->ip=0x072Au;break;
        case 0x072A: push(ram,c,c->cs);c->ip=0x072Bu;break;
        case 0x072B: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x072Cu)!=0xFA32u)return PR_U9E_0160_CODE;
            push(ram,c,0x072Eu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x072Eu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x072E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0731u;break;
        case 0x0731: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0734u;break;
        case 0x0734: al(c,(uint8_t)(0x007Cu));c->ip=0x0736u;break;
        case 0x0736: push(ram,c,c->ax);c->ip=0x0737u;break;
        case 0x0737: push(ram,c,c->cs);c->ip=0x0738u;break;
        case 0x0738: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0739u)!=0xFA25u)return PR_U9E_0160_CODE;
            push(ram,c,0x073Bu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x073Bu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x073B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x073Eu;break;
        case 0x073E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0741u;break;
        case 0x0741: al(c,(uint8_t)(0x007Du));c->ip=0x0743u;break;
        case 0x0743: push(ram,c,c->ax);c->ip=0x0744u;break;
        case 0x0744: push(ram,c,c->cs);c->ip=0x0745u;break;
        case 0x0745: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0746u)!=0xFA18u)return PR_U9E_0160_CODE;
            push(ram,c,0x0748u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0748u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0748: c->ip=0x0AB5u;break;
        case 0x074B: (void)arithmetic(c,(uint8_t)c->ax,0x000Au,0u,8u,1u);c->ip=0x074Du;break;
        case 0x074D: c->ip=((c->flags&0x40u)==0u)?0x07C7u:0x074Fu;break;
        case 0x074F: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0752u;break;
        case 0x0752: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0755u;break;
        case 0x0755: al(c,(uint8_t)(0x0001u));c->ip=0x0757u;break;
        case 0x0757: push(ram,c,c->ax);c->ip=0x0758u;break;
        case 0x0758: push(ram,c,c->cs);c->ip=0x0759u;break;
        case 0x0759: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x075Au)!=0xFA04u)return PR_U9E_0160_CODE;
            push(ram,c,0x075Cu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x075Cu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x075C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x075Fu;break;
        case 0x075F: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0762u;break;
        case 0x0762: al(c,(uint8_t)(0x0002u));c->ip=0x0764u;break;
        case 0x0764: push(ram,c,c->ax);c->ip=0x0765u;break;
        case 0x0765: push(ram,c,c->cs);c->ip=0x0766u;break;
        case 0x0766: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0767u)!=0xF9F7u)return PR_U9E_0160_CODE;
            push(ram,c,0x0769u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0769u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0769: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x076Cu;break;
        case 0x076C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x076Fu;break;
        case 0x076F: al(c,(uint8_t)(0x0021u));c->ip=0x0771u;break;
        case 0x0771: push(ram,c,c->ax);c->ip=0x0772u;break;
        case 0x0772: push(ram,c,c->cs);c->ip=0x0773u;break;
        case 0x0773: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0774u)!=0xF9EAu)return PR_U9E_0160_CODE;
            push(ram,c,0x0776u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0776u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0776: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0779u;break;
        case 0x0779: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x077Cu;break;
        case 0x077C: al(c,(uint8_t)(0x0024u));c->ip=0x077Eu;break;
        case 0x077E: push(ram,c,c->ax);c->ip=0x077Fu;break;
        case 0x077F: push(ram,c,c->cs);c->ip=0x0780u;break;
        case 0x0780: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0781u)!=0xF9DDu)return PR_U9E_0160_CODE;
            push(ram,c,0x0783u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0783u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0783: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0786u;break;
        case 0x0786: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0789u;break;
        case 0x0789: al(c,(uint8_t)(0x0031u));c->ip=0x078Bu;break;
        case 0x078B: push(ram,c,c->ax);c->ip=0x078Cu;break;
        case 0x078C: push(ram,c,c->cs);c->ip=0x078Du;break;
        case 0x078D: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x078Eu)!=0xF9D0u)return PR_U9E_0160_CODE;
            push(ram,c,0x0790u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0790u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0790: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0793u;break;
        case 0x0793: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0796u;break;
        case 0x0796: al(c,(uint8_t)(0x0003u));c->ip=0x0798u;break;
        case 0x0798: push(ram,c,c->ax);c->ip=0x0799u;break;
        case 0x0799: push(ram,c,c->cs);c->ip=0x079Au;break;
        case 0x079A: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x079Bu)!=0xF9C3u)return PR_U9E_0160_CODE;
            push(ram,c,0x079Du);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x079Du)return PR_U9E_0160_TRANSFER;
            break;
        case 0x079D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x07A0u;break;
        case 0x07A0: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x07A3u;break;
        case 0x07A3: al(c,(uint8_t)(0x0006u));c->ip=0x07A5u;break;
        case 0x07A5: push(ram,c,c->ax);c->ip=0x07A6u;break;
        case 0x07A6: push(ram,c,c->cs);c->ip=0x07A7u;break;
        case 0x07A7: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x07A8u)!=0xF9B6u)return PR_U9E_0160_CODE;
            push(ram,c,0x07AAu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x07AAu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x07AA: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x07ADu;break;
        case 0x07AD: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x07B0u;break;
        case 0x07B0: al(c,(uint8_t)(0x0012u));c->ip=0x07B2u;break;
        case 0x07B2: push(ram,c,c->ax);c->ip=0x07B3u;break;
        case 0x07B3: push(ram,c,c->cs);c->ip=0x07B4u;break;
        case 0x07B4: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x07B5u)!=0xF9A9u)return PR_U9E_0160_CODE;
            push(ram,c,0x07B7u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x07B7u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x07B7: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x07BAu;break;
        case 0x07BA: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x07BDu;break;
        case 0x07BD: al(c,(uint8_t)(0x001Au));c->ip=0x07BFu;break;
        case 0x07BF: push(ram,c,c->ax);c->ip=0x07C0u;break;
        case 0x07C0: push(ram,c,c->cs);c->ip=0x07C1u;break;
        case 0x07C1: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x07C2u)!=0xF99Cu)return PR_U9E_0160_CODE;
            push(ram,c,0x07C4u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x07C4u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x07C4: c->ip=0x0AB5u;break;
        case 0x07C7: (void)arithmetic(c,(uint8_t)c->ax,0x000Bu,0u,8u,1u);c->ip=0x07C9u;break;
        case 0x07C9: c->ip=((c->flags&0x40u)==0u)?0x0829u:0x07CBu;break;
        case 0x07CB: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x07CEu;break;
        case 0x07CE: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x07D1u;break;
        case 0x07D1: al(c,(uint8_t)(0x0021u));c->ip=0x07D3u;break;
        case 0x07D3: push(ram,c,c->ax);c->ip=0x07D4u;break;
        case 0x07D4: push(ram,c,c->cs);c->ip=0x07D5u;break;
        case 0x07D5: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x07D6u)!=0xF988u)return PR_U9E_0160_CODE;
            push(ram,c,0x07D8u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x07D8u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x07D8: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x07DBu;break;
        case 0x07DB: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x07DEu;break;
        case 0x07DE: al(c,(uint8_t)(0x0011u));c->ip=0x07E0u;break;
        case 0x07E0: push(ram,c,c->ax);c->ip=0x07E1u;break;
        case 0x07E1: push(ram,c,c->cs);c->ip=0x07E2u;break;
        case 0x07E2: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x07E3u)!=0xF97Bu)return PR_U9E_0160_CODE;
            push(ram,c,0x07E5u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x07E5u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x07E5: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x07E8u;break;
        case 0x07E8: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x07EBu;break;
        case 0x07EB: al(c,(uint8_t)(0x0008u));c->ip=0x07EDu;break;
        case 0x07ED: push(ram,c,c->ax);c->ip=0x07EEu;break;
        case 0x07EE: push(ram,c,c->cs);c->ip=0x07EFu;break;
        case 0x07EF: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x07F0u)!=0xF96Eu)return PR_U9E_0160_CODE;
            push(ram,c,0x07F2u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x07F2u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x07F2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x07F5u;break;
        case 0x07F5: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x07F8u;break;
        case 0x07F8: al(c,(uint8_t)(0x0009u));c->ip=0x07FAu;break;
        case 0x07FA: push(ram,c,c->ax);c->ip=0x07FBu;break;
        case 0x07FB: push(ram,c,c->cs);c->ip=0x07FCu;break;
        case 0x07FC: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x07FDu)!=0xF961u)return PR_U9E_0160_CODE;
            push(ram,c,0x07FFu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x07FFu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x07FF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0802u;break;
        case 0x0802: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0805u;break;
        case 0x0805: al(c,(uint8_t)(0x002Du));c->ip=0x0807u;break;
        case 0x0807: push(ram,c,c->ax);c->ip=0x0808u;break;
        case 0x0808: push(ram,c,c->cs);c->ip=0x0809u;break;
        case 0x0809: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x080Au)!=0xF954u)return PR_U9E_0160_CODE;
            push(ram,c,0x080Cu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x080Cu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x080C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x080Fu;break;
        case 0x080F: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0812u;break;
        case 0x0812: al(c,(uint8_t)(0x002Eu));c->ip=0x0814u;break;
        case 0x0814: push(ram,c,c->ax);c->ip=0x0815u;break;
        case 0x0815: push(ram,c,c->cs);c->ip=0x0816u;break;
        case 0x0816: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0817u)!=0xF947u)return PR_U9E_0160_CODE;
            push(ram,c,0x0819u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0819u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0819: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x081Cu;break;
        case 0x081C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x081Fu;break;
        case 0x081F: al(c,(uint8_t)(0x001Eu));c->ip=0x0821u;break;
        case 0x0821: push(ram,c,c->ax);c->ip=0x0822u;break;
        case 0x0822: push(ram,c,c->cs);c->ip=0x0823u;break;
        case 0x0823: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0824u)!=0xF93Au)return PR_U9E_0160_CODE;
            push(ram,c,0x0826u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0826u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0826: c->ip=0x0AB5u;break;
        case 0x0829: (void)arithmetic(c,(uint8_t)c->ax,0x000Cu,0u,8u,1u);c->ip=0x082Bu;break;
        case 0x082B: c->ip=((c->flags&0x40u)!=0u)?0x0830u:0x082Du;break;
        case 0x082D: c->ip=0x08F6u;break;
        case 0x0830: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0833u;break;
        case 0x0833: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0836u;break;
        case 0x0836: al(c,(uint8_t)(0x0008u));c->ip=0x0838u;break;
        case 0x0838: push(ram,c,c->ax);c->ip=0x0839u;break;
        case 0x0839: push(ram,c,c->cs);c->ip=0x083Au;break;
        case 0x083A: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x083Bu)!=0xF923u)return PR_U9E_0160_CODE;
            push(ram,c,0x083Du);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x083Du)return PR_U9E_0160_TRANSFER;
            break;
        case 0x083D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0840u;break;
        case 0x0840: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0843u;break;
        case 0x0843: al(c,(uint8_t)(0x0009u));c->ip=0x0845u;break;
        case 0x0845: push(ram,c,c->ax);c->ip=0x0846u;break;
        case 0x0846: push(ram,c,c->cs);c->ip=0x0847u;break;
        case 0x0847: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0848u)!=0xF916u)return PR_U9E_0160_CODE;
            push(ram,c,0x084Au);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x084Au)return PR_U9E_0160_TRANSFER;
            break;
        case 0x084A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x084Du;break;
        case 0x084D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0850u;break;
        case 0x0850: al(c,(uint8_t)(0x000Au));c->ip=0x0852u;break;
        case 0x0852: push(ram,c,c->ax);c->ip=0x0853u;break;
        case 0x0853: push(ram,c,c->cs);c->ip=0x0854u;break;
        case 0x0854: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0855u)!=0xF909u)return PR_U9E_0160_CODE;
            push(ram,c,0x0857u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0857u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0857: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x085Au;break;
        case 0x085A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x085Du;break;
        case 0x085D: al(c,(uint8_t)(0x0011u));c->ip=0x085Fu;break;
        case 0x085F: push(ram,c,c->ax);c->ip=0x0860u;break;
        case 0x0860: push(ram,c,c->cs);c->ip=0x0861u;break;
        case 0x0861: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0862u)!=0xF8FCu)return PR_U9E_0160_CODE;
            push(ram,c,0x0864u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0864u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0864: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0867u;break;
        case 0x0867: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x086Au;break;
        case 0x086A: al(c,(uint8_t)(0x0014u));c->ip=0x086Cu;break;
        case 0x086C: push(ram,c,c->ax);c->ip=0x086Du;break;
        case 0x086D: push(ram,c,c->cs);c->ip=0x086Eu;break;
        case 0x086E: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x086Fu)!=0xF8EFu)return PR_U9E_0160_CODE;
            push(ram,c,0x0871u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0871u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0871: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0874u;break;
        case 0x0874: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0877u;break;
        case 0x0877: al(c,(uint8_t)(0x0021u));c->ip=0x0879u;break;
        case 0x0879: push(ram,c,c->ax);c->ip=0x087Au;break;
        case 0x087A: push(ram,c,c->cs);c->ip=0x087Bu;break;
        case 0x087B: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x087Cu)!=0xF8E2u)return PR_U9E_0160_CODE;
            push(ram,c,0x087Eu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x087Eu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x087E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0881u;break;
        case 0x0881: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0884u;break;
        case 0x0884: al(c,(uint8_t)(0x0024u));c->ip=0x0886u;break;
        case 0x0886: push(ram,c,c->ax);c->ip=0x0887u;break;
        case 0x0887: push(ram,c,c->cs);c->ip=0x0888u;break;
        case 0x0888: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0889u)!=0xF8D5u)return PR_U9E_0160_CODE;
            push(ram,c,0x088Bu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x088Bu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x088B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x088Eu;break;
        case 0x088E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0891u;break;
        case 0x0891: al(c,(uint8_t)(0x002Du));c->ip=0x0893u;break;
        case 0x0893: push(ram,c,c->ax);c->ip=0x0894u;break;
        case 0x0894: push(ram,c,c->cs);c->ip=0x0895u;break;
        case 0x0895: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0896u)!=0xF8C8u)return PR_U9E_0160_CODE;
            push(ram,c,0x0898u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0898u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0898: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x089Bu;break;
        case 0x089B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x089Eu;break;
        case 0x089E: al(c,(uint8_t)(0x002Eu));c->ip=0x08A0u;break;
        case 0x08A0: push(ram,c,c->ax);c->ip=0x08A1u;break;
        case 0x08A1: push(ram,c,c->cs);c->ip=0x08A2u;break;
        case 0x08A2: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x08A3u)!=0xF8BBu)return PR_U9E_0160_CODE;
            push(ram,c,0x08A5u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x08A5u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x08A5: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x08A8u;break;
        case 0x08A8: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x08ABu;break;
        case 0x08AB: al(c,(uint8_t)(0x0031u));c->ip=0x08ADu;break;
        case 0x08AD: push(ram,c,c->ax);c->ip=0x08AEu;break;
        case 0x08AE: push(ram,c,c->cs);c->ip=0x08AFu;break;
        case 0x08AF: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x08B0u)!=0xF8AEu)return PR_U9E_0160_CODE;
            push(ram,c,0x08B2u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x08B2u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x08B2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x08B5u;break;
        case 0x08B5: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x08B8u;break;
        case 0x08B8: al(c,(uint8_t)(0x003Du));c->ip=0x08BAu;break;
        case 0x08BA: push(ram,c,c->ax);c->ip=0x08BBu;break;
        case 0x08BB: push(ram,c,c->cs);c->ip=0x08BCu;break;
        case 0x08BC: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x08BDu)!=0xF8A1u)return PR_U9E_0160_CODE;
            push(ram,c,0x08BFu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x08BFu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x08BF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x08C2u;break;
        case 0x08C2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x08C5u;break;
        case 0x08C5: al(c,(uint8_t)(0x006Fu));c->ip=0x08C7u;break;
        case 0x08C7: push(ram,c,c->ax);c->ip=0x08C8u;break;
        case 0x08C8: push(ram,c,c->cs);c->ip=0x08C9u;break;
        case 0x08C9: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x08CAu)!=0xF894u)return PR_U9E_0160_CODE;
            push(ram,c,0x08CCu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x08CCu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x08CC: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x08CFu;break;
        case 0x08CF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x08D2u;break;
        case 0x08D2: al(c,(uint8_t)(0x007Du));c->ip=0x08D4u;break;
        case 0x08D4: push(ram,c,c->ax);c->ip=0x08D5u;break;
        case 0x08D5: push(ram,c,c->cs);c->ip=0x08D6u;break;
        case 0x08D6: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x08D7u)!=0xF887u)return PR_U9E_0160_CODE;
            push(ram,c,0x08D9u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x08D9u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x08D9: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x08DCu;break;
        case 0x08DC: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x08DFu;break;
        case 0x08DF: al(c,(uint8_t)(0x005Au));c->ip=0x08E1u;break;
        case 0x08E1: push(ram,c,c->ax);c->ip=0x08E2u;break;
        case 0x08E2: push(ram,c,c->cs);c->ip=0x08E3u;break;
        case 0x08E3: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x08E4u)!=0xF87Au)return PR_U9E_0160_CODE;
            push(ram,c,0x08E6u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x08E6u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x08E6: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x08E9u;break;
        case 0x08E9: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x08ECu;break;
        case 0x08EC: al(c,(uint8_t)(0x0061u));c->ip=0x08EEu;break;
        case 0x08EE: push(ram,c,c->ax);c->ip=0x08EFu;break;
        case 0x08EF: push(ram,c,c->cs);c->ip=0x08F0u;break;
        case 0x08F0: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x08F1u)!=0xF86Du)return PR_U9E_0160_CODE;
            push(ram,c,0x08F3u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x08F3u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x08F3: c->ip=0x0AB5u;break;
        case 0x08F6: (void)arithmetic(c,(uint8_t)c->ax,0x000Du,0u,8u,1u);c->ip=0x08F8u;break;
        case 0x08F8: c->ip=((c->flags&0x40u)==0u)?0x093Eu:0x08FAu;break;
        case 0x08FA: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x08FDu;break;
        case 0x08FD: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0900u;break;
        case 0x0900: al(c,(uint8_t)(0x0063u));c->ip=0x0902u;break;
        case 0x0902: push(ram,c,c->ax);c->ip=0x0903u;break;
        case 0x0903: push(ram,c,c->cs);c->ip=0x0904u;break;
        case 0x0904: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0905u)!=0xF859u)return PR_U9E_0160_CODE;
            push(ram,c,0x0907u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0907u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0907: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x090Au;break;
        case 0x090A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x090Du;break;
        case 0x090D: al(c,(uint8_t)(0x0064u));c->ip=0x090Fu;break;
        case 0x090F: push(ram,c,c->ax);c->ip=0x0910u;break;
        case 0x0910: push(ram,c,c->cs);c->ip=0x0911u;break;
        case 0x0911: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0912u)!=0xF84Cu)return PR_U9E_0160_CODE;
            push(ram,c,0x0914u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0914u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0914: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0917u;break;
        case 0x0917: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x091Au;break;
        case 0x091A: al(c,(uint8_t)(0x0067u));c->ip=0x091Cu;break;
        case 0x091C: push(ram,c,c->ax);c->ip=0x091Du;break;
        case 0x091D: push(ram,c,c->cs);c->ip=0x091Eu;break;
        case 0x091E: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x091Fu)!=0xF83Fu)return PR_U9E_0160_CODE;
            push(ram,c,0x0921u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0921u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0921: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0924u;break;
        case 0x0924: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0927u;break;
        case 0x0927: al(c,(uint8_t)(0x004Bu));c->ip=0x0929u;break;
        case 0x0929: push(ram,c,c->ax);c->ip=0x092Au;break;
        case 0x092A: push(ram,c,c->cs);c->ip=0x092Bu;break;
        case 0x092B: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x092Cu)!=0xF832u)return PR_U9E_0160_CODE;
            push(ram,c,0x092Eu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x092Eu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x092E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0931u;break;
        case 0x0931: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0934u;break;
        case 0x0934: al(c,(uint8_t)(0x004Au));c->ip=0x0936u;break;
        case 0x0936: push(ram,c,c->ax);c->ip=0x0937u;break;
        case 0x0937: push(ram,c,c->cs);c->ip=0x0938u;break;
        case 0x0938: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0939u)!=0xF825u)return PR_U9E_0160_CODE;
            push(ram,c,0x093Bu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x093Bu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x093B: c->ip=0x0AB5u;break;
        case 0x093E: (void)arithmetic(c,(uint8_t)c->ax,0x000Eu,0u,8u,1u);c->ip=0x0940u;break;
        case 0x0940: c->ip=((c->flags&0x40u)==0u)?0x0979u:0x0942u;break;
        case 0x0942: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0945u;break;
        case 0x0945: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0948u;break;
        case 0x0948: al(c,(uint8_t)(0x0053u));c->ip=0x094Au;break;
        case 0x094A: push(ram,c,c->ax);c->ip=0x094Bu;break;
        case 0x094B: push(ram,c,c->cs);c->ip=0x094Cu;break;
        case 0x094C: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x094Du)!=0xF811u)return PR_U9E_0160_CODE;
            push(ram,c,0x094Fu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x094Fu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x094F: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0952u;break;
        case 0x0952: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0955u;break;
        case 0x0955: al(c,(uint8_t)(0x0054u));c->ip=0x0957u;break;
        case 0x0957: push(ram,c,c->ax);c->ip=0x0958u;break;
        case 0x0958: push(ram,c,c->cs);c->ip=0x0959u;break;
        case 0x0959: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x095Au)!=0xF804u)return PR_U9E_0160_CODE;
            push(ram,c,0x095Cu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x095Cu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x095C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x095Fu;break;
        case 0x095F: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0962u;break;
        case 0x0962: al(c,(uint8_t)(0x0058u));c->ip=0x0964u;break;
        case 0x0964: push(ram,c,c->ax);c->ip=0x0965u;break;
        case 0x0965: push(ram,c,c->cs);c->ip=0x0966u;break;
        case 0x0966: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0967u)!=0xF7F7u)return PR_U9E_0160_CODE;
            push(ram,c,0x0969u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0969u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0969: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x096Cu;break;
        case 0x096C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x096Fu;break;
        case 0x096F: al(c,(uint8_t)(0x0079u));c->ip=0x0971u;break;
        case 0x0971: push(ram,c,c->ax);c->ip=0x0972u;break;
        case 0x0972: push(ram,c,c->cs);c->ip=0x0973u;break;
        case 0x0973: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0974u)!=0xF7EAu)return PR_U9E_0160_CODE;
            push(ram,c,0x0976u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0976u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0976: c->ip=0x0AB5u;break;
        case 0x0979: (void)arithmetic(c,(uint8_t)c->ax,0x000Fu,0u,8u,1u);c->ip=0x097Bu;break;
        case 0x097B: c->ip=((c->flags&0x40u)==0u)?0x09C1u:0x097Du;break;
        case 0x097D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0980u;break;
        case 0x0980: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0983u;break;
        case 0x0983: al(c,(uint8_t)(0x0015u));c->ip=0x0985u;break;
        case 0x0985: push(ram,c,c->ax);c->ip=0x0986u;break;
        case 0x0986: push(ram,c,c->cs);c->ip=0x0987u;break;
        case 0x0987: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0988u)!=0xF7D6u)return PR_U9E_0160_CODE;
            push(ram,c,0x098Au);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x098Au)return PR_U9E_0160_TRANSFER;
            break;
        case 0x098A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x098Du;break;
        case 0x098D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0990u;break;
        case 0x0990: al(c,(uint8_t)(0x001Eu));c->ip=0x0992u;break;
        case 0x0992: push(ram,c,c->ax);c->ip=0x0993u;break;
        case 0x0993: push(ram,c,c->cs);c->ip=0x0994u;break;
        case 0x0994: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0995u)!=0xF7C9u)return PR_U9E_0160_CODE;
            push(ram,c,0x0997u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0997u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0997: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x099Au;break;
        case 0x099A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x099Du;break;
        case 0x099D: al(c,(uint8_t)(0x004Au));c->ip=0x099Fu;break;
        case 0x099F: push(ram,c,c->ax);c->ip=0x09A0u;break;
        case 0x09A0: push(ram,c,c->cs);c->ip=0x09A1u;break;
        case 0x09A1: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x09A2u)!=0xF7BCu)return PR_U9E_0160_CODE;
            push(ram,c,0x09A4u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x09A4u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x09A4: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x09A7u;break;
        case 0x09A7: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x09AAu;break;
        case 0x09AA: al(c,(uint8_t)(0x004Bu));c->ip=0x09ACu;break;
        case 0x09AC: push(ram,c,c->ax);c->ip=0x09ADu;break;
        case 0x09AD: push(ram,c,c->cs);c->ip=0x09AEu;break;
        case 0x09AE: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x09AFu)!=0xF7AFu)return PR_U9E_0160_CODE;
            push(ram,c,0x09B1u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x09B1u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x09B1: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x09B4u;break;
        case 0x09B4: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x09B7u;break;
        case 0x09B7: al(c,(uint8_t)(0x000Bu));c->ip=0x09B9u;break;
        case 0x09B9: push(ram,c,c->ax);c->ip=0x09BAu;break;
        case 0x09BA: push(ram,c,c->cs);c->ip=0x09BBu;break;
        case 0x09BB: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x09BCu)!=0xF7A2u)return PR_U9E_0160_CODE;
            push(ram,c,0x09BEu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x09BEu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x09BE: c->ip=0x0AB5u;break;
        case 0x09C1: (void)arithmetic(c,(uint8_t)c->ax,0x0010u,0u,8u,1u);c->ip=0x09C3u;break;
        case 0x09C3: c->ip=((c->flags&0x40u)==0u)?0x0A16u:0x09C5u;break;
        case 0x09C5: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x09C8u;break;
        case 0x09C8: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x09CBu;break;
        case 0x09CB: al(c,(uint8_t)(0x0019u));c->ip=0x09CDu;break;
        case 0x09CD: push(ram,c,c->ax);c->ip=0x09CEu;break;
        case 0x09CE: push(ram,c,c->cs);c->ip=0x09CFu;break;
        case 0x09CF: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x09D0u)!=0xF78Eu)return PR_U9E_0160_CODE;
            push(ram,c,0x09D2u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x09D2u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x09D2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x09D5u;break;
        case 0x09D5: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x09D8u;break;
        case 0x09D8: al(c,(uint8_t)(0x0047u));c->ip=0x09DAu;break;
        case 0x09DA: push(ram,c,c->ax);c->ip=0x09DBu;break;
        case 0x09DB: push(ram,c,c->cs);c->ip=0x09DCu;break;
        case 0x09DC: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x09DDu)!=0xF781u)return PR_U9E_0160_CODE;
            push(ram,c,0x09DFu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x09DFu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x09DF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x09E2u;break;
        case 0x09E2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x09E5u;break;
        case 0x09E5: al(c,(uint8_t)(0x0025u));c->ip=0x09E7u;break;
        case 0x09E7: push(ram,c,c->ax);c->ip=0x09E8u;break;
        case 0x09E8: push(ram,c,c->cs);c->ip=0x09E9u;break;
        case 0x09E9: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x09EAu)!=0xF774u)return PR_U9E_0160_CODE;
            push(ram,c,0x09ECu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x09ECu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x09EC: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x09EFu;break;
        case 0x09EF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x09F2u;break;
        case 0x09F2: al(c,(uint8_t)(0x002Fu));c->ip=0x09F4u;break;
        case 0x09F4: push(ram,c,c->ax);c->ip=0x09F5u;break;
        case 0x09F5: push(ram,c,c->cs);c->ip=0x09F6u;break;
        case 0x09F6: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x09F7u)!=0xF767u)return PR_U9E_0160_CODE;
            push(ram,c,0x09F9u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x09F9u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x09F9: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x09FCu;break;
        case 0x09FC: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x09FFu;break;
        case 0x09FF: al(c,(uint8_t)(0x0030u));c->ip=0x0A01u;break;
        case 0x0A01: push(ram,c,c->ax);c->ip=0x0A02u;break;
        case 0x0A02: push(ram,c,c->cs);c->ip=0x0A03u;break;
        case 0x0A03: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A04u)!=0xF75Au)return PR_U9E_0160_CODE;
            push(ram,c,0x0A06u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A06u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A06: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A09u;break;
        case 0x0A09: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0A0Cu;break;
        case 0x0A0C: al(c,(uint8_t)(0x0059u));c->ip=0x0A0Eu;break;
        case 0x0A0E: push(ram,c,c->ax);c->ip=0x0A0Fu;break;
        case 0x0A0F: push(ram,c,c->cs);c->ip=0x0A10u;break;
        case 0x0A10: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A11u)!=0xF74Du)return PR_U9E_0160_CODE;
            push(ram,c,0x0A13u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A13u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A13: c->ip=0x0AB5u;break;
        case 0x0A16: (void)arithmetic(c,(uint8_t)c->ax,0x0011u,0u,8u,1u);c->ip=0x0A18u;break;
        case 0x0A18: c->ip=((c->flags&0x40u)==0u)?0x0A43u:0x0A1Au;break;
        case 0x0A1A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A1Du;break;
        case 0x0A1D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0A20u;break;
        case 0x0A20: al(c,(uint8_t)(0x0001u));c->ip=0x0A22u;break;
        case 0x0A22: push(ram,c,c->ax);c->ip=0x0A23u;break;
        case 0x0A23: push(ram,c,c->cs);c->ip=0x0A24u;break;
        case 0x0A24: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A25u)!=0xF739u)return PR_U9E_0160_CODE;
            push(ram,c,0x0A27u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A27u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A27: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A2Au;break;
        case 0x0A2A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0A2Du;break;
        case 0x0A2D: al(c,(uint8_t)(0x0002u));c->ip=0x0A2Fu;break;
        case 0x0A2F: push(ram,c,c->ax);c->ip=0x0A30u;break;
        case 0x0A30: push(ram,c,c->cs);c->ip=0x0A31u;break;
        case 0x0A31: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A32u)!=0xF72Cu)return PR_U9E_0160_CODE;
            push(ram,c,0x0A34u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A34u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A34: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A37u;break;
        case 0x0A37: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0A3Au;break;
        case 0x0A3A: al(c,(uint8_t)(0x000Bu));c->ip=0x0A3Cu;break;
        case 0x0A3C: push(ram,c,c->ax);c->ip=0x0A3Du;break;
        case 0x0A3D: push(ram,c,c->cs);c->ip=0x0A3Eu;break;
        case 0x0A3E: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A3Fu)!=0xF71Fu)return PR_U9E_0160_CODE;
            push(ram,c,0x0A41u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A41u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A41: c->ip=0x0AB5u;break;
        case 0x0A43: (void)arithmetic(c,(uint8_t)c->ax,0x0012u,0u,8u,1u);c->ip=0x0A45u;break;
        case 0x0A45: c->ip=((c->flags&0x40u)==0u)?0x0A70u:0x0A47u;break;
        case 0x0A47: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A4Au;break;
        case 0x0A4A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0A4Du;break;
        case 0x0A4D: al(c,(uint8_t)(0x0027u));c->ip=0x0A4Fu;break;
        case 0x0A4F: push(ram,c,c->ax);c->ip=0x0A50u;break;
        case 0x0A50: push(ram,c,c->cs);c->ip=0x0A51u;break;
        case 0x0A51: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A52u)!=0xF70Cu)return PR_U9E_0160_CODE;
            push(ram,c,0x0A54u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A54u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A54: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A57u;break;
        case 0x0A57: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0A5Au;break;
        case 0x0A5A: al(c,(uint8_t)(0x002Au));c->ip=0x0A5Cu;break;
        case 0x0A5C: push(ram,c,c->ax);c->ip=0x0A5Du;break;
        case 0x0A5D: push(ram,c,c->cs);c->ip=0x0A5Eu;break;
        case 0x0A5E: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A5Fu)!=0xF6FFu)return PR_U9E_0160_CODE;
            push(ram,c,0x0A61u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A61u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A61: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A64u;break;
        case 0x0A64: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0A67u;break;
        case 0x0A67: al(c,(uint8_t)(0x003Au));c->ip=0x0A69u;break;
        case 0x0A69: push(ram,c,c->ax);c->ip=0x0A6Au;break;
        case 0x0A6A: push(ram,c,c->cs);c->ip=0x0A6Bu;break;
        case 0x0A6B: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A6Cu)!=0xF6F2u)return PR_U9E_0160_CODE;
            push(ram,c,0x0A6Eu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A6Eu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A6E: c->ip=0x0AB5u;break;
        case 0x0A70: (void)arithmetic(c,(uint8_t)c->ax,0x0013u,0u,8u,1u);c->ip=0x0A72u;break;
        case 0x0A72: c->ip=((c->flags&0x40u)==0u)?0x0AB5u:0x0A74u;break;
        case 0x0A74: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A77u;break;
        case 0x0A77: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0A7Au;break;
        case 0x0A7A: al(c,(uint8_t)(0x0062u));c->ip=0x0A7Cu;break;
        case 0x0A7C: push(ram,c,c->ax);c->ip=0x0A7Du;break;
        case 0x0A7D: push(ram,c,c->cs);c->ip=0x0A7Eu;break;
        case 0x0A7E: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A7Fu)!=0xF6DFu)return PR_U9E_0160_CODE;
            push(ram,c,0x0A81u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A81u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A81: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A84u;break;
        case 0x0A84: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0A87u;break;
        case 0x0A87: al(c,(uint8_t)(0x0017u));c->ip=0x0A89u;break;
        case 0x0A89: push(ram,c,c->ax);c->ip=0x0A8Au;break;
        case 0x0A8A: push(ram,c,c->cs);c->ip=0x0A8Bu;break;
        case 0x0A8B: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A8Cu)!=0xF6D2u)return PR_U9E_0160_CODE;
            push(ram,c,0x0A8Eu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A8Eu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A8E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A91u;break;
        case 0x0A91: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0A94u;break;
        case 0x0A94: al(c,(uint8_t)(0x0048u));c->ip=0x0A96u;break;
        case 0x0A96: push(ram,c,c->ax);c->ip=0x0A97u;break;
        case 0x0A97: push(ram,c,c->cs);c->ip=0x0A98u;break;
        case 0x0A98: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0A99u)!=0xF6C5u)return PR_U9E_0160_CODE;
            push(ram,c,0x0A9Bu);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0A9Bu)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0A9B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0A9Eu;break;
        case 0x0A9E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0AA1u;break;
        case 0x0AA1: al(c,(uint8_t)(0x0038u));c->ip=0x0AA3u;break;
        case 0x0AA3: push(ram,c,c->ax);c->ip=0x0AA4u;break;
        case 0x0AA4: push(ram,c,c->cs);c->ip=0x0AA5u;break;
        case 0x0AA5: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0AA6u)!=0xF6B8u)return PR_U9E_0160_CODE;
            push(ram,c,0x0AA8u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0AA8u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0AA8: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0AABu;break;
        case 0x0AAB: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0AAEu;break;
        case 0x0AAE: al(c,(uint8_t)(0x000Bu));c->ip=0x0AB0u;break;
        case 0x0AB0: push(ram,c,c->ax);c->ip=0x0AB1u;break;
        case 0x0AB1: push(ram,c,c->cs);c->ip=0x0AB2u;break;
        case 0x0AB2: if(ram[physical(c->cs,c->ip)]!=0xE8 ||
            read_word(ram,c->cs,0x0AB3u)!=0xF6ABu)return PR_U9E_0160_CODE;
            push(ram,c,0x0AB5u);c->ip=0x0160u;
            status=pr_u9e_sidecar_0160_cpu(ram,size,c,layout,limits,diagnostic);
            if(status!=PR_U9E_0160_OK)return status;
            if(c->cs!=caller || c->ip!=0x0AB5u)return PR_U9E_0160_TRANSFER;
            break;
        case 0x0AB5: c->sp=c->bp;c->ip=0x0AB7u;break;
        case 0x0AB7: c->bp=pop(ram,c);c->ip=0x0AB8u;break;
        case 0x0AB8: retf(ram,c,6u);return PR_U9E_0160_OK;
        default:return PR_U9E_0160_TRANSFER;
        }
    }
}
