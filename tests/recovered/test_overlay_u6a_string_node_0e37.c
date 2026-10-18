/* Synthetic state tests, with no original game bytes. */
#include "poolrad/recovered/overlay_u6a_string_node_0e37.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U6A_0E37_RAM_SIZE
#define CHECK(x) do { if(!(x)) { fprintf(stderr,"U6A +0E37 line %d: %s\n",__LINE__,#x);exit(1); } ++checks; } while(0)
enum { CS=0x6000,DS=0x1800,SS=0x2000,SYS=0x7000,SP=0x8000,HEAP=0x3200,OLD=0x3380,PARENT=0x9000 };
static uint8_t *ram;
static unsigned checks;
static size_t at(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(RAM-1u); }
static void w16(unsigned s,unsigned o,unsigned v) {
    size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1u)]=(uint8_t)(v>>8);
}
static uint16_t r16(unsigned s,unsigned o) {
    size_t p=at(s,o);return (uint16_t)(ram[p]|((uint16_t)ram[(p+1)&(RAM-1u)]<<8));
}
static pr_u6a_0e37_cpu world(unsigned length) {
    pr_u6a_0e37_cpu c={0x1234,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,
                       SP,CS,0x0E37,DS,0x8888,SS,0x0603};
    unsigned j;
    memset(ram,0xA5,RAM);
    w16(CS,0x0E4E,SYS);w16(CS,0x0E57,SYS);w16(CS,0x0EA8,SYS);
    w16(DS,0x4404,0);w16(DS,0x4406,HEAP);
    w16(DS,0x4408,0);w16(DS,0x440A,HEAP+0x100);
    w16(DS,0x440C,0);w16(DS,0x440E,HEAP);
    w16(DS,0x4414,0xA9);w16(DS,0x4416,SYS);
    w16(SS,PARENT-12,0x100);w16(SS,PARENT-10,OLD);
    w16(SS,SP,0x3456);w16(SS,SP+2,0x9000);w16(SS,SP+4,PARENT);
    w16(SS,SP+6,0x1000);w16(SS,SP+8,DS);w16(SS,SP+10,0xA55A);
    ram[at(DS,0x1000)]=(uint8_t)length;
    for(j=1;j<=255;++j)ram[at(DS,0x1000+j)]=(uint8_t)(j^0x5A);
    return c;
}
int main(void) {
    static const unsigned lengths[]={0,1,39,40,41,100,255};
    pr_u6a_0e37_cpu c,before;
    unsigned k,j,n;int callee;
    ram=malloc(RAM);CHECK(ram!=NULL);
    for(k=0;k<sizeof lengths/sizeof lengths[0];++k) {
        c=world(lengths[k]);n=lengths[k]>40?40:lengths[k];
        CHECK(pr_u6a_string_node_0e37(ram,RAM,&c,64,&callee)==PR_U6A_0E37_OK && callee==0);
        CHECK(c.cs==0x9000 && c.ip==0x3456 && c.sp==SP+12 && c.bp==0x789A);
        CHECK(c.ds==DS && (c.flags&0x600u)==0x200u);
        CHECK(r16(SS,PARENT-12)==0 && r16(SS,PARENT-10)==HEAP);
        CHECK(r16(OLD,0x12A)==0 && r16(OLD,0x12C)==HEAP);
        CHECK(r16(HEAP,0x2A)==0 && r16(HEAP,0x2C)==0);
        CHECK(ram[at(HEAP,0)]==n && ram[at(HEAP,0x29)]==0x5A);
        CHECK(ram[at(HEAP,0x28)]==(n==40?(40^0x5A):0xA5));
        for(j=1;j<=n;++j)CHECK(ram[at(HEAP,j)]==(uint8_t)(j^0x5A));
        CHECK(r16(DS,0x4404)==0 && r16(DS,0x4406)==HEAP+3);
    }
    /* Exact free block: advance its link without growing HeapPtr. */
    c=world(40);w16(DS,0x440C,0);w16(DS,0x440E,0x3010);
    w16(0x3010,0,0);w16(0x3010,2,HEAP);w16(0x3010,4,0);w16(0x3010,6,3);
    CHECK(pr_u6a_string_node_0e37(ram,RAM,&c,0,&callee)==PR_U6A_0E37_OK);
    CHECK(r16(SS,PARENT-10)==0x3010 && r16(DS,0x4406)==HEAP);
    /* Allocation failure occurs after the snapshot, before any node link. */
    c=world(255);w16(DS,0x440A,HEAP);
    CHECK(pr_u6a_string_node_0e37(ram,RAM,&c,64,&callee)==PR_U6A_0E37_GETMEM_STOP && callee==2);
    CHECK(c.cs==SYS && c.ip==0xE2 && c.ax==203);
    CHECK(ram[at(SS,SP-0x2B)]==40 && r16(SS,PARENT-10)==OLD);
    c=world(40);w16(DS,0x4414,0x1234);
    CHECK(pr_u6a_string_node_0e37(ram,RAM,&c,64,&callee)==PR_U6A_0E37_GETMEM_STOP && callee==3);
    CHECK(c.cs==SYS && c.ip==0x3B7 && r16(SS,PARENT-10)==OLD);
    c=world(40);w16(DS,0x440C,0);w16(DS,0x440E,0x3010);
    w16(0x3010,0,0);w16(0x3010,2,0x3010);w16(0x3010,4,0);w16(0x3010,6,1);
    CHECK(pr_u6a_string_node_0e37(ram,RAM,&c,2,&callee)==PR_U6A_0E37_GETMEM_STOP && callee==5);
    CHECK(c.cs==SYS && c.ip==0x319);
    c=world(40);before=c;
    CHECK(pr_u6a_string_node_0e37(NULL,RAM,&c,64,&callee)==PR_U6A_0E37_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && callee==-1);
    CHECK(pr_u6a_string_node_0e37(ram,RAM-1,&c,64,NULL)==PR_U6A_0E37_ARGUMENT);
    CHECK(pr_u6a_string_node_0e37(ram,RAM,NULL,64,NULL)==PR_U6A_0E37_ARGUMENT);
    c.sp=0x5F;CHECK(pr_u6a_string_node_0e37(ram,RAM,&c,64,NULL)==PR_U6A_0E37_ARGUMENT);
    c.sp=0xFFF5;CHECK(pr_u6a_string_node_0e37(ram,RAM,&c,64,NULL)==PR_U6A_0E37_ARGUMENT);
    c.sp=SP;c.ip=0;CHECK(pr_u6a_string_node_0e37(ram,RAM,&c,64,NULL)==PR_U6A_0E37_ARGUMENT);
    free(ram);printf("PASS: U6A +0E37 %u checks\n",checks);return 0;
}
