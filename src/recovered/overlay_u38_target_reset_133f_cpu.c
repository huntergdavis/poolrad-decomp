#include "poolrad/recovered/overlay_u38_target_reset_133f_cpu.h"
#define MASK (PR_RES_2637_RAM_SIZE-1u)
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(const uint8_t *r,uint16_t s,uint16_t o) { return word(r,physical(s,o)); }
static void push(uint8_t *r,pr_u38_133f_cpu *c,uint16_t v) { size_t p;c->sp=(uint16_t)(c->sp-2u);p=physical(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1)&MASK]=(uint8_t)(v>>8); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
pr_u38_133f_status pr_u38_target_reset_133f_cpu(uint8_t *r,size_t size,pr_u38_133f_cpu *c)
{
    size_t p;uint16_t caller,segment;uint8_t al;
    if(r==NULL || c==NULL || size!=PR_RES_2637_RAM_SIZE || c->ip!=0x133F || c->sp<0x10 || c->sp>0xFFF2)return PR_U38_133F_ARGUMENT;
    push(r,c,c->bp);c->bp=c->sp;push(r,c,rd(r,c->ss,(uint16_t)(c->bp+14)));push(r,c,rd(r,c->ss,(uint16_t)(c->bp+12)));
    c->ip=0x1348;
    if(r[physical(c->cs,c->ip)]!=0x9A || rd(r,c->cs,0x1349)!=0x2637)return PR_U38_133F_CODE;
    caller=c->cs;segment=rd(r,caller,0x134B);push(r,c,caller);push(r,c,0x134D);c->cs=segment;c->ip=0x2637;
    if(pr_res_00ba_target_reset_2637_cpu(r,size,c)!=PR_RES_2637_OK)return PR_U38_133F_CALLEE;
    if(c->cs!=caller || c->ip!=0x134D)return PR_U38_133F_TRANSFER;
    al=(uint8_t)c->ax;c->flags&=(uint16_t)~0x08D5u;
    if(parity(al))c->flags|=4;
    if(al==0)c->flags|=0x40;
    if(al&0x80)c->flags|=0x80;
    /* Both outcomes of JE1351 enter the same POP BP. */
    c->bp=rd(r,c->ss,c->sp);c->sp=(uint16_t)(c->sp+2);
    p=physical(c->ss,c->sp);c->ip=word(r,p);c->cs=word(r,(p+2)&MASK);c->sp=(uint16_t)(c->sp+14);
    return PR_U38_133F_OK;
}
