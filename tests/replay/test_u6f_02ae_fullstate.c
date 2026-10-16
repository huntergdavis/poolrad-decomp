#include "poolrad/replay/u6f_02ae_fullstate.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static uint8_t ram[0x100000], saved[0x100000], reference[0x100000];
static size_t at(uint16_t seg,uint16_t off) { return ((size_t)seg<<4)+off; }
static uint16_t word(const uint8_t *m,size_t p)
{ return (uint16_t)(m[p]|((uint16_t)m[p+1u]<<8)); }
static void put(uint16_t seg,uint16_t off,uint16_t v)
{ size_t p=at(seg,off); ram[p]=(uint8_t)v; ram[p+1u]=(uint8_t)(v>>8); }

typedef struct {
    unsigned dos_calls, ui_calls, mode;
    uint16_t first_result, expected_color, terminal_result;
    int invalid;
} events;

static pr_slot_a_directory_state initial(uint16_t return_cs,uint16_t return_ip)
{
    pr_slot_a_directory_state s={0};
    memset(ram,0,sizeof ram); s.ram=ram; s.ram_size=sizeof ram;
    s.cpu=(pr_slot_a_directory_cpu){
        .ax=0xaa66,.bx=0x1234,.cx=0x3322,.dx=0x2222,
        .si=0x4444,.di=0x5555,.bp=0x7000,.sp=0x6000,
        .cs=0x2a8e,.ds=0x1000,.es=0x1000,.ss=0x2000,
        .fs=0x3333,.gs=0x4444,.ip=0x02ae,.flags=0x7206};
    put(s.cpu.ss,s.cpu.sp,return_ip); put(s.cpu.ss,0x6002,return_cs);
    ram[at(s.cpu.ds,0x537a)]=3;
    memcpy(ram+at(s.cpu.ds,0x537b),"ABC",3);
    ram[at(s.cpu.cs,0x029a)]=2;
    memcpy(ram+at(s.cpu.cs,0x029b),"Q:",2);
    ram[at(s.cpu.cs,0x02ac)]=1; ram[at(s.cpu.cs,0x02ad)]='!';
    return s;
}

static pr_slot_a_directory_status dos_service(
    void *context,const pr_slot_a_directory_cpu *before,
    const uint8_t *m,size_t size,pr_slot_a_directory_cpu *after,
    pr_slot_a_directory_write *writes,size_t capacity,size_t *count)
{
    events *e=context;
    size_t p=at(before->ds,before->dx);
    assert(before->cs==0x131c && before->ip==0x1821);
    assert(before->ds==before->ss && (before->ax>>8)==0x39);
    assert(capacity>=1 && p+3<=size);
    assert(m[p]==(e->dos_calls ? 'X':'A') &&
           m[p+1u]==(e->dos_calls ? 'Y':'B') && m[p+2u]==0);
    *after=*before; after->ip=0x1823;
    after->ax=e->dos_calls ? 5u:e->first_result;
    after->flags=(uint16_t)(before->flags|1u);
    writes[0]=(pr_slot_a_directory_write){0x50,m[0x50],(uint8_t)(e->dos_calls+1u)};
    *count=1; ++e->dos_calls;
    return PR_SLOT_A_DIRECTORY_OK;
}

