#include "overlay_loaded_contracts.h"
/* U9E +0160 with a precise pending dynamic-dispatch boundary.
 * Primitives: accepted U3C6 getter / U5E +32E6 engines.
 * See docs/re/U9E_SIDECAR_0160_CPU.md. */
#include "poolrad/recovered/overlay_u9e_sidecar_0160_cpu.h"
#include "poolrad/recovered/overlay_u3c6_slot_getters_cpu.h"
#include "poolrad/recovered/overlay_u3c2_caller_0909.h"
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include <string.h>
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
static uint16_t shl(pr_u9e_0160_cpu *c, uint16_t left)
{
    uint16_t result=(uint16_t)(left << 1u);
    /* Intermediate AF follows the accepted U7F +0A01 normal/386 convention. */
    c->flags&=(uint16_t)~0x08D5u;
    c->flags|=0x10u;
    if (left&0x8000u)c->flags|=1u;
    if ((left^result)&0x8000u)c->flags|=0x800u;
    result_flags(c,result,0x8000u);return result;
}
static void retf(const uint8_t *ram, pr_u9e_0160_cpu *c, unsigned cleanup)
{
    c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+cleanup);
}


static void ah(pr_u9e_0160_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&255u)|((uint16_t)v<<8u)); }
static uint16_t logic(pr_u9e_0160_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }
static uint8_t increment(pr_u9e_0160_cpu *c,uint8_t v) { uint16_t cf=c->flags&1u;v=(uint8_t)arithmetic(c,v,1u,0u,8u,0u);c->flags=(uint16_t)((c->flags&~1u)|cf);return v; }



static void dl(pr_u9e_0160_cpu *c,uint8_t v) { c->dx=(uint16_t)((c->dx&0xFF00u)|v); }
static int bound(const uint8_t *r,const pr_u9e_0160_cpu *c,uint16_t off,uint16_t seg)
{
    return r[physical(c->cs,c->ip)]==0x9A &&
        read_word(r,c->cs,(uint16_t)(c->ip+1u))==off &&
        read_word(r,c->cs,(uint16_t)(c->ip+3u))==seg;
}
#define ADAPT(type,expr) do { type inner;_Static_assert(sizeof inner==sizeof *c,"CPU layout");     memcpy(&inner,c,sizeof inner);status=(int)(expr);memcpy(c,&inner,sizeof inner); } while(0)
static int dependency(uint8_t *ram,size_t size,pr_u9e_0160_cpu *c,int which,
    const pr_u9e_0160_layout *layout,const pr_u9e_0160_limits *l)
{
    int status;
    switch(which) {
    case 0: ADAPT(pr_res_2199_cpu,pr_res_00ba_2199_cpu(ram,size,&inner,l->lookup_loopbacks));break;
    case 1: ADAPT(pr_system_leaf_cpu,pr_system_set_in_0dc0(ram,size,&inner));break;
    case 2: ADAPT(pr_system_leaf_cpu,pr_system_move_1685(ram,size,&inner));break;
    case 3: ADAPT(pr_u3c6_getter_cpu,pr_u3c6_slot_getter_cpu(ram,size,&inner,l->slot_visits));break;
    default: ADAPT(pr_u3c2_0909_cpu,pr_u3c2_caller_0909(ram,size,&inner,layout->slots_cs,
        l->chain_iterations,l->line_iterations,l->sort_loopbacks));break;
    }
    return status;
}
#undef ADAPT

