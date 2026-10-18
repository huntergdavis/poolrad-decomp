/* Complete U3C2 entry 6. See docs/re/U3C2_CALLER_0909.md.
 * Memory/arithmetic primitives adapted from accepted U7F +0A01. */
#include "poolrad/recovered/overlay_u3c2_caller_0909.h"
#include "poolrad/recovered/overlay_u3c6_table_pair_0000.h"
#include "poolrad/recovered/overlay_u3c2_entry5_0422.h"
#include "poolrad/recovered/overlay_u3c2_entry4_057b.h"
#include "poolrad/recovered/overlay_u3c2_record_sort_002e_cpu.h"
#define MASK (PR_U3C2_0909_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_u3c2_0909_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u3c2_0909_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u3c2_0909_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void cbw(pr_u3c2_0909_cpu *c)
{
    c->ax=(uint16_t)((c->ax&0x80u) ? 0xFF00u|(c->ax&255u) : c->ax&255u);
}
static void result_flags(pr_u3c2_0909_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u3c2_0909_cpu *c, uint16_t left, uint16_t right,
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
static uint16_t shl(pr_u3c2_0909_cpu *c, uint16_t left)
{
    uint16_t result=(uint16_t)(left << 1u);
    /* Intermediate AF follows the accepted U7F +0A01 normal/386 convention. */
    c->flags&=(uint16_t)~0x08D5u;
    c->flags|=0x10u;
    if (left&0x8000u)c->flags|=1u;
    if ((left^result)&0x8000u)c->flags|=0x800u;
    result_flags(c,result,0x8000u);return result;
}
static void retf(const uint8_t *ram, pr_u3c2_0909_cpu *c, unsigned cleanup)
{
    c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+cleanup);
}


static void ah(pr_u3c2_0909_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&255u)|((uint16_t)v<<8u)); }
static void dl(pr_u3c2_0909_cpu *c,uint8_t v) { c->dx=(uint16_t)((c->dx&0xFF00u)|v); }
static uint16_t logic(pr_u3c2_0909_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }
static uint8_t increment(pr_u3c2_0909_cpu *c,uint8_t v) { uint16_t cf=c->flags&1u;v=(uint8_t)arithmetic(c,v,1u,0u,8u,0u);c->flags=(uint16_t)((c->flags&~1u)|cf);return v; }

#define COPY_CPU(dst,src) do { (dst)->ax=(src)->ax; (dst)->bx=(src)->bx; (dst)->cx=(src)->cx; (dst)->dx=(src)->dx; (dst)->si=(src)->si; (dst)->di=(src)->di; (dst)->bp=(src)->bp; (dst)->sp=(src)->sp; (dst)->cs=(src)->cs; (dst)->ip=(src)->ip; (dst)->ds=(src)->ds; (dst)->es=(src)->es; (dst)->ss=(src)->ss; (dst)->flags=(src)->flags; } while (0)
static int pair(uint8_t *ram,size_t size,pr_u3c2_0909_cpu *c)
{ pr_u3c6_0000_cpu inner;int status;COPY_CPU(&inner,c);status=(int)pr_u3c6_table_pair_0000(ram,size,&inner,PR_U3C6_0000_RETF_WRAPPED_SS);COPY_CPU(c,&inner);return status; }
static int predicate(uint8_t *ram,size_t size,pr_u3c2_0909_cpu *c)
{ pr_u3c2_057b_cpu inner;int status;COPY_CPU(&inner,c);status=(int)pr_u3c2_entry4_057b(ram,size,&inner);COPY_CPU(c,&inner);return status; }
static int line(uint8_t *ram,size_t size,pr_u3c2_0909_cpu *c, size_t limit)
{ pr_u3c2_0422_cpu inner;int status;COPY_CPU(&inner,c);status=(int)pr_u3c2_line_check_0422(ram,size,&inner,limit);COPY_CPU(c,&inner);return status; }
static int sort(uint8_t *ram,size_t size,pr_u3c2_0909_cpu *c, unsigned limit)
{ pr_u3c2_002e_cpu inner;int status;COPY_CPU(&inner,c);status=(int)pr_u3c2_record_sort_002e_cpu(ram,size,&inner,limit,NULL);COPY_CPU(c,&inner);return status; }
#undef COPY_CPU

