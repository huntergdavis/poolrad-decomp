#include "overlay_loaded_contracts.h"
/* Complete U5E +32E6 with raw U3C6 slot accessors.
 * Memory/arithmetic primitives adapted from accepted U3C2 +0909;
 * slot composition follows accepted U5E +1BF8. See U5E_CALLER_32E6.md. */
#include "poolrad/recovered/overlay_u5e_caller_32e6.h"
#include "poolrad/recovered/overlay_u3c2_caller_0909.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/overlay_u3c6_slot_getters_cpu.h"
#include <string.h>
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
    pr_u3c6_getter_cpu inner;pr_u3c6_getter_status status;
    _Static_assert(sizeof inner==sizeof *c,"getter CPU layout");
    memcpy(&inner,c,sizeof inner);
    status=pr_u3c6_slot_getter_cpu(ram,PR_U5E_32E6_RAM_SIZE,&inner,remaining);
    memcpy(c,&inner,sizeof inner);
    if(status==PR_U3C6_GETTER_OK)return PR_U5E_32E6_OK;
    if(status==PR_U3C6_GETTER_LIMIT)return PR_U5E_32E6_SLOT_LIMIT;
    if(status==PR_U3C6_GETTER_TRANSFER)return PR_U5E_32E6_TRANSFER;
    return PR_U5E_32E6_CALLEE;
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
    /* Read the required loaded 0xEA bindings before CPU/RAM mutation. */
    if (!(pr_overlay_stub_matches(ram, c->cs, 0x32F2u, 0x005Cu, 0x0BE7u, layout->slots_cs) &&
          pr_overlay_stub_matches(ram, c->cs, 0x32FEu, 0x0061u, 0x0C0Fu, layout->slots_cs) &&
          pr_overlay_stub_matches(ram, c->cs, 0x3311u, 0x0066u, 0x0C37u, layout->slots_cs) &&
          pr_overlay_stub_matches(ram, c->cs, 0x331Fu, 0x003Eu, 0x0909u, layout->chain_cs) &&
          pr_u3c2_0909_stubs_loaded(ram, layout->chain_cs, layout->slots_cs)))
        return PR_U5E_32E6_ARGUMENT;
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
