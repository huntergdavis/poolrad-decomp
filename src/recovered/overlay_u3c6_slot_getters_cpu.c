/* Raw slot getter API extracted from accepted U5E +32E6 execution.
 * See docs/re/U3C6_SLOT_GETTERS_CPU.md. */
#include "poolrad/recovered/overlay_u3c6_slot_getters_cpu.h"
#define MASK (PR_U3C6_GETTER_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_u3c6_getter_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u3c6_getter_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_u3c6_getter_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_u3c6_getter_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u3c6_getter_cpu *c, uint16_t left, uint16_t right,
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
static uint16_t shl(pr_u3c6_getter_cpu *c, uint16_t left)
{
    uint16_t result=(uint16_t)(left << 1u);
    /* Intermediate AF follows the accepted U7F +0A01 normal/386 convention. */
    c->flags&=(uint16_t)~0x08D5u;
    c->flags|=0x10u;
    if (left&0x8000u)c->flags|=1u;
    if ((left^result)&0x8000u)c->flags|=0x800u;
    result_flags(c,result,0x8000u);return result;
}
static void retf(const uint8_t *ram, pr_u3c6_getter_cpu *c, unsigned cleanup)
{
    c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+cleanup);
}


static void ah(pr_u3c6_getter_cpu *c,uint8_t v) { c->ax=(uint16_t)((c->ax&255u)|((uint16_t)v<<8u)); }
static uint16_t logic(pr_u3c6_getter_cpu *c,uint16_t v,unsigned bits) { c->flags&=(uint16_t)~0x08D5u;result_flags(c,v,bits==8u?0x80u:0x8000u);return v; }
static uint8_t increment(pr_u3c6_getter_cpu *c,uint8_t v) { uint16_t cf=c->flags&1u;v=(uint8_t)arithmetic(c,v,1u,0u,8u,0u);c->flags=(uint16_t)((c->flags&~1u)|cf);return v; }


pr_u3c6_getter_status pr_u3c6_slot_getter_cpu(uint8_t *ram,size_t size,pr_u3c6_getter_cpu *c,size_t remaining)
{
    uint16_t code_segment,return_ip=0u;unsigned direct;
    if(ram==NULL || c==NULL || size!=PR_U3C6_GETTER_RAM_SIZE ||
       (c->ip!=0x0BE7u && c->ip!=0x0C0Fu && c->ip!=0x0C37u && c->ip!=0x0C5Fu) ||
       c->sp<0x0020u || c->sp>0xFFF8u)return PR_U3C6_GETTER_ARGUMENT;
    code_segment=c->cs;direct=c->ip==0x0C5Fu;
    for (;;) {
        if(c->cs!=code_segment)return PR_U3C6_GETTER_TRANSFER;
        if(c->ip==0x0C71u) {
            if(remaining==0u)return PR_U3C6_GETTER_LIMIT;
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
        case 0x0C0C: retf(ram,c,4u);return PR_U3C6_GETTER_OK;
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
        case 0x0C34: retf(ram,c,4u);return PR_U3C6_GETTER_OK;
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
        case 0x0C5C: retf(ram,c,4u);return PR_U3C6_GETTER_OK;
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
        case 0x0CB6: retf(ram,c,4u);if(direct)return PR_U3C6_GETTER_OK;if(c->cs!=code_segment || c->ip!=return_ip)return PR_U3C6_GETTER_TRANSFER;break;
        default:return PR_U3C6_GETTER_TRANSFER;
        }
    }
}
