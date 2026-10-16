#include "poolrad/replay/u6f_1446_fullstate.h"
#include "poolrad/recovered/overlay_u6f_party_insert_1446_ss.h"
#include "poolrad/recovered/overlay_u9b_entry_zero_0000.h"

#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { \
    fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; \
} } while (0)
#define SIZE (1u<<20)
#define MASK (SIZE-1u)
static uint8_t ram[SIZE], reference[SIZE], before[SIZE];

static size_t at(uint16_t seg, uint16_t off)
{ return (((size_t)seg<<4)+off)&MASK; }
static uint16_t get(const uint8_t *m, uint16_t seg, uint16_t off)
{ size_t p=at(seg,off); return (uint16_t)(m[p]|((uint16_t)m[(p+1u)&MASK]<<8)); }
static void put(uint8_t *m, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p=at(seg,off); m[p]=(uint8_t)v; m[(p+1u)&MASK]=(uint8_t)(v>>8); }
static void far_put(uint16_t seg,uint16_t off,uint16_t vs,uint16_t vo)
{ put(ram,seg,off,vo); put(ram,seg,(uint16_t)(off+2u),vs); }

static pr_slot_a_directory_state initial(unsigned nodes,int stack_alias,
                                         int window_alias,int warm)
{
    pr_slot_a_directory_state s={0};
    uint16_t incoming_seg=stack_alias ? 0x2000u : 0x3000u;
    uint16_t incoming_off=stack_alias ? 0x5f33u : 0x0100u;
    unsigned i;
    memset(ram,0,sizeof ram); s.ram=ram; s.ram_size=sizeof ram;
    s.cpu=(pr_slot_a_directory_cpu){
        .ax=0x3e01,.bx=6,.cx=0x24,.dx=0,.si=0x44ba,.di=0x3626,
        .bp=0x7000,.sp=0x6000,.cs=0x2a8e,.ip=0x1446,
        .ds=0x1000,.es=0x2000,.ss=0x2000,.fs=0x3333,.gs=0x4444,.flags=0x7206
    };
    put(ram,0x2000,0x6000,0x3456); put(ram,0x2000,0x6002,0x1111);
    far_put(0x2000,0x6004,incoming_seg,incoming_off);
    ram[at(0x2a8e,0x1426)]=0xff;
    far_put(0x1000,0x49d6,window_alias ? incoming_seg : 0x4000u,
            window_alias ? (uint16_t)(incoming_off+0xb2u-0x67cu) : 0u);
    if (nodes) far_put(0x1000,0x5d96,0x5000,0x0100);
    for (i=0; i<nodes; ++i) {
        uint16_t seg=(uint16_t)(0x5000u+i*0x20u);
        ram[at(seg,0x01bf)]=(uint8_t)(i%8u);
        if (i+1u<nodes) far_put(seg,0x0204,(uint16_t)(seg+0x20u),0x0100);
    }
    if (warm) {
        ram[at(0x08d0,0x25)]=0xea;
        put(ram,0x08d0,0x26,0); put(ram,0x08d0,0x28,0x2ccd);
    } else { ram[at(0x08d0,0x25)]=0xcd; ram[at(0x08d0,0x26)]=0x3f; }
    return s;
}

static int reach_stub(pr_slot_a_directory_state *s)
{
    unsigned i;
    for (i=0; i<30000u; ++i) {
        if (s->cpu.cs==0x08d0 && s->cpu.ip==0x25) return 0;
        CHECK(pr_u6f_1446_step(s,NULL,NULL,NULL,0u)==PR_U6F_1446_STEPPED);
    }
    CHECK(0 && "external observation budget exhausted"); return 1;
}

static int prefix_case(unsigned nodes,int stack_alias,int window_alias)
{
    pr_slot_a_directory_state s=initial(nodes,stack_alias,window_alias,0);
    pr_u6f_insert_dos_memory memory={reference,sizeof reference,s.cpu.ds};
    pr_u6f_insert_ss_frame frame={s.cpu.cs,s.cpu.ss,s.cpu.sp,s.cpu.bp};
    pr_u6f_insert_ss_report report;
    memcpy(reference,ram,sizeof ram);
    CHECK(pr_u6f_party_insert_1446_ss(&memory,&frame,&report)==
          PR_U6F_INSERT_SS_AT_U9B_TRANSFER);
    CHECK(reach_stub(&s)==0);
    CHECK(memcmp(ram,reference,sizeof ram)==0);
    CHECK(s.cpu.sp==report.transfer_sp && s.cpu.bp==report.callee_bp);
    CHECK(s.cpu.fs==0x3333 && s.cpu.gs==0x4444);
    memcpy(before,ram,sizeof ram);
    {
        pr_slot_a_directory_cpu c=s.cpu;
        CHECK(pr_u6f_1446_step(&s,NULL,NULL,NULL,0u)==PR_U6F_1446_BLOCKED_COLD_DISPATCH);
        CHECK(memcmp(&c,&s.cpu,sizeof c)==0 && memcmp(before,ram,sizeof ram)==0);
    }
    return 0;
}

