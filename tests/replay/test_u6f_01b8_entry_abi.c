#include "poolrad/replay/u6f_01b8_entry_abi.h"
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
static pr_u6f_01b8_entry_status step(pr_slot_a_directory_state *s,event *e)
{
    pr_slot_a_directory_write journal[1];
    return pr_u6f_01b8_entry_step(s,e?dos:NULL,e,journal,1);
}
static int unchanged(pr_slot_a_directory_state *s,event *e,pr_u6f_01b8_entry_status expected)
{
    pr_slot_a_directory_cpu cpu=s->cpu; memcpy(saved,ram,sizeof ram);
    CHECK(step(s,e)==expected); CHECK(!memcmp(&cpu,&s->cpu,sizeof cpu));
    CHECK(!memcmp(saved,ram,sizeof ram)); return 0;
}
static int entry_frame(void)
{
    pr_slot_a_directory_state s=initial(); pr_slot_a_directory_cpu before=s.cpu;
    CHECK(step(&s,NULL)==PR_U6F_01B8_ENTRY_STEPPED);
    CHECK(s.cpu.sp==0x5ffe && get(s.cpu.ss,0x5ffe)==before.bp);
    CHECK(step(&s,NULL)==PR_U6F_01B8_ENTRY_STEPPED);
    CHECK(s.cpu.bp==0x5ffe);
    CHECK(step(&s,NULL)==PR_U6F_01B8_ENTRY_STEPPED);
    CHECK(s.cpu.sp==0x5cf8);
    CHECK(step(&s,NULL)==PR_U6F_01B8_ENTRY_STEPPED);
    CHECK(s.cpu.sp==0x5cf6 && get(s.cpu.ss,0x5cf6)==0x2a8e);
    CHECK(step(&s,NULL)==PR_U6F_01B8_ENTRY_STEPPED);
    CHECK(s.cpu.cs==0x2a8e && s.cpu.ip==0x02ae && s.cpu.sp==0x5cf4);
    CHECK(get(s.cpu.ss,0x5cf4)==0x01c3);
    CHECK(get(s.cpu.ss,0x6000)==0xabcd && get(s.cpu.ss,0x6002)==0x2345);
    CHECK(get(s.cpu.ss,0x6004)==0x1234 && get(s.cpu.ss,0x6006)==0x4567);
    before.bp=s.cpu.bp; before.sp=s.cpu.sp; before.ip=s.cpu.ip;
    before.flags=s.cpu.flags;
    CHECK(!memcmp(&before,&s.cpu,sizeof before));
    return 0;
}
static int subtraction_flags(void)
{
    const uint16_t input[]={0x0306,0x0305,0x8000};
    const uint16_t expected[]={0x0044,0x0095,0x0814};
    unsigned i;
    for(i=0;i<3;++i){ pr_slot_a_directory_state s=initial();
        s.cpu.ip=0x1bb; s.cpu.sp=input[i]; s.cpu.flags=0x7602;
        CHECK(step(&s,NULL)==PR_U6F_01B8_ENTRY_STEPPED);
        CHECK(s.cpu.sp==(uint16_t)(input[i]-0x306));
        CHECK((s.cpu.flags&0x08d5)==expected[i]);
        CHECK((s.cpu.flags&~0x08d5u)==(0x7602u&~0x08d5u));
    } return 0;
}
static int callee_interval(uint16_t result,int carry,int ui)
{
    pr_slot_a_directory_state s=initial(); event e={result,carry,0,0};
    unsigned budget; pr_u6f_01b8_entry_status r=PR_U6F_01B8_ENTRY_STEPPED;
    for(budget=0;budget<500;++budget){ r=step(&s,&e); if(r!=PR_U6F_01B8_ENTRY_STEPPED)break; }
    CHECK(budget<500); /* External observation budget, never a source retry cap. */
    CHECK(e.calls==1 && ram[0x50]==0x9b);
    CHECK(s.cpu.fs==0x3456 && s.cpu.gs==0x789a);
    if(ui){ CHECK(r==PR_U6F_01B8_ENTRY_BLOCKED_UI && s.cpu.ip==0x02e9);
        CHECK(unchanged(&s,&e,PR_U6F_01B8_ENTRY_BLOCKED_UI)==0);
    }else{ CHECK(r==PR_U6F_01B8_ENTRY_BOUNDARY);
        CHECK(s.cpu.cs==0x2a8e && s.cpu.ip==0x1c3);
        CHECK(s.cpu.sp==0x5cf8 && s.cpu.bp==0x5ffe);
        CHECK(get(s.cpu.ss,0x5ffe)==0x7000);
        CHECK(get(s.cpu.ss,0x6000)==0xabcd && get(s.cpu.ss,0x6006)==0x4567);
        CHECK(unchanged(&s,&e,PR_U6F_01B8_ENTRY_BOUNDARY)==0);
    } return 0;
}
static int blocked_and_transfer(void)
{
    pr_slot_a_directory_state s=initial(); event e={5,1,1,0}; unsigned i;
    for(i=0;i<400 && !(s.cpu.cs==0x131c&&s.cpu.ip==0x1821);++i)
        CHECK(step(&s,NULL)==PR_U6F_01B8_ENTRY_STEPPED);
    CHECK(i<400); CHECK(unchanged(&s,NULL,PR_U6F_01B8_ENTRY_BLOCKED_DOS)==0);
    CHECK(unchanged(&s,&e,PR_U6F_01B8_ENTRY_BAD_EVENT)==0);
    s=initial(); s.cpu.ip=0x1c0; s.cpu.sp=1;
    CHECK(unchanged(&s,NULL,PR_U6F_01B8_ENTRY_BLOCKED_MEMORY)==0);
    s.cpu.sp=0; CHECK(step(&s,NULL)==PR_U6F_01B8_ENTRY_STEPPED);
    CHECK(s.cpu.sp==0xfffe && get(s.cpu.ss,0xfffe)==0x1c3);
    s=initial(); s.cpu.cs=0xffff;
    CHECK(unchanged(&s,NULL,PR_U6F_01B8_ENTRY_BLOCKED_PC)==0);
    s=initial(); s.cpu.ip=0x032e; s.cpu.sp=0x5000;
    put(s.cpu.ss,0x5000,0x1234); put(s.cpu.ss,0x5002,0x5678);
    CHECK(step(&s,NULL)==PR_U6F_01B8_ENTRY_TRANSFER);
    CHECK(s.cpu.cs==0x5678 && s.cpu.ip==0x1234 && s.cpu.sp==0x5004);
    s=initial(); s.cpu.ip=0x0239;
    CHECK(unchanged(&s,NULL,PR_U6F_01B8_ENTRY_BLOCKED_PC)==0);
    CHECK(pr_u6f_01b8_entry_step(NULL,NULL,NULL,NULL,0)==PR_U6F_01B8_ENTRY_ARGUMENT);
    CHECK(pr_u6f_01b8_entry_step(&s,NULL,NULL,NULL,1)==PR_U6F_01B8_ENTRY_ARGUMENT);
    return 0;
}
int main(void)
{
    CHECK(entry_frame()==0); CHECK(subtraction_flags()==0);
    CHECK(callee_interval(0,0,0)==0); CHECK(callee_interval(5,1,0)==0);
    CHECK(callee_interval(3,1,1)==0); CHECK(blocked_and_transfer()==0);
    puts("bounded U6F01B8 entry/02AE return synthetic ABI checks PASS"); return 0;
}
