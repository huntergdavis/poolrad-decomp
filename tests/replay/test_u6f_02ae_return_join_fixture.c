#include "poolrad/replay/u6f_02ae_return_join_fixture.h"
#include <stdio.h>
#include <string.h>

#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
static uint8_t ram[655360], saved[655360];
static size_t at(uint16_t seg, uint16_t off) { return ((size_t)seg<<4)+off; }
static void put(uint16_t seg,uint16_t off,uint16_t value)
{ size_t p=at(seg,off); ram[p]=(uint8_t)value; ram[p+1]=(uint8_t)(value>>8); }
static uint16_t get(uint16_t seg,uint16_t off)
{ size_t p=at(seg,off); return (uint16_t)(ram[p]|((uint16_t)ram[p+1]<<8)); }
static pr_slot_a_directory_state initial(void)
{
    pr_slot_a_directory_state s={0};
    memset(ram,0x55,sizeof ram); s.ram=ram; s.ram_size=sizeof ram;
    s.cpu=(pr_slot_a_directory_cpu){.ax=0xabcd,.bx=1,.cx=2,.dx=3,
        .si=4,.di=5,.bp=0x7000,.sp=0x6000,.cs=0x2a8e,.ip=0x1b8,
        .ds=0x1000,.es=0x1100,.ss=0x2000,.fs=0x3456,.gs=0x789a,.flags=0x7206};
    put(s.cpu.ss,0x6000,0xabcd); put(s.cpu.ss,0x6002,0x2345);
    put(s.cpu.ss,0x6004,0x1234); put(s.cpu.ss,0x6006,0x4567);
    put(s.cpu.ds,0x4426,0); ram[at(s.cpu.ds,0x537a)]=5;
    memset(ram+at(s.cpu.ds,0x537b),'X',5);
    return s;
}
typedef struct { uint16_t result; int carry,invalid; unsigned calls; } event;
static pr_slot_a_directory_status dos(void *context,
    const pr_slot_a_directory_cpu *before,const uint8_t *memory,size_t size,
    pr_slot_a_directory_cpu *after,pr_slot_a_directory_write *writes,
    size_t capacity,size_t *count)
{
    event *e=context;
    if (before->cs!=0x131c || before->ip!=0x1821 ||
        (before->ax>>8)!=0x39 || size<0x51 || capacity<1)
        return PR_SLOT_A_DIRECTORY_BAD_EVENT;
    ++e->calls; *after=*before; after->ip=0x1823; after->ax=e->result;
    after->flags=(uint16_t)((before->flags&~1u)|(e->carry?1u:0u));
    writes[0]=(pr_slot_a_directory_write){0x50,memory[0x50],0x9b};
    if(e->invalid) writes[0].old_byte^=1u;
    *count=1; return PR_SLOT_A_DIRECTORY_OK;
}

