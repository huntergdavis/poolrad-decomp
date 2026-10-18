/* Complete U1C +18F8 ON GOTO / ON GOSUB. See U1C_ON_GOTO_18F8_CPU.md.
 * Memory/flags primitives adapted from accepted U3C2 +0909; live stub
 * composition follows U1C +00F4. No operand/heap callbacks. */
#include "poolrad/recovered/overlay_u1c_on_goto_18f8_cpu.h"
#include "poolrad/recovered/overlay_u25_loader_0000.h"
#include "poolrad/recovered/overlay_u25_slot_value_0136.h"
#include "poolrad/recovered/overlay_u25_alloc_1951_cpu.h"
#include <string.h>
#define MASK (PR_U1C_18F8_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_u1c_18f8_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u1c_18f8_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u1c_18f8_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_u1c_18f8_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u1c_18f8_cpu *c, uint16_t left, uint16_t right,
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
static void retf(const uint8_t *ram, pr_u1c_18f8_cpu *c, unsigned cleanup)
{
    c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+cleanup);
}


static void ah(pr_u1c_18f8_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&255u)|((uint16_t)v<<8u)); }
static uint16_t logic(pr_u1c_18f8_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }
static uint16_t step(pr_u1c_18f8_cpu *c,uint16_t v,unsigned bits,unsigned subtract)
{
    uint16_t cf=c->flags&1u;
    v=arithmetic(c,v,1u,0u,bits,subtract);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return v;
}
_Static_assert(sizeof(pr_u1c_18f8_cpu)==sizeof(pr_u25_0000_cpu),"loader CPU layout");
_Static_assert(sizeof(pr_u1c_18f8_cpu)==sizeof(pr_u25_slot_value_cpu),"slot CPU layout");
_Static_assert(sizeof(pr_u1c_18f8_cpu)==sizeof(pr_u25_1951_cpu),"allocator CPU layout");
static pr_u1c_18f8_status call(uint8_t *ram,size_t size,pr_u1c_18f8_cpu *c,
    const pr_u1c_18f8_limits *limits,pr_u1c_18f8_diagnostic *d,
    uint16_t offset,uint16_t target,pr_u1c_18f8_callee which)
{
    uint16_t site=c->ip,caller=c->cs,next=(uint16_t)(site+5u),stub;
    int status;
    if(ram[physical(caller,site)]!=0x9Au ||
       read_word(ram,caller,(uint16_t)(site+1u))!=offset)return PR_U1C_18F8_CODE;
    stub=read_word(ram,caller,(uint16_t)(site+3u));
    if(ram[physical(stub,offset)]!=0xEAu ||
       read_word(ram,stub,(uint16_t)(offset+1u))!=target)return PR_U1C_18F8_CODE;
    push(ram,c,caller);push(ram,c,next);
    c->cs=read_word(ram,stub,(uint16_t)(offset+3u));c->ip=target;
    d->callee=which;d->nested_callee=-1;d->nested_status=-1;
    if(which==PR_U1C_18F8_LOAD || which==PR_U1C_18F8_WORD) {
        pr_u25_0000_cpu inner;memcpy(&inner,c,sizeof inner);
        status=which==PR_U1C_18F8_LOAD ?
            (int)pr_u25_load_operands_0000(ram,size,&inner,limits->system_cs,
                limits->max_operands,limits->max_string_iterations) :
            (int)pr_u25_word_06af(ram,size,&inner);
        memcpy(c,&inner,sizeof inner);
    } else if(which==PR_U1C_18F8_SLOT) {
        pr_u25_slot_value_cpu inner;memcpy(&inner,c,sizeof inner);
        status=(int)pr_u25_slot_value_0136(ram,size,&inner);
        memcpy(c,&inner,sizeof inner);
    } else {
        pr_u25_1951_cpu inner;pr_u25_1951_callee nested;
        memcpy(&inner,c,sizeof inner);
        status=(int)pr_u25_alloc_1951_cpu(ram,size,&inner,limits->max_heap_steps,
            &nested,&d->nested_status);
        d->nested_callee=(int)nested;memcpy(c,&inner,sizeof inner);
    }
    d->status=status;
    if(status!=0)return PR_U1C_18F8_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U1C_18F8_TRANSFER;
    return PR_U1C_18F8_OK;
}
pr_u1c_18f8_status pr_u1c_on_goto_18f8_cpu(uint8_t *ram,size_t size,
    pr_u1c_18f8_cpu *c,const pr_u1c_18f8_limits *limits,
    pr_u1c_18f8_diagnostic *diagnostic)
{
    pr_u1c_18f8_diagnostic local={PR_U1C_18F8_NO_CALLEE,-1,-1,-1};
    pr_u1c_18f8_status status;uint16_t caller;size_t remaining;
    if(diagnostic==NULL)diagnostic=&local;
    *diagnostic=local;
    if(ram==NULL || c==NULL || limits==NULL || size!=PR_U1C_18F8_RAM_SIZE ||
       c->ip!=0x18F8u || c->sp<0x0238u || c->sp>0xFFFCu ||
       limits->max_operands==0u || limits->max_string_iterations==0u ||
       limits->max_caller_iterations==0u)return PR_U1C_18F8_ARGUMENT;
    caller=c->cs;remaining=limits->max_caller_iterations;
    for(;;) {
        if(c->cs!=caller)return PR_U1C_18F8_TRANSFER;
        if(c->ip==0x1931u) {
            if(remaining==0u)return PR_U1C_18F8_CALLER_LIMIT;
            --remaining;
        }
        switch(c->ip) {
        case 0x18F8: push(ram,c,c->bp);c->ip=0x18F9u;break;
        case 0x18F9: c->bp=c->sp;c->ip=0x18FBu;break;
        case 0x18FB: c->sp=arithmetic(c,c->sp,0x0006u,0u,16u,1u);c->ip=0x18FEu;break;
        case 0x18FE: ram[physical(c->ss,(uint16_t)(c->bp+-5))]=(uint8_t)(0x0000u);c->ip=0x1902u;break;
        case 0x1902: al(c,(uint8_t)(0x0002u));c->ip=0x1904u;break;
        case 0x1904: push(ram,c,c->ax);c->ip=0x1905u;break;
        case 0x1905: status=call(ram,size,c,limits,diagnostic,0x002Au,0x0000u,PR_U1C_18F8_LOAD);if(status!=0)return status;break;
        case 0x190A: al(c,(uint8_t)(0x0001u));c->ip=0x190Cu;break;
        case 0x190C: push(ram,c,c->ax);c->ip=0x190Du;break;
        case 0x190D: status=call(ram,size,c,limits,diagnostic,0x0025u,0x0136u,PR_U1C_18F8_SLOT);if(status!=0)return status;break;
        case 0x1912: ram[physical(c->ss,(uint16_t)(c->bp+-1))]=(uint8_t)((uint8_t)c->ax);c->ip=0x1915u;break;
        case 0x1915: al(c,(uint8_t)(0x0002u));c->ip=0x1917u;break;
        case 0x1917: push(ram,c,c->ax);c->ip=0x1918u;break;
        case 0x1918: status=call(ram,size,c,limits,diagnostic,0x0025u,0x0136u,PR_U1C_18F8_SLOT);if(status!=0)return status;break;
        case 0x191D: ram[physical(c->ss,(uint16_t)(c->bp+-2))]=(uint8_t)((uint8_t)c->ax);c->ip=0x1920u;break;
        case 0x1920: put_word(ram,c->ds,(uint16_t)(18925),step(c,read_word(ram,c->ds,(uint16_t)(18925)),16u,1u));c->ip=0x1924u;break;
        case 0x1924: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-2))]));c->ip=0x1927u;break;
        case 0x1927: push(ram,c,c->ax);c->ip=0x1928u;break;
        case 0x1928: status=call(ram,size,c,limits,diagnostic,0x002Au,0x0000u,PR_U1C_18F8_LOAD);if(status!=0)return status;break;
        case 0x192D: ram[physical(c->ss,(uint16_t)(c->bp+-4))]=(uint8_t)(0x0000u);c->ip=0x1931u;break;
        case 0x1931: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x1934u;break;
        case 0x1934: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-2))],0u,8u,1u);c->ip=0x1937u;break;
        case 0x1937: c->ip=((c->flags&1u)==0u)?0x1989u:0x1939u;break;
        case 0x1939: (void)arithmetic(c,ram[physical(c->ss,(uint16_t)(c->bp+-5))],0x0000u,0u,8u,1u);c->ip=0x193Du;break;
        case 0x193D: c->ip=((c->flags&0x40u)==0u)?0x1989u:0x193Fu;break;
        case 0x193F: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-1))]));c->ip=0x1942u;break;
        case 0x1942: (void)arithmetic(c,(uint8_t)c->ax,ram[physical(c->ss,(uint16_t)(c->bp+-4))],0u,8u,1u);c->ip=0x1945u;break;
        case 0x1945: c->ip=((c->flags&0x40u)==0u)?0x1984u:0x1947u;break;
        case 0x1947: (void)arithmetic(c,ram[physical(c->ds,(uint16_t)(28543))],0x0025u,0u,8u,1u);c->ip=0x194Cu;break;
        case 0x194C: c->ip=((c->flags&0x40u)==0u)?0x1972u:0x194Eu;break;
        case 0x194E: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x1951u;break;
        case 0x1951: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x1953u;break;
        case 0x1953: c->ax=step(c,c->ax,16u,0u);c->ip=0x1954u;break;
        case 0x1954: c->di=c->ax;c->ip=0x1956u;break;
        case 0x1956: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+28805))]));c->ip=0x195Au;break;
        case 0x195A: push(ram,c,c->ax);c->ip=0x195Bu;break;
        case 0x195B: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x195Eu;break;
        case 0x195E: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x1960u;break;
        case 0x1960: c->ax=step(c,c->ax,16u,0u);c->ip=0x1961u;break;
        case 0x1961: c->di=c->ax;c->ip=0x1963u;break;
        case 0x1963: al(c,(uint8_t)(ram[physical(c->ds,(uint16_t)(c->di+28741))]));c->ip=0x1967u;break;
        case 0x1967: push(ram,c,c->ax);c->ip=0x1968u;break;
        case 0x1968: status=call(ram,size,c,limits,diagnostic,0x004Du,0x06AFu,PR_U1C_18F8_WORD);if(status!=0)return status;break;
        case 0x196D: put_word(ram,c->ds,(uint16_t)(18925),c->ax);c->ip=0x1970u;break;
        case 0x1970: c->ip=0x197Eu;break;
        case 0x1972: al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+-4))]));c->ip=0x1975u;break;
        case 0x1975: ah(c,(uint8_t)(logic(c,(uint16_t)((uint8_t)(c->ax>>8u)^(uint8_t)(c->ax>>8u)),8u)));c->ip=0x1977u;break;
        case 0x1977: c->ax=step(c,c->ax,16u,0u);c->ip=0x1978u;break;
        case 0x1978: push(ram,c,c->ax);c->ip=0x1979u;break;
        case 0x1979: status=call(ram,size,c,limits,diagnostic,0x0075u,0x1951u,PR_U1C_18F8_ALLOC);if(status!=0)return status;break;
        case 0x197E: ram[physical(c->ss,(uint16_t)(c->bp+-5))]=(uint8_t)(0x0001u);c->ip=0x1982u;break;
        case 0x1982: c->ip=0x1987u;break;
        case 0x1984: ram[physical(c->ss,(uint16_t)(c->bp+-4))]=(uint8_t)(step(c,ram[physical(c->ss,(uint16_t)(c->bp+-4))],8u,0u));c->ip=0x1987u;break;
        case 0x1987: c->ip=0x1931u;break;
        case 0x1989: c->sp=c->bp;c->ip=0x198Bu;break;
        case 0x198B: c->bp=pop(ram,c);c->ip=0x198Cu;break;
        case 0x198C: retf(ram,c,0u);return PR_U1C_18F8_OK;
        default:return PR_U1C_18F8_TRANSFER;
        }
    }
}
