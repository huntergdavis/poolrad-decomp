#include "poolrad/replay/slot_a_goto_body.h"
#include <stddef.h>
#include <string.h>
static size_t phys(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & 0xfffffu; }
static uint16_t rd(const uint8_t *r, uint16_t s, uint16_t o) { return (uint16_t)(r[phys(s,o)] | ((uint16_t)r[phys(s,(uint16_t)(o+1))] << 8)); }
static int span(const uint8_t *r, size_t p, const uint8_t *ref, size_t n) { return p <= 0x100000u-n && !memcmp(r+p,ref,n); }
int pr_slot_a_goto_body(uint8_t *r, size_t n, const pr_goto_refs *q, uint16_t system_cs,
    pr_u1c_3171_cpu *cpu, pr_u1c_goto_status *status, pr_u1c_goto_callee *callee, int *callee_status)
{
    pr_u1c_goto_cpu raw; pr_u1c_goto_limits limits={system_cs,1,1}; pr_u1c_goto_status result;
    uint16_t ls,ws,lc,wc,off,seg,pc; size_t src[3], ds, p, i;
    _Static_assert(sizeof raw == sizeof *cpu,"CPU size");
#define SAME(f) _Static_assert(offsetof(pr_u1c_goto_cpu,f)==offsetof(pr_u1c_3171_cpu,f),"CPU field " #f)
    SAME(ax); SAME(bx); SAME(cx); SAME(dx); SAME(si); SAME(di); SAME(bp); SAME(sp); SAME(cs); SAME(ip); SAME(ds); SAME(es); SAME(ss); SAME(flags);
#undef SAME
    if (!r || n!=0x100000u || !q || !q->body || !q->loader_stub || !q->word_stub || !q->loader || !q->word || !cpu || !status || !callee || !callee_status || cpu->ip!=0x00f4 || cpu->sp<0x240 || cpu->sp>0xfffc) return 1;
    if (!span(r,phys(cpu->cs,0x00f4),q->body,29) || q->body[6]!=0x9a || q->body[7]!=0x2a || q->body[8] || q->body[19]!=0x9a || q->body[20]!=0x4d || q->body[21]) return 1;
    ls=(uint16_t)(q->body[9]|((uint16_t)q->body[10]<<8)); ws=(uint16_t)(q->body[22]|((uint16_t)q->body[23]<<8));
    if (!span(r,phys(ls,0x2a),q->loader_stub,5) || !span(r,phys(ws,0x4d),q->word_stub,5) || q->loader_stub[0]!=0xea || q->loader_stub[1] || q->loader_stub[2] || q->word_stub[0]!=0xea || q->word_stub[1]!=0xaf || q->word_stub[2]!=6) return 1;
    lc=(uint16_t)(q->loader_stub[3]|((uint16_t)q->loader_stub[4]<<8)); wc=(uint16_t)(q->word_stub[3]|((uint16_t)q->word_stub[4]<<8));
    if (!span(r,phys(lc,0),q->loader,310) || !span(r,phys(wc,0x6af),q->word,36)) return 1;
    off=rd(r,cpu->ds,0x49de); seg=rd(r,cpu->ds,0x49e0); pc=rd(r,cpu->ds,0x49ed);
    if (pc!=0x993b) return 1;
    ds=phys(cpu->ds,0);
    for(i=0;i<3;i++) { src[i]=phys(seg,(uint16_t)(off+pc+0x6701u+i)); if (((src[i]-ds)&0xfffffu)<0x10000u) return 1; }
    if (r[src[0]]!=1 || r[src[1]]!=0x6b || r[src[2]]!=0x99) return 1;
    for(i=0;i<=0x243;i++) { p=phys(cpu->ss,(uint16_t)(cpu->sp-0x240u+i)); if (((p-ds)&0xfffffu)<0x10000u || p==src[0] || p==src[1] || p==src[2]) return 1; }
    memcpy(&raw,cpu,sizeof raw); result=pr_u1c_goto_00f4_cpu(r,n,&raw,&limits,callee,callee_status); memcpy(cpu,&raw,sizeof raw); *status=result; return 0;
}
