#include "poolrad/replay/u6f_01c3_text_record_prefix.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[1048576],saved[1048576];
static size_t at(uint16_t seg,uint16_t off){return ((size_t)seg<<4)+off;}
static uint16_t word(uint16_t seg,uint16_t off){size_t p=at(seg,off);return (uint16_t)(ram[p]|((uint16_t)ram[p+1]<<8));}
static void put(uint16_t seg,uint16_t off,uint16_t v){size_t p=at(seg,off);ram[p]=(uint8_t)v;ram[p+1]=(uint8_t)(v>>8);}
static pr_slot_a_directory_state initial(unsigned n,unsigned m)
{
    pr_slot_a_directory_state s={0};unsigned i;
    memset(ram,0x5a,sizeof ram);s.ram=ram;s.ram_size=sizeof ram;
    s.cpu=(pr_slot_a_directory_cpu){.ax=0xa55a,.bx=0x1234,.cx=0x4567,.dx=0x789a,
        .si=0x3333,.di=0x4444,.bp=0x8000,.sp=0x7cfa,.cs=0x2a8e,.ip=0x1c3,
        .ds=0x1000,.es=0x1100,.ss=0x2000,.fs=0x3456,.gs=0x789a,.flags=0x7606};
    ram[at(s.cpu.ds,0x537a)]=(uint8_t)n;ram[at(s.cpu.cs,0x197)]=(uint8_t)m;
    for(i=0;i<n;++i)ram[at(s.cpu.ds,(uint16_t)(0x537b+i))]='A';
    for(i=0;i<m;++i)ram[at(s.cpu.cs,(uint16_t)(0x198+i))]='B';
    return s;
}
static int path(unsigned n,unsigned m)
{
    pr_slot_a_directory_state s=initial(n,m);unsigned i,budget,seen=0;
    pr_u6f_01c3_status r=PR_U6F_01C3_STEPPED;
    for(budget=0;budget<128;++budget){
        if(s.cpu.cs==0x131c&&(s.cpu.ip==0xb0b||s.cpu.ip==0xb98)){
            CHECK(s.cpu.sp==0x7cea);
            CHECK(word(s.cpu.ss,s.cpu.sp)==(s.cpu.ip==0xb0b?0x1d9:0x1e3));
            CHECK(word(s.cpu.ss,(uint16_t)(s.cpu.sp+2))==0x2a8e);++seen;
        }
        r=pr_u6f_01c3_text_record_step(&s);if(r!=PR_U6F_01C3_STEPPED)break;
    }
    CHECK(budget<128&&r==PR_U6F_01C3_AT_04F7&&seen==2);
    CHECK(s.cpu.ip==0x1e3&&s.cpu.cs==0x2a8e&&s.cpu.sp==0x7cf2&&s.cpu.bp==0x8000);
    CHECK(word(s.cpu.ss,0x7cf2)==0x7dfa&&word(s.cpu.ss,0x7cf4)==s.cpu.ss);
    CHECK(word(s.cpu.ss,0x7cf6)==0x7efc&&word(s.cpu.ss,0x7cf8)==s.cpu.ss);
    CHECK(s.cpu.fs==0x3456&&s.cpu.gs==0x789a&&!(s.cpu.flags&0x400));
    CHECK(ram[at(s.cpu.ss,0x7dfa)]==(n+m>255?255:n+m));
    for(i=0;i<n;++i)CHECK(ram[at(s.cpu.ss,(uint16_t)(0x7dfb+i))]=='A');
    for(i=0;i<m&&n+i<255;++i)CHECK(ram[at(s.cpu.ss,(uint16_t)(0x7dfb+n+i))]=='B');
    CHECK(ram[at(s.cpu.ss,0x7efc)]==0x5a); /* No04F7 record init in disjoint case. */
    memcpy(saved,ram,sizeof ram);{pr_slot_a_directory_cpu before=s.cpu;
        CHECK(pr_u6f_01c3_text_record_step(&s)==PR_U6F_01C3_AT_04F7);
        CHECK(!memcmp(&before,&s.cpu,sizeof before)&&!memcmp(saved,ram,sizeof ram));}
    return 0;
}
static int unchanged(pr_slot_a_directory_state *s,pr_u6f_01c3_status expected)
{
    pr_slot_a_directory_cpu before=s->cpu;memcpy(saved,ram,sizeof ram);
    CHECK(pr_u6f_01c3_text_record_step(s)==expected);
    CHECK(!memcmp(&before,&s->cpu,sizeof before)&&!memcmp(saved,ram,sizeof ram));return 0;
}
static int reject_and_transfer(void)
{
    pr_slot_a_directory_state s=initial(3,2);
    s.cpu.ip=0x1d4;s.cpu.sp=2;CHECK(unchanged(&s,PR_U6F_01C3_BLOCKED_MEMORY)==0);
    s.cpu.ip=0x1c7;s.cpu.sp=1;CHECK(unchanged(&s,PR_U6F_01C3_BLOCKED_MEMORY)==0);
    s.cpu.cs=0x131c;s.cpu.ip=0xbbd;s.cpu.flags|=0x400;
    CHECK(unchanged(&s,PR_U6F_01C3_BLOCKED_PC)==0);
    s=initial(3,2);s.cpu.ip=0x1e8;CHECK(unchanged(&s,PR_U6F_01C3_BLOCKED_PC)==0);
    s.cpu.cs=0x131c;s.cpu.ip=0xb22;s.cpu.sp=0x5000;
    put(s.cpu.ss,0x5000,0x4321);put(s.cpu.ss,0x5002,0x5678);
    CHECK(pr_u6f_01c3_text_record_step(&s)==PR_U6F_01C3_TRANSFER);
    CHECK(s.cpu.cs==0x5678&&s.cpu.ip==0x4321&&s.cpu.sp==0x5008);
    s.cpu.cs=0x131c;s.cpu.ip=0xbc1;s.cpu.sp=0x5000;
    CHECK(pr_u6f_01c3_text_record_step(&s)==PR_U6F_01C3_TRANSFER);
    CHECK(s.cpu.cs==0x5678&&s.cpu.ip==0x4321&&s.cpu.sp==0x5008);
    s=initial(3,2);s.cpu.cs=0x131c;s.cpu.ip=0xb10;s.cpu.bx=0xfff7;
    CHECK(unchanged(&s,PR_U6F_01C3_BLOCKED_MEMORY)==0);
    s.cpu.ip=0xb9d;CHECK(unchanged(&s,PR_U6F_01C3_BLOCKED_MEMORY)==0);
    s=initial(3,2);s.ram_size=sizeof ram+1u;
    CHECK(unchanged(&s,PR_U6F_01C3_BLOCKED_MEMORY)==0);
    CHECK(pr_u6f_01c3_text_record_step(NULL)==PR_U6F_01C3_ARGUMENT);return 0;
}
int main(void)
{
    CHECK(path(0,0)==0);CHECK(path(3,2)==0);CHECK(path(250,31)==0);
    CHECK(path(255,1)==0);CHECK(reject_and_transfer()==0);
    puts("bounded01C3 prefix synthetic CPU16/frame/saturation checks PASS");return 0;
}
