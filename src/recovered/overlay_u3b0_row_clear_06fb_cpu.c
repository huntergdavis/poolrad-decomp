/* U3B0 entry3: original 22-byte caller, composed with raw 0709:05EB. */
#include "poolrad/recovered/overlay_u3b0_row_clear_06fb_cpu.h"
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg<<4u)+off)&MASK;
}
static uint16_t word(pr_bus *bus, uint16_t seg, uint16_t off)
{
    size_t a=physical(seg,off);
    return (uint16_t)(pr_bus_read8(bus,a)|((uint16_t)pr_bus_read8(bus,(a+1u)&MASK)<<8u));
}
static void push(pr_bus *bus, pr_u3b0_06fb_cpu *c, uint16_t value)
{
    size_t a;
    c->sp=(uint16_t)(c->sp-2u);a=physical(c->ss,c->sp);
    pr_bus_write8(bus,a,(uint8_t)value);
    pr_bus_write8(bus,(a+1u)&MASK,(uint8_t)(value>>8u));
}
static uint16_t pop(pr_bus *bus, pr_u3b0_06fb_cpu *c)
{
    uint16_t value=word(bus,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2u);
    return value;
}
static int ordinary(uint16_t seg)
{
    size_t a=(size_t)seg<<4u;
    return a>0xAFFFFu || a+0xFFFFu<0xA0000u;
}
pr_u3b0_06fb_status pr_u3b0_row_clear_06fb_cpu(pr_bus *bus, size_t size,
    pr_u3b0_06fb_cpu *c, size_t max_loopbacks, pr_u3b0_06fb_diagnostic *d)
{
    pr_u3b0_06fb_diagnostic local={-1,{-1,-1,-1,-1}};
    uint16_t caller,target;
    if(d==NULL)d=&local;
    *d=local;
    if(bus==NULL || bus->ram==NULL || bus->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0x06FBu ||
       c->sp<0x003Au || c->sp>0xFFFCu || !ordinary(c->ss) || !ordinary(c->ds))
        return PR_U3B0_06FB_ARGUMENT;
    caller=c->cs;
    push(bus,c,c->bp);c->bp=c->sp;                    /* 06FB..06FC */
    c->ax&=0xFF00u;push(bus,c,c->ax);                 /* 06FE..0700 */
    c->ax=(uint16_t)((c->ax&0xFF00u)|24u);
    push(bus,c,c->ax);                               /* 0701..0703 */
    c->ax&=0xFF00u;push(bus,c,c->ax);                 /* 0704..0706 */
    c->ax=(uint16_t)((c->ax&0xFF00u)|40u);
    push(bus,c,c->ax);c->ip=0x070Au;                  /* 0707..0709 */
    if(pr_bus_read8(bus,physical(caller,0x070Au))!=0x9Au ||
       word(bus,caller,0x070Bu)!=0x05EBu)return PR_U3B0_06FB_CODE;
    target=word(bus,caller,0x070Du);
    push(bus,c,caller);push(bus,c,0x070Fu);
    c->cs=target;c->ip=0x05EBu;
    d->space_status=pr_r0709_space_05eb_cpu(bus,size,c,max_loopbacks,&d->space);
    if(d->space_status!=PR_R0709_SPACE_CPU_OK)return PR_U3B0_06FB_CALLEE;
    if(c->cs!=caller || c->ip!=0x070Fu)return PR_U3B0_06FB_TRANSFER;
    c->bp=pop(bus,c);                                 /* 070F */
    c->ip=pop(bus,c);c->cs=pop(bus,c);                 /* 0710 */
    return PR_U3B0_06FB_OK;
}
