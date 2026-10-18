/* Complete executable U6A +0E37..+0EBE; trailing queue bytes are data.
 * Primitives adapted from accepted U3C2 +0909. See U6A_STRING_NODE_0E37.md. */
#include "poolrad/recovered/overlay_u6a_string_node_0e37.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#define MASK (PR_U6A_0E37_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_u6a_0e37_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u6a_0e37_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u6a_0e37_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_u6a_0e37_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u6a_0e37_cpu *c, uint16_t left, uint16_t right,
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
static void retf(const uint8_t *ram,pr_u6a_0e37_cpu *c,unsigned cleanup)
{
    size_t at=physical(c->ss,c->sp);
    c->ip=word_at(ram,at);c->cs=word_at(ram,(at+2u)&MASK);
    c->sp=(uint16_t)(c->sp+4u+cleanup);
}
static uint16_t logic(pr_u6a_0e37_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }

#define COPY_CPU(dst,src) do { (dst)->ax=(src)->ax; (dst)->bx=(src)->bx; (dst)->cx=(src)->cx; (dst)->dx=(src)->dx; (dst)->si=(src)->si; (dst)->di=(src)->di; (dst)->bp=(src)->bp; (dst)->sp=(src)->sp; (dst)->cs=(src)->cs; (dst)->ip=(src)->ip; (dst)->ds=(src)->ds; (dst)->es=(src)->es; (dst)->ss=(src)->ss; (dst)->flags=(src)->flags; } while (0)
pr_u6a_0e37_status pr_u6a_string_node_0e37(
    uint8_t *ram,size_t size,pr_u6a_0e37_cpu *c,unsigned max_heap_steps,int *callee_status)
{
    uint16_t caller_cs,callee_cs;size_t at;int status;
    if(callee_status!=NULL)*callee_status=-1;
    if(ram==NULL || c==NULL || size!=PR_U6A_0E37_RAM_SIZE ||
       c->ip!=0x0E37u || c->sp<0x0060u || c->sp>0xFFF4u)
        return PR_U6A_0E37_ARGUMENT;
    caller_cs=c->cs;
    /* +0E37 */ push(ram,c,c->bp);
    c->ip=0x0E38u;
    /* +0E38 */ c->bp=c->sp;
    c->ip=0x0E3Au;
    /* +0E3A */ c->sp=arithmetic(c,c->sp,0x002Au,0u,16u,1u);
    c->ip=0x0E3Du;
    /* +0E3D */ at=physical(c->ss,(uint16_t)(c->bp+8));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);
    c->ip=0x0E40u;
    /* +0E40 */ push(ram,c,c->es);
    c->ip=0x0E41u;
    /* +0E41 */ push(ram,c,c->di);
    c->ip=0x0E42u;
    /* +0E42 */ c->di=(uint16_t)(c->bp+-41);
    c->ip=0x0E45u;
    /* +0E45 */ push(ram,c,c->ss);
    c->ip=0x0E46u;
    /* +0E46 */ push(ram,c,c->di);
    c->ip=0x0E47u;
    /* +0E47 */ c->ax=0x0028u;
    c->ip=0x0E4Au;
    /* +0E4A */ push(ram,c,c->ax);
    c->ip=0x0E4Bu;
    /* +0E4B */ callee_cs=read_word(ram,c->cs,0x0E4Eu);push(ram,c,c->cs);push(ram,c,0x0E50u);c->cs=callee_cs;c->ip=0x0B25u;
    { pr_system_0b25_cpu inner;COPY_CPU(&inner,c);status=(int)pr_system_strstore_0b25(ram,size,&inner);COPY_CPU(c,&inner); }
    if(callee_status!=NULL)*callee_status=status;
    if(status!=0)return PR_U6A_0E37_STRSTORE_STOP;
    if(c->cs!=caller_cs || c->ip!=0x0E50u)return PR_U6A_0E37_TRANSFER;
    c->ip=0x0E50u;
    /* +0E50 */ c->ax=0x002Eu;
    c->ip=0x0E53u;
    /* +0E53 */ push(ram,c,c->ax);
    c->ip=0x0E54u;
    /* +0E54 */ callee_cs=read_word(ram,c->cs,0x0E57u);push(ram,c,c->cs);push(ram,c,0x0E59u);c->cs=callee_cs;c->ip=0x023Fu;
    { pr_system_getmem_cpu inner;COPY_CPU(&inner,c);status=(int)pr_system_getmem_023f(ram,size,&inner,max_heap_steps);COPY_CPU(c,&inner); }
    if(callee_status!=NULL)*callee_status=status;
    if(status!=0)return PR_U6A_0E37_GETMEM_STOP;
    if(c->cs!=caller_cs || c->ip!=0x0E59u)return PR_U6A_0E37_TRANSFER;
    c->ip=0x0E59u;
    /* +0E59 */ c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));
    c->ip=0x0E5Cu;
    /* +0E5C */ at=physical(c->ss,(uint16_t)(c->di+-12));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);
    c->ip=0x0E60u;
    /* +0E60 */ put_word(ram,c->es,(uint16_t)(c->di+42),c->ax);
    c->ip=0x0E64u;
    /* +0E64 */ put_word(ram,c->es,(uint16_t)(c->di+44),c->dx);
    c->ip=0x0E68u;
    /* +0E68 */ c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));
    c->ip=0x0E6Bu;
    /* +0E6B */ at=physical(c->ss,(uint16_t)(c->di+-12));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);
    c->ip=0x0E6Fu;
    /* +0E6F */ c->ax=read_word(ram,c->es,(uint16_t)(c->di+42));
    c->ip=0x0E73u;
    /* +0E73 */ c->dx=read_word(ram,c->es,(uint16_t)(c->di+44));
    c->ip=0x0E77u;
    /* +0E77 */ c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));
    c->ip=0x0E7Au;
    /* +0E7A */ put_word(ram,c->ss,(uint16_t)(c->di+-12),c->ax);
    c->ip=0x0E7Eu;
    /* +0E7E */ put_word(ram,c->ss,(uint16_t)(c->di+-10),c->dx);
    c->ip=0x0E82u;
    /* +0E82 */ c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));
    c->ip=0x0E85u;
    /* +0E85 */ at=physical(c->ss,(uint16_t)(c->di+-12));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);
    c->ip=0x0E89u;
    /* +0E89 */ c->ax=logic(c,(uint16_t)(c->ax^c->ax),16u);
    c->ip=0x0E8Bu;
    /* +0E8B */ put_word(ram,c->es,(uint16_t)(c->di+42),c->ax);
    c->ip=0x0E8Fu;
    /* +0E8F */ put_word(ram,c->es,(uint16_t)(c->di+44),c->ax);
    c->ip=0x0E93u;
    /* +0E93 */ c->di=(uint16_t)(c->bp+-41);
    c->ip=0x0E96u;
    /* +0E96 */ push(ram,c,c->ss);
    c->ip=0x0E97u;
    /* +0E97 */ push(ram,c,c->di);
    c->ip=0x0E98u;
    /* +0E98 */ c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));
    c->ip=0x0E9Bu;
    /* +0E9B */ at=physical(c->ss,(uint16_t)(c->di+-12));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);
    c->ip=0x0E9Fu;
    /* +0E9F */ push(ram,c,c->es);
    c->ip=0x0EA0u;
    /* +0EA0 */ push(ram,c,c->di);
    c->ip=0x0EA1u;
    /* +0EA1 */ c->ax=0x0028u;
    c->ip=0x0EA4u;
    /* +0EA4 */ push(ram,c,c->ax);
    c->ip=0x0EA5u;
    /* +0EA5 */ callee_cs=read_word(ram,c->cs,0x0EA8u);push(ram,c,c->cs);push(ram,c,0x0EAAu);c->cs=callee_cs;c->ip=0x0B25u;
    { pr_system_0b25_cpu inner;COPY_CPU(&inner,c);status=(int)pr_system_strstore_0b25(ram,size,&inner);COPY_CPU(c,&inner); }
    if(callee_status!=NULL)*callee_status=status;
    if(status!=0)return PR_U6A_0E37_STRSTORE_STOP;
    if(c->cs!=caller_cs || c->ip!=0x0EAAu)return PR_U6A_0E37_TRANSFER;
    c->ip=0x0EAAu;
    /* +0EAA */ al(c,(uint8_t)(ram[physical(c->ss,(uint16_t)(c->bp+12))]));
    c->ip=0x0EADu;
    /* +0EAD */ c->di=read_word(ram,c->ss,(uint16_t)(c->bp+6));
    c->ip=0x0EB0u;
    /* +0EB0 */ at=physical(c->ss,(uint16_t)(c->di+-12));c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);
    c->ip=0x0EB4u;
    /* +0EB4 */ ram[physical(c->es,(uint16_t)(c->di+41))]=(uint8_t)((uint8_t)c->ax);
    c->ip=0x0EB8u;
    /* +0EB8 */ c->sp=c->bp;
    c->ip=0x0EBAu;
    /* +0EBA */ c->bp=pop(ram,c);
    c->ip=0x0EBBu;
    /* +0EBB */ retf(ram,c,8u);return PR_U6A_0E37_OK;
}
#undef COPY_CPU
