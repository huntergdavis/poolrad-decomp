/* Complete U5E +32E6 with raw U3C6 slot accessors.
 * Memory/arithmetic primitives adapted from accepted U3C2 +0909;
 * slot composition follows accepted U5E +1BF8. See U5E_CALLER_32E6.md. */
#include "poolrad/recovered/overlay_u5e_caller_32e6.h"
#include "poolrad/recovered/overlay_u3c2_caller_0909.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#define MASK (PR_U5E_32E6_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_u5e_32e6_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u5e_32e6_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u5e_32e6_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_u5e_32e6_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u5e_32e6_cpu *c, uint16_t left, uint16_t right,
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
static uint16_t shl(pr_u5e_32e6_cpu *c, uint16_t left)
{
    uint16_t result=(uint16_t)(left << 1u);
    /* Intermediate AF follows the accepted U7F +0A01 normal/386 convention. */
    c->flags&=(uint16_t)~0x08D5u;
    c->flags|=0x10u;
    if (left&0x8000u)c->flags|=1u;
    if ((left^result)&0x8000u)c->flags|=0x800u;
    result_flags(c,result,0x8000u);return result;
}
static void retf(const uint8_t *ram, pr_u5e_32e6_cpu *c, unsigned cleanup)
{
    c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+cleanup);
}


static void ah(pr_u5e_32e6_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&255u)|((uint16_t)v<<8u)); }
static uint16_t logic(pr_u5e_32e6_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }
static uint8_t increment(pr_u5e_32e6_cpu *c,uint8_t v) { uint16_t cf=c->flags&1u;v=(uint8_t)arithmetic(c,v,1u,0u,8u,0u);c->flags=(uint16_t)((c->flags&~1u)|cf);return v; }


