#include "poolrad/recovered/resident_00ba_display_1816_cpu.h"
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
static pr_r00ba_display_cpu setup(pr_bus *b,unsigned branch)
{
    pr_r00ba_display_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8000,0x1816,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001,1,2,3,0x800,0x4000};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<7u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(c.ds,0x4A1E)]=1;
    put(b->ram,c.ds,0x4A16,0xA000);put(b->ram,c.ds,0x4A18,0xA200);
    put(b->ram,c.ds,0x5E20,0x1000);put(b->ram,c.ds,0x5E22,0x7000);
    for(i=0;i<4u;++i)b->ram[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    for(i=0;i<256u;++i)b->ram[p(c.ds,0x25D5+i)]=(uint8_t)(i%4u);
    for(i=0;i<4u;++i)put(b->ram,c.ds,0x25CD+2u*i,i*0x5555u);
    for(i=0;i<8u;++i){put(b->ram,c.ds,0x25E5+2u*i,0xC000u>>(2u*i));b->ram[p(0x7000,0x1008+i)]=(uint8_t)(0x81u^(i*13u));}
    b->ram[p(0x8800,0x207)]=0x9A;put(b->ram,0x8800,0x208,0x182A);put(b->ram,0x8800,0x20A,0x6000);
    b->ram[p(0x4000,0x800)]=3;memset(b->ram+p(0x4000,0x801),'a',3);
    b->ram[p(0x8800,0x63C)]=0x9A;put(b->ram,0x8800,0x63D,0xB25);put(b->ram,0x8800,0x63F,0x6000);
    b->ram[p(0x8800,0x691)]=0xE8;put(b->ram,0x8800,0x692,0xFB4B);
    b->ram[p(0x4000,0x90D)]=(uint8_t)(branch!=0);b->ram[p(0x4000,0x90E)]=(uint8_t)(branch==1?1:2);
    b->ram[p(c.cs,0x1813)]=2;b->ram[p(c.cs,0x1814)]='a';b->ram[p(c.cs,0x1815)]='a';
    for(i=0;i<3u;++i) {
        const unsigned at[]={0x1853,0x1882,0x1887};
        b->ram[p(c.cs,at[i])]=0x9A;put(b->ram,c.cs,at[i]+1u,i==1?0xB0B:0x626);
        put(b->ram,c.cs,at[i]+3u,i==1?0x6000:0x8800);
    }
    outs=writes=0;refuse=-1;return c;
}
static int run(pr_bus *b,pr_r00ba_display_cpu *c,size_t limit,size_t glyph_limit,pr_r00ba_display_cpu_diagnostic *d)
{ return pr_r00ba_display_1816_cpu(b,PR_BUS_RAM_SIZE,c,limit,glyph_limit,d); }
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*saved_ega=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_r00ba_display_cpu c,before;pr_r00ba_display_cpu_diagnostic d;
    unsigned branch,flag,plane,row,page,x;
    CHECK(ram && copy && ega && saved_ega);
    for(branch=0;branch<3u;++branch)for(flag=0;flag<2u;++flag) {
        const unsigned colors[]={12,14,11};unsigned count=flag?5:3,color=colors[branch];
        c=setup(&bus,branch);put(ram,c.ss,c.sp+4u,flag);
        CHECK(run(&bus,&c,256,65536,&d)==PR_R00BA_DISPLAY_CPU_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x800E && c.bp==0x7777);
        CHECK((c.flags&0x600u)==0x200u && ram[p(c.ss,0x8008)]==3);
        CHECK(ram[p(c.ss,0x7FFD)]==color && outs==34u*count && writes==64u*count);
        CHECK(d.callee==(flag?2:0) && d.status==0 && d.printer.callee==1 && d.printer.status==0);
        if(flag)CHECK(ram[p(c.ss,0x7FFA)]==2 && memcmp(ram+p(c.ss,0x7FFB),"aa",2)==0);
        for(plane=0;plane<4u;++plane)for(row=0;row<8u;++row)for(page=0;page<2u;++page) {
            unsigned at=0x283u+row*40u+page*0x2000u;
            for(x=0;x<count;++x)CHECK(ega->plane[plane][at+x]==((color&(1u<<plane))?(0x81u^(row*13u)):0));
            CHECK(ega->plane[plane][at-1u]==0xA5 && ega->plane[plane][at+count]==0xA5);
        }
    }
    /* The literal payload is supplied memory, not hard-coded output. */
    c=setup(&bus,0);ram[p(c.cs,0x1814)]='b';ram[p(c.cs,0x1815)]='c';ram[p(0x4000,0x800)]=0;
    CHECK(run(&bus,&c,256,65536,&d)==PR_R00BA_DISPLAY_CPU_OK);
    CHECK(ram[p(c.ss,0x7FFA)]==2 && memcmp(ram+p(c.ss,0x7FFB),"bc",2)==0 && outs==68);
    /* Each printer can reject y; the caller still reaches its suffix copy. */
    c=setup(&bus,0);put(ram,c.ss,c.sp+6u,25);
    CHECK(run(&bus,&c,256,65536,&d)==PR_R00BA_DISPLAY_CPU_OK && outs==0 && writes==0);
    CHECK(d.callee==2 && ram[p(c.ss,0x7FFA)]==2);
    /* Ordinary output overwrites node.length with FF before the late read.
     * The suffix column becomes 0102, then its two glyphs advance it to0104. */
    c=setup(&bus,2);put(ram,c.ss,c.sp+10u,0x283);
    ram[p(0x4000,0x283)]=1;ram[p(0x4000,0x284)]='a';
    ram[p(0x4000,0x390)]=1;ram[p(0x4000,0x391)]=2;
    put(ram,c.ds,0x4A16,0x4000);put(ram,c.ds,0x4A18,0x4200);
    memset(ram+p(0x7000,0x1000),0xFF,2048);
    CHECK(run(&bus,&c,256,65536,&d)==PR_R00BA_DISPLAY_CPU_OK);
    CHECK(ram[p(0x4000,0x283)]==255 && ram[p(c.ss,0x7FF8)]==4 && ram[p(c.ss,0x7FF9)]==1);
    CHECK(outs==102 && writes==0 && d.callee==2);
    for(x=0;x<6u;++x) {
        const unsigned sites[]={0x1853,0x1882,0x1887,0x63C,0x691,0x207};
        c=setup(&bus,0);ram[p(x<3?c.cs:0x8800,sites[x])]=0x90;
        CHECK(run(&bus,&c,256,65536,&d)==(x<3?PR_R00BA_DISPLAY_CPU_CODE:PR_R00BA_DISPLAY_CPU_CALLEE));
        CHECK(c.ip==sites[x]);
        if(x>=3)CHECK(d.callee==0 && d.status==(x==5?3:2));
    }
    c=setup(&bus,0);CHECK(run(&bus,&c,0,65536,&d)==PR_R00BA_DISPLAY_CPU_CALLEE);
    CHECK(c.ip==0x69F && d.callee==0 && d.status==6 && outs==34);
    c=setup(&bus,0);CHECK(run(&bus,&c,256,0,&d)==PR_R00BA_DISPLAY_CPU_CALLEE);
    CHECK(c.ip==0x45B && d.callee==0 && d.status==3 && d.printer.status==6 && outs==2);
    c=setup(&bus,0);ram[p(0x4000,0x800)]=1;
    CHECK(run(&bus,&c,0,65536,&d)==PR_R00BA_DISPLAY_CPU_CALLEE);
    CHECK(c.ip==0x69F && d.callee==2 && d.status==6 && outs==68);
    c=setup(&bus,0);ram[p(0x4000,0x800)]=0;ram[p(0x8800,0x207)]=0x90;
    CHECK(run(&bus,&c,256,65536,&d)==PR_R00BA_DISPLAY_CPU_CALLEE);
    CHECK(c.ip==0x207 && d.callee==2 && d.status==3 && d.printer.callee==1 && d.printer.status==2);
    c=setup(&bus,0);ram[p(0x4000,0x800)]=0;refuse=0;
    CHECK(run(&bus,&c,256,65536,&d)==PR_R00BA_DISPLAY_CPU_CALLEE);
    CHECK(c.ip==0x3AD && d.callee==2 && d.status==3 && d.printer.status==4 && outs==0 && writes==0);
    c=setup(&bus,0);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(saved_ega,ega,sizeof *ega);
    CHECK(pr_r00ba_display_1816_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,0,&d)==PR_R00BA_DISPLAY_CPU_ARGUMENT);
    CHECK(pr_r00ba_display_1816_cpu(&bus,1,&c,0,0,&d)==PR_R00BA_DISPLAY_CPU_ARGUMENT);
    CHECK(pr_r00ba_display_1816_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,0,&d)==PR_R00BA_DISPLAY_CPU_ARGUMENT);
    for(x=0;x<6u;++x) {
        c=before;if(x==0)c.ip=0x1817;if(x==1)c.sp=0x143;if(x==2)c.sp=0xFFF3;if(x==3)c.ds=0xA000;if(x==4)c.ss=0x9FFF;if(x==5)c.cs=0xA000;
        {pr_r00ba_display_cpu rejected=c;CHECK(run(&bus,&c,0,0,&d)==PR_R00BA_DISPLAY_CPU_ARGUMENT);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.callee==-1 && d.status==-1 && d.printer.callee==-1);}
    }
    CHECK(memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved_ega,ega,sizeof *ega)==0);
    for(x=0;x<2u;++x) {
        c=before;memcpy(ram,copy,PR_BUS_RAM_SIZE);
        if(x==0)put(ram,c.ss,c.sp+12u,0xA000);else ram[p(c.cs,0x1813)]=3;
        {uint8_t *rejected=malloc(PR_BUS_RAM_SIZE);CHECK(rejected);memcpy(rejected,ram,PR_BUS_RAM_SIZE);
         CHECK(run(&bus,&c,0,0,&d)==PR_R00BA_DISPLAY_CPU_ARGUMENT);
         CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(rejected,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved_ega,ega,sizeof *ega)==0);free(rejected);}
    }
    free(saved_ega);free(ega);free(copy);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
