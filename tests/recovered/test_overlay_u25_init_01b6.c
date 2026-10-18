#include "poolrad/recovered/overlay_u25_init_01b6.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[PR_U25_0000_RAM_SIZE], saved[PR_U25_0000_RAM_SIZE];
static unsigned checks;
#define CHECK(x) do { assert(x); ++checks; } while (0)
static size_t at(unsigned s, unsigned o) { return ((s << 4) + (o & 65535u)) & 0xFFFFFu; }
static void word(unsigned s, unsigned o, unsigned v) { size_t p=at(s,o); ram[p]=(uint8_t)v; ram[(p+1)&0xFFFFFu]=(uint8_t)(v>>8); }
static unsigned get(unsigned s,unsigned o) { size_t p=at(s,o); return ram[p] | ((unsigned)ram[(p+1)&0xFFFFFu]<<8); }
static pr_u25_01b6_cpu fixture(unsigned restored) {
    pr_u25_01b6_cpu c={0xABCD,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0xF000,0x3000,0x1B6,0x5000,0x1111,0x2000,0x602};
    memset(ram,0xA5,sizeof ram);
    word(c.ds,0x49D2,0x1000);word(c.ds,0x49D4,c.ds);
    word(c.ds,0x49D6,0x3000);word(c.ds,0x49D8,c.ds);
    word(c.ds,0x49DE,0x9000);word(c.ds,0x49E0,c.ds);
    ram[at(c.ds,0x49F8)]=(uint8_t)restored;
    for(unsigned i=0;i<5;++i) {
        ram[at(c.ds,0x9001+4*i)]=1;
        ram[at(c.ds,0x9002+4*i)]=(uint8_t)(i+1);
        ram[at(c.ds,0x9003+4*i)]=0x12;
        ram[at(c.ds,0x9004+4*i)]=0x7E;
    }
    word(c.ss,c.sp,0x9000);word(c.ss,c.sp+2,0xA000);return c;
}
static pr_u25_01b6_status run(pr_u25_01b6_cpu *c,size_t loops) { return pr_u25_script_init_01b6(ram,sizeof ram,c,0x4000,loops,32,512); }
int main(void) {
    pr_u25_01b6_cpu c=fixture(0),before;
    CHECK(run(&c,42)==PR_U25_01B6_OK);
    CHECK(c.cs==0xA000 && c.ip==0x9000 && c.sp==0xF004 && c.bp==0x7777);
    CHECK(c.ax==0 && c.flags==0x246);
    for(unsigned i=0;i<5;++i)CHECK(get(c.ds,0x49E3+2*i)==0x1201+i);
    CHECK(get(c.ds,0x49ED)==0x9914);
    CHECK(ram[at(c.ds,0x6F68)]==0x41 && ram[at(c.ds,0x6F69)]==9);
    CHECK(ram[at(c.ds,0x84E5)]==1 && ram[at(c.ds,0x6F83)]==8);
    CHECK(get(c.ds,0x35C2)==255 && get(c.ds,0x35A4)==0 && get(c.ds,0x35A6)==0);
    CHECK(get(c.ds,0x11CA)==0 && get(c.ds,0x11CC)==1);
    for(unsigned i=0;i<32;++i)CHECK(get(c.ds,0x1200+2*i)==0);
    for(unsigned i=0;i<10;++i)CHECK(get(c.ds,0x36F2+2*i)==0);
    c=fixture(1);CHECK(run(&c,1)==PR_U25_01B6_OK);
    CHECK(ram[at(c.ds,0x49F8)]==0 && get(c.ds,0x1200)==0xA5A5 && get(c.ds,0x36F2)==0xA5A5);
    c=fixture(0);CHECK(run(&c,1)==PR_U25_01B6_LOOP_LIMIT);
    CHECK(c.cs==0x3000 && c.ip==0x2D7 && get(c.ds,0x1200)==0 && get(c.ds,0x1202)==0xA5A5);
    c=fixture(0);CHECK(run(&c,32)==PR_U25_01B6_LOOP_LIMIT);
    CHECK(c.ip==0x2FC && get(c.ds,0x123E)==0 && get(c.ds,0x36F2)==0xA5A5);
    /* The late live write into the restored byte changes which branch executes. */
    c=fixture(0);word(c.ds,0x49D2,0x49F8-0x1CC);CHECK(run(&c,1)==PR_U25_01B6_OK);
    CHECK(ram[at(c.ds,0x49F8)]==0 && get(c.ds,0x36F2)==0xA5A5);
    /* The first indirect store may change the outer RETF destination. */
    c=fixture(1);word(c.ds,0x49D6,c.sp-0x5C2);word(c.ds,0x49D8,c.ss);
    CHECK(run(&c,42)==PR_U25_01B6_OK);CHECK(c.ip==255 && c.cs==0xA000);
    for(unsigned sp=0;sp<2;++sp) {
        c=fixture(0);c.sp=sp ? 0xFFFD : 0x235;before=c;memcpy(saved,ram,sizeof ram);
        CHECK(run(&c,42)==PR_U25_01B6_ARGUMENT);CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(ram,saved,sizeof ram)==0);
    }
    c=fixture(0);before=c;CHECK(run(&c,0)==PR_U25_01B6_ARGUMENT);CHECK(memcmp(&c,&before,sizeof c)==0);
    CHECK(pr_u25_script_init_01b6(NULL,sizeof ram,&c,0x4000,42,32,512)==PR_U25_01B6_ARGUMENT);
    printf("PASS: %u initializer checks\n",checks);return 0;
}
