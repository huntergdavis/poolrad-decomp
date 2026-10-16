#include "poolrad/replay/u6f_02ae_return_join_fixture.h"

static const uint8_t *span(const pr_slot_a_directory_state *s,uint16_t off,size_t n)
{
    size_t p=((size_t)s->cpu.ss<<4)+off;
    if((size_t)off+n>0x10000u || p>s->ram_size || n>s->ram_size-p) return NULL;
    return s->ram+p;
}
static uint16_t word(const uint8_t *p)
{ return (uint16_t)(p[0]|((uint16_t)p[1]<<8)); }
static pr_u6f_02ae_join_cpu14 project(const pr_slot_a_directory_cpu *c)
{
    pr_u6f_02ae_join_cpu14 result={c->ax,c->bx,c->cx,c->dx,c->si,c->di,
        c->bp,c->sp,c->cs,c->ip,c->ds,c->es,c->ss,c->flags};
    return result;
}
static uint16_t changes(const pr_u6f_02ae_join_cpu14 *a,const pr_u6f_02ae_join_cpu14 *b)
{
    const uint16_t x[]={a->ax,a->bx,a->cx,a->dx,a->si,a->di,a->bp,a->sp,a->cs,a->ip,a->ds,a->es,a->ss,a->flags};
    const uint16_t y[]={b->ax,b->bx,b->cx,b->dx,b->si,b->di,b->bp,b->sp,b->cs,b->ip,b->ds,b->es,b->ss,b->flags};
    unsigned i; uint16_t mask=0;
    for(i=0;i<14;++i) if(x[i]!=y[i]) mask|=(uint16_t)(1u<<i);
    return mask;
}
pr_u6f_02ae_return_join_status pr_u6f_02ae_return_join_fixture(
    pr_slot_a_directory_state *s,pr_u6f_02ae_return_join_report *report)
{
    pr_slot_a_directory_state staged;
    pr_u6f_02ae_return_join_report r={0};
    const uint8_t *saved,*frame;
    pr_u6f_01b8_entry_status status=PR_U6F_01B8_ENTRY_STEPPED;
    unsigned i;
    if(!s || !report) return PR_U6F_02AE_JOIN_ARGUMENT;
    if(!s->ram || s->ram_size>0x100000u) return PR_U6F_02AE_JOIN_BLOCKED_MEMORY;
    if(s->cpu.cs!=0x2a8eu || s->cpu.ip!=0x032bu) return PR_U6F_02AE_JOIN_BLOCKED_PC;
    saved=span(s,s->cpu.bp,2u);
    r.frame_offset=(uint16_t)(s->cpu.bp+2u);
    frame=span(s,r.frame_offset,4u);
    if(!saved || !frame) return PR_U6F_02AE_JOIN_BLOCKED_MEMORY;
    r.frame_ss=s->cpu.ss; r.saved_caller_bp=word(saved);
    r.return_ip=word(frame); r.return_cs=word(frame+2);
    r.before=project(&s->cpu); r.fs_before=s->cpu.fs; r.gs_before=s->cpu.gs;
    staged=*s;
    /* Fixed three-instruction source tail, not a host retry/loop cap. */
    for(i=0;i<3;++i){
        status=pr_u6f_01b8_entry_step(&staged,NULL,NULL,NULL,0u);
        if(status==PR_U6F_01B8_ENTRY_BLOCKED_MEMORY) return PR_U6F_02AE_JOIN_BLOCKED_MEMORY;
        if(i<2 && status!=PR_U6F_01B8_ENTRY_STEPPED) return PR_U6F_02AE_JOIN_BLOCKED_PC;
    }
    if(status!=PR_U6F_01B8_ENTRY_BOUNDARY && status!=PR_U6F_01B8_ENTRY_TRANSFER)
        return PR_U6F_02AE_JOIN_BLOCKED_PC;
    r.after=project(&staged.cpu); r.changed_mask=changes(&r.before,&r.after);
    r.fs_after=staged.cpu.fs; r.gs_after=staged.cpu.gs;
    r.ram_changed_bytes=0; /* All three reused source operations read RAM only. */
    s->cpu=staged.cpu; *report=r;
    return status==PR_U6F_01B8_ENTRY_BOUNDARY ? PR_U6F_02AE_JOINED_01C3 : PR_U6F_02AE_JOIN_TRANSFER;
}