pr_u3c2_0909_status pr_u3c2_caller_0909(
    uint8_t *ram,size_t size,pr_u3c2_0909_cpu *c,uint16_t pair_cs,
    size_t max_caller_iterations,size_t max_line_iterations,unsigned max_sort_loopbacks)
{
    uint16_t code_segment;int status;
    if (ram==NULL || c==NULL || size!=PR_U3C2_0909_RAM_SIZE ||
        c->ip!=0x0909u || c->sp<0x008Eu || c->sp>0xFFEEu ||
        max_caller_iterations==0u || max_line_iterations==0u || max_sort_loopbacks==0u)
        return PR_U3C2_0909_ARGUMENT;
    code_segment=c->cs;
    for (;;) {
        if (c->cs!=code_segment) return PR_U3C2_0909_TRANSFER;
        if (c->ip==0x0918u || c->ip==0x0993u || c->ip==0x09BAu ||
            c->ip==0x0A48u || c->ip==0x0A61u || c->ip==0x0B73u) {
            if (max_caller_iterations==0u) return PR_U3C2_0909_CALLER_LIMIT;
            --max_caller_iterations;
        }
        switch(c->ip) {
        case 0x0909: push(ram,c,c->bp);c->ip=0x090Au;break;
        case 0x090A: c->bp=c->sp;c->ip=0x090Cu;break;
        case 0x090C: c->sp=arithmetic(c,c->sp,0x0020u,0u,16u,1u);c->ip=0x090Fu;break;
        case 0x090F: ram[physical(c->ss,(uint16_t)(c->bp+-3))]=(uint8_t)(0x0000u);c->ip=0x0913u;break;
        case 0x0913: c->ip=0x0918u;break;
        case 0x0915: ram[physical(c->ss,(uint16_t)(c->bp+-3))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0918u;break;
        case 0x0918: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+10))]));c->ip=0x091Bu;break;
        case 0x091B: push(ram,c,c->ax);c->ip=0x091Cu;break;
        case 0x091C: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x091Fu;break;
        case 0x091F: push(ram,c,c->ax);c->ip=0x0920u;break;
        case 0x0920: c->di=(uint16_t)(c->bp+-10);c->ip=0x0923u;break;
        case 0x0923: push(ram,c,c->ss);c->ip=0x0924u;break;
        case 0x0924: push(ram,c,c->di);c->ip=0x0925u;break;
        case 0x0925: c->di=(uint16_t)(c->bp+-11);c->ip=0x0928u;break;
        case 0x0928: push(ram,c,c->ss);c->ip=0x0929u;break;
        case 0x0929: push(ram,c,c->di);c->ip=0x092Au;break;
        case 0x092A: push(ram,c,c->cs);push(ram,c,0x092Fu);c->cs=pair_cs;c->ip=0u;status=pair(ram,size,c);if(status!=0)return PR_U3C2_0909_CALLEE;if(c->cs!=code_segment || c->ip!=0x092Fu)return PR_U3C2_0909_TRANSFER;break;
        case 0x092F: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax | (uint8_t)c->ax),8u)));c->ip=0x0931u;break;
        case 0x0931: c->ip=((c->flags&0x40u)!=0u)?0x0965u:0x0933u;break;
        case 0x0933: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-10))]));c->ip=0x0936u;break;
        case 0x0936: cbw(c);c->ip=0x0937u;break;
        case 0x0937: c->dx=c->ax;c->ip=0x0939u;break;
        case 0x0939: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+18))]));c->ip=0x093Cu;break;
        case 0x093C: cbw(c);c->ip=0x093Du;break;
        case 0x093D: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,0u);c->ip=0x093Fu;break;
        case 0x093F: dl(c,(uint8_t)((uint8_t)c->ax));c->ip=0x0941u;break;
        case 0x0941: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0944u;break;
        case 0x0944: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0946u;break;
        case 0x0946: c->di=c->ax;c->ip=0x0948u;break;
        case 0x0948: ram[physical(c->ss,(uint16_t)(c->bp+c->di+-15))]=(uint8_t)((uint8_t)c->dx);c->ip=0x094Bu;break;
        case 0x094B: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-11))]));c->ip=0x094Eu;break;
        case 0x094E: cbw(c);c->ip=0x094Fu;break;
        case 0x094F: c->dx=c->ax;c->ip=0x0951u;break;
        case 0x0951: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+16))]));c->ip=0x0954u;break;
        case 0x0954: cbw(c);c->ip=0x0955u;break;
        case 0x0955: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,0u);c->ip=0x0957u;break;
        case 0x0957: dl(c,(uint8_t)((uint8_t)c->ax));c->ip=0x0959u;break;
        case 0x0959: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x095Cu;break;
        case 0x095C: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x095Eu;break;
        case 0x095E: c->di=c->ax;c->ip=0x0960u;break;
        case 0x0960: ram[physical(c->ss,(uint16_t)(c->bp+c->di+-19))]=(uint8_t)((uint8_t)c->dx);c->ip=0x0963u;break;
        case 0x0963: c->ip=0x0970u;break;
        case 0x0965: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0968u;break;
        case 0x0968: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x096Au;break;
        case 0x096A: c->di=c->ax;c->ip=0x096Cu;break;
        case 0x096C: ram[physical(c->ss,(uint16_t)(c->bp+c->di+-15))]=(uint8_t)(0x00FFu);c->ip=0x0970u;break;
        case 0x0970: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-3))],0x0003u,0u,8u,1u);c->ip=0x0974u;break;
        case 0x0974: c->ip=((c->flags&0x40u)==0u)?0x0915u:0x0976u;break;
        case 0x0976: ram[physical(c->ds,(uint16_t)(26394))]=(uint8_t)(0x0000u);c->ip=0x097Bu;break;
        case 0x097B: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(24362))]));c->ip=0x097Eu;break;
        case 0x097E: ram[physical(c->ss,(uint16_t)(c->bp+-32))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0981u;break;
        case 0x0981: al(c,(uint8_t)(0x0001u));c->ip=0x0983u;break;
        case 0x0983: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-32))],0u,8u,1u);c->ip=0x0986u;break;
        case 0x0986: c->ip=((c->flags&0x41u)!=0u)?0x098Bu:0x0988u;break;
        case 0x0988: c->ip=0x0BCFu;break;
        case 0x098B: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)((uint8_t)c->ax);c->ip=0x098Eu;break;
        case 0x098E: c->ip=0x0993u;break;
        case 0x0990: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0993u;break;
        case 0x0993: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0996u;break;
        case 0x0996: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0998u;break;
        case 0x0998: c->di=c->ax;c->ip=0x099Au;break;
        case 0x099A: c->di=shl(c,c->di);c->ip=0x099Cu;break;
        case 0x099C: c->di=shl(c,c->di);c->ip=0x099Eu;break;
        case 0x099E: (void)arithmetic(c,ram[physical(c->ds,(uint16_t)(c->di+24362))],0x0000u,0u,8u,1u);c->ip=0x09A3u;break;
        case 0x09A3: c->ip=((c->flags&0x41u)==0u)?0x09A8u:0x09A5u;break;
        case 0x09A5: c->ip=0x0BC4u;break;
        case 0x09A8: ram[physical(c->ss,(uint16_t)(c->bp+-7))]=(uint8_t)(0x0000u);c->ip=0x09ACu;break;
        case 0x09AC: put_word(ram,c->ss,(uint16_t)(c->bp+-31),0x00FFu);c->ip=0x09B1u;break;
        case 0x09B1: ram[physical(c->ss,(uint16_t)(c->bp+-4))]=(uint8_t)(0x0000u);c->ip=0x09B5u;break;
        case 0x09B5: c->ip=0x09BAu;break;
        case 0x09B7: ram[physical(c->ss,(uint16_t)(c->bp+-4))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x09BAu;break;
        case 0x09BA: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x09BDu;break;
        case 0x09BD: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x09BFu;break;
        case 0x09BF: c->di=c->ax;c->ip=0x09C1u;break;
        case 0x09C1: c->di=shl(c,c->di);c->ip=0x09C3u;break;
        case 0x09C3: c->di=shl(c,c->di);c->ip=0x09C5u;break;
        case 0x09C5: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24362))]));c->ip=0x09C9u;break;
        case 0x09C9: push(ram,c,c->ax);c->ip=0x09CAu;break;
        case 0x09CA: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x09CDu;break;
        case 0x09CD: push(ram,c,c->ax);c->ip=0x09CEu;break;
        case 0x09CE: c->di=(uint16_t)(c->bp+-10);c->ip=0x09D1u;break;
        case 0x09D1: push(ram,c,c->ss);c->ip=0x09D2u;break;
        case 0x09D2: push(ram,c,c->di);c->ip=0x09D3u;break;
        case 0x09D3: c->di=(uint16_t)(c->bp+-11);c->ip=0x09D6u;break;
        case 0x09D6: push(ram,c,c->ss);c->ip=0x09D7u;break;
        case 0x09D7: push(ram,c,c->di);c->ip=0x09D8u;break;
        case 0x09D8: push(ram,c,c->cs);push(ram,c,0x09DDu);c->cs=pair_cs;c->ip=0u;status=pair(ram,size,c);if(status!=0)return PR_U3C2_0909_CALLEE;if(c->cs!=code_segment || c->ip!=0x09DDu)return PR_U3C2_0909_TRANSFER;break;
        case 0x09DD: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax | (uint8_t)c->ax),8u)));c->ip=0x09DFu;break;
        case 0x09DF: c->ip=((c->flags&0x40u)!=0u)?0x0A2Bu:0x09E1u;break;
        case 0x09E1: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-10))]));c->ip=0x09E4u;break;
        case 0x09E4: cbw(c);c->ip=0x09E5u;break;
        case 0x09E5: c->dx=c->ax;c->ip=0x09E7u;break;
        case 0x09E7: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x09EAu;break;
        case 0x09EA: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x09ECu;break;
        case 0x09EC: c->di=c->ax;c->ip=0x09EEu;break;
        case 0x09EE: c->di=shl(c,c->di);c->ip=0x09F0u;break;
        case 0x09F0: c->di=shl(c,c->di);c->ip=0x09F2u;break;
        case 0x09F2: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24359))]));c->ip=0x09F6u;break;
        case 0x09F6: cbw(c);c->ip=0x09F7u;break;
        case 0x09F7: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,0u);c->ip=0x09F9u;break;
        case 0x09F9: dl(c,(uint8_t)((uint8_t)c->ax));c->ip=0x09FBu;break;
        case 0x09FB: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x09FEu;break;
        case 0x09FE: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0A00u;break;
        case 0x0A00: c->di=c->ax;c->ip=0x0A02u;break;
        case 0x0A02: ram[physical(c->ss,(uint16_t)(c->bp+c->di+-23))]=(uint8_t)((uint8_t)c->dx);c->ip=0x0A05u;break;
        case 0x0A05: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-11))]));c->ip=0x0A08u;break;
        case 0x0A08: cbw(c);c->ip=0x0A09u;break;
        case 0x0A09: c->dx=c->ax;c->ip=0x0A0Bu;break;
        case 0x0A0B: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0A0Eu;break;
        case 0x0A0E: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0A10u;break;
        case 0x0A10: c->di=c->ax;c->ip=0x0A12u;break;
        case 0x0A12: c->di=shl(c,c->di);c->ip=0x0A14u;break;
        case 0x0A14: c->di=shl(c,c->di);c->ip=0x0A16u;break;
        case 0x0A16: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24360))]));c->ip=0x0A1Au;break;
        case 0x0A1A: cbw(c);c->ip=0x0A1Bu;break;
        case 0x0A1B: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,0u);c->ip=0x0A1Du;break;
        case 0x0A1D: dl(c,(uint8_t)((uint8_t)c->ax));c->ip=0x0A1Fu;break;
        case 0x0A1F: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x0A22u;break;
        case 0x0A22: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0A24u;break;
        case 0x0A24: c->di=c->ax;c->ip=0x0A26u;break;
        case 0x0A26: ram[physical(c->ss,(uint16_t)(c->bp+c->di+-27))]=(uint8_t)((uint8_t)c->dx);c->ip=0x0A29u;break;
        case 0x0A29: c->ip=0x0A36u;break;
        case 0x0A2B: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x0A2Eu;break;
        case 0x0A2E: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0A30u;break;
        case 0x0A30: c->di=c->ax;c->ip=0x0A32u;break;
        case 0x0A32: ram[physical(c->ss,(uint16_t)(c->bp+c->di+-23))]=(uint8_t)(0x00FFu);c->ip=0x0A36u;break;
        case 0x0A36: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-4))],0x0003u,0u,8u,1u);c->ip=0x0A3Au;break;
        case 0x0A3A: c->ip=((c->flags&0x40u)!=0u)?0x0A3Fu:0x0A3Cu;break;
        case 0x0A3C: c->ip=0x09B7u;break;
        case 0x0A3F: ram[physical(c->ss,(uint16_t)(c->bp+-4))]=(uint8_t)(0x0000u);c->ip=0x0A43u;break;
        case 0x0A43: c->ip=0x0A48u;break;
        case 0x0A45: ram[physical(c->ss,(uint16_t)(c->bp+-4))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x0A48u;break;
        case 0x0A48: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x0A4Bu;break;
        case 0x0A4B: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0A4Du;break;
        case 0x0A4D: c->di=c->ax;c->ip=0x0A4Fu;break;
        case 0x0A4F: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+c->di+-23))],0x0000u,0u,8u,1u);c->ip=0x0A53u;break;
        case 0x0A53: c->ip=(((c->flags>>7u)^(c->flags>>11u))%2u==0u)?0x0A58u:0x0A55u;break;
        case 0x0A55: c->ip=0x0B23u;break;
        case 0x0A58: ram[physical(c->ss,(uint16_t)(c->bp+-3))]=(uint8_t)(0x0000u);c->ip=0x0A5Cu;break;
        case 0x0A5C: c->ip=0x0A61u;break;
        case 0x0A5E: ram[physical(c->ss,(uint16_t)(c->bp+-3))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0A61u;break;
        case 0x0A61: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0A64u;break;
        case 0x0A64: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0A66u;break;
        case 0x0A66: c->di=c->ax;c->ip=0x0A68u;break;
        case 0x0A68: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+c->di+-15))],0x0000u,0u,8u,1u);c->ip=0x0A6Cu;break;
        case 0x0A6C: c->ip=(((c->flags>>7u)^(c->flags>>11u))%2u==0u)?0x0A71u:0x0A6Eu;break;
        case 0x0A6E: c->ip=0x0B1Au;break;
        case 0x0A71: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0A74u;break;
        case 0x0A74: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0A76u;break;
        case 0x0A76: c->di=c->ax;c->ip=0x0A78u;break;
        case 0x0A78: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-15))]));c->ip=0x0A7Bu;break;
        case 0x0A7B: push(ram,c,c->ax);c->ip=0x0A7Cu;break;
        case 0x0A7C: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0A7Fu;break;
        case 0x0A7F: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0A81u;break;
        case 0x0A81: c->di=c->ax;c->ip=0x0A83u;break;
        case 0x0A83: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-19))]));c->ip=0x0A86u;break;
        case 0x0A86: push(ram,c,c->ax);c->ip=0x0A87u;break;
        case 0x0A87: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x0A8Au;break;
        case 0x0A8A: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0A8Cu;break;
        case 0x0A8C: c->di=c->ax;c->ip=0x0A8Eu;break;
        case 0x0A8E: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-23))]));c->ip=0x0A91u;break;
        case 0x0A91: push(ram,c,c->ax);c->ip=0x0A92u;break;
        case 0x0A92: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x0A95u;break;
        case 0x0A95: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0A97u;break;
        case 0x0A97: c->di=c->ax;c->ip=0x0A99u;break;
        case 0x0A99: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-27))]));c->ip=0x0A9Cu;break;
        case 0x0A9C: push(ram,c,c->ax);c->ip=0x0A9Du;break;
        case 0x0A9D: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+12))]));c->ip=0x0AA0u;break;
        case 0x0AA0: push(ram,c,c->ax);c->ip=0x0AA1u;break;
        case 0x0AA1: push(ram,c,c->cs);c->ip=0x0AA2u;break;
        case 0x0AA2: push(ram,c,0x0AA5u);c->ip=0x057Bu;status=predicate(ram,size,c);if(status!=0)return PR_U3C2_0909_CALLEE;if(c->cs!=code_segment || c->ip!=0x0AA5u)return PR_U3C2_0909_TRANSFER;break;
        case 0x0AA5: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax | (uint8_t)c->ax),8u)));c->ip=0x0AA7u;break;
        case 0x0AA7: c->ip=((c->flags&0x40u)!=0u)?0x0B1Au:0x0AA9u;break;
        case 0x0AA9: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x0AACu;break;
        case 0x0AAC: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0AAEu;break;
        case 0x0AAE: c->di=c->ax;c->ip=0x0AB0u;break;
        case 0x0AB0: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-23))]));c->ip=0x0AB3u;break;
        case 0x0AB3: ram[physical(c->ss,(uint16_t)(c->bp+-8))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0AB6u;break;
        case 0x0AB6: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x0AB9u;break;
        case 0x0AB9: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0ABBu;break;
        case 0x0ABB: c->di=c->ax;c->ip=0x0ABDu;break;
        case 0x0ABD: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-27))]));c->ip=0x0AC0u;break;
        case 0x0AC0: ram[physical(c->ss,(uint16_t)(c->bp+-9))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0AC3u;break;
        case 0x0AC3: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+14));c->ip=0x0AC6u;break;
        case 0x0AC6: put_word(ram,c->ss,(uint16_t)(c->bp+-29),c->ax);c->ip=0x0AC9u;break;
        case 0x0AC9: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0ACCu;break;
        case 0x0ACC: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0ACEu;break;
        case 0x0ACE: c->di=c->ax;c->ip=0x0AD0u;break;
        case 0x0AD0: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-15))]));c->ip=0x0AD3u;break;
        case 0x0AD3: push(ram,c,c->ax);c->ip=0x0AD4u;break;
        case 0x0AD4: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0AD7u;break;
        case 0x0AD7: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0AD9u;break;
        case 0x0AD9: c->di=c->ax;c->ip=0x0ADBu;break;
        case 0x0ADB: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-19))]));c->ip=0x0ADEu;break;
        case 0x0ADE: push(ram,c,c->ax);c->ip=0x0ADFu;break;
        case 0x0ADF: c->di=(uint16_t)(c->bp+-8);c->ip=0x0AE2u;break;
        case 0x0AE2: push(ram,c,c->ss);c->ip=0x0AE3u;break;
        case 0x0AE3: push(ram,c,c->di);c->ip=0x0AE4u;break;
        case 0x0AE4: c->di=(uint16_t)(c->bp+-9);c->ip=0x0AE7u;break;
        case 0x0AE7: push(ram,c,c->ss);c->ip=0x0AE8u;break;
        case 0x0AE8: push(ram,c,c->di);c->ip=0x0AE9u;break;
        case 0x0AE9: c->di=(uint16_t)(c->bp+-29);c->ip=0x0AECu;break;
        case 0x0AEC: push(ram,c,c->ss);c->ip=0x0AEDu;break;
        case 0x0AED: push(ram,c,c->di);c->ip=0x0AEEu;break;
        case 0x0AEE: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0AF1u;break;
        case 0x0AF1: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0AF4u;break;
        case 0x0AF4: push(ram,c,c->cs);c->ip=0x0AF5u;break;
        case 0x0AF5: push(ram,c,0x0AF8u);c->ip=0x0422u;status=line(ram,size,c,max_line_iterations);if(status==PR_U3C2_0422_LIMIT)return PR_U3C2_0909_LINE_LIMIT;if(status!=0)return PR_U3C2_0909_CALLEE;if(c->cs!=code_segment || c->ip!=0x0AF8u)return PR_U3C2_0909_TRANSFER;break;
        case 0x0AF8: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax | (uint8_t)c->ax),8u)));c->ip=0x0AFAu;break;
        case 0x0AFA: c->ip=((c->flags&0x40u)!=0u)?0x0B1Au:0x0AFCu;break;
        case 0x0AFC: ram[physical(c->ss,(uint16_t)(c->bp+-7))]=(uint8_t)(0x0001u);c->ip=0x0B00u;break;
        case 0x0B00: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-29));c->ip=0x0B03u;break;
        case 0x0B03: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-31)),0u,16u,1u);c->ip=0x0B06u;break;
        case 0x0B06: c->ip=((c->flags&1u)==0u)?0x0B1Au:0x0B08u;break;
        case 0x0B08: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-29));c->ip=0x0B0Bu;break;
        case 0x0B0B: put_word(ram,c->ss,(uint16_t)(c->bp+-31),c->ax);c->ip=0x0B0Eu;break;
        case 0x0B0E: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0B11u;break;
        case 0x0B11: ram[physical(c->ss,(uint16_t)(c->bp+-5))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0B14u;break;
        case 0x0B14: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x0B17u;break;
        case 0x0B17: ram[physical(c->ss,(uint16_t)(c->bp+-6))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0B1Au;break;
        case 0x0B1A: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-3))],0x0003u,0u,8u,1u);c->ip=0x0B1Eu;break;
        case 0x0B1E: c->ip=((c->flags&0x40u)!=0u)?0x0B23u:0x0B20u;break;
        case 0x0B20: c->ip=0x0A5Eu;break;
        case 0x0B23: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-4))],0x0003u,0u,8u,1u);c->ip=0x0B27u;break;
        case 0x0B27: c->ip=((c->flags&0x40u)!=0u)?0x0B2Cu:0x0B29u;break;
        case 0x0B29: c->ip=0x0A45u;break;
        case 0x0B2C: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-7))],0x0000u,0u,8u,1u);c->ip=0x0B30u;break;
        case 0x0B30: c->ip=((c->flags&0x40u)==0u)?0x0B35u:0x0B32u;break;
        case 0x0B32: c->ip=0x0BC4u;break;
        case 0x0B35: ram[physical(c->ds,(uint16_t)(26394))]=(uint8_t)(increment(c,ram[physical(c->ds,(uint16_t)(26394))]));c->ip=0x0B39u;break;
        case 0x0B39: dl(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0B3Cu;break;
        case 0x0B3C: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(26394))]));c->ip=0x0B3Fu;break;
        case 0x0B3F: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0B41u;break;
        case 0x0B41: c->di=c->ax;c->ip=0x0B43u;break;
        case 0x0B43: c->si=c->di;c->ip=0x0B45u;break;
        case 0x0B45: c->di=shl(c,c->di);c->ip=0x0B47u;break;
        case 0x0B47: c->di=arithmetic(c,c->di,c->si,0u,16u,0u);c->ip=0x0B49u;break;
        case 0x0B49: ram[physical(c->ds,(uint16_t)(c->di+26392))]=(uint8_t)((uint8_t)c->dx);c->ip=0x0B4Du;break;
        case 0x0B4D: dl(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-31))]));c->ip=0x0B50u;break;
        case 0x0B50: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(26394))]));c->ip=0x0B53u;break;
        case 0x0B53: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0B55u;break;
        case 0x0B55: c->di=c->ax;c->ip=0x0B57u;break;
        case 0x0B57: c->si=c->di;c->ip=0x0B59u;break;
        case 0x0B59: c->di=shl(c,c->di);c->ip=0x0B5Bu;break;
        case 0x0B5B: c->di=arithmetic(c,c->di,c->si,0u,16u,0u);c->ip=0x0B5Du;break;
        case 0x0B5D: ram[physical(c->ds,(uint16_t)(c->di+26393))]=(uint8_t)((uint8_t)c->dx);c->ip=0x0B61u;break;
        case 0x0B61: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(0x0000u);c->ip=0x0B65u;break;
        case 0x0B65: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+12))],0x0008u,0u,8u,1u);c->ip=0x0B69u;break;
        case 0x0B69: c->ip=((c->flags&1u)==0u)?0x0B73u:0x0B6Bu;break;
        case 0x0B6B: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+12))]));c->ip=0x0B6Eu;break;
        case 0x0B6E: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0B71u;break;
        case 0x0B71: c->ip=0x0BB0u;break;
        case 0x0B73: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-5))]));c->ip=0x0B76u;break;
        case 0x0B76: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0B78u;break;
        case 0x0B78: c->di=c->ax;c->ip=0x0B7Au;break;
        case 0x0B7A: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-15))]));c->ip=0x0B7Du;break;
        case 0x0B7D: push(ram,c,c->ax);c->ip=0x0B7Eu;break;
        case 0x0B7E: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-5))]));c->ip=0x0B81u;break;
        case 0x0B81: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0B83u;break;
        case 0x0B83: c->di=c->ax;c->ip=0x0B85u;break;
        case 0x0B85: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-19))]));c->ip=0x0B88u;break;
        case 0x0B88: push(ram,c,c->ax);c->ip=0x0B89u;break;
        case 0x0B89: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-6))]));c->ip=0x0B8Cu;break;
        case 0x0B8C: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0B8Eu;break;
        case 0x0B8E: c->di=c->ax;c->ip=0x0B90u;break;
        case 0x0B90: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-23))]));c->ip=0x0B93u;break;
        case 0x0B93: push(ram,c,c->ax);c->ip=0x0B94u;break;
        case 0x0B94: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-6))]));c->ip=0x0B97u;break;
        case 0x0B97: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0B99u;break;
        case 0x0B99: c->di=c->ax;c->ip=0x0B9Bu;break;
        case 0x0B9B: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+c->di+-27))]));c->ip=0x0B9Eu;break;
        case 0x0B9E: push(ram,c,c->ax);c->ip=0x0B9Fu;break;
        case 0x0B9F: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x0BA2u;break;
        case 0x0BA2: push(ram,c,c->ax);c->ip=0x0BA3u;break;
        case 0x0BA3: push(ram,c,c->cs);c->ip=0x0BA4u;break;
        case 0x0BA4: push(ram,c,0x0BA7u);c->ip=0x057Bu;status=predicate(ram,size,c);if(status!=0)return PR_U3C2_0909_CALLEE;if(c->cs!=code_segment || c->ip!=0x0BA7u)return PR_U3C2_0909_TRANSFER;break;
        case 0x0BA7: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax | (uint8_t)c->ax),8u)));c->ip=0x0BA9u;break;
        case 0x0BA9: c->ip=((c->flags&0x40u)==0u)?0x0BB0u:0x0BABu;break;
        case 0x0BAB: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x0BAEu;break;
        case 0x0BAE: c->ip=0x0B73u;break;
        case 0x0BB0: dl(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x0BB3u;break;
        case 0x0BB3: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(26394))]));c->ip=0x0BB6u;break;
        case 0x0BB6: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u) ^ (uint8_t)(c->ax>>8u)),8u)));c->ip=0x0BB8u;break;
        case 0x0BB8: c->di=c->ax;c->ip=0x0BBAu;break;
        case 0x0BBA: c->si=c->di;c->ip=0x0BBCu;break;
        case 0x0BBC: c->di=shl(c,c->di);c->ip=0x0BBEu;break;
        case 0x0BBE: c->di=arithmetic(c,c->di,c->si,0u,16u,0u);c->ip=0x0BC0u;break;
        case 0x0BC0: ram[physical(c->ds,(uint16_t)(c->di+26394))]=(uint8_t)((uint8_t)c->dx);c->ip=0x0BC4u;break;
        case 0x0BC4: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0BC7u;break;
        case 0x0BC7: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-32))],0u,8u,1u);c->ip=0x0BCAu;break;
        case 0x0BCA: c->ip=((c->flags&0x40u)!=0u)?0x0BCFu:0x0BCCu;break;
        case 0x0BCC: c->ip=0x0990u;break;
        case 0x0BCF: push(ram,c,c->cs);c->ip=0x0BD0u;break;
        case 0x0BD0: push(ram,c,0x0BD3u);c->ip=0x002Eu;status=sort(ram,size,c,max_sort_loopbacks);if(status==PR_U3C2_002E_LIMIT)return PR_U3C2_0909_SORT_LIMIT;if(status!=0)return PR_U3C2_0909_CALLEE;if(c->cs!=code_segment || c->ip!=0x0BD3u)return PR_U3C2_0909_TRANSFER;break;
        case 0x0BD3: c->sp=c->bp;c->ip=0x0BD5u;break;
        case 0x0BD5: c->bp=pop(ram,c);c->ip=0x0BD6u;break;
        case 0x0BD6: retf(ram,c,14u);return PR_U3C2_0909_OK;
        default:return PR_U3C2_0909_TRANSFER;
        }
    }
}
