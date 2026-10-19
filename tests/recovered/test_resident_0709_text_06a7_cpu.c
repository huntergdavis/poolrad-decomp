#include "poolrad/recovered/resident_0709_text_06a7_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,outs,writes;
static int refuse=-1;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned s,unsigned o) { return ((size_t)s*16u+(o&65535u))&1048575u; }
static void put(uint8_t *m,unsigned s,unsigned o,unsigned v) { size_t a=p(s,o);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8); }
static int out(void *ctx,uint16_t port,uint16_t value,unsigned size,uint16_t cs,uint16_t ip)
{
    (void)ctx;(void)port;(void)value;(void)size;(void)cs;(void)ip;
    if(refuse>=0 && outs==(unsigned)refuse)return 1;
    ++outs;return 0;
}
static void written(void *ctx,uint16_t offset,uint8_t value) { (void)ctx;(void)offset;(void)value;++writes; }
static pr_r0709_text_cpu setup(pr_bus *b,unsigned color)
{
    pr_r0709_text_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8800,0x6A7,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001,0x9000};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<3u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(c.ds,0x4A1E)]=1;
    put(b->ram,c.ds,0x4A16,0xA000);put(b->ram,c.ds,0x4A18,0xA200);
    put(b->ram,c.ds,0x5E20,0x1000);put(b->ram,c.ds,0x5E22,0x7000);
    for(i=0;i<4u;++i)b->ram[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    for(i=0;i<256u;++i)b->ram[p(c.ds,0x25D5+i)]=(uint8_t)(i%4u);
    for(i=0;i<4u;++i)put(b->ram,c.ds,0x25CD+2u*i,i*0x5555u);
    for(i=0;i<8u;++i){put(b->ram,c.ds,0x25E5+2u*i,0xC000u>>(2u*i));b->ram[p(0x7000,0x1008+i)]=(uint8_t)(0x81u^(i*13u));}
    b->ram[p(c.cs,0x207)]=0x9A;put(b->ram,c.cs,0x208,0x182A);put(b->ram,c.cs,0x20A,0x6000);
    b->ram[p(c.ss,0x8EFD)]=1;b->ram[p(c.ss,0x8EFC)]=3;
    b->ram[p(c.ss,0x900C)]=(uint8_t)color;memset(b->ram+p(c.ss,0x8F01),'a',3);
    b->ram[p(c.ds,0x5E24)]=3;b->ram[p(c.ds,0x5E25)]=2;
    b->ram[p(c.ds,0x4A00)]=0;b->ram[p(c.ds,0x49E2)]=1;
    put(b->ram,c.ds,0x84FC,2);b->ram[0x400]=0x42;
    b->ram[p(c.cs,0x6E8)]=0xE8;put(b->ram,c.cs,0x6E9,0xFAF4);
    b->ram[p(c.cs,0x6FD)]=0x9A;put(b->ram,c.cs,0x6FE,0x29C);put(b->ram,c.cs,0x700,0x7800);
    outs=writes=0;refuse=-1;return c;
}
static int run(pr_bus *b,pr_r0709_text_cpu *c,size_t limit,size_t glyph_limit,uint64_t delay_limit,pr_r0709_text_cpu_diagnostic *d)
{ return pr_r0709_text_06a7_cpu(b,PR_BUS_RAM_SIZE,c,limit,glyph_limit,delay_limit,d); }
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*saved=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_r0709_text_cpu c,before;pr_r0709_text_cpu_diagnostic d;
    unsigned color,plane,row,page,x;
    CHECK(ram && copy && ega && saved);
    for(color=0;color<16u;++color) {
        c=setup(&bus,color);CHECK(run(&bus,&c,3,65536,0,&d)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8006 && c.bp==0x7777);
        CHECK(ram[p(c.ss,0x8EFD)]==4 && ram[p(c.ds,0x5E24)]==6);
        CHECK(memcmp(ram+p(c.ss,0x8F01),"aaa",3)==0 && ram[p(c.ss,0x8EFC)]==3);
        CHECK(outs==102 && writes==192 && ega->seq[2]==15);
        CHECK(d.callee==0 && d.status==0 && d.glyph_callee==0 && d.glyph_status==0);
        for(plane=0;plane<4u;++plane)for(row=0;row<8u;++row)for(page=0;page<2u;++page) {
            unsigned at=0x283u+row*40u+page*0x2000u;
            for(x=0;x<3u;++x)CHECK(ega->plane[plane][at+x]==((color&(1u<<plane))?(0x81u^(row*13u)):0));
            CHECK(ega->plane[plane][at-1u]==0xA5 && ega->plane[plane][at+3u]==0xA5);
        }
    }
    /* Three delay calls each need79 loopbacks; the budget resets per call. */
    c=setup(&bus,5);ram[p(c.ds,0x4A00)]=1;
    CHECK(run(&bus,&c,3,65536,79,&d)==0 && d.callee==1 && d.status==0);
    CHECK(c.es==0x40 && c.dx==0 && c.cx==0 && ram[p(c.ss,0x8EFD)]==4 && outs==102);
    c=setup(&bus,5);ram[p(c.ds,0x4A00)]=1;
    CHECK(run(&bus,&c,3,65536,0,&d)==3 && d.callee==1 && d.status==2);
    CHECK(c.cs==0x7800 && c.ip==0x2C0 && c.bx==3 && outs==34 && ram[p(c.ss,0x8EFD)]==1);
    c=setup(&bus,5);ram[p(c.ds,0x4A00)]=1;ram[p(c.ds,0x49E2)]=0;
    CHECK(run(&bus,&c,3,65536,0,&d)==0 && d.callee==1);
    c=setup(&bus,5);ram[p(c.ss,0x8EFD)]=4;
    CHECK(run(&bus,&c,0,0,0,NULL)==0 && outs==0 && writes==0 && ram[p(c.ds,0x5E24)]==3);
    c=setup(&bus,5);CHECK(run(&bus,&c,0,65536,0,&d)==6 && c.ip==0x70E);
    CHECK(outs==34 && writes==64 && ram[p(c.ss,0x8EFD)]==2 && ram[p(c.ds,0x5E24)]==4);
    c=setup(&bus,5);ram[p(c.ds,0x4A1E)]=3;ram[p(c.ss,0x8EFD)]=255;ram[p(c.ss,0x8EFC)]=255;
    CHECK(run(&bus,&c,2,65536,0,&d)==6 && ram[p(c.ss,0x8EFD)]==2 && ram[p(c.ds,0x5E24)]==6);
    c=setup(&bus,5);ram[p(c.ds,0x5E24)]=255;
    CHECK(run(&bus,&c,3,65536,0,&d)==0 && ram[p(c.ds,0x5E24)]==2 && outs==68 && writes==128);
    c=setup(&bus,5);ram[p(c.ds,0x5E25)]=25;
    CHECK(run(&bus,&c,3,0,0,&d)==0 && outs==0 && ram[p(c.ss,0x8EFD)]==4 && ram[p(c.ds,0x5E24)]==6);
    c=setup(&bus,5);CHECK(run(&bus,&c,3,0,0,&d)==3 && c.ip==0x45B && d.callee==0 && d.status==6);
    c=setup(&bus,5);refuse=1;CHECK(run(&bus,&c,3,65536,0,&d)==3 && c.ip==0x3D5 && d.status==4);
    for(x=0;x<3u;++x) {
        const unsigned sites[]={0x6E8,0x6FD,0x207};c=setup(&bus,5);ram[p(c.cs,sites[x])]=0x90;ram[p(c.ds,0x4A00)]=1;
        CHECK(run(&bus,&c,3,65536,100,&d)==(x==2?3:2) && c.ip==sites[x]);
        if(x==1)CHECK(c.ax==10 && c.dx==0 && c.sp==0x7FFC && outs==34 && d.callee==0);
        else CHECK(outs==0 && writes==0);
    }
    c=setup(&bus,5);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(saved,ega,sizeof *ega);
    CHECK(pr_r0709_text_06a7_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,0,0,&d)==1);
    CHECK(pr_r0709_text_06a7_cpu(&bus,1,&c,0,0,0,&d)==1);
    CHECK(pr_r0709_text_06a7_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,0,0,&d)==1);
    for(x=0;x<5u;++x) {
        c=before;if(x==0)c.ip=0x6A8;if(x==1)c.sp=0x2B;if(x==2)c.sp=0xFFFB;if(x==3)c.ds=0xA000;if(x==4)c.ss=0x9FFF;
        {pr_r0709_text_cpu rejected=c;CHECK(run(&bus,&c,0,0,0,&d)==1);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.callee==-1 && d.status==-1 && d.glyph_callee==-1);}
    }
    CHECK(memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved,ega,sizeof *ega)==0);
    free(saved);free(ega);free(copy);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
