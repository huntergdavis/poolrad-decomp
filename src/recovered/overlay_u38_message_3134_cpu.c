/* U38 +3134: original 70-byte gated message caller, composed with raw
 * 0AF8:0B0B and 00BA:1693. See docs/re/U38_MESSAGE_3134_CPU.md. */
#include "poolrad/recovered/overlay_u38_message_3134_cpu.h"
#define MASK (PR_BUS_RAM_SIZE-1u)
#define COPY(to,from) do { \
    (to).ax=(from).ax; (to).bx=(from).bx; (to).cx=(from).cx; (to).dx=(from).dx; \
    (to).si=(from).si; (to).di=(from).di; (to).bp=(from).bp; (to).sp=(from).sp; \
    (to).cs=(from).cs; (to).ip=(from).ip; (to).ds=(from).ds; (to).es=(from).es; \
    (to).ss=(from).ss; (to).flags=(from).flags; } while(0)
static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg<<4u)+off)&MASK;
}
static uint8_t byte_at(pr_bus *bus, uint16_t seg, uint16_t off)
{
    return pr_bus_read8(bus,physical(seg,off));
}
static uint16_t word(pr_bus *bus, uint16_t seg, uint16_t off)
{
    size_t a=physical(seg,off);
    return (uint16_t)(pr_bus_read8(bus,a)|((uint16_t)pr_bus_read8(bus,(a+1u)&MASK)<<8u));
}
static void put_word(pr_bus *bus, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a=physical(seg,off);
    pr_bus_write8(bus,a,(uint8_t)value);
    pr_bus_write8(bus,(a+1u)&MASK,(uint8_t)(value>>8u));
}
static void push(pr_bus *bus, pr_u38_3134_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);
    put_word(bus,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *bus, pr_u38_3134_cpu *c)
{
    uint16_t value=word(bus,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);
    return value;
}
static uint16_t local(const pr_u38_3134_cpu *c, int displacement)
{
    return (uint16_t)(c->bp+displacement);
}
static unsigned parity(uint8_t v)
{
    v^=(uint8_t)(v>>4u);v^=(uint8_t)(v>>2u);v^=(uint8_t)(v>>1u);
    return (v&1u)==0u;
}
/* SUB/CMP flags; every defined flag is written, undefined ones do not exist. */
static void subtract(pr_u38_3134_cpu *c, uint16_t a, uint16_t b, unsigned bits)
{
    uint32_t mask=bits==8u?0xFFu:0xFFFFu,sign=bits==8u?0x80u:0x8000u;
    uint32_t r=((uint32_t)a-b)&mask;
    c->flags&=(uint16_t)~0x08D5u;
    if(a<b)c->flags|=0x0001u;
    if(parity((uint8_t)r))c->flags|=0x0004u;
    if((a^b^r)&0x10u)c->flags|=0x0010u;
    if(r==0u)c->flags|=0x0040u;
    if(r&sign)c->flags|=0x0080u;
    if(((a^b)&(a^r))&sign)c->flags|=0x0800u;
}
static int ordinary(uint16_t seg)
{
    size_t a=(size_t)seg<<4u;
    return a>0xAFFFFu || a+0xFFFFu<0xA0000u;
}
static pr_u38_3134_status far_call(pr_bus *bus, pr_u38_3134_cpu *c, uint16_t site,
    uint16_t target, pr_u38_3134_callee which, const pr_u38_3134_limits *l,
    pr_u38_3134_diagnostic *d)
{
    uint16_t caller=c->cs,next=(uint16_t)(site+5u),segment;
    c->ip=site;
    if(byte_at(bus,caller,site)!=0x9Au || word(bus,caller,(uint16_t)(site+1u))!=target)
        return PR_U38_3134_CODE;
    segment=word(bus,caller,(uint16_t)(site+3u));
    push(bus,c,caller);push(bus,c,next);
    c->cs=segment;c->ip=target;d->callee=(int)which;
    if(which==PR_U38_3134_STRLOAD) {
        pr_system_leaf_cpu inner;
        COPY(inner,*c);
        d->status=(int)pr_system_strload_0b0b(bus->ram,PR_BUS_RAM_SIZE,&inner);
        COPY(*c,inner);
    } else {
        pr_r00ba_1693_cpu inner;
        COPY(inner,*c);
        d->status=(int)pr_r00ba_display_1693_cpu(bus,PR_BUS_RAM_SIZE,&inner,
            l->clear_loopbacks,l->rect_loopbacks,l->printer_loopbacks,
            l->glyph_loopbacks,l->delay_loopbacks,&d->display);
        COPY(*c,inner);
    }
    if(d->status!=0)return PR_U38_3134_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U38_3134_TRANSFER;
    return PR_U38_3134_OK;
}
pr_u38_3134_status pr_u38_message_3134_cpu(pr_bus *bus, size_t size,
    pr_u38_3134_cpu *c, const pr_u38_3134_limits *l, pr_u38_3134_diagnostic *d)
{
    pr_u38_3134_diagnostic local_diag={-1,-1,{-1,-1,-1,{-1,-1,{-1,-1}},{-1,-1,-1,-1}}};
    pr_u38_3134_status status;
    uint16_t a_off,a_seg;
    if(d==NULL)d=&local_diag;
    *d=local_diag;
    if(bus==NULL || bus->ram==NULL || bus->ega==NULL || c==NULL || l==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0x3134u || c->sp<0x0192u || c->sp>0xFFF2u ||
       !ordinary(c->cs) || !ordinary(c->ds) || !ordinary(c->ss))
        return PR_U38_3134_ARGUMENT;
    push(bus,c,c->bp);c->bp=c->sp;                                /* 3134..3135 */
    subtract(c,c->sp,0x1Eu,16u);c->sp=(uint16_t)(c->sp-0x1Eu);   /* 3137 */
    c->ax=word(bus,c->ss,local(c,8));c->dx=word(bus,c->ss,local(c,10));
    put_word(bus,c->ss,local(c,-4),c->ax);put_word(bus,c->ss,local(c,-2),c->dx);
    subtract(c,byte_at(bus,c->ss,local(c,6)),0u,8u);              /* 3146 */
    if((c->flags&0x0040u)==0u)goto done;                          /* 314A/314C */
    c->di=word(bus,c->ss,local(c,12));c->es=word(bus,c->ss,local(c,14)); /* 314E */
    subtract(c,byte_at(bus,c->es,(uint16_t)(c->di+0x10u)),0x13u,8u);     /* 3151 */
    if((c->flags&0x0001u)==0u)goto done;                          /* 3156 JAE */
    a_off=word(bus,c->ss,local(c,-4));a_seg=word(bus,c->ss,local(c,-2)); /* 3158 */
    c->di=a_off;c->es=a_seg;
    pr_bus_write8(bus,physical(c->es,(uint16_t)(c->di+0x34u)),0u);       /* 315B */
    c->di=local(c,-0x1D);push(bus,c,c->ss);push(bus,c,c->di);     /* 3160..3164 */
    c->di=0x311Bu;push(bus,c,c->cs);push(bus,c,c->di);            /* 3165..3169 */
    status=far_call(bus,c,0x316Au,0x0B0Bu,PR_U38_3134_STRLOAD,l,d);
    if(status!=PR_U38_3134_OK)return status;
    status=far_call(bus,c,0x316Fu,0x1693u,PR_U38_3134_DISPLAY,l,d);
    if(status!=PR_U38_3134_OK)return status;
done:
    c->sp=c->bp;c->bp=pop(bus,c);                                 /* 3174..3176 */
    c->ip=pop(bus,c);c->cs=pop(bus,c);c->sp=(uint16_t)(c->sp+0x0Au); /* 3177 */
    return PR_U38_3134_OK;
}