static uint8_t entry_ram[655360];
static int from_entry(uint16_t result,int carry)
{
    pr_slot_a_directory_state s=initial();
    pr_u6f_02ae_return_join_report r;
    pr_slot_a_directory_write journal[1];
    event e={result,carry,0,0};
    unsigned budget; size_t i,net=0;
    memcpy(entry_ram,ram,sizeof ram);
    for(budget=0;budget<500 && !(s.cpu.cs==0x2a8e && s.cpu.ip==0x32b);++budget)
        CHECK(pr_u6f_01b8_entry_step(&s,dos,&e,journal,1)==PR_U6F_01B8_ENTRY_STEPPED);
    CHECK(budget<500 && e.calls==1);
    memcpy(saved,ram,sizeof ram);
    CHECK(pr_u6f_02ae_return_join_fixture(&s,&r)==PR_U6F_02AE_JOINED_01C3);
    CHECK(r.frame_ss==s.cpu.ss && r.frame_offset==0x5cf4);
    CHECK(r.saved_caller_bp==0x5ffe && r.return_ip==0x1c3 && r.return_cs==0x2a8e);
    CHECK(r.after.sp==0x5cf8 && r.after.bp==0x5ffe && r.after.ip==0x1c3);
    CHECK(r.before.cs==r.after.cs && r.before.flags==r.after.flags);
    CHECK((r.changed_mask & (1u<<9))!=0);
    CHECK((r.changed_mask & (uint16_t)~0x02c0u)==0);
    CHECK(r.fs_before==0x3456 && r.fs_after==0x3456);
    CHECK(r.gs_before==0x789a && r.gs_after==0x789a);
    CHECK(r.ram_changed_bytes==0 && !memcmp(saved,ram,sizeof ram));
    CHECK(get(s.cpu.ss,0x6000)==0xabcd && get(s.cpu.ss,0x6006)==0x4567);
    for(i=0;i<sizeof ram;++i) if(entry_ram[i]!=ram[i]) ++net;
    CHECK(net>0);
    printf("synthetic DOS result %u: entry-to-join RAM net=%zu; return-tail RAM=0; CPU14 mask=%04x\n",result,net,r.changed_mask);
    return 0;
}
static int unchanged_join(pr_slot_a_directory_state *s,pr_u6f_02ae_return_join_status expected)
{
    pr_slot_a_directory_cpu cpu=s->cpu;
    pr_u6f_02ae_return_join_report r,before;
    memset(&r,0xa5,sizeof r); before=r; memcpy(saved,ram,sizeof ram);
    CHECK(pr_u6f_02ae_return_join_fixture(s,&r)==expected);
    CHECK(!memcmp(&r,&before,sizeof r) && !memcmp(&cpu,&s->cpu,sizeof cpu));
    CHECK(!memcmp(saved,ram,sizeof ram)); return 0;
}
static int explicit_boundaries(void)
{
    pr_slot_a_directory_state s=initial();
    pr_u6f_02ae_return_join_report r;
    s.cpu.ip=0x32b; s.cpu.bp=0x5000;
    put(s.cpu.ss,0x5000,0x7777); put(s.cpu.ss,0x5002,0x4321);
    put(s.cpu.ss,0x5004,0x1234); memcpy(saved,ram,sizeof ram);
    CHECK(pr_u6f_02ae_return_join_fixture(&s,&r)==PR_U6F_02AE_JOIN_TRANSFER);
    CHECK(s.cpu.ip==0x4321 && s.cpu.cs==0x1234 && s.cpu.sp==0x5006 && s.cpu.bp==0x7777);
    CHECK(r.return_ip==0x4321 && r.return_cs==0x1234 && r.changed_mask==0x3c0);
    CHECK(!memcmp(saved,ram,sizeof ram));
    s=initial(); s.cpu.ip=0x32b; s.cpu.bp=0xffff;
    CHECK(unchanged_join(&s,PR_U6F_02AE_JOIN_BLOCKED_MEMORY)==0);
    s.cpu.bp=0xfffc;
    CHECK(unchanged_join(&s,PR_U6F_02AE_JOIN_BLOCKED_MEMORY)==0);
    s.cpu.bp=0xfffe; put(s.cpu.ss,0xfffe,0x8888);
    put(s.cpu.ss,0,0x1c3); put(s.cpu.ss,2,0x2a8e);
    CHECK(pr_u6f_02ae_return_join_fixture(&s,&r)==PR_U6F_02AE_JOINED_01C3);
    CHECK(s.cpu.sp==4 && s.cpu.bp==0x8888 && r.frame_offset==0);
    s=initial(); s.cpu.ip=0x1c3;
    CHECK(unchanged_join(&s,PR_U6F_02AE_JOIN_BLOCKED_PC)==0);
    s.cpu.ip=0x32b; s.ram_size=at(s.cpu.ss,s.cpu.bp)+1;
    CHECK(unchanged_join(&s,PR_U6F_02AE_JOIN_BLOCKED_MEMORY)==0);
    CHECK(pr_u6f_02ae_return_join_fixture(&s,NULL)==PR_U6F_02AE_JOIN_ARGUMENT);
    CHECK(pr_u6f_02ae_return_join_fixture(NULL,&r)==PR_U6F_02AE_JOIN_ARGUMENT);
    return 0;
}
static int pending_services(int ui)
{
    pr_slot_a_directory_state s=initial();
    pr_slot_a_directory_write journal[1]; event e={3,1,0,0}; unsigned budget;
    pr_u6f_01b8_entry_status status=PR_U6F_01B8_ENTRY_STEPPED;
    for(budget=0;budget<500;++budget){
        status=pr_u6f_01b8_entry_step(&s,ui?dos:NULL,&e,journal,1);
        if(status!=PR_U6F_01B8_ENTRY_STEPPED) break;
    }
    CHECK(budget<500); /* Test observation budget only. */
    CHECK(status==(ui?PR_U6F_01B8_ENTRY_BLOCKED_UI:PR_U6F_01B8_ENTRY_BLOCKED_DOS));
    CHECK(unchanged_join(&s,PR_U6F_02AE_JOIN_BLOCKED_PC)==0);
    return 0;
}
int main(void)
{
    CHECK(from_entry(0,0)==0); CHECK(from_entry(5,1)==0);
    CHECK(explicit_boundaries()==0); CHECK(pending_services(0)==0);
    CHECK(pending_services(1)==0);
    puts("actual source-composed02AE return join fixture PASS"); return 0;
}
