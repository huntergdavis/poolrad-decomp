#include "poolrad/recovered/overlay_u33_party_attach_12e1_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM (1u << 20)
#define DS 0x1000u
#define SS 0x2000u
#define CS 0x8000u
#define RES 0x9000u
#define SYS 0xA000u
#define HEAP 0x6000u
#define WINDOW 0x5A00u
static uint8_t *ram, *saved_ram;
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr, "%d: %s\n", __LINE__, #x); exit(1); } } while (0)
static size_t at(uint16_t seg, uint16_t off)
{ return (((size_t)seg << 4) + off) & (RAM - 1u); }
static void w16(uint16_t seg, uint16_t off, uint16_t value)
{ size_t p=at(seg,off);ram[p]=(uint8_t)value;ram[(p+1u)&(RAM-1u)]=(uint8_t)(value>>8); }
static uint16_t r16(uint16_t seg, uint16_t off)
{ size_t p=at(seg,off);return (uint16_t)(ram[p]|((uint16_t)ram[(p+1u)&(RAM-1u)]<<8)); }
static uint16_t member(unsigned index) { return (uint16_t)(0x3000u+index*0x20u); }
static void bind(uint16_t ip,uint16_t target,uint16_t segment)
{ ram[at(CS,ip)]=0x9Au;w16(CS,(uint16_t)(ip+1u),target);w16(CS,(uint16_t)(ip+3u),segment); }
static pr_u33_12e1_cpu setup(unsigned count)
{
    pr_u33_12e1_cpu c={0};unsigned i;
    memset(ram,0,RAM);
    c.ax=0xABCD;c.bx=0x1234;c.cx=0x3456;c.dx=0x5678;c.si=0x789A;c.di=0x9ABC;
    c.bp=0x7777;c.sp=0x8000;c.cs=CS;c.ip=0x12E1;c.ds=DS;c.ss=SS;c.es=0x1111;c.flags=0x0402;
    w16(SS,0x8000,0x9000);w16(SS,0x8002,0xC000);
    bind(0x1309,0x0BB8,RES);bind(0x1315,0x023F,SYS);bind(0x1338,0x16A8,SYS);
    w16(DS,0x5D96,count?0x100u:0u);w16(DS,0x5D98,count?member(0):0u);
    w16(DS,0x49D6,0x100);w16(DS,0x49D8,WINDOW);
    w16(WINDOW,0x77C,0);w16(WINDOW,0x68C,3);
    ram[at(DS,0x6AAF)]=20;ram[at(DS,0x2FC)]=253;
    w16(DS,0x4404,0);w16(DS,0x4406,HEAP);
    w16(DS,0x4408,0);w16(DS,0x440A,0x7000);
    w16(DS,0x440C,0);w16(DS,0x440E,HEAP);
    w16(DS,0x4414,0x00A9);w16(DS,0x4416,SYS);
    memset(ram+at(HEAP,0),0xA5,0x10000);
    for(i=0;i<count;++i) {
        uint16_t seg=member(i);
        ram[at(seg,0x110)]=18;ram[at(seg,0x116)]=50;ram[at(seg,0x113)]=12;
        if(i+1u<count){w16(seg,0x204,0x100);w16(seg,0x206,member(i+1u));}
    }
    return c;
}
static int run(pr_u33_12e1_cpu *c,size_t *budget,int *diag)
{ return pr_u33_party_attach_12e1_cpu(ram,RAM,c,RES,SYS,budget,32u,diag); }
static void returned(const pr_u33_12e1_cpu *c)
{ CHECK(c->cs==0xC000 && c->ip==0x9000 && c->sp==0x8004 && c->bp==0x7777); }
static size_t attachment(unsigned index)
{ return at(r16(member(index),0x20A),r16(member(index),0x208)); }
static void semantics(void)
{
    pr_u33_12e1_cpu c;size_t budget,p;int diag;unsigned i,j;
    c=setup(0);budget=100000;diag=99;
    CHECK(run(&c,&budget,&diag)==PR_U33_12E1_RETURNED);returned(&c);
    CHECK(diag==-1 && r16(DS,0x4406)==HEAP && r16(DS,0x4404)==0);
    CHECK((c.flags&0x0400u)!=0);
    c=setup(3);w16(WINDOW,0x77C,1);
    ram[at(member(1),0x20E)]=1;ram[at(member(1),0x184)]=103;
    ram[at(member(2),0x184)]=103;
    budget=100000;CHECK(run(&c,&budget,&diag)==PR_U33_12E1_RETURNED);returned(&c);
    CHECK(diag==PR_SYSTEM_GETMEM_RETURNED && (c.flags&0x0400u)==0);
    for(i=0;i<3;++i) {
        p=attachment(i);CHECK(p==at(HEAP,0)+24u*i);
        for(j=0;j<22;++j) {
            uint8_t expected=j==9u?(i==1u?1u:253u):(j==19u&&i>0u?1u:0u);
            CHECK(ram[p+j]==expected);
        }
        CHECK(ram[p+22u]==0xA5 && ram[p+23u]==0xA5);
    }
    CHECK(ram[at(member(0),0x184)]==0);
    CHECK(ram[at(member(1),0x184)]==103);
    CHECK(ram[at(member(2),0x184)]==131);
    CHECK(r16(DS,0x4406)==HEAP+4u && r16(DS,0x4404)==8u);
    for(i=0;i<4;++i) {
        const uint8_t value[4]={0,128,102,103};
        c=setup(1);ram[at(member(0),0x184)]=value[i];budget=100000;
        CHECK(run(&c,&budget,&diag)==PR_U33_12E1_RETURNED);
        CHECK(ram[at(member(0),0x184)]==(i==2u?102u:131u));
    }
    c=setup(257);budget=1000000;
    CHECK(run(&c,&budget,&diag)==PR_U33_12E1_RETURNED);returned(&c);
    CHECK(ram[attachment(254)+19u]==1 && ram[attachment(255)+19u]==0 && ram[attachment(256)+19u]==1);
    CHECK(ram[at(member(254),0x184)]==131 && ram[at(member(255),0x184)]==0 && ram[at(member(256),0x184)]==131);
}
static void allocator_paths(void)
{
    pr_u33_12e1_cpu c;size_t budget;int diag;unsigned split;
    for(split=0;split<2;++split) {
        c=setup(1);w16(DS,0x4406,0x6800);
        w16(HEAP,0,0);w16(HEAP,2,0x6800);
        w16(HEAP,4,8);w16(HEAP,6,(uint16_t)(split?3u:1u));
        budget=100000;CHECK(run(&c,&budget,&diag)==PR_U33_12E1_RETURNED);
        CHECK(attachment(0)==at(HEAP,0));
        CHECK(r16(DS,0x4406)==0x6800 && r16(DS,0x4404)==0);
        CHECK(r16(DS,0x440E)==(split?HEAP+1u:0x6800u));
        CHECK(r16(DS,0x440C)==(split?8u:0u));
    }
    c=setup(2);w16(DS,0x4408,8);w16(DS,0x440A,HEAP+1u);budget=100000;
    CHECK(run(&c,&budget,&diag)==PR_U33_12E1_CALLEE);
    CHECK(diag==PR_SYSTEM_GETMEM_RUNERROR && c.cs==SYS && c.ip==0x00E2 && c.ax==203);
    CHECK(attachment(0)==at(HEAP,0) && ram[attachment(0)+19u]==1);
    CHECK(r16(member(1),0x208)==0 && r16(member(1),0x20A)==0);
    c=setup(1);w16(DS,0x4414,0x1234);budget=100000;
    CHECK(run(&c,&budget,&diag)==PR_U33_12E1_CALLEE);
    CHECK(diag==PR_SYSTEM_GETMEM_HEAP_ERROR && c.cs==SYS && c.ip==0x03B7);
    c=setup(1);w16(DS,0x4406,0x6800);w16(HEAP,0,0);w16(HEAP,2,HEAP);
    w16(HEAP,4,0);w16(HEAP,6,0);budget=100000;
    CHECK(pr_u33_party_attach_12e1_cpu(ram,RAM,&c,RES,SYS,&budget,2u,&diag)==PR_U33_12E1_CALLEE);
    CHECK(diag==PR_SYSTEM_GETMEM_LIMIT && c.cs==SYS && c.ip==0x0319);
}
static void guards_and_limits(void)
{
    pr_u33_12e1_cpu c,before;size_t budget;int diag;unsigned i;
    const uint16_t sites[3]={0x1309,0x1315,0x1338};
    for(i=0;i<3;++i) {
        c=setup(1);ram[at(CS,sites[i])]=0;before=c;memcpy(saved_ram,ram,RAM);budget=100;diag=42;
        CHECK(run(&c,&budget,&diag)==PR_U33_12E1_ARGUMENT);
        CHECK(budget==100 && diag==42 && !memcmp(&c,&before,sizeof(c)) && !memcmp(ram,saved_ram,RAM));
    }
    c=setup(1);before=c;memcpy(saved_ram,ram,RAM);budget=0;diag=42;
    CHECK(run(&c,&budget,&diag)==PR_U33_12E1_LIMIT);
    CHECK(diag==-1 && !memcmp(&c,&before,sizeof(c)) && !memcmp(ram,saved_ram,RAM));
    c=setup(1);w16(member(0),0x204,0x100);w16(member(0),0x206,member(0));budget=1000;
    CHECK(run(&c,&budget,&diag)==PR_U33_12E1_LIMIT && budget==0);
    c=setup(0);budget=100;CHECK(pr_u33_party_attach_12e1_cpu(NULL,RAM,&c,RES,SYS,&budget,2u,&diag)==PR_U33_12E1_ARGUMENT);
    CHECK(pr_u33_party_attach_12e1_cpu(ram,RAM-1u,&c,RES,SYS,&budget,2u,&diag)==PR_U33_12E1_ARGUMENT);
    CHECK(pr_u33_party_attach_12e1_cpu(ram,RAM,NULL,RES,SYS,&budget,2u,&diag)==PR_U33_12E1_ARGUMENT);
    CHECK(pr_u33_party_attach_12e1_cpu(ram,RAM,&c,RES,SYS,NULL,2u,&diag)==PR_U33_12E1_ARGUMENT);
    c.sp=0xFFFE;CHECK(run(&c,&budget,&diag)==PR_U33_12E1_ARGUMENT);
}
int main(void)
{
    ram=malloc(RAM);saved_ram=malloc(RAM);CHECK(ram!=NULL && saved_ram!=NULL);
    semantics();allocator_paths();guards_and_limits();
    free(saved_ram);free(ram);printf("U33 +12E1: %u synthetic checks passed\n",checks);return 0;
}
