/* Synthetic frames, lists and binding operands; no original game body. */
#include "poolrad/recovered/overlay_u9e_list_release_0026_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define RAM PR_U9E_0026_RAM_SIZE
#define CHECK(x) do { if(!(x)){fprintf(stderr,"U9E +0026 line %d: %s\n",__LINE__,#x);exit(1);}++checks; } while(0)
enum { CS=0x8800,DS=0x1800,SS=0x2800,SP=0x8000,SYS=0xAF8,REC=0x5000 };
static uint8_t *ram;
static unsigned checks;
static size_t at(unsigned s,unsigned o) { return (((size_t)s<<4)+(o&65535u))&(RAM-1u); }
static void w8(unsigned s,unsigned o,unsigned v) { ram[at(s,o)]=(uint8_t)v; }
static void w16(unsigned s,unsigned o,unsigned v) { size_t p=at(s,o);ram[p]=(uint8_t)v;ram[(p+1)&(RAM-1u)]=(uint8_t)(v>>8); }
static unsigned r16(unsigned s,unsigned o) { size_t p=at(s,o);return ram[p]|((unsigned)ram[(p+1)&(RAM-1u)]<<8); }
static pr_u9e_0026_cpu world(unsigned key,unsigned active) {
    pr_u9e_0026_cpu c={0xA534,0x2345,0x3456,0x4567,0x5678,0x6789,0x789A,SP,CS,0x26,DS,0x8888,SS,0xED7};
    memset(ram,0xA5,RAM);
    w8(CS,0x135,0x9A);w16(CS,0x136,0x254);w16(CS,0x138,SYS);
    w8(CS,0xA1,0xE8);w16(CS,0xA2,0xFF5C);
    w16(DS,0x4400,0);w16(DS,0x4402,0x3000);w16(DS,0x4404,0);w16(DS,0x4406,0x3400);
    w16(DS,0x440C,0);w16(DS,0x440E,0x3400);w16(DS,0x4410,0);w16(DS,0x4412,0);
    w16(REC,0x17F,0);w16(REC,0x181,0x3100);
    for(unsigned j=0;j<3;++j){unsigned s=0x3100+j*0x20;w8(s,0,j==1?key:(key+1)&255);w8(s,4,active);w16(s,5,0);w16(s,7,j==2?0:s+0x20);}
    w16(DS,0x6828+4*key,0x1234+key);w16(DS,0x682A+4*key,0x6000+key);
    w16(SS,SP,0x7000);w16(SS,SP+2,0x9001);w16(SS,SP+4,0);w16(SS,SP+6,0x3120);
    w16(SS,SP+8,0xA500|key);w16(SS,SP+10,0x100);w16(SS,SP+12,REC);return c;
}
int main(void) {
    pr_u9e_0026_cpu c,before;pr_u9e_0026_limits l={64,64};pr_u9e_0026_diagnostic d;
    ram=malloc(RAM);CHECK(ram!=NULL);
    for(unsigned key=0;key<256;++key) {
        c=world(key,0);if(key&1)w16(SS,SP+6,0); /* nil supplied item: search */
        CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==PR_U9E_0026_OK);
        CHECK(c.sp==SP+14 && c.cs==0x9001 && c.ip==0x7000 && c.bp==0x789A);
        CHECK(r16(0x3100,7)==0x3140 && r16(REC,0x181)==0x3100 && d.freemem_status==0);
        CHECK(r16(DS,0x440C)==0 && r16(DS,0x440E)==0x3120 && r16(0x3120,4)==0 && r16(0x3120,6)==1);
        c=world(key,1);
        CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==PR_U9E_0026_PENDING);
        CHECK(c.cs==CS && c.ip==0x1E && c.sp==SP-38 && c.bp==SP-28 && c.ax==key && c.di==4*key);
        CHECK((c.flags&0x10)!=0 && d.freemem_status==-1 && d.handler_offset==0x1234+key && d.handler_segment==0x6000+key);
        CHECK(r16(SS,SP-38)==0x3101 && r16(SS,SP-36)==0 && r16(SS,SP-34)==0x3120 && r16(SS,SP-32)==0x100 && r16(SS,SP-30)==REC);
        CHECK(r16(SS,SP-26)==0xA4 && r16(SS,SP-24)==CS && r16(0x3100,7)==0x3120 && r16(DS,0x440E)==0x3400);
    }
    c=world(0x42,0);w16(SS,SP+6,0);w16(REC,0x181,0);w8(CS,0x135,0);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==0 && d.freemem_status==-1 && c.sp==SP+14);
    c=world(0x42,0);w16(SS,SP+6,0x3100);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==0 && r16(REC,0x181)==0x3120);
    c=world(0x42,0);w16(SS,SP+6,0x3140);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==0 && r16(0x3120,7)==0);
    c=world(0x42,0);w16(REC,0x181,0);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==0 && r16(0,5)==0 && r16(0,7)==0x3140);
    c=world(0x42,0);w16(DS,0x4406,0x3120);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==PR_U9E_0026_CALLEE && d.freemem_status==2 && c.ax==204 && c.ip==0xE2);
    CHECK(r16(0x3100,7)==0x3140); /* unlink happened before invalid free */
    c=world(0x42,0);w8(CS,0x135,0x90);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==PR_U9E_0026_CODE && c.ip==0x135 && c.sp==SP-16 && d.freemem_status==-1);
    c=world(0x42,1);w8(CS,0xA1,0x90);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==PR_U9E_0026_CODE && c.ip==0xA1 && c.sp==SP-24);
    l.max_list_loopbacks=0;c=world(0x42,0);w16(SS,SP+6,0);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==PR_U9E_0026_LIMIT && c.ip==0x52);
    c=world(0x42,0);w16(SS,SP+6,0x3140);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,&d)==PR_U9E_0026_LIMIT && c.ip==0xE5);
    c=world(0x42,0); /* immediate predecessor needs no loopback */
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,NULL)==0);
    c=world(0x42,0);before=c;
    CHECK(pr_u9e_list_release_0026_cpu(NULL,RAM,&c,&l,&d)==PR_U9E_0026_ARGUMENT && memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,NULL,&l,NULL)==PR_U9E_0026_ARGUMENT);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,NULL,NULL)==PR_U9E_0026_ARGUMENT);
    CHECK(pr_u9e_list_release_0026_cpu(ram,RAM-1,&c,&l,NULL)==PR_U9E_0026_ARGUMENT);
    c.sp=0x3F;CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,NULL)==PR_U9E_0026_ARGUMENT);
    c.sp=0xFFF3;CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,NULL)==PR_U9E_0026_ARGUMENT);
    c=before;c.ip=0;CHECK(pr_u9e_list_release_0026_cpu(ram,RAM,&c,&l,NULL)==PR_U9E_0026_ARGUMENT);
    free(ram);printf("PASS U9E +0026: %u checks\n",checks);return 0;
}
