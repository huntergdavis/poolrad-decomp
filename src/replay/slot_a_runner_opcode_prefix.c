#include "poolrad/replay/slot_a_runner_opcode_prefix.h"
#include "poolrad/recovered/overlay_u1c_script_load_al_318d.h"
#include "poolrad/recovered/overlay_u1c_script_store_al_3190.h"
#include "poolrad/recovered/overlay_u1c_script_load_ax_3193.h"
#include "poolrad/recovered/overlay_u1c_script_les_di_3196.h"
static void publish(pr_u1c_3171_cpu *c,const pr_rec_u1c_33bd_cpu *x)
{ c->cs=x->cs;c->ip=x->ip;c->ds=x->ds;c->es=x->es;c->ss=x->ss;c->sp=x->sp;c->bp=x->bp;c->ax=x->ax;c->bx=x->bx;c->cx=x->cx;c->dx=x->dx;c->si=x->si;c->di=x->di;c->flags=x->flags; }
uint16_t pr_slot_a_runner_opcode_prefix(uint8_t *ram,size_t size,pr_u1c_3171_cpu *c)
{
    pr_rec_u1c_33bd_cpu x;
    if(!ram||!c) return 0x318du;
    x=(pr_rec_u1c_33bd_cpu){c->cs,c->ip,c->ds,c->es,c->ss,c->sp,c->bp,c->ax,c->bx,c->cx,c->dx,c->si,c->di,c->flags};
    if(!pr_rec_u1c_318d_load_latch_al(&x,ram,size)) return 0x318du;
    publish(c,&x);
    if(!pr_rec_u1c_3190_store_latch_al(&x,ram,size)) return 0x3190u;
    publish(c,&x);
    if(!pr_rec_u1c_3193_load_script_ax(&x,ram,size)) return 0x3193u;
    publish(c,&x);
    if(!pr_rec_u1c_3196_les_script_pointer(&x,ram,size)) return 0x3196u;
    publish(c,&x);
    return 0x319au;
}
