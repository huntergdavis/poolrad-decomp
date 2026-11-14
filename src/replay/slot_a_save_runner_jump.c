#include "poolrad/replay/slot_a_save_runner_jump.h"
#include "poolrad/recovered/overlay_u1c_script_jmp_31a8.h"
int pr_slot_a_save_runner_jump(pr_u1c_3171_cpu *c,const uint8_t *ram,size_t size)
{
    pr_rec_u1c_33bd_cpu x;
    if(!c || !ram) return 0;
    x=(pr_rec_u1c_33bd_cpu){c->cs,c->ip,c->ds,c->es,c->ss,c->sp,c->bp,c->ax,c->bx,c->cx,c->dx,c->si,c->di,c->flags};
    if(!pr_rec_u1c_31a8_jmp_loop(&x,ram,size)) return 0;
    c->ip=x.ip; return 1;
}