#define COPY_CPU(dst,src) do { (dst)->ax=(src)->ax; (dst)->bx=(src)->bx; (dst)->cx=(src)->cx; (dst)->dx=(src)->dx; (dst)->si=(src)->si; (dst)->di=(src)->di; (dst)->bp=(src)->bp; (dst)->sp=(src)->sp; (dst)->cs=(src)->cs; (dst)->ip=(src)->ip; (dst)->ds=(src)->ds; (dst)->es=(src)->es; (dst)->ss=(src)->ss; (dst)->flags=(src)->flags; } while (0)
static int chain(uint8_t *ram,size_t size,pr_u5e_32e6_cpu *c,
                 const pr_u5e_32e6_layout *layout,const pr_u5e_32e6_limits *limits)
{
    pr_u3c2_0909_cpu inner;int status;COPY_CPU(&inner,c);
    status=(int)pr_u3c2_caller_0909(ram,size,&inner,layout->slots_cs,
        limits->chain_iterations,limits->line_iterations,limits->sort_loopbacks);
    COPY_CPU(c,&inner);return status;
}
static int move(uint8_t *ram,size_t size,pr_u5e_32e6_cpu *c)
{
    pr_system_leaf_cpu inner;int status;COPY_CPU(&inner,c);
    status=(int)pr_system_move_0a3f(ram,size,&inner);COPY_CPU(c,&inner);return status;
}
#undef COPY_CPU
static pr_u5e_32e6_status slots(uint8_t *ram,pr_u5e_32e6_cpu *c,size_t remaining)
{
    uint16_t code_segment=c->cs,return_ip=0u;
    for (;;) {
        if(c->cs!=code_segment)return PR_U5E_32E6_TRANSFER;
        if(c->ip==0x0C71u) {
            if(remaining==0u)return PR_U5E_32E6_SLOT_LIMIT;
            --remaining;
        }
        switch(c->ip) {
        case 0x0BE7: push(ram,c,c->bp);c->ip=0x0BE8u;break;
        case 0x0BE8: c->bp=c->sp;c->ip=0x0BEAu;break;
        case 0x0BEA: c->sp=arithmetic(c,c->sp,0x0002u,0u,16u,1u);c->ip=0x0BEDu;break;
        case 0x0BED: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0BF0u;break;
        case 0x0BF0: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0BF3u;break;
        case 0x0BF3: push(ram,c,c->cs);c->ip=0x0BF4u;break;
        case 0x0BF4: push(ram,c,0x0BF7u);return_ip=0x0BF7u;c->ip=0x0C5Fu;break;
        case 0x0BF7: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x0BF9u;break;
        case 0x0BF9: c->di=c->ax;c->ip=0x0BFBu;break;
        case 0x0BFB: c->di=shl(c,c->di);c->ip=0x0BFDu;break;
        case 0x0BFD: c->di=shl(c,c->di);c->ip=0x0BFFu;break;
        case 0x0BFF: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24359))]));c->ip=0x0C03u;break;
        case 0x0C03: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0C06u;break;
        case 0x0C06: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0C09u;break;
        case 0x0C09: c->sp=c->bp;c->ip=0x0C0Bu;break;
        case 0x0C0B: c->bp=pop(ram,c);c->ip=0x0C0Cu;break;
        case 0x0C0C: retf(ram,c,4u);return PR_U5E_32E6_OK;
        case 0x0C0F: push(ram,c,c->bp);c->ip=0x0C10u;break;
        case 0x0C10: c->bp=c->sp;c->ip=0x0C12u;break;
        case 0x0C12: c->sp=arithmetic(c,c->sp,0x0002u,0u,16u,1u);c->ip=0x0C15u;break;
        case 0x0C15: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0C18u;break;
        case 0x0C18: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0C1Bu;break;
        case 0x0C1B: push(ram,c,c->cs);c->ip=0x0C1Cu;break;
        case 0x0C1C: push(ram,c,0x0C1Fu);return_ip=0x0C1Fu;c->ip=0x0C5Fu;break;
        case 0x0C1F: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x0C21u;break;
        case 0x0C21: c->di=c->ax;c->ip=0x0C23u;break;
        case 0x0C23: c->di=shl(c,c->di);c->ip=0x0C25u;break;
        case 0x0C25: c->di=shl(c,c->di);c->ip=0x0C27u;break;
        case 0x0C27: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24360))]));c->ip=0x0C2Bu;break;
        case 0x0C2B: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0C2Eu;break;
        case 0x0C2E: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0C31u;break;
        case 0x0C31: c->sp=c->bp;c->ip=0x0C33u;break;
        case 0x0C33: c->bp=pop(ram,c);c->ip=0x0C34u;break;
        case 0x0C34: retf(ram,c,4u);return PR_U5E_32E6_OK;
        case 0x0C37: push(ram,c,c->bp);c->ip=0x0C38u;break;
        case 0x0C38: c->bp=c->sp;c->ip=0x0C3Au;break;
        case 0x0C3A: c->sp=arithmetic(c,c->sp,0x0002u,0u,16u,1u);c->ip=0x0C3Du;break;
        case 0x0C3D: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0C40u;break;
        case 0x0C40: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+6)));c->ip=0x0C43u;break;
        case 0x0C43: push(ram,c,c->cs);c->ip=0x0C44u;break;
        case 0x0C44: push(ram,c,0x0C47u);return_ip=0x0C47u;c->ip=0x0C5Fu;break;
        case 0x0C47: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x0C49u;break;
        case 0x0C49: c->di=c->ax;c->ip=0x0C4Bu;break;
        case 0x0C4B: c->di=shl(c,c->di);c->ip=0x0C4Du;break;
        case 0x0C4D: c->di=shl(c,c->di);c->ip=0x0C4Fu;break;
        case 0x0C4F: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+24362))]));c->ip=0x0C53u;break;
        case 0x0C53: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0C56u;break;
        case 0x0C56: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0C59u;break;
        case 0x0C59: c->sp=c->bp;c->ip=0x0C5Bu;break;
        case 0x0C5B: c->bp=pop(ram,c);c->ip=0x0C5Cu;break;
        case 0x0C5C: retf(ram,c,4u);return PR_U5E_32E6_OK;
        case 0x0C5F: push(ram,c,c->bp);c->ip=0x0C60u;break;
        case 0x0C60: c->bp=c->sp;c->ip=0x0C62u;break;
        case 0x0C62: c->sp=arithmetic(c,c->sp,0x0004u,0u,16u,1u);c->ip=0x0C65u;break;
        case 0x0C65: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(0x0000u);c->ip=0x0C69u;break;
        case 0x0C69: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)(0x0000u);c->ip=0x0C6Du;break;
        case 0x0C6D: ram[physical(c->ss,(uint16_t)(c->bp+-3))]=(uint8_t)(0x0000u);c->ip=0x0C71u;break;
        case 0x0C71: ram[physical(c->ss,(uint16_t)(c->bp+-3))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0C74u;break;
        case 0x0C74: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0C77u;break;
        case 0x0C77: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x0C79u;break;
        case 0x0C79: c->di=c->ax;c->ip=0x0C7Bu;break;
        case 0x0C7B: c->di=shl(c,c->di);c->ip=0x0C7Du;break;
        case 0x0C7D: c->di=shl(c,c->di);c->ip=0x0C7Fu;break;
        case 0x0C7F: c->ax=read_word(ram,c->ds,(uint16_t)(c->di+26041));c->ip=0x0C83u;break;
        case 0x0C83: c->dx=read_word(ram,c->ds,(uint16_t)(c->di+26043));c->ip=0x0C87u;break;
        case 0x0C87: (void)arithmetic(c,c->dx,read_word(ram,c->ss,(uint16_t)(c->bp+8)),0u,16u,1u);c->ip=0x0C8Au;break;
        case 0x0C8A: c->ip=((c->flags&0x40u)==0u)?0x0C95u:0x0C8Cu;break;
        case 0x0C8C: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+6)),0u,16u,1u);c->ip=0x0C8Fu;break;
        case 0x0C8F: c->ip=((c->flags&0x40u)==0u)?0x0C95u:0x0C91u;break;
        case 0x0C91: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)(0x0001u);c->ip=0x0C95u;break;
        case 0x0C95: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0x0000u,0u,8u,1u);c->ip=0x0C99u;break;
        case 0x0C99: c->ip=((c->flags&0x40u)==0u)?0x0CA4u:0x0C9Bu;break;
        case 0x0C9B: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0C9Eu;break;
        case 0x0C9E: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ds,(uint16_t)(24362))],0u,8u,1u);c->ip=0x0CA2u;break;
        case 0x0CA2: c->ip=((c->flags&0x40u)==0u)?0x0C71u:0x0CA4u;break;
        case 0x0CA4: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0x0000u,0u,8u,1u);c->ip=0x0CA8u;break;
        case 0x0CA8: c->ip=((c->flags&0x40u)!=0u)?0x0CB0u:0x0CAAu;break;
        case 0x0CAA: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-3))]));c->ip=0x0CADu;break;
        case 0x0CAD: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0CB0u;break;
        case 0x0CB0: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x0CB3u;break;
        case 0x0CB3: c->sp=c->bp;c->ip=0x0CB5u;break;
        case 0x0CB5: c->bp=pop(ram,c);c->ip=0x0CB6u;break;
        case 0x0CB6: retf(ram,c,4u);if(c->cs!=code_segment || c->ip!=return_ip)return PR_U5E_32E6_TRANSFER;break;
        default:return PR_U5E_32E6_TRANSFER;
        }
    }
}
pr_u5e_32e6_status pr_u5e_caller_32e6(
    uint8_t *ram,size_t size,pr_u5e_32e6_cpu *c,
    const pr_u5e_32e6_layout *layout,const pr_u5e_32e6_limits *limits)
{
    uint16_t code_segment;size_t remaining;int status;
    if(ram==NULL || c==NULL || layout==NULL || limits==NULL ||
       size!=PR_U5E_32E6_RAM_SIZE || c->ip!=0x32E6u ||
       c->sp<0x00A4u || c->sp>0xFFF6u ||
       limits->slot_iterations==0u || limits->chain_iterations==0u ||
       limits->line_iterations==0u || limits->sort_loopbacks==0u ||
       limits->copy_iterations==0u)return PR_U5E_32E6_ARGUMENT;
    code_segment=c->cs;remaining=limits->copy_iterations;
    for (;;) {
        if(c->cs!=code_segment)return PR_U5E_32E6_TRANSFER;
        if(c->ip==0x3349u) {
            if(remaining==0u)return PR_U5E_32E6_COPY_LIMIT;
            --remaining;
        }
        switch(c->ip) {
        case 0x32E6: push(ram,c,c->bp);c->ip=0x32E7u;break;
        case 0x32E7: c->bp=c->sp;c->ip=0x32E9u;break;
        case 0x32E9: c->sp=arithmetic(c,c->sp,0x0002u,0u,16u,1u);c->ip=0x32ECu;break;
        case 0x32EC: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+10)));c->ip=0x32EFu;break;
        case 0x32EF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x32F2u;break;
        case 0x32F2: push(ram,c,c->cs);push(ram,c,0x32F7u);c->cs=layout->slots_cs;c->ip=0x0BE7u;status=(int)slots(ram,c,limits->slot_iterations);if(status!=0)return (pr_u5e_32e6_status)status;if(c->cs!=code_segment || c->ip!=0x32F7u)return PR_U5E_32E6_TRANSFER;break;
        case 0x32F7: push(ram,c,c->ax);c->ip=0x32F8u;break;
        case 0x32F8: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+10)));c->ip=0x32FBu;break;
        case 0x32FB: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x32FEu;break;
        case 0x32FE: push(ram,c,c->cs);push(ram,c,0x3303u);c->cs=layout->slots_cs;c->ip=0x0C0Fu;status=(int)slots(ram,c,limits->slot_iterations);if(status!=0)return (pr_u5e_32e6_status)status;if(c->cs!=code_segment || c->ip!=0x3303u)return PR_U5E_32E6_TRANSFER;break;
        case 0x3303: push(ram,c,c->ax);c->ip=0x3304u;break;
        case 0x3304: c->ax=0x007Fu;c->ip=0x3307u;break;
        case 0x3307: push(ram,c,c->ax);c->ip=0x3308u;break;
        case 0x3308: al(c,(uint8_t)(0x00FFu));c->ip=0x330Au;break;
        case 0x330A: push(ram,c,c->ax);c->ip=0x330Bu;break;
        case 0x330B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+10)));c->ip=0x330Eu;break;
        case 0x330E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x3311u;break;
        case 0x3311: push(ram,c,c->cs);push(ram,c,0x3316u);c->cs=layout->slots_cs;c->ip=0x0C37u;status=(int)slots(ram,c,limits->slot_iterations);if(status!=0)return (pr_u5e_32e6_status)status;if(c->cs!=code_segment || c->ip!=0x3316u)return PR_U5E_32E6_TRANSFER;break;
        case 0x3316: push(ram,c,c->ax);c->ip=0x3317u;break;
        case 0x3317: push(ram,c,read_word(ram,c->ds,(uint16_t)(26392)));c->ip=0x331Bu;break;
        case 0x331B: push(ram,c,read_word(ram,c->ds,(uint16_t)(26390)));c->ip=0x331Fu;break;
        case 0x331F: push(ram,c,c->cs);push(ram,c,0x3324u);c->cs=layout->chain_cs;c->ip=0x0909u;status=chain(ram,size,c,layout,limits);if(status==PR_U3C2_0909_CALLER_LIMIT)return PR_U5E_32E6_CHAIN_LIMIT;if(status==PR_U3C2_0909_LINE_LIMIT)return PR_U5E_32E6_LINE_LIMIT;if(status==PR_U3C2_0909_SORT_LIMIT)return PR_U5E_32E6_SORT_LIMIT;if(status!=0)return PR_U5E_32E6_CALLEE;if(c->cs!=code_segment || c->ip!=0x3324u)return PR_U5E_32E6_TRANSFER;break;
        case 0x3324: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(26394))]));c->ip=0x3327u;break;
        case 0x3327: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));c->ip=0x332Au;break;
        case 0x332A: ram[physical(c->ss,(uint16_t)(c->di+-218))]=(uint8_t)((uint8_t)c->ax);c->ip=0x332Fu;break;
        case 0x332F: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));c->ip=0x3332u;break;
        case 0x3332: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->di+-218))]));c->ip=0x3337u;break;
        case 0x3337: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)((uint8_t)c->ax);c->ip=0x333Au;break;
        case 0x333A: al(c,(uint8_t)(0x0001u));c->ip=0x333Cu;break;
        case 0x333C: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0u,8u,1u);c->ip=0x333Fu;break;
        case 0x333F: c->ip=((c->flags&0x41u)==0u)?0x3383u:0x3341u;break;
        case 0x3341: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)((uint8_t)c->ax);c->ip=0x3344u;break;
        case 0x3344: c->ip=0x3349u;break;
        case 0x3346: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x3349u;break;
        case 0x3349: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x334Cu;break;
        case 0x334C: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x334Eu;break;
        case 0x334E: c->di=c->ax;c->ip=0x3350u;break;
        case 0x3350: c->si=c->di;c->ip=0x3352u;break;
        case 0x3352: c->di=shl(c,c->di);c->ip=0x3354u;break;
        case 0x3354: c->di=arithmetic(c,c->di,c->si,0u,16u,0u);c->ip=0x3356u;break;
        case 0x3356: c->di=arithmetic(c,c->di,0x6718u,0u,16u,0u);c->ip=0x335Au;break;
        case 0x335A: push(ram,c,c->ds);c->ip=0x335Bu;break;
        case 0x335B: push(ram,c,c->di);c->ip=0x335Cu;break;
        case 0x335C: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x335Fu;break;
        case 0x335F: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x3361u;break;
        case 0x3361: c->si=c->ax;c->ip=0x3363u;break;
        case 0x3363: c->ax=shl(c,c->ax);c->ip=0x3365u;break;
        case 0x3365: c->ax=arithmetic(c,c->ax,c->si,0u,16u,0u);c->ip=0x3367u;break;
        case 0x3367: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));c->ip=0x336Au;break;
        case 0x336A: c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x336Cu;break;
        case 0x336C: c->di=arithmetic(c,c->di,0xFF25u,0u,16u,0u);c->ip=0x3370u;break;
        case 0x3370: push(ram,c,c->ss);c->ip=0x3371u;break;
        case 0x3371: push(ram,c,c->di);c->ip=0x3372u;break;
        case 0x3372: c->ax=0x0003u;c->ip=0x3375u;break;
        case 0x3375: push(ram,c,c->ax);c->ip=0x3376u;break;
        case 0x3376: push(ram,c,c->cs);push(ram,c,0x337Bu);c->cs=layout->system_cs;c->ip=0x0A3Fu;status=move(ram,size,c);if(status!=0)return PR_U5E_32E6_CALLEE;if(c->cs!=code_segment || c->ip!=0x337Bu)return PR_U5E_32E6_TRANSFER;break;
        case 0x337B: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x337Eu;break;
        case 0x337E: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0u,8u,1u);c->ip=0x3381u;break;
        case 0x3381: c->ip=((c->flags&0x40u)==0u)?0x3346u:0x3383u;break;
        case 0x3383: c->sp=c->bp;c->ip=0x3385u;break;
        case 0x3385: c->bp=pop(ram,c);c->ip=0x3386u;break;
        case 0x3386: retf(ram,c,6u);return PR_U5E_32E6_OK;
        default:return PR_U5E_32E6_TRANSFER;
        }
    }
}
