/* U7F entry 2, [0508,05E0); docs/re/U7F_CLEAR_0508.md.
 * Memory/flag primitives adapted from accepted U7F +0A01. */
#include "poolrad/recovered/overlay_u7f_clear_0508.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
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
static void push(uint8_t *ram, pr_u7f_0508_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(const uint8_t *ram, pr_u7f_0508_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static uint16_t arg(const uint8_t *ram, const pr_u7f_0508_cpu *c, unsigned off)
{
    return read_word(ram,c->ss,(uint16_t)(c->bp+off));
}
static void les(const uint8_t *ram, pr_u7f_0508_cpu *c, unsigned off)
{
    size_t at=physical(c->ss,(uint16_t)(c->bp+off));
    c->di=word_at(ram,at);c->es=word_at(ram,(at+2u)&MASK);
}
static void al(pr_u7f_0508_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void cbw(pr_u7f_0508_cpu *c)
{
    c->ax=(uint16_t)((c->ax&0x80u) ? 0xFF00u|(c->ax&255u) : c->ax&255u);
}
static void result_flags(pr_u7f_0508_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_u7f_0508_cpu *c, uint16_t left, uint16_t right,
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
static uint16_t shl(pr_u7f_0508_cpu *c, uint16_t left)
{
    uint16_t result=(uint16_t)(left << 1u);
    /* AF is architecturally undefined; native normal-core 386 pins it to 1. */
    c->flags&=(uint16_t)~0x08D5u;
    c->flags|=0x10u;
    if (left&0x8000u)c->flags|=1u;
    if ((left^result)&0x8000u)c->flags|=0x800u;
    result_flags(c,result,0x8000u);return result;
}
static void retf(const uint8_t *ram, pr_u7f_0508_cpu *c, unsigned cleanup)
{
    c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+cleanup);
}


static void fill(uint8_t *ram, size_t size, pr_u7f_0508_cpu *c)
{
    pr_system_leaf_cpu runtime;
#define COPY_TO(n) runtime.n=c->n
    COPY_TO(ax); COPY_TO(bx); COPY_TO(cx); COPY_TO(dx); COPY_TO(si);
    COPY_TO(di); COPY_TO(bp); COPY_TO(sp); COPY_TO(cs); COPY_TO(ip);
    COPY_TO(ds); COPY_TO(es); COPY_TO(ss); COPY_TO(flags);
#undef COPY_TO
    (void)pr_system_fillchar_16a8(ram,size,&runtime);
#define COPY_FROM(n) c->n=runtime.n
    COPY_FROM(ax); COPY_FROM(bx); COPY_FROM(cx); COPY_FROM(dx); COPY_FROM(si);
    COPY_FROM(di); COPY_FROM(bp); COPY_FROM(sp); COPY_FROM(cs); COPY_FROM(ip);
    COPY_FROM(ds); COPY_FROM(es); COPY_FROM(ss); COPY_FROM(flags);
#undef COPY_FROM
}
static uint8_t local(const uint8_t *ram, const pr_u7f_0508_cpu *c, unsigned n)
{
    return ram[physical(c->ss,(uint16_t)(c->bp-n))];
}
static void put_local(uint8_t *ram, const pr_u7f_0508_cpu *c, unsigned n, uint8_t value)
{
    ram[physical(c->ss,(uint16_t)(c->bp-n))]=value;
}

pr_u7f_0508_status pr_u7f_clear_0508(
    uint8_t *ram, size_t size, pr_u7f_0508_cpu *c, uint16_t system_cs,
    size_t max_nodes, size_t max_items)
{
    uint16_t code_segment, value, carry;
    if (ram==NULL || c==NULL || size!=PR_U7F_0020_RAM_SIZE ||
        c->ip!=0x0508u || c->sp<0x14u || c->sp>0xFFFCu ||
        max_nodes==0u || max_items==0u)
        return PR_U7F_0508_ARGUMENT;
    code_segment=c->cs;
    push(ram,c,c->bp);                                    /* 0508 */
    c->bp=c->sp;                                         /* 0509 */
    c->sp=arithmetic(c,c->sp,6u,0u,16u,1u);               /* 050B */
    c->ax=read_word(ram,c->ds,0x5D96u);                   /* 050E */
    c->dx=read_word(ram,c->ds,0x5D98u);                   /* 0511 */
    put_word(ram,c->ss,(uint16_t)(c->bp-4u),c->ax);       /* 0515 */
    put_word(ram,c->ss,(uint16_t)(c->bp-2u),c->dx);       /* 0518 */
    ram[physical(c->ds,0x6DC0u)]=1u;                     /* 051B */
L0520:
    c->ip=0x0520u;
    if (max_nodes==0u) return PR_U7F_0508_NODE_LIMIT;
    --max_nodes;
    c->ax=arg(ram,c,0xFFFCu);                            /* 0520 */
    c->ax=(uint16_t)(c->ax|arg(ram,c,0xFFFEu));           /* 0523 */
    c->flags&=(uint16_t)~0x08D5u;
    result_flags(c,c->ax,0x8000u);
    if ((c->flags&0x40u)!=0u) goto L05DC;                /* 0526,0528 */
    les(ram,c,0xFFFCu);                                  /* 052B */
    (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+0x84u))],0u,0u,8u,1u);
    if ((c->flags&0x40u)!=0u) goto L0544;                /* 052E,0534 */
    les(ram,c,0xFFFCu);                                  /* 0536 */
    (void)arithmetic(c,ram[physical(c->es,(uint16_t)(c->di+0x84u))],0xB3u,0u,8u,1u);
    if ((c->flags&0x40u)==0u) goto L05C6;                /* 0539,053F,0541 */
L0544:
    put_local(ram,c,5u,0u);                              /* 0544,0548 */
    goto L054D;
L054A:
    carry=c->flags&1u;
    value=arithmetic(c,local(ram,c,5u),1u,0u,8u,0u);
    c->flags=(uint16_t)((c->flags&~1u)|carry);
    put_local(ram,c,5u,(uint8_t)value);                   /* 054A */
L054D:
    c->ip=0x054Du;
    if (max_items==0u) return PR_U7F_0508_ITEM_LIMIT;
    --max_items;
    al(c,local(ram,c,5u));                               /* 054D */
    put_local(ram,c,6u,(uint8_t)c->ax);                  /* 0550 */
    al(c,local(ram,c,6u)); cbw(c);                        /* 0553,0556 */
    c->di=c->ax;                                        /* 0557 */
    c->di=shl(c,c->di); c->di=shl(c,c->di);               /* 0559,055B */
    c->cx=read_word(ram,c->ds,(uint16_t)(c->di+0x67F4u)); /* 055D */
    c->bx=read_word(ram,c->ds,(uint16_t)(c->di+0x67F6u)); /* 0561 */
    al(c,local(ram,c,6u)); cbw(c);                        /* 0565,0568 */
    c->ax=shl(c,c->ax);                                  /* 0569 */
    les(ram,c,0xFFFCu);                                  /* 056B */
    c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);           /* 056E */
    c->ax=read_word(ram,c->es,(uint16_t)(c->di+0x88u));   /* 0570 */
    c->dx=0u; c->flags&=(uint16_t)~0x08D5u;
    result_flags(c,0u,0x8000u);                           /* 0575 */
    c->ax=arithmetic(c,c->ax,c->cx,0u,16u,0u);           /* 0577 */
    c->dx=arithmetic(c,c->dx,c->bx,c->flags&1u,16u,0u);  /* 0579 */
    c->cx=c->ax; c->bx=c->dx;                            /* 057B,057D */
    al(c,local(ram,c,6u)); cbw(c); c->di=c->ax;            /* 057F,0582,0583 */
    c->di=shl(c,c->di); c->di=shl(c,c->di);               /* 0585,0587 */
    put_word(ram,c->ds,(uint16_t)(c->di+0x67F4u),c->cx); /* 0589 */
    put_word(ram,c->ds,(uint16_t)(c->di+0x67F6u),c->bx); /* 058D */
    value=arg(ram,c,0xFFFEu); push(ram,c,value);          /* 0591 */
    value=arg(ram,c,0xFFFCu); push(ram,c,value);          /* 0594 */
    al(c,local(ram,c,6u)); cbw(c);                        /* 0597,059A */
    c->ax=shl(c,c->ax); les(ram,c,0xFFFCu);               /* 059B,059D */
    c->di=arithmetic(c,c->di,c->ax,0u,16u,0u);           /* 05A0 */
    value=read_word(ram,c->es,(uint16_t)(c->di+0x88u));
    push(ram,c,value);                                   /* 05A2 */
    push(ram,c,c->cs); push(ram,c,0x05ABu); c->ip=0x20u; /* 05A7,05A8 */
    (void)pr_u7f_record_subtract_0020(ram,size,c);
    if (c->cs!=code_segment || c->ip!=0x05ABu)
        return PR_U7F_0508_TRANSFER;
    (void)arithmetic(c,local(ram,c,5u),6u,0u,8u,1u);      /* 05AB */
    if ((c->flags&0x40u)==0u) goto L054A;                /* 05AF */
    les(ram,c,0xFFFCu);                                  /* 05B1 */
    c->di=arithmetic(c,c->di,0x88u,0u,16u,0u);           /* 05B4 */
    push(ram,c,c->es); push(ram,c,c->di);                 /* 05B8,05B9 */
    c->ax=0xEu; push(ram,c,c->ax);                        /* 05BA,05BD */
    al(c,0u); push(ram,c,c->ax);                          /* 05BE,05C0 */
    push(ram,c,c->cs); push(ram,c,0x05C6u);              /* 05C1 */
    c->cs=system_cs; c->ip=0x16A8u;
    fill(ram,size,c);
    if (c->cs!=code_segment || c->ip!=0x05C6u)
        return PR_U7F_0508_TRANSFER;
L05C6:
    les(ram,c,0xFFFCu);                                  /* 05C6 */
    c->ax=read_word(ram,c->es,(uint16_t)(c->di+0x104u));  /* 05C9 */
    c->dx=read_word(ram,c->es,(uint16_t)(c->di+0x106u));  /* 05CE */
    put_word(ram,c->ss,(uint16_t)(c->bp-4u),c->ax);       /* 05D3 */
    put_word(ram,c->ss,(uint16_t)(c->bp-2u),c->dx);       /* 05D6 */
    goto L0520;                                         /* 05D9 */
L05DC:
    c->sp=c->bp; c->bp=pop(ram,c); retf(ram,c,0u);        /* 05DC,05DE,05DF */
    return PR_U7F_0508_OK;
}
