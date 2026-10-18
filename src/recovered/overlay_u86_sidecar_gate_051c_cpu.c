/* Complete U86 +051C; docs/re/U86_SIDECAR_GATE_051C_CPU.md.
 * Ordered memory/flag primitives reused from accepted U86 +044E. */
#include "poolrad/recovered/overlay_u86_sidecar_gate_051c_cpu.h"
#include "poolrad/recovered/overlay_u86_record_predicate_323d.h"
#include <string.h>
#define MASK (PR_U86_051C_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_u86_051c_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u86_051c_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u86_051c_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_u86_051c_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u86_051c_cpu *c, uint16_t left, uint16_t right,
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
static void retf(const uint8_t *ram,pr_u86_051c_cpu *c,unsigned cleanup)
{
    size_t p=physical(c->ss,c->sp);
    c->ip=word_at(ram,p);c->cs=word_at(ram,(p+2u)&MASK);
    c->sp=(uint16_t)(c->sp+4u+cleanup);
}
static void ah(pr_u86_051c_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&255u)|((uint16_t)v<<8u)); }
static uint16_t logic(pr_u86_051c_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }
static uint16_t step(pr_u86_051c_cpu *c,uint16_t v,unsigned bits,unsigned subtract)
{
    uint16_t cf=c->flags&1u;
    v=arithmetic(c,v,1u,0u,bits,subtract);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return v;
}
static uint16_t shift1(pr_u86_051c_cpu *c,uint16_t v) {
    uint16_t result=(uint16_t)(v<<1u);(void)logic(c,result,16u);
    if(v&0x8000u)c->flags|=1u;
    if((v^result)&0x8000u)c->flags|=0x800u;
    return result;
}