typedef struct { unsigned calls; int bad_old,bad_target,bad_pc,overflow,omit_frame; } event;
static void append_write(pr_slot_a_directory_write *w,size_t *count,
                          const uint8_t *m,size_t p,uint8_t v)
{
    size_t i=*count;
    uint8_t old=m[p];
    while (i) { --i; if (w[i].linear==p) { old=w[i].new_byte; break; } }
    w[*count]=(pr_slot_a_directory_write){(uint32_t)p,old,v}; ++*count;
}
static void append_word(pr_slot_a_directory_write *w,size_t *count,
                         const uint8_t *m,size_t p,uint16_t v)
{
    append_write(w,count,m,p,(uint8_t)v);
    append_write(w,count,m,(p+1u)&MASK,(uint8_t)(v>>8));
}
static pr_u6f_1446_status synthetic_cold(
    void *context,const pr_slot_a_directory_cpu *c,
    const uint8_t *m,size_t size,pr_slot_a_directory_cpu *after,
    pr_slot_a_directory_write *w,size_t capacity,size_t *count)
{
    event *e=context;
    if (size!=SIZE || capacity<16u) return PR_U6F_1446_BLOCKED_COLD_DISPATCH;
    ++e->calls; *count=0u; *after=*c;
    if (!e->omit_frame) {
        append_word(w,count,m,at(c->ss,(uint16_t)(c->sp-2u)),c->flags);
        append_word(w,count,m,at(c->ss,(uint16_t)(c->sp-4u)),c->cs);
        append_word(w,count,m,at(c->ss,(uint16_t)(c->sp-6u)),0x0027);
        append_word(w,count,m,at(c->ss,(uint16_t)(c->sp-6u)),0x0025);
    }
    append_write(w,count,m,at(0x08d0,0x25),0xea);
    append_word(w,count,m,at(0x08d0,0x26),0);
    append_word(w,count,m,at(0x08d0,0x28),e->bad_target ? 0x2cce : 0x2ccd);
    append_word(w,count,m,at(c->ds,0x43e0),1u);
    append_write(w,count,m,0x80u,0xaau);
    if (e->bad_old) w[0].old_byte^=1u;
    if (e->bad_pc) after->cs=0;
    if (e->overflow) *count=capacity+1u;
    return PR_U6F_1446_STEPPED;
}

static int event_rejection(void)
{
    pr_slot_a_directory_state s=initial(0u,0,0,0);
    pr_slot_a_directory_write w[16];
    unsigned kind;
    CHECK(reach_stub(&s)==0);
    for (kind=0; kind<5u; ++kind) {
        event e={0}; pr_slot_a_directory_cpu c=s.cpu;
        e.bad_old=kind==0u; e.bad_target=kind==1u;
        e.bad_pc=kind==2u; e.overflow=kind==3u;
        e.omit_frame=kind==4u;
        memcpy(before,ram,sizeof ram);
        CHECK(pr_u6f_1446_step(&s,synthetic_cold,&e,w,16u)==PR_U6F_1446_BAD_EVENT);
        CHECK(memcmp(&c,&s.cpu,sizeof c)==0 && memcmp(before,ram,sizeof ram)==0);
    }
    {
        event e={0};
        CHECK(pr_u6f_1446_step(&s,synthetic_cold,&e,w,16u)==PR_U6F_1446_STEPPED);
        CHECK(e.calls==1u && ram[0x80]==0xaa);
        CHECK(get(ram,s.cpu.ss,(uint16_t)(s.cpu.sp-6u))==0x25);
        CHECK(pr_u6f_1446_step(&s,NULL,NULL,NULL,0u)==PR_U6F_1446_STEPPED);
        CHECK(s.cpu.cs==0x2ccd && s.cpu.ip==0);
    }
    return 0;
}

