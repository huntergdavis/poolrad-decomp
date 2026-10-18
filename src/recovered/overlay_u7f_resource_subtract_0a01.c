/* GAME.OVR unit 0x7F entry 8; docs/re/U7F_RESOURCE_SUBTRACT_0A01.md. */
#include "poolrad/recovered/overlay_u7f_resource_subtract_0a01.h"
#define MASK (PR_U7F_0020_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_u7f_0a01_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u7f_0a01_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static uint16_t arg(const uint8_t *ram, const pr_u7f_0a01_cpu *c, unsigned off)
{
    return read_word(ram,c->ss,(uint16_t)(c->bp+off));
}
static void les(const uint8_t *ram, pr_u7f_0a01_cpu *c, unsigned off)
{
    size_t at=physical(c->ss,(uint16_t)(c->bp+off));
    c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);
}
static void al(pr_u7f_0a01_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void cbw(pr_u7f_0a01_cpu *c)
{
    c->ax=(uint16_t)((c->ax&0x80u) ? 0xFF00u|(c->ax&255u) : c->ax&255u);
}
static void result_flags(pr_u7f_0a01_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u7f_0a01_cpu *c, uint16_t left, uint16_t right,
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
static uint16_t shl(pr_u7f_0a01_cpu *c, uint16_t left)
{
    uint16_t result=(uint16_t)(left << 1u);
    /* AF is architecturally undefined; native normal-core 386 pins it to 1. */
    c->flags&=(uint16_t)~0x08D5u;
    c->flags|=0x10u;
    if (left&0x8000u)c->flags|=1u;
    if ((left^result)&0x8000u)c->flags|=0x800u;
    result_flags(c,result,0x8000u);return result;
}
static void retf(const uint8_t *ram, pr_u7f_0a01_cpu *c, unsigned cleanup)
{
    c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+cleanup);
}

pr_u7f_0a01_status pr_u7f_resource_subtract_0a01(
    uint8_t *ram, size_t size, pr_u7f_0a01_cpu *c)
{
    uint16_t code_segment,value;
    if (ram==NULL || c==NULL || size!=PR_U7F_0020_RAM_SIZE || c->ip!=0x0A01u)
        return PR_U7F_0A01_ARGUMENT;
    code_segment=c->cs;
    for (;;) {
        if (c->cs!=code_segment)return PR_U7F_0A01_CONTROL_EXIT;
        switch (c->ip) {
        case 0x0A01:push(ram,c,c->bp);c->ip=0x0A02;break;
        case 0x0A02:c->bp=c->sp;c->ip=0x0A04;break;
        case 0x0A04:al(c,ram[physical(c->ss,(uint16_t)(c->bp+6u))]);c->ip=0x0A07;break;
        case 0x0A07:cbw(c);c->ip=0x0A08;break;
        case 0x0A08:c->ax=shl(c,c->ax);c->ip=0x0A0A;break;
        case 0x0A0A:les(ram,c,10u);c->ip=0x0A0D;break;
        case 0x0A0D:c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x0A0F;break;
        case 0x0A0F:c->ax=read_word(ram,c->es,(uint16_t)(c->di+0x88u));c->ip=0x0A14;break;
        case 0x0A14:c->ax=arithmetic(c,c->ax,arg(ram,c,8u),0u,16u,1u);c->ip=0x0A17;break;
        case 0x0A17:c->dx=c->ax;c->ip=0x0A19;break;
        case 0x0A19:al(c,ram[physical(c->ss,(uint16_t)(c->bp+6u))]);c->ip=0x0A1C;break;
        case 0x0A1C:cbw(c);c->ip=0x0A1D;break;
        case 0x0A1D:c->ax=shl(c,c->ax);c->ip=0x0A1F;break;
        case 0x0A1F:les(ram,c,10u);c->ip=0x0A22;break;
        case 0x0A22:c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);c->ip=0x0A24;break;
        case 0x0A24:put_word(ram,c->es,(uint16_t)(c->di+0x88u),c->dx);c->ip=0x0A29;break;
        case 0x0A29:value=arg(ram,c,12u);push(ram,c,value);c->ip=0x0A2C;break;
        case 0x0A2C:value=arg(ram,c,10u);push(ram,c,value);c->ip=0x0A2F;break;
        case 0x0A2F:value=arg(ram,c,8u);push(ram,c,value);c->ip=0x0A32;break;
        case 0x0A32:push(ram,c,c->cs);c->ip=0x0A33;break;
        case 0x0A33:push(ram,c,0x0A36u);c->ip=0x0020;break;
        case 0x0020:
            /* The approved callee includes ordered aliases and RETF 6.
             * Resume its actual target rather than assuming +0A36. */
            (void)pr_u7f_record_subtract_0020(ram,size,c);break;
        case 0x0021:c->bp=c->sp;c->ip=0x0023;break;
        case 0x0023:les(ram,c,8u);c->ip=0x0026;break;
        case 0x0026:c->ax=read_word(ram,c->es,(uint16_t)(c->di+0x102u));c->ip=0x002B;break;
        case 0x002B:c->ax=arithmetic(c,c->ax,arg(ram,c,6u),0u,16u,1u);c->ip=0x002E;break;
        case 0x002E:les(ram,c,8u);c->ip=0x0031;break;
        case 0x0031:put_word(ram,c->es,(uint16_t)(c->di+0x102u),c->ax);c->ip=0x0036;break;
        case 0x0036:c->bp=pop(ram,c);c->ip=0x0037;break;
        case 0x0037:retf(ram,c,6u);break;
        case 0x0A36:
            (void)arithmetic(c,ram[physical(c->ds,0x49F3u)],6u,0u,8u,1u);
            c->ip=(c->flags&0x40u) ? 0x0A44u : 0x0A3Du;break;
        case 0x0A3B:c->ip=(c->flags&0x40u) ? 0x0A44u : 0x0A3Du;break;
        case 0x0A3D:
            (void)arithmetic(c,ram[physical(c->ds,0x49F3u)],1u,0u,8u,1u);
            c->ip=(c->flags&0x40u) ? 0x0A44u : 0x0A79u;break;
        case 0x0A42:c->ip=(c->flags&0x40u) ? 0x0A44u : 0x0A79u;break;
        case 0x0A44:c->ax=arg(ram,c,8u);c->ip=0x0A47;break;
        case 0x0A47:
            c->dx=0;c->flags&=(uint16_t)~0x08D5u;result_flags(c,0u,0x8000u);c->ip=0x0A49;break;
        case 0x0A49:c->cx=c->ax;c->ip=0x0A4B;break;
        case 0x0A4B:c->bx=c->dx;c->ip=0x0A4D;break;
        case 0x0A4D:al(c,ram[physical(c->ss,(uint16_t)(c->bp+6u))]);c->ip=0x0A50;break;
        case 0x0A50:cbw(c);c->ip=0x0A51;break;
        case 0x0A51:c->di=c->ax;c->ip=0x0A53;break;
        case 0x0A53:c->di=shl(c,c->di);c->ip=0x0A55;break;
        case 0x0A55:c->di=shl(c,c->di);c->ip=0x0A57;break;
        case 0x0A57:c->ax=read_word(ram,c->ds,(uint16_t)(c->di+0x67F4u));c->ip=0x0A5B;break;
        case 0x0A5B:c->dx=read_word(ram,c->ds,(uint16_t)(c->di+0x67F6u));c->ip=0x0A5F;break;
        case 0x0A5F:c->ax=arithmetic(c,c->ax,c->cx,0u,16u,0u);c->ip=0x0A61;break;
        case 0x0A61:c->dx=arithmetic(c,c->dx,c->bx,c->flags&1u,16u,0u);c->ip=0x0A63;break;
        case 0x0A63:c->cx=c->ax;c->ip=0x0A65;break;
        case 0x0A65:c->bx=c->dx;c->ip=0x0A67;break;
        case 0x0A67:al(c,ram[physical(c->ss,(uint16_t)(c->bp+6u))]);c->ip=0x0A6A;break;
        case 0x0A6A:cbw(c);c->ip=0x0A6B;break;
        case 0x0A6B:c->di=c->ax;c->ip=0x0A6D;break;
        case 0x0A6D:c->di=shl(c,c->di);c->ip=0x0A6F;break;
        case 0x0A6F:c->di=shl(c,c->di);c->ip=0x0A71;break;
        case 0x0A71:put_word(ram,c->ds,(uint16_t)(c->di+0x67F4u),c->cx);c->ip=0x0A75;break;
        case 0x0A75:put_word(ram,c->ds,(uint16_t)(c->di+0x67F6u),c->bx);c->ip=0x0A79;break;
        case 0x0A79:c->bp=pop(ram,c);c->ip=0x0A7A;break;
        case 0x0A7A:retf(ram,c,8u);return PR_U7F_0A01_OK;
        default:return PR_U7F_0A01_CONTROL_EXIT;
        }
    }
}