_Static_assert(sizeof(pr_u86_051c_cpu)==sizeof(pr_u86_323d_cpu),"predicate CPU layout");
_Static_assert(sizeof(pr_u86_051c_cpu)==sizeof(pr_u86_044e_cpu),"sidecar CPU layout");
static pr_u86_051c_status call(uint8_t *ram,size_t size,pr_u86_051c_cpu *c,
    const pr_u86_051c_limits *l,pr_u86_051c_diagnostic *d,unsigned sidecar)
{
    uint16_t caller=c->cs,site=c->ip,next=(uint16_t)(site+3u);
    uint16_t target=sidecar?0x044Eu:0x323Du;
    if(ram[physical(caller,site)]!=0xE8u ||
       read_word(ram,caller,(uint16_t)(site+1u))!=(uint16_t)(target-next))
        return PR_U86_051C_CODE;
    push(ram,c,next);c->ip=target;
    d->callee=sidecar?PR_U86_051C_SIDECAR:PR_U86_051C_PREDICATE;
    d->sidecar=(pr_u86_044e_diagnostic){PR_U86_044E_NO_CALLEE,-1,-1,-1};
    if(sidecar) {
        pr_u86_044e_cpu inner;memcpy(&inner,c,sizeof inner);
        d->status=(int)pr_u86_sidecar_list_044e(ram,size,&inner,&l->sidecar,&d->sidecar);
        memcpy(c,&inner,sizeof inner);
    } else {
        pr_u86_323d_cpu inner;memcpy(&inner,c,sizeof inner);
        d->status=(int)pr_u86_record_predicate_323d(ram,size,&inner);
        memcpy(c,&inner,sizeof inner);
    }
    if(d->status!=0)return PR_U86_051C_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U86_051C_TRANSFER;
    return PR_U86_051C_OK;
}
pr_u86_051c_status pr_u86_sidecar_gate_051c_cpu(uint8_t *ram,size_t size,pr_u86_051c_cpu *c,
    const pr_u86_051c_limits *l,pr_u86_051c_diagnostic *d)
{
    pr_u86_051c_diagnostic local={PR_U86_051C_NO_CALLEE,-1,{PR_U86_044E_NO_CALLEE,-1,-1,-1}};
    pr_u86_051c_status status;size_t at,clears,records;uint16_t caller;
    if(d==NULL)d=&local;
    *d=local;
    if(ram==NULL || c==NULL || l==NULL || size!=PR_U86_051C_RAM_SIZE || c->ip!=0x051Cu ||
       c->sp<0x0160u || c->sp>0xFFF8u || l->max_clear_visits==0u || l->max_record_visits==0u ||
       l->sidecar.max_caller_visits==0u || l->sidecar.max_list_visits==0u)
        return PR_U86_051C_ARGUMENT;
    clears=l->max_clear_visits;records=l->max_record_visits;caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_U86_051C_TRANSFER;
        if(c->ip==0x052Bu) { if(clears==0u)return PR_U86_051C_LOOP_LIMIT;--clears; }
        if(c->ip==0x0560u) { if(records==0u)return PR_U86_051C_LOOP_LIMIT;--records; }
        switch(c->ip) {
        case 0x051C: push(ram,c,c->bp);c->ip=0x051D;break;
        case 0x051D: c->bp=c->sp;c->ip=0x051F;break;
        case 0x051F: c->sp=arithmetic(c,c->sp,2,0,16,1);c->ip=0x0522;break;
        case 0x0522: ram[physical(c->ss,(uint16_t)(c->bp-1))]=0;c->ip=0x0526;break;
        case 0x0526: c->ip=0x052B;break;
        case 0x0528: at=physical(c->ss,(uint16_t)(c->bp-1));ram[at]=(uint8_t)step(c,ram[at],8,0);c->ip=0x052B;break;
        case 0x052B: al(c,ram[physical(c->ss,(uint16_t)(c->bp-1))]);c->ip=0x052E;break;
        case 0x052E: ah(c,(uint8_t)logic(c,0,8));c->ip=0x0530;break;
        case 0x0530: c->di=c->ax;c->ip=0x0532;break;
        case 0x0532: c->di=shift1(c,c->di);c->ip=0x0534;break;
        case 0x0534: c->di=shift1(c,c->di);c->ip=0x0536;break;
        case 0x0536: c->ax=logic(c,0,16);c->ip=0x0538;break;
        case 0x0538: put_word(ram,c->ds,(uint16_t)(c->di+0x477F),c->ax);c->ip=0x053C;break;
        case 0x053C: put_word(ram,c->ds,(uint16_t)(c->di+0x4781),c->ax);c->ip=0x0540;break;
        case 0x0540: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp-1))],0x30,0,8,1);c->ip=0x0544;break;
        case 0x0544: c->ip=(c->flags&0x40)?0x0546:0x0528;break;
        case 0x0546: ram[physical(c->ds,0x4843)]=0;c->ip=0x054B;break;
        case 0x054B: at=physical(c->ds,0x5D92);c->di=word_at(ram,at);c->es=word_at(ram,(at+2)&MASK);c->ip=0x054F;break;
        case 0x054F: c->ax=read_word(ram,c->es,(uint16_t)(c->di+0xC8));c->ip=0x0554;break;
        case 0x0554: c->dx=read_word(ram,c->es,(uint16_t)(c->di+0xCA));c->ip=0x0559;break;
        case 0x0559: put_word(ram,c->ds,0x6B80,c->ax);c->ip=0x055C;break;
        case 0x055C: put_word(ram,c->ds,0x6B82,c->dx);c->ip=0x0560;break;
        case 0x0560: c->ax=read_word(ram,c->ds,0x6B80);c->ip=0x0563;break;
        case 0x0563: c->ax=logic(c,(uint16_t)(c->ax|read_word(ram,c->ds,0x6B82)),16);c->ip=0x0567;break;
        case 0x0567: c->ip=(c->flags&0x40)?0x0596:0x0569;break;
        case 0x0569: push(ram,c,read_word(ram,c->ds,0x6B82));c->ip=0x056D;break;
        case 0x056D: push(ram,c,read_word(ram,c->ds,0x6B80));c->ip=0x0571;break;
        case 0x0571: push(ram,c,c->cs);c->ip=0x0572;break;
        case 0x0572: status=call(ram,size,c,l,d,0);if(status!=0)return status;break;
        case 0x0575: al(c,(uint8_t)logic(c,(uint8_t)c->ax,8));c->ip=0x0577;break;
        case 0x0577: c->ip=(c->flags&0x40)?0x0581:0x0579;break;
        case 0x0579: al(c,ram[physical(c->ss,(uint16_t)(c->bp+6))]);c->ip=0x057C;break;
        case 0x057C: push(ram,c,c->ax);c->ip=0x057D;break;
        case 0x057D: push(ram,c,c->cs);c->ip=0x057E;break;
        case 0x057E: status=call(ram,size,c,l,d,1);if(status!=0)return status;break;
        case 0x0581: at=physical(c->ds,0x6B80);c->di=word_at(ram,at);c->es=word_at(ram,(at+2)&MASK);c->ip=0x0585;break;
        case 0x0585: c->ax=read_word(ram,c->es,(uint16_t)(c->di+0x2A));c->ip=0x0589;break;
        case 0x0589: c->dx=read_word(ram,c->es,(uint16_t)(c->di+0x2C));c->ip=0x058D;break;
        case 0x058D: put_word(ram,c->ds,0x6B80,c->ax);c->ip=0x0590;break;
        case 0x0590: put_word(ram,c->ds,0x6B82,c->dx);c->ip=0x0594;break;
        case 0x0594: c->ip=0x0560;break;
        case 0x0596: c->sp=c->bp;c->ip=0x0598;break;
        case 0x0598: c->bp=pop(ram,c);c->ip=0x0599;break;
        case 0x0599: retf(ram,c,2);return PR_U86_051C_OK;
        default:return PR_U86_051C_TRANSFER;
        }
    }
}