static int full_case(uint16_t return_cs,uint16_t return_ip,unsigned classes,int cold)
{
    pr_slot_a_directory_state s=initial(classes ? 1u : 0u,0,0,!cold);
    pr_slot_a_directory_write writes[16];
    pr_u9b_0000_cpu u;
    event e={0};
    unsigned i;
    put(ram,0x2000,0x6000,return_ip); put(ram,0x2000,0x6002,return_cs);
    if (classes) {
        ram[at(0x3000,0x0196)]=classes==1u ? 2u : 255u;
        ram[at(0x3000,0x019b)]=classes==1u ? 2u : 255u;
        ram[at(0x3000,0x019c)]=classes==1u ? 1u : 255u;
    }
    CHECK(reach_stub(&s)==0);
    if (cold) {
        CHECK(pr_u6f_1446_step(&s,synthetic_cold,&e,writes,16u)==PR_U6F_1446_STEPPED);
        CHECK(e.calls==1u);
    }
    CHECK(pr_u6f_1446_step(&s,NULL,NULL,NULL,0u)==PR_U6F_1446_STEPPED);
    CHECK(s.cpu.cs==0x2ccd && s.cpu.ip==0u);
    u=(pr_u9b_0000_cpu){s.cpu.ax,s.cpu.bx,s.cpu.cx,s.cpu.dx,s.cpu.si,s.cpu.di,
        s.cpu.bp,s.cpu.sp,s.cpu.cs,s.cpu.ip,s.cpu.ds,s.cpu.es,s.cpu.ss,s.cpu.flags};
    memcpy(reference,ram,sizeof ram);
    CHECK(pr_u9b_entry_zero_0000(reference,sizeof reference,&u)==PR_U9B_0000_OK);
    CHECK(pr_u6f_1446_step(&s,NULL,NULL,NULL,0u)==PR_U6F_1446_STEPPED);
    CHECK(memcmp(reference,ram,sizeof ram)==0);
#define REG(name) CHECK(s.cpu.name==u.name)
    REG(ax); REG(bx); REG(cx); REG(dx); REG(si); REG(di); REG(bp); REG(sp);
    REG(cs); REG(ip); REG(ds); REG(es); REG(ss); REG(flags);
#undef REG
    for (i=0; i<4u; ++i) {
        pr_u6f_1446_status result=pr_u6f_1446_step(&s,NULL,NULL,NULL,0u);
        if (result==PR_U6F_1446_RETURNED) {
            CHECK(s.cpu.cs==return_cs && s.cpu.ip==return_ip);
            CHECK(s.cpu.sp==0x6008 && s.cpu.bp==0x7000);
            CHECK(s.cpu.fs==0x3333 && s.cpu.gs==0x4444); return 0;
        }
        CHECK(result==PR_U6F_1446_STEPPED);
    }
    CHECK(0 && "outer RETF4 not reached"); return 1;
}

static int wrap_and_bounds(void)
{
    pr_slot_a_directory_state s=initial(0u,0,0,1);
    s.cpu.ip=0x1569; s.cpu.es=0xffff; s.cpu.di=0xf;
    /* Place the window word across physical FFFFF -> 00000. */
    s.cpu.di=(uint16_t)(0xfu-0x67cu);
    ram[0xfffff]=0xff; ram[0]=0;
    CHECK(pr_u6f_1446_step(&s,NULL,NULL,NULL,0u)==PR_U6F_1446_STEPPED);
    CHECK(ram[0xfffff]==0 && ram[0]==1);
    s.cpu.ip=0x157c; s.cpu.ss=0xffff; s.cpu.sp=0xf;
    put(ram,0xffff,0xf,0x4567); put(ram,0xffff,0x11,0x1234);
    CHECK(pr_u6f_1446_step(&s,NULL,NULL,NULL,0u)==PR_U6F_1446_RETURNED);
    CHECK(s.cpu.ip==0x4567 && s.cpu.cs==0x1234 && s.cpu.sp==0x17);
    s.ram_size=SIZE-1u;
    CHECK(pr_u6f_1446_step(&s,NULL,NULL,NULL,0u)==PR_U6F_1446_ARGUMENT);
    CHECK(pr_u6f_1446_step(NULL,NULL,NULL,NULL,0u)==PR_U6F_1446_ARGUMENT);
    return 0;
}

int main(void)
{
    CHECK(prefix_case(0u,0,0)==0); CHECK(prefix_case(1u,0,0)==0);
    CHECK(prefix_case(8u,0,0)==0); CHECK(prefix_case(300u,0,0)==0);
    CHECK(prefix_case(1u,1,0)==0); CHECK(prefix_case(1u,0,1)==0);
    CHECK(event_rejection()==0);
    CHECK(full_case(0x2a8e,0x1419,0u,0)==0);
    CHECK(full_case(0x2a8e,0x19e6,1u,0)==0);
    CHECK(full_case(0x2222,0x318f,2u,0)==0);
    CHECK(full_case(0x2222,0x3230,1u,1)==0);
    CHECK(wrap_and_bounds()==0);
    puts("U6F1446: six full-RAM prefix references; four full-state invocations; "
         "cold/warm, event rejection, generic returns and wrap gates passed");
    return 0;
}
