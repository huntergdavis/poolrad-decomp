#include "poolrad/replay/u3b0_wait_048e.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE-1u)
static size_t a(uint16_t s,uint16_t o){return (((size_t)s<<4u)+o)&MASK;}
static uint8_t rb(pr_bus*b,uint16_t s,uint16_t o){return pr_bus_read8(b,a(s,o));}
/* Ordered reads matter: EGA's second byte read leaves the final latches. */
static uint16_t rw(pr_bus*b,uint16_t s,uint16_t o){size_t p=a(s,o);uint8_t lo,hi;lo=pr_bus_read8(b,p);hi=pr_bus_read8(b,(p+1u)&MASK);return (uint16_t)(lo|((uint16_t)hi<<8u));}
static int ordinary(uint16_t s){return s<=0x9000u||s>=0xb000u;}
static void cmp0(pr_r0899_cpu*c,unsigned v,unsigned bits){
    unsigned mask=bits==8?0xffu:0xffffu,sign=bits==8?0x80u:0x8000u,p=v&255u;
    v&=mask;p^=p>>4;p^=p>>2;p^=p>>1;
    c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|((p&1u)==0u?4u:0u)|
        (v==0u?0x40u:0u)|((v&sign)?0x80u:0u));
}
pr_u3b0_wait_status pr_u3b0_wait_048e(pr_bus*b,size_t n,pr_r0899_cpu*c,
    const pr_u3b0_pass_env*e,pr_u3b0_wait_diagnostic*d){
    uint16_t off,seg,v;uint8_t gate;
    if(!b||!b->ram||!b->ega||!c||!e||!d||n!=PR_BUS_RAM_SIZE||c->ip!=0x048eu||
       c->bp<0x0090u||c->bp>0xffe3u||c->sp<0x0116u||c->sp>0xfffcu||
       !ordinary(c->cs)||!ordinary(c->ds)||!ordinary(c->ss))return PR_U3B0_WAIT_ARGUMENT;
    memset(d,0,sizeof *d);
    off=rw(b,c->ds,0x49d2u);seg=rw(b,c->ds,0x49d4u);c->di=off;c->es=seg;
    v=rw(b,c->es,(uint16_t)(c->di+0x03feu));cmp0(c,v,16u);
    if(v==0u){gate=rb(b,c->ss,(uint16_t)(c->bp+0x0au));cmp0(c,gate,8u);if(gate==0u)goto clock_gate;}
    gate=rb(b,c->ds,0x6abfu);cmp0(c,gate,8u);
    if(gate!=0u){c->ip=0x04adu;return PR_U3B0_WAIT_ANIMATION;}
clock_gate:
    v=rw(b,c->ds,0x6b30u);cmp0(c,v,16u);
    if((v&0x8000u)!=0u)goto poll;
    if(v!=0u){c->ip=0x0540u;return PR_U3B0_WAIT_CLOCK;}
    v=rw(b,c->ds,0x6b2eu);cmp0(c,v,16u);
    if(v!=0u){c->ip=0x0540u;return PR_U3B0_WAIT_CLOCK;}
poll:
    c->ip=0x0566u;d->status=pr_u3b0_prompt_pass(b,n,c,e,&d->pass);
    return PR_U3B0_WAIT_PASS;
}