pr_u9e_0160_status pr_u9e_sidecar_0160_cpu(uint8_t *ram,size_t size,pr_u9e_0160_cpu *c,
    const pr_u9e_0160_layout *layout,const pr_u9e_0160_limits *l,pr_u9e_0160_diagnostic *d)
{
    pr_u9e_0160_diagnostic fallback={-1,-1,0,0};uint16_t caller;size_t party=0,results=0,p;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || c==NULL || layout==NULL || l==NULL || size!=PR_U9E_0160_RAM_SIZE ||
       c->ip!=0x160u || c->sp<0x186u || c->sp>0xFFF6u ||
       l->chain_iterations==0u || l->line_iterations==0u || l->sort_loopbacks==0u)
        return PR_U9E_0160_ARGUMENT;
    /* Read the required loaded 0xEA bindings before CPU/RAM mutation. */
    if (!(pr_u3c2_0909_stubs_loaded(ram, layout->chain_cs, layout->slots_cs)))
        return PR_U9E_0160_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_U9E_0160_TRANSFER;
        if(c->ip==0x1C3u) { if(party==l->party_visits)return PR_U9E_0160_LIMIT;++party; }
        if(c->ip==0x26Eu) { if(results==l->result_visits)return PR_U9E_0160_LIMIT;++results; }
        switch(c->ip) {
        case 0x001E:
            p=physical(c->ds,(uint16_t)(c->di+0x6828u));
            d->handler_offset=word_at(ram,p);d->handler_segment=word_at(ram,(p+2u)&MASK);
            return PR_U9E_0160_PENDING;
        case 0x0000: push(ram,c,c->bp);c->ip=0x0001u;break;
        case 0x0001: c->bp=c->sp;c->ip=0x0003u;break;
        case 0x0003: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+14)));c->ip=0x0006u;break;
        case 0x0006: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+12)));c->ip=0x0009u;break;
        case 0x0009: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+10)));c->ip=0x000Cu;break;
        case 0x000C: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x000Fu;break;
        case 0x000F: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+6))]));c->ip=0x0012u;break;
        case 0x0012: push(ram,c,c->ax);c->ip=0x0013u;break;
        case 0x0013: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+16))]));c->ip=0x0016u;break;
        case 0x0016: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x0018u;break;
        case 0x0018: c->di=c->ax;c->ip=0x001Au;break;
        case 0x001A: c->di=shl(c,c->di);c->ip=0x001Cu;break;
        case 0x001C: c->di=shl(c,c->di);c->ip=0x001Eu;break;
        case 0x0160: push(ram,c,c->bp);c->ip=0x0161u;break;
        case 0x0161: c->bp=c->sp;c->ip=0x0163u;break;
        case 0x0163: c->sp=arithmetic(c,c->sp,0x00E4u,0u,16u,1u);c->ip=0x0167u;break;
        case 0x0167: ram[physical(c->ss,(uint16_t)(c->bp+-225))]=(uint8_t)(0x0000u);c->ip=0x016Cu;break;
        case 0x016C: c->ax=read_word(ram,c->ds,(uint16_t)(23958));c->ip=0x016Fu;break;
        case 0x016F: c->dx=read_word(ram,c->ds,(uint16_t)(23960));c->ip=0x0173u;break;
        case 0x0173: put_word(ram,c->ss,(uint16_t)(c->bp+-220),c->ax);c->ip=0x0177u;break;
        case 0x0177: put_word(ram,c->ss,(uint16_t)(c->bp+-218),c->dx);c->ip=0x017Bu;break;
        case 0x017B: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+10)));c->ip=0x017Eu;break;
        case 0x017E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0181u;break;
        case 0x0181: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+6))]));c->ip=0x0184u;break;
        case 0x0184: push(ram,c,c->ax);c->ip=0x0185u;break;
        case 0x0185: c->di=(uint16_t)(c->bp+-224);c->ip=0x0189u;break;
        case 0x0189: push(ram,c,c->ss);c->ip=0x018Au;break;
        case 0x018A: push(ram,c,c->di);c->ip=0x018Bu;break;
        case 0x018B: if(!bound(ram,c,0x2199u,layout->resident_cs))return PR_U9E_0160_CODE;push(ram,c,c->cs);push(ram,c,0x0190u);c->cs=layout->resident_cs;c->ip=0x2199u;d->callee=0;d->status=dependency(ram,size,c,0,layout,l);if(d->status!=0)return PR_U9E_0160_CALLEE;if(c->cs!=caller || c->ip!=0x0190u)return PR_U9E_0160_TRANSFER;break;
        case 0x0190: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax|(uint8_t)c->ax),8u)));c->ip=0x0192u;break;
        case 0x0192: c->ip=((c->flags&0x40u)!=0u)?0x019Cu:0x0194u;break;
        case 0x0194: ram[physical(c->ss,(uint16_t)(c->bp+-225))]=(uint8_t)(0x0001u);c->ip=0x0199u;break;
        case 0x0199: c->ip=0x02D4u;break;
        case 0x019C: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+6))]));c->ip=0x019Fu;break;
        case 0x019F: push(ram,c,c->ax);c->ip=0x01A0u;break;
        case 0x01A0: c->di=0x0140u;c->ip=0x01A3u;break;
        case 0x01A3: push(ram,c,c->cs);c->ip=0x01A4u;break;
        case 0x01A4: push(ram,c,c->di);c->ip=0x01A5u;break;
        case 0x01A5: if(!bound(ram,c,0x0DC0u,layout->system_cs))return PR_U9E_0160_CODE;push(ram,c,c->cs);push(ram,c,0x01AAu);c->cs=layout->system_cs;c->ip=0x0DC0u;d->callee=1;d->status=dependency(ram,size,c,1,layout,l);if(d->status!=0)return PR_U9E_0160_CALLEE;if(c->cs!=caller || c->ip!=0x01AAu)return PR_U9E_0160_TRANSFER;break;
        case 0x01AA: c->ip=((c->flags&0x40u)==0u)?0x01AFu:0x01ACu;break;
        case 0x01AC: c->ip=0x02D4u;break;
        case 0x01AF: c->di=0x671Bu;c->ip=0x01B2u;break;
        case 0x01B2: push(ram,c,c->ds);c->ip=0x01B3u;break;
        case 0x01B3: push(ram,c,c->di);c->ip=0x01B4u;break;
        case 0x01B4: c->di=(uint16_t)(c->bp+-216);c->ip=0x01B8u;break;
        case 0x01B8: push(ram,c,c->ss);c->ip=0x01B9u;break;
        case 0x01B9: push(ram,c,c->di);c->ip=0x01BAu;break;
        case 0x01BA: c->ax=0x00D8u;c->ip=0x01BDu;break;
        case 0x01BD: push(ram,c,c->ax);c->ip=0x01BEu;break;
        case 0x01BE: if(!bound(ram,c,0x1685u,layout->system_cs))return PR_U9E_0160_CODE;push(ram,c,c->cs);push(ram,c,0x01C3u);c->cs=layout->system_cs;c->ip=0x1685u;d->callee=2;d->status=dependency(ram,size,c,2,layout,l);if(d->status!=0)return PR_U9E_0160_CALLEE;if(c->cs!=caller || c->ip!=0x01C3u)return PR_U9E_0160_TRANSFER;break;
        case 0x01C3: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-220));c->ip=0x01C7u;break;
        case 0x01C7: c->ax=logic(c,(uint16_t)(c->ax|read_word(ram,c->ss,(uint16_t)(c->bp+-218))),16u);c->ip=0x01CBu;break;
        case 0x01CB: c->ip=((c->flags&0x40u)==0u)?0x01D0u:0x01CDu;break;
        case 0x01CD: c->ip=0x02C0u;break;
        case 0x01D0: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-225))],0x0000u,0u,8u,1u);c->ip=0x01D5u;break;
        case 0x01D5: c->ip=((c->flags&0x40u)!=0u)?0x01DAu:0x01D7u;break;
        case 0x01D7: c->ip=0x02C0u;break;
        case 0x01DA: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-218)));c->ip=0x01DEu;break;
        case 0x01DE: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-220)));c->ip=0x01E2u;break;
        case 0x01E2: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+6))]));c->ip=0x01E5u;break;
        case 0x01E5: push(ram,c,c->ax);c->ip=0x01E6u;break;
        case 0x01E6: c->di=(uint16_t)(c->bp+-224);c->ip=0x01EAu;break;
        case 0x01EA: push(ram,c,c->ss);c->ip=0x01EBu;break;
        case 0x01EB: push(ram,c,c->di);c->ip=0x01ECu;break;
        case 0x01EC: if(!bound(ram,c,0x2199u,layout->resident_cs))return PR_U9E_0160_CODE;push(ram,c,c->cs);push(ram,c,0x01F1u);c->cs=layout->resident_cs;c->ip=0x2199u;d->callee=0;d->status=dependency(ram,size,c,0,layout,l);if(d->status!=0)return PR_U9E_0160_CALLEE;if(c->cs!=caller || c->ip!=0x01F1u)return PR_U9E_0160_TRANSFER;break;
        case 0x01F1: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax|(uint8_t)c->ax),8u)));c->ip=0x01F3u;break;
        case 0x01F3: c->ip=((c->flags&0x40u)==0u)?0x01F8u:0x01F5u;break;
        case 0x01F5: c->ip=0x02A7u;break;
        case 0x01F8: (void)arithmetic(c,ram[physical(c->ds,(uint16_t)(18931))],0x0005u,0u,8u,1u);c->ip=0x01FDu;break;
        case 0x01FD: c->ip=((c->flags&0x40u)!=0u)?0x0202u:0x01FFu;break;
        case 0x01FF: c->ip=0x02A2u;break;
        case 0x0202: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+6))],0x0031u,0u,8u,1u);c->ip=0x0206u;break;
        case 0x0206: c->ip=((c->flags&0x40u)==0u)?0x020Fu:0x0208u;break;
        case 0x0208: ram[physical(c->ss,(uint16_t)(c->bp+-227))]=(uint8_t)(0x0006u);c->ip=0x020Du;break;
        case 0x020D: c->ip=0x0214u;break;
        case 0x020F: ram[physical(c->ss,(uint16_t)(c->bp+-227))]=(uint8_t)(0x0001u);c->ip=0x0214u;break;
        case 0x0214: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-218)));c->ip=0x0218u;break;
        case 0x0218: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-220)));c->ip=0x021Cu;break;
        case 0x021C: if(!bound(ram,c,0x0BE7u,layout->slots_cs))return PR_U9E_0160_CODE;push(ram,c,c->cs);push(ram,c,0x0221u);c->cs=layout->slots_cs;c->ip=0x0BE7u;d->callee=3;d->status=dependency(ram,size,c,3,layout,l);if(d->status!=0)return PR_U9E_0160_CALLEE;if(c->cs!=caller || c->ip!=0x0221u)return PR_U9E_0160_TRANSFER;break;
        case 0x0221: push(ram,c,c->ax);c->ip=0x0222u;break;
        case 0x0222: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-218)));c->ip=0x0226u;break;
        case 0x0226: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-220)));c->ip=0x022Au;break;
        case 0x022A: if(!bound(ram,c,0x0C0Fu,layout->slots_cs))return PR_U9E_0160_CODE;push(ram,c,c->cs);push(ram,c,0x022Fu);c->cs=layout->slots_cs;c->ip=0x0C0Fu;d->callee=3;d->status=dependency(ram,size,c,3,layout,l);if(d->status!=0)return PR_U9E_0160_CALLEE;if(c->cs!=caller || c->ip!=0x022Fu)return PR_U9E_0160_TRANSFER;break;
        case 0x022F: push(ram,c,c->ax);c->ip=0x0230u;break;
        case 0x0230: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-227))]));c->ip=0x0234u;break;
        case 0x0234: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x0236u;break;
        case 0x0236: push(ram,c,c->ax);c->ip=0x0237u;break;
        case 0x0237: al(c,(uint8_t)(0x00FFu));c->ip=0x0239u;break;
        case 0x0239: push(ram,c,c->ax);c->ip=0x023Au;break;
        case 0x023A: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-218)));c->ip=0x023Eu;break;
        case 0x023E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-220)));c->ip=0x0242u;break;
        case 0x0242: if(!bound(ram,c,0x0C37u,layout->slots_cs))return PR_U9E_0160_CODE;push(ram,c,c->cs);push(ram,c,0x0247u);c->cs=layout->slots_cs;c->ip=0x0C37u;d->callee=3;d->status=dependency(ram,size,c,3,layout,l);if(d->status!=0)return PR_U9E_0160_CALLEE;if(c->cs!=caller || c->ip!=0x0247u)return PR_U9E_0160_TRANSFER;break;
        case 0x0247: push(ram,c,c->ax);c->ip=0x0248u;break;
        case 0x0248: push(ram,c,read_word(ram,c->ds,(uint16_t)(26392)));c->ip=0x024Cu;break;
        case 0x024C: push(ram,c,read_word(ram,c->ds,(uint16_t)(26390)));c->ip=0x0250u;break;
        case 0x0250: if(!bound(ram,c,0x0909u,layout->chain_cs))return PR_U9E_0160_CODE;push(ram,c,c->cs);push(ram,c,0x0255u);c->cs=layout->chain_cs;c->ip=0x0909u;d->callee=4;d->status=dependency(ram,size,c,4,layout,l);if(d->status!=0)return PR_U9E_0160_CALLEE;if(c->cs!=caller || c->ip!=0x0255u)return PR_U9E_0160_TRANSFER;break;
        case 0x0255: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(26394))]));c->ip=0x0258u;break;
        case 0x0258: ram[physical(c->ss,(uint16_t)(c->bp+-228))]=(uint8_t)((uint8_t)c->ax);c->ip=0x025Cu;break;
        case 0x025C: al(c,(uint8_t)(0x0001u));c->ip=0x025Eu;break;
        case 0x025E: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-228))],0u,8u,1u);c->ip=0x0262u;break;
        case 0x0262: c->ip=((c->flags&0x41u)==0u)?0x02A0u:0x0264u;break;
        case 0x0264: ram[physical(c->ss,(uint16_t)(c->bp+-226))]=(uint8_t)((uint8_t)c->ax);c->ip=0x0268u;break;
        case 0x0268: c->ip=0x026Eu;break;
        case 0x026A: ram[physical(c->ss,(uint16_t)(c->bp+-226))]=(uint8_t)(increment(c,ram[physical(c->ss,(uint16_t)(c->bp+-226))]));c->ip=0x026Eu;break;
        case 0x026E: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+10)));c->ip=0x0271u;break;
        case 0x0271: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x0274u;break;
        case 0x0274: if(!bound(ram,c,0x0C5Fu,layout->slots_cs))return PR_U9E_0160_CODE;push(ram,c,c->cs);push(ram,c,0x0279u);c->cs=layout->slots_cs;c->ip=0x0C5Fu;d->callee=3;d->status=dependency(ram,size,c,3,layout,l);if(d->status!=0)return PR_U9E_0160_CALLEE;if(c->cs!=caller || c->ip!=0x0279u)return PR_U9E_0160_TRANSFER;break;
        case 0x0279: dl(c,(uint8_t)((uint8_t)c->ax));c->ip=0x027Bu;break;
        case 0x027B: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-226))]));c->ip=0x027Fu;break;
        case 0x027F: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x0281u;break;
        case 0x0281: c->di=c->ax;c->ip=0x0283u;break;
        case 0x0283: c->si=c->di;c->ip=0x0285u;break;
        case 0x0285: c->di=shl(c,c->di);c->ip=0x0287u;break;
        case 0x0287: c->di=arithmetic(c,c->di,c->si,0u,16u,0u);c->ip=0x0289u;break;
        case 0x0289: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+26392))]));c->ip=0x028Du;break;
        case 0x028D: (void)arithmetic(c,(uint8_t)c->ax,(uint8_t)c->dx,0u,8u,1u);c->ip=0x028Fu;break;
        case 0x028F: c->ip=((c->flags&0x40u)==0u)?0x0296u:0x0291u;break;
        case 0x0291: ram[physical(c->ss,(uint16_t)(c->bp+-225))]=(uint8_t)(0x0001u);c->ip=0x0296u;break;
        case 0x0296: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-226))]));c->ip=0x029Au;break;
        case 0x029A: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-228))],0u,8u,1u);c->ip=0x029Eu;break;
        case 0x029E: c->ip=((c->flags&0x40u)==0u)?0x026Au:0x02A0u;break;
        case 0x02A0: c->ip=0x02A7u;break;
        case 0x02A2: ram[physical(c->ss,(uint16_t)(c->bp+-225))]=(uint8_t)(0x0001u);c->ip=0x02A7u;break;
        case 0x02A7: p=physical(c->ss,(uint16_t)(c->bp+-220));c->di=word_at(ram,p);c->es=word_at(ram,(p+2u)&MASK);c->ip=0x02ABu;break;
        case 0x02AB: c->ax=read_word(ram,c->es,(uint16_t)(c->di+260));c->ip=0x02B0u;break;
        case 0x02B0: c->dx=read_word(ram,c->es,(uint16_t)(c->di+262));c->ip=0x02B5u;break;
        case 0x02B5: put_word(ram,c->ss,(uint16_t)(c->bp+-220),c->ax);c->ip=0x02B9u;break;
        case 0x02B9: put_word(ram,c->ss,(uint16_t)(c->bp+-218),c->dx);c->ip=0x02BDu;break;
        case 0x02BD: c->ip=0x01C3u;break;
        case 0x02C0: c->di=(uint16_t)(c->bp+-216);c->ip=0x02C4u;break;
        case 0x02C4: push(ram,c,c->ss);c->ip=0x02C5u;break;
        case 0x02C5: push(ram,c,c->di);c->ip=0x02C6u;break;
        case 0x02C6: c->di=0x671Bu;c->ip=0x02C9u;break;
        case 0x02C9: push(ram,c,c->ds);c->ip=0x02CAu;break;
        case 0x02CA: push(ram,c,c->di);c->ip=0x02CBu;break;
        case 0x02CB: c->ax=0x00D8u;c->ip=0x02CEu;break;
        case 0x02CE: push(ram,c,c->ax);c->ip=0x02CFu;break;
        case 0x02CF: if(!bound(ram,c,0x1685u,layout->system_cs))return PR_U9E_0160_CODE;push(ram,c,c->cs);push(ram,c,0x02D4u);c->cs=layout->system_cs;c->ip=0x1685u;d->callee=2;d->status=dependency(ram,size,c,2,layout,l);if(d->status!=0)return PR_U9E_0160_CALLEE;if(c->cs!=caller || c->ip!=0x02D4u)return PR_U9E_0160_TRANSFER;break;
        case 0x02D4: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-225))],0x0000u,0u,8u,1u);c->ip=0x02D9u;break;
        case 0x02D9: c->ip=((c->flags&0x40u)!=0u)?0x02F4u:0x02DBu;break;
        case 0x02DB: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+6))]));c->ip=0x02DEu;break;
        case 0x02DE: push(ram,c,c->ax);c->ip=0x02DFu;break;
        case 0x02DF: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+10)));c->ip=0x02E2u;break;
        case 0x02E2: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+8)));c->ip=0x02E5u;break;
        case 0x02E5: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-222)));c->ip=0x02E9u;break;
        case 0x02E9: push(ram,c,read_word(ram,c->ss,(uint16_t)(c->bp+-224)));c->ip=0x02EDu;break;
        case 0x02ED: al(c,(uint8_t)(0x0000u));c->ip=0x02EFu;break;
        case 0x02EF: push(ram,c,c->ax);c->ip=0x02F0u;break;
        case 0x02F0: push(ram,c,c->cs);c->ip=0x02F1u;break;
        case 0x02F1: if(ram[physical(c->cs,c->ip)]!=0xE8 || read_word(ram,c->cs,0x2F2u)!=0xFD0Cu)return PR_U9E_0160_CODE;push(ram,c,0x2F4u);c->ip=0;break;
        case 0x02F4: c->sp=c->bp;c->ip=0x02F6u;break;
        case 0x02F6: c->bp=pop(ram,c);c->ip=0x02F7u;break;
        case 0x02F7: retf(ram,c,6u);return PR_U9E_0160_OK;
        default:return PR_U9E_0160_TRANSFER;
        }
    }
}
