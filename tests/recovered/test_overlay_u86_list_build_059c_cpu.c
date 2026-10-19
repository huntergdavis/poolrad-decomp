/* Synthetic sidecars, object slots, strings and heap; no game bytes. */
#include "poolrad/recovered/overlay_u86_list_build_059c_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U86_059C_RAM_SIZE
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"U86 +059C line %d: %s\n",__LINE__,#x);exit(1); } ++checks; } while(0)
enum { CS=0x8000,DS=0x1800,SS=0x2800,SP=0x8000,SYSTEM=0x0AF8,RES=0xBA,OBJ=0x5200,HEAP=0x3400 };
static uint8_t *ram;
static unsigned checks;
static size_t at(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(RAM-1u); }
static void w8(unsigned s,unsigned o,unsigned v) { ram[at(s,o)]=(uint8_t)v; }
static void w16(unsigned s,unsigned o,unsigned v) { size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1u)]=(uint8_t)(v>>8); }
static unsigned r16(unsigned s,unsigned o) { size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1u)]<<8); }
static pr_u86_059c_cpu world(unsigned mode) {
    pr_u86_059c_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x59C,DS,0x8888,SS,0x0603};
    const unsigned sites[]={0x2AE,0x31F,0x38B,0x39C,0x3DE,0x3F3,0x403,0x41C,0x42A};
    const unsigned targets[]={0x23F,0x23F,0xB25,0x23F,0xB25,0xB25,0xB0B,0xB98,0xB25};
    memset(ram,0,RAM);
    w8(CS,0x572,0xE8);w16(CS,0x573,0x2CC8);
    w8(CS,0x57E,0xE8);w16(CS,0x57F,0xFECD);
    w8(CS,0x464,0x9A);w16(CS,0x465,0x2199);w16(CS,0x467,RES);
    w8(CS,0x4EE,0xE8);w16(CS,0x4EF,0xFD9C);
    for(unsigned j=0;j<9;++j) { w8(CS,sites[j],0x9A);w16(CS,sites[j]+1,targets[j]);w16(CS,sites[j]+3,SYSTEM); }
    w8(CS,0x287,2);w8(CS,0x288,'A');w8(CS,0x289,' ');
    w8(CS,0x28A,2);w8(CS,0x28B,'B');w8(CS,0x28C,' ');
    w16(DS,0x5D92,0x300);w16(DS,0x5D94,OBJ);w16(DS,0x6B80,0x100);w16(DS,0x6B82,OBJ);
    w16(OBJ,0x3C8,0x100);w16(OBJ,0x3CA,OBJ);
    w8(DS,0x55A2,12);w8(DS,0x4843,255);
    for(unsigned j=0;j<196;++j)w8(DS,0x477F+j,0xA5);
    w8(OBJ,0x12E,2);w8(OBJ,0x13C,1);w8(OBJ,0x13D,0x80);w8(OBJ,0x13E,0x81);
    w16(DS,0x4406,HEAP);w16(DS,0x440A,HEAP+0x100);w16(DS,0x440E,HEAP);
    w16(DS,0x4414,0xA9);w16(DS,0x4416,SYSTEM);
    {const unsigned sites[]={0x60E,0x64A,0x67A,0x68E,0x69C,0x6A9,0x70A};
     const unsigned target[]={0x28D,0x28D,0x28D,0x44E,0x51C,0x51C,0x28D};
     for(unsigned j=0;j<7;++j) {w8(CS,sites[j],0xE8);w16(CS,sites[j]+1,target[j]-sites[j]-3);}}
    for(unsigned item=0;item<128;++item) {w8(DS,0x28E9+41*item,1);w8(DS,0x28EA+41*item,item);}
    for(unsigned item=0;item<256;++item) {w8(DS,0x31FB+16*item,0);w8(DS,0x31FA+16*item,0);}
    {const unsigned carried[]={1,0x82,0,127,128,255};
     for(unsigned j=0;j<6;++j)w8(OBJ,0x317+j,carried[j]);}
    for(unsigned item=1;item<=56;++item)w8(OBJ,0x332+item,item==1 || item==2 || item==55);
    w8(OBJ,0x3B1,1);
    w16(DS,0x4766,0x1234);w16(DS,0x4768,0x5678);
    for(unsigned j=0;j<21;++j)w8(DS,0x476A+j,0xAA);
    w16(SS,SP,0x7000);w16(SS,SP+2,0x9001);w16(SS,SP+4,mode);return c;
}
static unsigned nodes(void) {
    unsigned off=r16(DS,0x4766),seg=r16(DS,0x4768),n=0;
    while(seg || off) {
        CHECK(++n<120);
        CHECK(ram[at(seg,off+0x29)]==0);
        unsigned next=r16(seg,off+0x2A);seg=r16(seg,off+0x2C);off=next;
    }
    return n;
}
int main(void) {
    pr_u86_059c_limits limits={400,400,{400,400,{400,400,64,64}}},tight;
    pr_u86_059c_diagnostic d;pr_u86_059c_cpu c,before;
    uint8_t *saved;
    ram=malloc(RAM);saved=malloc(RAM);CHECK(ram && saved);
    for(unsigned mode=0;mode<256;++mode) {
        unsigned want=mode==0?2:mode==1?3:mode==2 || mode==3?3:mode==4?53:mode==5?3:mode==6?1:0;
        c=world(mode);
        CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,&limits,&d)==PR_U86_059C_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+6 && c.bp==0x789A);
        CHECK((c.ax&255)==(want!=0) && nodes()==want);
        CHECK((r16(DS,0x4766)!=0 || r16(DS,0x4768)!=0)==(want!=0));
        if(mode==0)CHECK(ram[at(DS,0x476A)]==1 && ram[at(DS,0x476B)]==127);
        if(mode==1)CHECK(ram[at(DS,0x476A)]==1 && ram[at(DS,0x476B)]==2 && ram[at(DS,0x476C)]==55);
        if(mode==5)CHECK(ram[at(DS,0x476A)]==2 && ram[at(DS,0x476B)]==0 && ram[at(DS,0x476C)]==127);
        if(mode==6)CHECK(ram[at(DS,0x476A)]==1 && ram[at(DS,0x4843)]==1);
        if(mode>6) {
            CHECK(d.callee==-1 && (c.flags&0x400)!=0);
            for(unsigned j=0;j<21;++j)CHECK(ram[at(DS,0x476A+j)]==0);
        }
        if(mode<7)CHECK(d.callee==(mode==2?1:mode==3 || mode==6?2:0) && d.status==0);
    }
    for(unsigned mode=0;mode<7;++mode) {
        c=world(mode);w16(DS,0x440A,HEAP);
        CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,&limits,&d)==PR_U86_059C_CALLEE);
        CHECK(c.cs==SYSTEM && c.ip==0xE2 && c.ax==203);
        if(d.callee==0)CHECK(d.status==3 && d.append.callee==0 && d.append.status==2);
        else if(d.callee==1)CHECK(d.status==3 && d.sidecar.callee==1 && d.sidecar.status==3 && d.sidecar.nested_status==2);
        else CHECK(d.status==3 && d.gate.callee==1 && d.gate.status==3 && d.gate.sidecar.nested_status==2);
    }
    {const unsigned sites[]={0x60E,0x64A,0x67A,0x68E,0x69C,0x6A9,0x70A},modes[]={0,5,1,2,3,6,4};
     for(unsigned j=0;j<7;++j) {
        c=world(modes[j]);w16(CS,sites[j]+1,0);
        CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,&limits,&d)==PR_U86_059C_CODE);
        CHECK(c.ip==sites[j] && c.sp==SP-8 && r16(SS,c.sp)==CS && d.callee==-1);
     }}
    c=world(0);tight=limits;tight.max_history_visits=1;
    CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,&tight,&d)==PR_U86_059C_LOOP_LIMIT);
    CHECK(c.ip==0x5B7 && ram[at(DS,0x476A)]==0 && ram[at(DS,0x476B)]==0xAA && ram[at(SS,SP-4)]==1);
    c=world(0);tight=limits;tight.max_item_visits=1;
    CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,&tight,&d)==PR_U86_059C_LOOP_LIMIT);
    CHECK(c.ip==0x5D9 && nodes()==1 && d.callee==0 && d.status==0);
    c=world(3);tight=limits;tight.gate.max_clear_visits=1;
    CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,&tight,&d)==PR_U86_059C_CALLEE);
    CHECK(c.ip==0x52B && d.callee==2 && d.status==5 && d.gate.callee==-1);
    c=world(0);tight=limits;tight.gate.sidecar.max_list_visits=1;
    /* The two-item list has only one visited tail; allow this boundary. */
    CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,&tight,&d)==PR_U86_059C_OK);
    c=world(7);c.ss=DS;c.sp=0x4770;w16(DS,c.sp,0x7000);w16(DS,c.sp+2,0x9001);w16(DS,c.sp+4,7);
    tight=limits;tight.max_history_visits=30;
    CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,&tight,&d)==PR_U86_059C_LOOP_LIMIT && c.ip==0x5B7);
    c=world(0);
    w16(DS,0x4404,6);w16(DS,0x4406,SS+0x7FF);w16(DS,0x4408,0);w16(DS,0x440A,SS+0x900);w16(DS,0x440C,6);w16(DS,0x440E,SS+0x7FF);
    CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,&limits,&d)==PR_U86_059C_TRANSFER);
    CHECK(c.sp==SP-4 && d.callee==0 && d.status==0 && (c.cs!=CS || c.ip!=0x611));
    c=world(7);before=c;memcpy(saved,ram,RAM);
    CHECK(pr_u86_list_build_059c_cpu(NULL,RAM,&c,&limits,&d)==PR_U86_059C_ARGUMENT);
    CHECK(pr_u86_list_build_059c_cpu(ram,RAM,NULL,&limits,&d)==PR_U86_059C_ARGUMENT);
    CHECK(pr_u86_list_build_059c_cpu(ram,RAM-1,&c,&limits,&d)==PR_U86_059C_ARGUMENT);
    CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,NULL,&d)==PR_U86_059C_ARGUMENT);
    for(unsigned j=0;j<8;++j) {
        tight=limits;c=before;
        if(j==0)c.ip=0;
        if(j==1)c.sp=0x169;
        if(j==2)c.sp=0xFFFB;
        if(j==3)tight.max_history_visits=0;
        if(j==4)tight.max_item_visits=0;
        if(j==5)tight.gate.max_clear_visits=0;
        if(j==6)tight.gate.max_record_visits=0;
        if(j==7)tight.gate.sidecar.max_list_visits=0;
        pr_u86_059c_cpu initial=c;
        CHECK(pr_u86_list_build_059c_cpu(ram,RAM,&c,&tight,&d)==PR_U86_059C_ARGUMENT);
        CHECK(memcmp(&c,&initial,sizeof c)==0 && memcmp(ram,saved,RAM)==0 && d.callee==-1 && d.append.callee==-1 && d.gate.callee==-1);
    }
    free(saved);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
