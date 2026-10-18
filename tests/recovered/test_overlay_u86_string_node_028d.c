/* Synthetic strings, list nodes and call bindings; no original game bytes. */
#include "poolrad/recovered/overlay_u86_string_node_028d.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U86_028D_RAM_SIZE
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"U86 +028D line %d: %s\n",__LINE__,#x);exit(1); } ++checks; } while(0)
enum { CS=0x8000,SYS=0x7000,DS=0x1800,SS=0x2800,SP=0x8000,HEAP=0x3400,NODES=0x5000 };
static uint8_t *ram;
static unsigned checks;
static size_t at(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(RAM-1u); }
static void w8(unsigned s,unsigned o,unsigned v) { ram[at(s,o)]=(uint8_t)v; }
static void w16(unsigned s,unsigned o,unsigned v) { size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1u)&(RAM-1u)]=(uint8_t)(v>>8); }
static unsigned r16(unsigned s,unsigned o) { size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1u)&(RAM-1u)]<<8); }
static void binding(unsigned s,unsigned target) { w8(CS,s,0x9A);w16(CS,s+1,target);w16(CS,s+3,SYS); }
static pr_u86_028d_cpu world(unsigned arg,unsigned length,unsigned group,unsigned old_group,unsigned name_length) {
    pr_u86_028d_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x28D,DS,0x8888,SS,0x0603};
    unsigned j,item=arg&127u;
    memset(ram,0xA5,RAM);
    binding(0x2AE,0x23F);binding(0x31F,0x23F);binding(0x38B,0xB25);binding(0x39C,0x23F);
    binding(0x3DE,0xB25);binding(0x3F3,0xB25);binding(0x403,0xB0B);binding(0x41C,0xB98);binding(0x42A,0xB25);
    w8(CS,0x287,2);w8(CS,0x288,'H');w8(CS,0x289,':');w8(CS,0x28A,2);w8(CS,0x28B,'L');w8(CS,0x28C,':');
    w16(DS,0x4404,0);w16(DS,0x4406,HEAP);w16(DS,0x4408,0);w16(DS,0x440A,HEAP+0x100);
    w16(DS,0x440C,0);w16(DS,0x440E,HEAP);w16(DS,0x4414,0xA9);w16(DS,0x4416,SYS);
    w16(DS,0x4766,0);w16(DS,0x4768,length?NODES:0);
    for(j=0;j<length;++j) {
        unsigned off=48*j;w8(NODES,off+0x29,0);w16(NODES,off+0x2A,j+1<length?off+48:0);
        w16(NODES,off+0x2C,j+1<length?NODES:0);w8(DS,0x476A+j,5);
    }
    w8(DS,0x31FB+16*5,old_group);w8(DS,0x31FB+16*item,group);
    w8(DS,0xA79+41*group,45);
    for(j=1;j<=45;++j)w8(DS,0xA79+41*group+j,'G');
    w8(DS,0x28E9+41*item,name_length);
    for(j=1;j<=name_length;++j)w8(DS,0x28E9+41*item+j,j^0x5A);
    w16(SS,SP,0x7000);w16(SS,SP+2,0x9001);w16(SS,SP+4,arg);return c;
}
int main(void) {
    static const unsigned lengths[]={0,1,38,39,40,41,254,255};
    pr_u86_028d_cpu c,before;pr_u86_028d_diagnostic d;
    unsigned k,old,heading,high,j,n,node;
    ram=malloc(RAM);CHECK(ram!=NULL);
    for(k=0;k<sizeof lengths/sizeof lengths[0];++k) for(old=0;old<2;++old) for(heading=0;heading<2;++heading) for(high=0;high<2;++high) {
        c=world(3+128*high,old?3:0,heading?2:0,0,lengths[k]);
        CHECK(pr_u86_string_node_028d(ram,RAM,&c,400,64,&d)==PR_U86_028D_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==SP+6 && c.bp==0x789A && c.ds==DS);
        CHECK(r16(DS,0x4768)==(old?NODES:HEAP));
        if(old)CHECK(r16(NODES,96+0x2A)==0 && r16(NODES,96+0x2C)==HEAP);
        if(heading) {
            CHECK(ram[at(HEAP,0)]==40 && ram[at(HEAP,0x29)]==1);
            CHECK(r16(HEAP,0x2A)==0 && r16(HEAP,0x2C)==HEAP+3);
            for(j=1;j<=40;++j)CHECK(ram[at(HEAP,j)]=='G');
        }
        node=HEAP+3*heading;n=lengths[k]>38?38:lengths[k];
        CHECK(ram[at(node,0)]==n+2 && ram[at(node,1)]==(high?'H':'L') && ram[at(node,2)]==':');
        for(j=1;j<=n;++j)CHECK(ram[at(node,j+2)]==(uint8_t)(j^0x5A));
        CHECK(ram[at(node,0x29)]==0 && r16(node,0x2A)==0 && r16(node,0x2C)==0);
        CHECK(ram[at(DS,0x476A+(old?3:0))]==3 && r16(DS,0x4406)==HEAP+3*(heading+1));
        CHECK(d.callee==PR_U86_028D_STRSTORE && d.status==0 && (c.flags&0x400u)==0);
    }
    c=world(3,0,2,0,12);w16(DS,0x440A,HEAP+3);
    CHECK(pr_u86_string_node_028d(ram,RAM,&c,400,64,&d)==PR_U86_028D_CALLEE);
    CHECK(d.callee==PR_U86_028D_GETMEM && d.status==2 && c.cs==SYS && c.ip==0xE2 && c.ax==203);
    CHECK(r16(DS,0x4768)==HEAP && ram[at(HEAP,0x29)]==1 && r16(HEAP,0x2C)==0);
    c=world(3,2,2,0,12);w16(NODES,48+0x2A,0);w16(NODES,48+0x2C,NODES);
    CHECK(pr_u86_string_node_028d(ram,RAM,&c,5,64,&d)==PR_U86_028D_LIST_LIMIT);
    CHECK(c.cs==CS && c.ip==0x2D4 && d.callee==-1);
    c=world(3,0,2,0,12);w16(DS,0x4414,0x1234);
    CHECK(pr_u86_string_node_028d(ram,RAM,&c,400,64,&d)==PR_U86_028D_CALLEE && d.status==3);
    c=world(3,0,0,0,12);w8(CS,0x403,0x90);
    CHECK(pr_u86_string_node_028d(ram,RAM,&c,400,64,&d)==PR_U86_028D_CODE && c.ip==0x403);
    c=world(3,0,0,0,12);before=c;
    CHECK(pr_u86_string_node_028d(NULL,RAM,&c,400,64,&d)==PR_U86_028D_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && d.callee==-1 && d.status==-1);
    CHECK(pr_u86_string_node_028d(ram,RAM,NULL,400,64,NULL)==PR_U86_028D_ARGUMENT);
    CHECK(pr_u86_string_node_028d(ram,RAM-1,&c,400,64,NULL)==PR_U86_028D_ARGUMENT);
    CHECK(pr_u86_string_node_028d(ram,RAM,&c,0,64,NULL)==PR_U86_028D_ARGUMENT);
    c.sp=0x13F;CHECK(pr_u86_string_node_028d(ram,RAM,&c,400,64,NULL)==PR_U86_028D_ARGUMENT);
    c.sp=0xFFFB;CHECK(pr_u86_string_node_028d(ram,RAM,&c,400,64,NULL)==PR_U86_028D_ARGUMENT);
    c=before;c.ip=0;CHECK(pr_u86_string_node_028d(ram,RAM,&c,400,64,NULL)==PR_U86_028D_ARGUMENT);
    c=world(3,0,0,0,12);CHECK(pr_u86_string_node_028d(ram,RAM,&c,400,64,NULL)==PR_U86_028D_OK);
    free(ram);printf("PASS: U86 +028D %u checks\n",checks);return 0;
}
