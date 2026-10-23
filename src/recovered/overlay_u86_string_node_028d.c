/* Complete U86 +028D; docs/re/U86_STRING_NODE_028D.md.
 * Memory/flags primitives follow accepted U1C +18F8/U3C2 +0909;
 * MUL profile follows accepted U86 +07D6. */
#include "poolrad/recovered/overlay_u86_string_node_028d.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#define MASK (PR_U86_028D_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_u86_028d_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u86_028d_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u86_028d_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_u86_028d_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u86_028d_cpu *c, uint16_t left, uint16_t right,
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
static void retf(const uint8_t *ram,pr_u86_028d_cpu *c,unsigned cleanup)
{
    size_t p=physical(c->ss,c->sp);
    c->ip=word_at(ram,p);c->cs=word_at(ram,(p+2u)&MASK);
    c->sp=(uint16_t)(c->sp+4u+cleanup);
}
static void ah(pr_u86_028d_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&255u)|((uint16_t)v<<8u)); }
static uint16_t logic(pr_u86_028d_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }
static uint16_t step(pr_u86_028d_cpu *c,uint16_t v,unsigned bits,unsigned subtract)
{
    uint16_t cf=c->flags&1u;
    v=arithmetic(c,v,1u,0u,bits,subtract);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return v;
}
static void cbw(pr_u86_028d_cpu *c) { c->ax=(uint16_t)((c->ax&0x80u)?(c->ax|0xFF00u):(c->ax&255u)); }
static void cl(pr_u86_028d_cpu *c,uint8_t v) { c->cx=(uint16_t)((c->cx&0xFF00u)|v); }
static void dl(pr_u86_028d_cpu *c,uint8_t v) { c->dx=(uint16_t)((c->dx&0xFF00u)|v); }
static uint16_t shift4(pr_u86_028d_cpu *c,uint16_t v) {
    uint16_t result=(uint16_t)(v<<4u);
    (void)logic(c,result,16u);c->flags|=0x10u; /* pinned: AF set; SHL OF = MSB(result) ^ CF */
    if(v&0x1000u)c->flags|=1u;
    if(((result>>15)^((v>>12)&1u))&1u)c->flags|=0x800u;
    return result;
}
static void mul16(pr_u86_028d_cpu *c,uint16_t v) {
    uint32_t p=(uint32_t)c->ax*v;uint8_t parity=(uint8_t)(p^(p>>8u)^(p>>16u)^(p>>24u));
    c->ax=(uint16_t)p;c->dx=(uint16_t)(p>>16u);
    c->flags&=(uint16_t)~0x0845u;
    parity^=(uint8_t)(parity>>4u);parity^=(uint8_t)(parity>>2u);parity^=(uint8_t)(parity>>1u);
    if(!(parity&1u))c->flags|=4u;
    if(c->ax==0u)c->flags|=0x40u;
    if(c->dx!=0u)c->flags|=0x801u;
}