/* Synthetic complete-event fixture, not an execution of original UI. */
static pr_u6f_02ae_full_status ui_service(
    void *context,const pr_slot_a_directory_cpu *before,
    const uint8_t *m,size_t size,pr_slot_a_directory_cpu *after,
    pr_slot_a_directory_write *w,size_t capacity,size_t *count)
{
    events *e=context;
    size_t frame=at(before->ss,before->sp), msg, result;
    assert(before->cs==0x0f2d && before->ip==0x0bf5);
    assert(capacity>=8 && frame+10<=size);
    assert(word(m,frame)==0x031f && word(m,frame+2u)==0x2a8e);
    msg=at(word(m,frame+6u),word(m,frame+4u));
    assert(msg+5<=size && m[msg]==4 && !memcmp(m+msg+1u,"Q:A!",4));
    assert(word(m,frame+8u)==e->expected_color);
    result=at(before->ss,(uint16_t)(before->bp-2u));
    *after=*before; after->cs=0x2a8e; after->ip=0x031f;
    after->sp=(uint16_t)(before->sp+10u);
    after->ax=0xbe7a; after->flags=0x7213;
    w[0]=(pr_slot_a_directory_write){0x51,m[0x51],0x61};
    w[1]=(pr_slot_a_directory_write){0x51,0x61,0x62};
    *count=2;
    if (e->mode) {
        w[2]=(pr_slot_a_directory_write){(uint32_t)result,m[result],(uint8_t)e->terminal_result};
        w[3]=(pr_slot_a_directory_write){(uint32_t)(result+1u),m[result+1u],0};
        *count=4;
    } else {
        size_t path=at(before->ds,0x537b);
        w[2]=(pr_slot_a_directory_write){(uint32_t)path,m[path],'X'};
        w[3]=(pr_slot_a_directory_write){(uint32_t)(path+1u),m[path+1u],'Y'};
        *count=4;
    }
    if (e->invalid==1) w[1].old_byte^=1u;
    if (e->invalid==2) after->sp++;
    if (e->invalid==3) after->ip++;
    if (e->invalid==4) *count=capacity+1u;
    if (e->invalid==5) w[0].linear=(uint32_t)size;
    if (e->invalid==6) {
        w[4]=(pr_slot_a_directory_write){(uint32_t)frame,m[frame],0};
        *count=5;
    }
    ++e->ui_calls;
    if (e->invalid==7) return PR_U6F_02AE_FULL_BLOCKED_UI;
    return PR_U6F_02AE_FULL_STEPPED;
}

static pr_u6f_02ae_full_status step(pr_slot_a_directory_state *s,events *e,int ui)
{
    pr_slot_a_directory_write w[8];
    return pr_u6f_02ae_fullstate_step(s,e ? dos_service:NULL,e,
        ui ? ui_service:NULL,e,w,8u);
}

static void reach(pr_slot_a_directory_state *s,events *e,uint16_t cs,uint16_t ip)
{
    unsigned i;
    for (i=0; i<1000u && !(s->cpu.cs==cs && s->cpu.ip==ip); ++i)
        assert(step(s,e,0)==PR_U6F_02AE_FULL_STEPPED);
    assert(s->cpu.cs==cs && s->cpu.ip==ip);
}

static void invocation(uint16_t ret_cs,uint16_t ret_ip,unsigned mode,uint16_t terminal)
{
    pr_slot_a_directory_state s=initial(ret_cs,ret_ip);
    events e={.first_result=0xab03,.expected_color=0xab0e,
              .mode=mode,.terminal_result=terminal};
    unsigned i;
    for (i=0; i<1000u; ++i) {
        pr_u6f_02ae_full_status r=step(&s,&e,1);
        if (r==PR_U6F_02AE_FULL_RETURNED) break;
        assert(r==PR_U6F_02AE_FULL_STEPPED);
    }
    assert(i<1000u && e.ui_calls==1u && e.dos_calls==(mode ? 1u:2u));
    assert(s.cpu.cs==ret_cs && s.cpu.ip==ret_ip && s.cpu.sp==0x6004);
    assert(s.cpu.bp==0x7000 && s.cpu.fs==0x3333 && s.cpu.gs==0x4444);
    assert(ram[0x50]==e.dos_calls && ram[0x51]==0x62);
    assert(word(ram,at(0x2000,0x5ffcu))==(mode ? terminal:5u));
    if (mode) {
        assert(s.cpu.ax==0xbe7a); /* no invented UI register restore */
        assert(s.cpu.flags==0x7246);
    }
    else assert(ram[at(0x1000,0x537b)]=='X' && ram[at(0x1000,0x537c)]=='Y');
}

