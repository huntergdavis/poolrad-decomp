#include "poolrad/recovered/resident_00ba_display_1693_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks;
#define CHECK(x) do { ++checks; if(!(x)) { fprintf(stderr,"line%d: %s\n",__LINE__,#x); exit(1); } } while(0)
static unsigned char ram[PR_BUS_RAM_SIZE];
static pr_ega ega;
static size_t addr(unsigned s,unsigned o) {return ((s<<4)+(o&65535u))&(PR_BUS_RAM_SIZE-1u);}
static void word(unsigned s,unsigned o,unsigned v) {size_t a=addr(s,o);ram[a]=(unsigned char)v;ram[(a+1)&(PR_BUS_RAM_SIZE-1u)]=(unsigned char)(v>>8);}
static pr_r00ba_1693_cpu fixture(unsigned length,unsigned source) {
    pr_r00ba_1693_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8200,0x1693,0x5000,0x1111,0x2000,0x402};
    memset(ram,0xA5,sizeof ram);pr_ega_reset(&ega);
    word(c.ss,c.sp,0x7000);word(c.ss,c.sp+2,0x9001);word(c.ss,c.sp+4,source);word(c.ss,c.sp+6,0x4000);
    ram[addr(c.cs,0x16A7)]=0x9A;word(c.cs,0x16A8,0xB25);word(c.cs,0x16AA,0x6000);
    ram[addr(0x4000,source)]=(unsigned char)length;
    for(unsigned i=0;i<length;++i)ram[addr(0x4000,source+1+i)]=(unsigned char)(i*13+65);
    return c;
}
int main(void) {
    pr_bus bus={ram,&ega,NULL,NULL,NULL};pr_r00ba_1693_diagnostic d;
    for(unsigned n=0;n<256;++n)for(unsigned wrap=0;wrap<2;++wrap) {
        pr_r00ba_1693_cpu c=fixture(n,wrap?0xFFFE:0x800);unsigned take=n>40?40:n;
        CHECK(pr_r00ba_display_1693_cpu(&bus,sizeof ram,&c,100,100,100,100,100,&d)==PR_R00BA_1693_CODE);
        CHECK(c.ip==0x16B8 && c.cs==0x8200 && c.bp==0x7FFE && c.sp==0x7FCC);
        CHECK(d.callee==0 && d.status==0 && d.delay_status==-1);
        CHECK(ram[addr(c.ss,0x7FD5)]==take);
        for(unsigned j=0;j<take;++j)CHECK(ram[addr(c.ss,0x7FD6+j)]==(unsigned char)(j*13+65));
        CHECK(c.ax==0x18 && c.flags==0x46 && c.es==c.ss && c.ds==0x5000);
    }
    /* Source one byte before the destination: forward LODSB/STOSB is live,
     * so writing the length replaces every subsequently consumed byte. */
    {
        pr_r00ba_1693_cpu c=fixture(0,0x800);word(c.ss,c.sp+4,0x7FD4);word(c.ss,c.sp+6,c.ss);
        ram[addr(c.ss,0x7FD4)]=4;ram[addr(c.ss,0x7FD5)]=0xAA;
        CHECK(pr_r00ba_display_1693_cpu(&bus,sizeof ram,&c,100,100,100,100,100,&d)==PR_R00BA_1693_CODE);
        for(unsigned j=0;j<5;++j)CHECK(ram[addr(c.ss,0x7FD5+j)]==4);
    }
    for(unsigned bad=0;bad<6;++bad) {
        pr_r00ba_1693_cpu c=fixture(3,0x800),before;
        if(bad==0)c.sp=0x169;
        if(bad==1)c.sp=0xFFF9;
        if(bad==2)c.ip=0x1694;
        if(bad==3)c.ds=0xA000;
        if(bad==4)c.ss=0xA000;
        if(bad==5)word(c.ss,c.sp+6,0xA000);
        before=c;unsigned char *copy=malloc(sizeof ram);CHECK(copy!=NULL);memcpy(copy,ram,sizeof ram);
        CHECK(pr_r00ba_display_1693_cpu(&bus,sizeof ram,&c,100,100,100,100,100,&d)==PR_R00BA_1693_ARGUMENT);
        CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(copy,ram,sizeof ram)==0 && d.callee==-1);free(copy);
    }
    printf("%u semantic checks passed\n",checks);return 0;
}