#define COPY_CPU(dst,src) do { (dst)->ax=(src)->ax; (dst)->bx=(src)->bx; (dst)->cx=(src)->cx; (dst)->dx=(src)->dx; (dst)->si=(src)->si; (dst)->di=(src)->di; (dst)->bp=(src)->bp; (dst)->sp=(src)->sp; (dst)->cs=(src)->cs; (dst)->ip=(src)->ip; (dst)->ds=(src)->ds; (dst)->es=(src)->es; (dst)->ss=(src)->ss; (dst)->flags=(src)->flags; } while (0)
static pr_u86_028d_status call(uint8_t *ram,size_t size,pr_u86_028d_cpu *c,
    unsigned heap_steps,pr_u86_028d_diagnostic *d,uint16_t target,
    pr_u86_028d_callee which)
{
    uint16_t caller=c->cs,site=c->ip,next=(uint16_t)(site+5u),segment;
    int status;
    if(ram[physical(caller,site)]!=0x9Au ||
       read_word(ram,caller,(uint16_t)(site+1u))!=target)return PR_U86_028D_CODE;
    segment=read_word(ram,caller,(uint16_t)(site+3u));
    push(ram,c,caller);push(ram,c,next);c->cs=segment;c->ip=target;d->callee=which;
    if(which==PR_U86_028D_GETMEM) {
        pr_system_getmem_cpu inner;COPY_CPU(&inner,c);
        status=(int)pr_system_getmem_023f(ram,size,&inner,heap_steps);COPY_CPU(c,&inner);
    } else if(which==PR_U86_028D_STRSTORE) {
        pr_system_0b25_cpu inner;COPY_CPU(&inner,c);
        status=(int)pr_system_strstore_0b25(ram,size,&inner);COPY_CPU(c,&inner);
    } else {
        pr_system_leaf_cpu inner;COPY_CPU(&inner,c);
        status=(int)(which==PR_U86_028D_STRLOAD ? pr_system_strload_0b0b(ram,size,&inner) :
            pr_system_strcat_0b98(ram,size,&inner));COPY_CPU(c,&inner);
    }
    d->status=status;
    if(status!=0)return PR_U86_028D_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U86_028D_TRANSFER;
    return PR_U86_028D_OK;
}
#undef COPY_CPU
pr_u86_028d_status pr_u86_string_node_028d(uint8_t *ram,size_t size,
    pr_u86_028d_cpu *c,size_t max_list_visits,unsigned max_heap_steps,
    pr_u86_028d_diagnostic *diagnostic)
{
    pr_u86_028d_diagnostic local={PR_U86_028D_NO_CALLEE,-1};
    pr_u86_028d_status status;uint16_t caller;size_t at;
    if(diagnostic==NULL)diagnostic=&local;
    *diagnostic=local;
    if(ram==NULL || c==NULL || size!=PR_U86_028D_RAM_SIZE || c->ip!=0x028Du ||
       c->sp<0x0140u || c->sp>0xFFFAu || max_list_visits==0u)return PR_U86_028D_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_U86_028D_TRANSFER;
        if(c->ip==0x02D4u) {
            if(max_list_visits==0u)return PR_U86_028D_LIST_LIMIT;
            --max_list_visits;
        }
        switch(c->ip) {
        case 0x028D: push(ram,c,c->bp);c->ip=0x028Eu;break;
        case 0x028E: c->bp=c->sp;c->ip=0x0290u;break;
        case 0x0290: c->sp=arithmetic(c,c->sp,0x0106u,0u,16u,1u);c->ip=0x0294u;break;
        case 0x0294: c->ax=read_word(ram,c->ds,(uint16_t)(18278));c->ip=0x0297u;break;
        case 0x0297: c->dx=read_word(ram,c->ds,(uint16_t)(18280));c->ip=0x029Bu;break;
        case 0x029B: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x029Eu;break;
        case 0x029E: put_word(ram,c->ss,(uint16_t)(c->bp+-2),c->dx);c->ip=0x02A1u;break;
        case 0x02A1: c->ax=read_word(ram,c->ds,(uint16_t)(18278));c->ip=0x02A4u;break;
        case 0x02A4: c->ax=logic(c,(uint16_t)(c->ax|read_word(ram,c->ds,(uint16_t)(18280))),16u);c->ip=0x02A8u;break;
        case 0x02A8: c->ip=((c->flags&0x40u)==0u)?0x02D0u:0x02AAu;break;
        case 0x02AA: c->ax=0x002Eu;c->ip=0x02ADu;break;
        case 0x02AD: push(ram,c,c->ax);c->ip=0x02AEu;break;
        case 0x02AE: status=call(ram,size,c,max_heap_steps,diagnostic,0x023Fu,PR_U86_028D_GETMEM);if(status!=0)return status;break;
        case 0x02B3: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x02B6u;break;
        case 0x02B6: put_word(ram,c->ss,(uint16_t)(c->bp+-2),c->dx);c->ip=0x02B9u;break;
        case 0x02B9: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-4));c->ip=0x02BCu;break;
        case 0x02BC: c->dx=read_word(ram,c->ss,(uint16_t)(c->bp+-2));c->ip=0x02BFu;break;
        case 0x02BF: put_word(ram,c->ds,(uint16_t)(18278),c->ax);c->ip=0x02C2u;break;
        case 0x02C2: put_word(ram,c->ds,(uint16_t)(18280),c->dx);c->ip=0x02C6u;break;
        case 0x02C6: ram[physical(c->ss,(uint16_t)(c->bp+-6))]=(uint8_t)(0x0000u);c->ip=0x02CAu;break;
        case 0x02CA: ram[physical(c->ss,(uint16_t)(c->bp+-5))]=(uint8_t)(0x0000u);c->ip=0x02CEu;break;
        case 0x02CE: c->ip=0x0340u;break;
        case 0x02D0: ram[physical(c->ss,(uint16_t)(c->bp+-5))]=(uint8_t)(0x0001u);c->ip=0x02D4u;break;
        case 0x02D4: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x02D7u;break;
        case 0x02D7: c->ax=read_word(ram,c->es,(uint16_t)(c->di+42));c->ip=0x02DBu;break;
        case 0x02DB: c->ax=logic(c,(uint16_t)(c->ax|read_word(ram,c->es,(uint16_t)(c->di+44))),16u);c->ip=0x02DFu;break;
        case 0x02DF: c->ip=((c->flags&0x40u)!=0u)?0x0301u:0x02E1u;break;
        case 0x02E1: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x02E4u;break;
        case 0x02E4: (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+41))],0x0000u,0u,8u,1u);c->ip=0x02E9u;break;
        case 0x02E9: c->ip=((c->flags&0x40u)==0u)?0x02EEu:0x02EBu;break;
        case 0x02EB: ram[physical(c->ss,(uint16_t)(c->bp+-5))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp+-5))],8u,0u));c->ip=0x02EEu;break;
        case 0x02EE: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x02F1u;break;
        case 0x02F1: c->ax=read_word(ram,c->es,(uint16_t)(c->di+42));c->ip=0x02F5u;break;
        case 0x02F5: c->dx=read_word(ram,c->es,(uint16_t)(c->di+44));c->ip=0x02F9u;break;
        case 0x02F9: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x02FCu;break;
        case 0x02FC: put_word(ram,c->ss,(uint16_t)(c->bp+-2),c->dx);c->ip=0x02FFu;break;
        case 0x02FF: c->ip=0x02D4u;break;
        case 0x0301: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-5))]));c->ip=0x0304u;break;
        case 0x0304: cbw(c);c->ip=0x0305u;break;
        case 0x0305: c->ax=step(c,c->ax,16u,1u);c->ip=0x0306u;break;
        case 0x0306: c->di=c->ax;c->ip=0x0308u;break;
        case 0x0308: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+18282))]));c->ip=0x030Cu;break;
        case 0x030C: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x030Eu;break;
        case 0x030E: c->di=c->ax;c->ip=0x0310u;break;
        case 0x0310: cl(c,(uint8_t)(0x0004u));c->ip=0x0312u;break;
        case 0x0312: c->di=shift4(c,c->di);c->ip=0x0314u;break;
        case 0x0314: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+12795))]));c->ip=0x0318u;break;
        case 0x0318: ram[physical(c->ss,(uint16_t)(c->bp+-6))]=(uint8_t)((uint8_t)c->ax);c->ip=0x031Bu;break;
        case 0x031B: c->ax=0x002Eu;c->ip=0x031Eu;break;
        case 0x031E: push(ram,c,c->ax);c->ip=0x031Fu;break;
        case 0x031F: status=call(ram,size,c,max_heap_steps,diagnostic,0x023Fu,PR_U86_028D_GETMEM);if(status!=0)return status;break;
        case 0x0324: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0327u;break;
        case 0x0327: put_word(ram,c->es,(uint16_t)(c->di+42),c->ax);c->ip=0x032Bu;break;
        case 0x032B: put_word(ram,c->es,(uint16_t)(c->di+44),c->dx);c->ip=0x032Fu;break;
        case 0x032F: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0332u;break;
        case 0x0332: c->ax=read_word(ram,c->es,(uint16_t)(c->di+42));c->ip=0x0336u;break;
        case 0x0336: c->dx=read_word(ram,c->es,(uint16_t)(c->di+44));c->ip=0x033Au;break;
        case 0x033A: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x033Du;break;
        case 0x033D: put_word(ram,c->ss,(uint16_t)(c->bp+-2),c->dx);c->ip=0x0340u;break;
        case 0x0340: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0343u;break;
        case 0x0343: c->ax=logic(c,(uint16_t)(c->ax^c->ax),16u);c->ip=0x0345u;break;
        case 0x0345: put_word(ram,c->es,(uint16_t)(c->di+42),c->ax);c->ip=0x0349u;break;
        case 0x0349: put_word(ram,c->es,(uint16_t)(c->di+44),c->ax);c->ip=0x034Du;break;
        case 0x034D: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+6))]));c->ip=0x0350u;break;
        case 0x0350: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&0x007Fu),8u)));c->ip=0x0352u;break;
        case 0x0352: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x0354u;break;
        case 0x0354: c->di=c->ax;c->ip=0x0356u;break;
        case 0x0356: cl(c,(uint8_t)(0x0004u));c->ip=0x0358u;break;
        case 0x0358: c->di=shift4(c,c->di);c->ip=0x035Au;break;
        case 0x035A: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+12795))]));c->ip=0x035Eu;break;
        case 0x035E: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-6))],0u,8u,1u);c->ip=0x0361u;break;
        case 0x0361: c->ip=((c->flags&0x40u)!=0u)?0x03CAu:0x0363u;break;
        case 0x0363: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+6))]));c->ip=0x0366u;break;
        case 0x0366: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&0x007Fu),8u)));c->ip=0x0368u;break;
        case 0x0368: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x036Au;break;
        case 0x036A: c->di=c->ax;c->ip=0x036Cu;break;
        case 0x036C: cl(c,(uint8_t)(0x0004u));c->ip=0x036Eu;break;
        case 0x036E: c->di=shift4(c,c->di);c->ip=0x0370u;break;
        case 0x0370: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+12795))]));c->ip=0x0374u;break;
        case 0x0374: cbw(c);c->ip=0x0375u;break;
        case 0x0375: c->dx=0x0029u;c->ip=0x0378u;break;
        case 0x0378: mul16(c,c->dx);c->ip=0x037Au;break;
        case 0x037A: c->di=c->ax;c->ip=0x037Cu;break;
        case 0x037C: c->di=arithmetic(c,c->di,0x0A79u,0u,16u,0u);c->ip=0x0380u;break;
        case 0x0380: push(ram,c,c->ds);c->ip=0x0381u;break;
        case 0x0381: push(ram,c,c->di);c->ip=0x0382u;break;
        case 0x0382: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0385u;break;
        case 0x0385: push(ram,c,c->es);c->ip=0x0386u;break;
        case 0x0386: push(ram,c,c->di);c->ip=0x0387u;break;
        case 0x0387: c->ax=0x0028u;c->ip=0x038Au;break;
        case 0x038A: push(ram,c,c->ax);c->ip=0x038Bu;break;
        case 0x038B: status=call(ram,size,c,max_heap_steps,diagnostic,0x0B25u,PR_U86_028D_STRSTORE);if(status!=0)return status;break;
        case 0x0390: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0393u;break;
        case 0x0393: ram[physical(c->es,(uint16_t)(c->di+41))]=(uint8_t)(0x0001u);c->ip=0x0398u;break;
        case 0x0398: c->ax=0x002Eu;c->ip=0x039Bu;break;
        case 0x039B: push(ram,c,c->ax);c->ip=0x039Cu;break;
        case 0x039C: status=call(ram,size,c,max_heap_steps,diagnostic,0x023Fu,PR_U86_028D_GETMEM);if(status!=0)return status;break;
        case 0x03A1: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x03A4u;break;
        case 0x03A4: put_word(ram,c->es,(uint16_t)(c->di+42),c->ax);c->ip=0x03A8u;break;
        case 0x03A8: put_word(ram,c->es,(uint16_t)(c->di+44),c->dx);c->ip=0x03ACu;break;
        case 0x03AC: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x03AFu;break;
        case 0x03AF: c->ax=read_word(ram,c->es,(uint16_t)(c->di+42));c->ip=0x03B3u;break;
        case 0x03B3: c->dx=read_word(ram,c->es,(uint16_t)(c->di+44));c->ip=0x03B7u;break;
        case 0x03B7: put_word(ram,c->ss,(uint16_t)(c->bp+-4),c->ax);c->ip=0x03BAu;break;
        case 0x03BA: put_word(ram,c->ss,(uint16_t)(c->bp+-2),c->dx);c->ip=0x03BDu;break;
        case 0x03BD: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x03C0u;break;
        case 0x03C0: c->ax=logic(c,(uint16_t)(c->ax^c->ax),16u);c->ip=0x03C2u;break;
        case 0x03C2: put_word(ram,c->es,(uint16_t)(c->di+42),c->ax);c->ip=0x03C6u;break;
        case 0x03C6: put_word(ram,c->es,(uint16_t)(c->di+44),c->ax);c->ip=0x03CAu;break;
        case 0x03CA: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+6))],0x007Fu,0u,8u,1u);c->ip=0x03CEu;break;
        case 0x03CE: c->ip=((c->flags&0x41u)!=0u)?0x03E5u:0x03D0u;break;
        case 0x03D0: c->di=0x0287u;c->ip=0x03D3u;break;
        case 0x03D3: push(ram,c,c->cs);c->ip=0x03D4u;break;
        case 0x03D4: push(ram,c,c->di);c->ip=0x03D5u;break;
        case 0x03D5: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x03D8u;break;
        case 0x03D8: push(ram,c,c->es);c->ip=0x03D9u;break;
        case 0x03D9: push(ram,c,c->di);c->ip=0x03DAu;break;
        case 0x03DA: c->ax=0x0028u;c->ip=0x03DDu;break;
        case 0x03DD: push(ram,c,c->ax);c->ip=0x03DEu;break;
        case 0x03DE: status=call(ram,size,c,max_heap_steps,diagnostic,0x0B25u,PR_U86_028D_STRSTORE);if(status!=0)return status;break;
        case 0x03E3: c->ip=0x03F8u;break;
        case 0x03E5: c->di=0x028Au;c->ip=0x03E8u;break;
        case 0x03E8: push(ram,c,c->cs);c->ip=0x03E9u;break;
        case 0x03E9: push(ram,c,c->di);c->ip=0x03EAu;break;
        case 0x03EA: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x03EDu;break;
        case 0x03ED: push(ram,c,c->es);c->ip=0x03EEu;break;
        case 0x03EE: push(ram,c,c->di);c->ip=0x03EFu;break;
        case 0x03EF: c->ax=0x0028u;c->ip=0x03F2u;break;
        case 0x03F2: push(ram,c,c->ax);c->ip=0x03F3u;break;
        case 0x03F3: status=call(ram,size,c,max_heap_steps,diagnostic,0x0B25u,PR_U86_028D_STRSTORE);if(status!=0)return status;break;
        case 0x03F8: c->di=(uint16_t)(c->bp+-262);c->ip=0x03FCu;break;
        case 0x03FC: push(ram,c,c->ss);c->ip=0x03FDu;break;
        case 0x03FD: push(ram,c,c->di);c->ip=0x03FEu;break;
        case 0x03FE: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0401u;break;
        case 0x0401: push(ram,c,c->es);c->ip=0x0402u;break;
        case 0x0402: push(ram,c,c->di);c->ip=0x0403u;break;
        case 0x0403: status=call(ram,size,c,max_heap_steps,diagnostic,0x0B0Bu,PR_U86_028D_STRLOAD);if(status!=0)return status;break;
        case 0x0408: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+6))]));c->ip=0x040Bu;break;
        case 0x040B: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&0x007Fu),8u)));c->ip=0x040Du;break;
        case 0x040D: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x040Fu;break;
        case 0x040F: c->dx=0x0029u;c->ip=0x0412u;break;
        case 0x0412: mul16(c,c->dx);c->ip=0x0414u;break;
        case 0x0414: c->di=c->ax;c->ip=0x0416u;break;
        case 0x0416: c->di=arithmetic(c,c->di,0x28E9u,0u,16u,0u);c->ip=0x041Au;break;
        case 0x041A: push(ram,c,c->ds);c->ip=0x041Bu;break;
        case 0x041B: push(ram,c,c->di);c->ip=0x041Cu;break;
        case 0x041C: status=call(ram,size,c,max_heap_steps,diagnostic,0x0B98u,PR_U86_028D_STRCAT);if(status!=0)return status;break;
        case 0x0421: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0424u;break;
        case 0x0424: push(ram,c,c->es);c->ip=0x0425u;break;
        case 0x0425: push(ram,c,c->di);c->ip=0x0426u;break;
        case 0x0426: c->ax=0x0028u;c->ip=0x0429u;break;
        case 0x0429: push(ram,c,c->ax);c->ip=0x042Au;break;
        case 0x042A: status=call(ram,size,c,max_heap_steps,diagnostic,0x0B25u,PR_U86_028D_STRSTORE);if(status!=0)return status;break;
        case 0x042F: at=physical(c->ss,(uint16_t)(c->bp+-4));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);c->ip=0x0432u;break;
        case 0x0432: ram[physical(c->es,(uint16_t)(c->di+41))]=(uint8_t)(0x0000u);c->ip=0x0437u;break;
        case 0x0437: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+6))]));c->ip=0x043Au;break;
        case 0x043A: al(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)c->ax&0x007Fu),8u)));c->ip=0x043Cu;break;
        case 0x043C: dl(c,(uint8_t)((uint8_t)c->ax));c->ip=0x043Eu;break;
        case 0x043E: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-5))]));c->ip=0x0441u;break;
        case 0x0441: cbw(c);c->ip=0x0442u;break;
        case 0x0442: c->di=c->ax;c->ip=0x0444u;break;
        case 0x0444: ram[physical(c->ds,(uint16_t)(c->di+18282))]=(uint8_t)((uint8_t)c->dx);c->ip=0x0448u;break;
        case 0x0448: c->sp=c->bp;c->ip=0x044Au;break;
        case 0x044A: c->bp=pop(ram,c);c->ip=0x044Bu;break;
        case 0x044B: retf(ram,c,2u);return PR_U86_028D_OK;
        default:return PR_U86_028D_TRANSFER;
        }
    }
}
