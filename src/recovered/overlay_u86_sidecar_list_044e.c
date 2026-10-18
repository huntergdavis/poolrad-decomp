/* Complete U86 +044E; docs/re/U86_SIDECAR_LIST_044E.md.
 * Ordered memory/flags primitives reused from accepted U86 +028D. */
#include "poolrad/recovered/overlay_u86_sidecar_list_044e.h"
#include "poolrad/recovered/overlay_u86_string_node_028d.h"
#include "poolrad/recovered/resident_00ba_sidecar_lookup_2199_cpu.h"
#include <string.h>
#define MASK (PR_U86_044E_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_u86_044e_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u86_044e_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u86_044e_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_u86_044e_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u86_044e_cpu *c, uint16_t left, uint16_t right,
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
static void retf(const uint8_t *ram,pr_u86_044e_cpu *c,unsigned cleanup)
{
    size_t p=physical(c->ss,c->sp);
    c->ip=word_at(ram,p);c->cs=word_at(ram,(p+2u)&MASK);
    c->sp=(uint16_t)(c->sp+4u+cleanup);
}
static void ah(pr_u86_044e_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&255u)|((uint16_t)v<<8u)); }
static uint16_t logic(pr_u86_044e_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }
static uint16_t step(pr_u86_044e_cpu *c,uint16_t v,unsigned bits,unsigned subtract)
{
    uint16_t cf=c->flags&1u;
    v=arithmetic(c,v,1u,0u,bits,subtract);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return v;
}
static void cl(pr_u86_044e_cpu *c,uint8_t v) { c->cx=(uint16_t)((c->cx&0xFF00u)|v); }
static uint16_t shift4(pr_u86_044e_cpu *c,uint16_t v) {
    uint16_t result=(uint16_t)(v<<4u),of=c->flags&0x800u;
    (void)logic(c,result,16u);c->flags|=of;
    if(v&0x1000u)c->flags|=1u;
    return result;
}
static uint16_t shift1(pr_u86_044e_cpu *c,uint16_t v) {
    uint16_t result=(uint16_t)(v<<1u);(void)logic(c,result,16u);
    if(v&0x8000u)c->flags|=1u;
    if((v^result)&0x8000u)c->flags|=0x800u;
    return result;
}
_Static_assert(sizeof(pr_u86_044e_cpu)==sizeof(pr_u86_028d_cpu),"append CPU layout");
_Static_assert(sizeof(pr_u86_044e_cpu)==sizeof(pr_res_2199_cpu),"lookup CPU layout");
static pr_u86_044e_status call(uint8_t *ram,size_t size,pr_u86_044e_cpu *c,
    const pr_u86_044e_limits *l,pr_u86_044e_diagnostic *d,unsigned append)
{
    uint16_t caller=c->cs,site=c->ip,next=(uint16_t)(site+(append?3u:5u)),segment=caller;
    if(append) {
        if(ram[physical(caller,site)]!=0xE8u || read_word(ram,caller,(uint16_t)(site+1u))!=0xFD9Cu)
            return PR_U86_044E_CODE;
    } else {
        if(ram[physical(caller,site)]!=0x9Au || read_word(ram,caller,(uint16_t)(site+1u))!=0x2199u)
            return PR_U86_044E_CODE;
        segment=read_word(ram,caller,(uint16_t)(site+3u));push(ram,c,caller);
    }
    push(ram,c,next);c->cs=segment;c->ip=append?0x028Du:0x2199u;
    d->callee=append?PR_U86_044E_APPEND:PR_U86_044E_LOOKUP;d->nested_callee=-1;d->nested_status=-1;
    if(append) {
        pr_u86_028d_cpu inner;pr_u86_028d_diagnostic nested;memcpy(&inner,c,sizeof inner);
        d->status=(int)pr_u86_string_node_028d(ram,size,&inner,l->max_list_visits,l->max_heap_steps,&nested);
        d->nested_callee=(int)nested.callee;d->nested_status=nested.status;memcpy(c,&inner,sizeof inner);
    } else {
        pr_res_2199_cpu inner;memcpy(&inner,c,sizeof inner);
        d->status=(int)pr_res_00ba_2199_cpu(ram,size,&inner,l->max_lookup_loopbacks);memcpy(c,&inner,sizeof inner);
    }
    if(d->status!=0)return PR_U86_044E_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U86_044E_TRANSFER;
    return PR_U86_044E_OK;
}
pr_u86_044e_status pr_u86_sidecar_list_044e(uint8_t *ram,size_t size,pr_u86_044e_cpu *c,
    const pr_u86_044e_limits *l,pr_u86_044e_diagnostic *d)
{
    pr_u86_044e_diagnostic local={PR_U86_044E_NO_CALLEE,-1,-1,-1};
    pr_u86_044e_status status;size_t at,visits;uint16_t caller;
    if(d==NULL)d=&local;
    *d=local;
    if(ram==NULL || c==NULL || l==NULL || size!=PR_U86_044E_RAM_SIZE || c->ip!=0x044Eu ||
       c->sp<0x0150u || c->sp>0xFFFAu || l->max_caller_visits==0u || l->max_list_visits==0u)
        return PR_U86_044E_ARGUMENT;
    visits=l->max_caller_visits;caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_U86_044E_TRANSFER;
        if(c->ip==0x04ADu) { if(visits==0u)return PR_U86_044E_LOOP_LIMIT;--visits; }
        switch(c->ip) {
        case 0x044E: push(ram,c,c->bp);c->ip=0x044F;break;
        case 0x044F: c->bp=c->sp;c->ip=0x0451;break;
        case 0x0451: c->sp=arithmetic(c,c->sp,6,0,16,1);c->ip=0x0454;break;
        case 0x0454: push(ram,c,read_word(ram,c->ds,0x5D94));c->ip=0x0458;break;
        case 0x0458: push(ram,c,read_word(ram,c->ds,0x5D92));c->ip=0x045C;break;
        case 0x045C: al(c,0x10);c->ip=0x045E;break;
        case 0x045E: push(ram,c,c->ax);c->ip=0x045F;break;
        case 0x045F: c->di=(uint16_t)(c->bp-5u);c->ip=0x0462;break;
        case 0x0462: push(ram,c,c->ss);c->ip=0x0463;break;
        case 0x0463: push(ram,c,c->di);c->ip=0x0464;break;
        case 0x0464: status=call(ram,size,c,l,d,0);if(status!=0)return status;break;
        case 0x0469: al(c,(uint8_t)logic(c,(uint8_t)c->ax,8));c->ip=0x046B;break;
        case 0x046B: c->ip=(c->flags&0x40)?0x046D:0x0490;break;
        case 0x046D: at=physical(c->ds,0x5D92);c->di=word_at(ram,at);c->es=word_at(ram,(at+2)&MASK);c->ip=0x0471;break;
        case 0x0471: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+0x96))],0,0,8,1);c->ip=0x0477;break;
        case 0x0477: c->ip=((c->flags&0x40)||!!(c->flags&0x80)!=!!(c->flags&0x800))?0x0499:0x0479;break;
        case 0x0479: at=physical(c->ds,0x6B80);c->di=word_at(ram,at);c->es=word_at(ram,(at+2)&MASK);c->ip=0x047D;break;
        case 0x047D: al(c,ram[physical(c->es,(uint16_t)(c->di+0x2E))]);c->ip=0x0481;break;
        case 0x0481: ah(c,(uint8_t)logic(c,0,8));c->ip=0x0483;break;
        case 0x0483: c->di=c->ax;c->ip=0x0485;break;
        case 0x0485: cl(c,4);c->ip=0x0487;break;
        case 0x0487: c->di=shift4(c,c->di);c->ip=0x0489;break;
        case 0x0489: (void)arithmetic(c,ram[physical(c->ds,(uint16_t)(c->di+0x5582))],12,0,8,1);c->ip=0x048E;break;
        case 0x048E: c->ip=(c->flags&0x40)?0x0490:0x0499;break;
        case 0x0490: at=physical(c->ds,0x6B80);c->di=word_at(ram,at);c->es=word_at(ram,(at+2)&MASK);c->ip=0x0494;break;
        case 0x0494: ram[physical(c->es,(uint16_t)(c->di+0x35))]=0;c->ip=0x0499;break;
        case 0x0499: at=physical(c->ds,0x6B80);c->di=word_at(ram,at);c->es=word_at(ram,(at+2)&MASK);c->ip=0x049D;break;
        case 0x049D: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+0x35))],0,0,8,1);c->ip=0x04A2;break;
        case 0x04A2: c->ip=(c->flags&0x40)?0x04A4:0x0516;break;
        case 0x04A4: ram[physical(c->ss,(uint16_t)(c->bp-1))]=1;c->ip=0x04A8;break;
        case 0x04A8: c->ip=0x04AD;break;
        case 0x04AA: at=physical(c->ss,(uint16_t)(c->bp-1));ram[at]=(uint8_t)step(c,ram[at],8,0);c->ip=0x04AD;break;
        case 0x04AD: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+6))],0,0,8,1);c->ip=0x04B1;break;
        case 0x04B1: c->ip=(c->flags&0x40)?0x04C5:0x04B3;break;
        case 0x04B3: al(c,ram[physical(c->ss,(uint16_t)(c->bp-1))]);c->ip=0x04B6;break;
        case 0x04B6: ah(c,(uint8_t)logic(c,0,8));c->ip=0x04B8;break;
        case 0x04B8: at=physical(c->ds,0x6B80);c->di=word_at(ram,at);c->es=word_at(ram,(at+2)&MASK);c->ip=0x04BC;break;
        case 0x04BC: c->di=arithmetic(c,c->di,c->ax,0,16,0);c->ip=0x04BE;break;
        case 0x04BE: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+0x3B))],0x80,0,8,1);c->ip=0x04C3;break;
        case 0x04C3: c->ip=(c->flags&0x41)?0x04C5:0x04DD;break;
        case 0x04C5: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+6))],0,0,8,1);c->ip=0x04C9;break;
        case 0x04C9: c->ip=(c->flags&0x40)?0x04CB:0x0510;break;
        case 0x04CB: al(c,ram[physical(c->ss,(uint16_t)(c->bp-1))]);c->ip=0x04CE;break;
        case 0x04CE: ah(c,(uint8_t)logic(c,0,8));c->ip=0x04D0;break;
        case 0x04D0: at=physical(c->ds,0x6B80);c->di=word_at(ram,at);c->es=word_at(ram,(at+2)&MASK);c->ip=0x04D4;break;
        case 0x04D4: c->di=arithmetic(c,c->di,c->ax,0,16,0);c->ip=0x04D6;break;
        case 0x04D6: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+0x3B))],0,0,8,1);c->ip=0x04DB;break;
        case 0x04DB: c->ip=(c->flags&0x41)?0x0510:0x04DD;break;
        case 0x04DD: al(c,ram[physical(c->ss,(uint16_t)(c->bp-1))]);c->ip=0x04E0;break;
        case 0x04E0: ah(c,(uint8_t)logic(c,0,8));c->ip=0x04E2;break;
        case 0x04E2: at=physical(c->ds,0x6B80);c->di=word_at(ram,at);c->es=word_at(ram,(at+2)&MASK);c->ip=0x04E6;break;
        case 0x04E6: c->di=arithmetic(c,c->di,c->ax,0,16,0);c->ip=0x04E8;break;
        case 0x04E8: al(c,ram[physical(c->es,(uint16_t)(c->di+0x3B))]);c->ip=0x04EC;break;
        case 0x04EC: push(ram,c,c->ax);c->ip=0x04ED;break;
        case 0x04ED: push(ram,c,c->cs);c->ip=0x04EE;break;
        case 0x04EE: status=call(ram,size,c,l,d,1);if(status!=0)return status;break;
        case 0x04F1: c->cx=read_word(ram,c->ds,0x6B80);c->ip=0x04F5;break;
        case 0x04F5: c->bx=read_word(ram,c->ds,0x6B82);c->ip=0x04F9;break;
        case 0x04F9: al(c,ram[physical(c->ds,0x4843)]);c->ip=0x04FC;break;
        case 0x04FC: ah(c,(uint8_t)logic(c,0,8));c->ip=0x04FE;break;
        case 0x04FE: c->di=c->ax;c->ip=0x0500;break;
        case 0x0500: c->di=shift1(c,c->di);c->ip=0x0502;break;
        case 0x0502: c->di=shift1(c,c->di);c->ip=0x0504;break;
        case 0x0504: put_word(ram,c->ds,(uint16_t)(c->di+0x477F),c->cx);c->ip=0x0508;break;
        case 0x0508: put_word(ram,c->ds,(uint16_t)(c->di+0x4781),c->bx);c->ip=0x050C;break;
        case 0x050C: at=physical(c->ds,0x4843);ram[at]=(uint8_t)step(c,ram[at],8,0);c->ip=0x0510;break;
        case 0x0510: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp-1))],3,0,8,1);c->ip=0x0514;break;
        case 0x0514: c->ip=(c->flags&0x40)?0x0516:0x04AA;break;
        case 0x0516: c->sp=c->bp;c->ip=0x0518;break;
        case 0x0518: c->bp=pop(ram,c);c->ip=0x0519;break;
        case 0x0519: retf(ram,c,2);return PR_U86_044E_OK;
        default:return PR_U86_044E_TRANSFER;
        }
    }
}