static void rejection_and_blocking(void)
{
    pr_slot_a_directory_state s=initial(0x2a8e,0x19ea);
    pr_slot_a_directory_cpu before;
    events e={.first_result=3,.expected_color=0x000e};
    int i;
    reach(&s,&e,0x0f2d,0x0bf5);
    before=s.cpu; memcpy(saved,ram,sizeof ram);
    assert(step(&s,&e,0)==PR_U6F_02AE_FULL_BLOCKED_UI);
    assert(!memcmp(&before,&s.cpu,sizeof before) && !memcmp(saved,ram,sizeof ram));
    for (i=1; i<=6; ++i) {
        e.invalid=i;
        assert(step(&s,&e,1)==PR_U6F_02AE_FULL_BAD_EVENT);
        assert(!memcmp(&before,&s.cpu,sizeof before));
        assert(!memcmp(saved,ram,sizeof ram));
    }
    e.invalid=7;
    assert(step(&s,&e,1)==PR_U6F_02AE_FULL_BLOCKED_UI);
    assert(!memcmp(&before,&s.cpu,sizeof before) && !memcmp(saved,ram,sizeof ram));
    e.invalid=0;
    assert(step(&s,&e,1)==PR_U6F_02AE_FULL_STEPPED);
    assert(s.cpu.ax==0xbe7a && s.cpu.flags==0x7213 && s.cpu.sp==0x5dfc);
    s=initial(0x2a8e,0x0027); s.cpu.ip=0x02f7; s.cpu.sp=2;
    before=s.cpu; memcpy(saved,ram,sizeof ram);
    assert(step(&s,NULL,0)==PR_U6F_02AE_FULL_BLOCKED_MEMORY);
    assert(!memcmp(&before,&s.cpu,sizeof before) && !memcmp(saved,ram,sizeof ram));
    s.cpu.ip=0x02eb; s.cpu.sp=1;
    before=s.cpu;
    assert(step(&s,NULL,0)==PR_U6F_02AE_FULL_BLOCKED_MEMORY);
    assert(!memcmp(&before,&s.cpu,sizeof before) && !memcmp(saved,ram,sizeof ram));
    assert(pr_u6f_02ae_fullstate_step(NULL,NULL,NULL,NULL,NULL,NULL,0)==
           PR_U6F_02AE_FULL_ARGUMENT);
    assert(pr_u6f_02ae_fullstate_step(&s,NULL,NULL,NULL,NULL,NULL,1)==
           PR_U6F_02AE_FULL_ARGUMENT);
}

static void prefix_reference(uint16_t result)
{
    pr_slot_a_directory_state s=initial(0x1234,0x5678), ref=s;
    pr_slot_a_directory_write w[8];
    events e={.first_result=result}, other=e;
    unsigned i;
    memcpy(reference,ram,sizeof ram); ref.ram=reference;
    for (i=0; i<500u; ++i) {
        pr_u6f_02ae_status a=pr_u6f_02ae_step(&ref,dos_service,&other,w,8);
        pr_u6f_02ae_full_status b=step(&s,&e,0);
        assert(!memcmp(&s.cpu,&ref.cpu,sizeof s.cpu));
        assert(!memcmp(ram,reference,sizeof ram));
        if (a==PR_U6F_02AE_RETURNED) {
            assert(b==PR_U6F_02AE_FULL_RETURNED); break;
        }
        assert(a==PR_U6F_02AE_STEPPED && b==PR_U6F_02AE_FULL_STEPPED);
    }
    assert(i<500u && e.dos_calls==1 && other.dos_calls==1);
}

static void string_limit(void)
{
    pr_slot_a_directory_state s=initial(0x2a8e,0x0027);
    size_t p=at(s.cpu.cs,0x029a), dest;
    /* Synthetic length reads beyond the shorter literal into fabricated
     * RAM. It exercises concatenation limits, not a reached native input. */
    ram[p]=254;
    ram[at(s.cpu.cs,0x02ac)]=1; ram[at(s.cpu.cs,0x02ad)]='!';
    s.cpu.ip=0x02e9; s.cpu.bp=0x5ffe; s.cpu.sp=0x5dfc;
    put(s.cpu.ss,(uint16_t)(s.cpu.bp-2u),3);
    reach(&s,NULL,0x0f2d,0x0bf5);
    dest=at(s.cpu.ss,(uint16_t)(s.cpu.bp-0x202u));
    assert(ram[dest]==255 && ram[dest+255u]=='A');
    assert(ram[dest+256u]==1 && ram[dest+257u]=='A');
}

int main(void)
{
    prefix_reference(0); prefix_reference(5);
    invocation(0x2a8e,0x0027,0,0); /* earlier U6F entry caller */
    invocation(0x2a8e,0x19ea,0,0); /* saved post-insertion caller */
    invocation(0x1234,0x5678,1,0);
    invocation(0x2a8e,0x0027,1,5);
    rejection_and_blocking(); string_limit();
    puts("U6F02AE full caller: two full-state prefix refs, four synthetic invocations, live retry/result, generic returns, six rejected UI events and string/span gates passed");
    return 0;
}
