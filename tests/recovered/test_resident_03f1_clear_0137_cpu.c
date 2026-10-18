#include "poolrad/recovered/resident_03f1_clear_0137_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,outs,writes;
static int refuse=-1;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned s,unsigned o) { return ((size_t)s*16u+(o&65535u))&1048575u; }
static void put(uint8_t *m,unsigned s,unsigned o,unsigned v) { size_t a=p(s,o);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8); }
static unsigned word(const uint8_t *m,unsigned s,unsigned o) { size_t a=p(s,o);return m[a]|((unsigned)m[(a+1u)&1048575u]<<8); }
static int out(void *ctx,uint16_t port,uint16_t value,unsigned size,uint16_t cs,uint16_t ip)
{
    (void)ctx;(void)port;(void)value;(void)size;(void)cs;(void)ip;
    if(refuse>=0 && outs==(unsigned)refuse)return 1;
    ++outs;return 0;
}
static void written(void *ctx,uint16_t offset,uint8_t value) { (void)ctx;(void)offset;(void)value;++writes; }
static pr_r03f1_clear_cpu setup(pr_bus *b,unsigned mode)
{
    pr_r03f1_clear_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8000,0x137,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001,2,4,1,2};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<6u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(c.ds,0x4A1A)]=0;
    b->ram[p(0x8800,0x140)]=0x9A;put(b->ram,0x8800,0x141,0x16A8);put(b->ram,0x8800,0x143,0x6000);
    b->ram[p(c.ds,0x4A1E)]=(uint8_t)mode;
    for(i=0;i<8u;++i) {
        const unsigned sites[]={0x1A5,0x1C7,0x27E,0x29B,0x2B8,0x2D5,0x1F1,0x20C};
        b->ram[p(c.cs,sites[i])]=0x9A;put(b->ram,c.cs,sites[i]+1u,i<6?0x16A8:0xB7);
        put(b->ram,c.cs,sites[i]+3u,i<6?0x6000:0x8800);
    }
    outs=writes=0;refuse=-1;return c;
}
static int run(pr_bus *b,pr_r03f1_clear_cpu *c,size_t limit,size_t rect_limit,pr_r03f1_clear_cpu_diagnostic *d)
{ return pr_r03f1_clear_0137_cpu(b,PR_BUS_RAM_SIZE,c,limit,rect_limit,d); }
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*saved_ega=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_r03f1_clear_cpu c,before;pr_r03f1_clear_cpu_diagnostic d;
    unsigned mode,active,plane,row,page,x;
    CHECK(ram && copy && ega && saved_ega);
    for(active=0;active<8u;++active) {
        c=setup(&bus,1);ram[p(c.ds,0x4A1A)]=(uint8_t)active;
        /* Fifteen taken row branches per rectangle: each gets a fresh budget. */
        CHECK(run(&bus,&c,0,15,&d)==PR_R03F1_CLEAR_CPU_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x800C && c.bp==0x7777);
        CHECK((c.flags&0x400u)==0 && ram[p(c.ds,0x4A1A)]==active && word(ram,c.ds,0x4A18)==0xA000+active*0x200);
        CHECK(outs==44 && writes==96 && d.callee==2 && d.status==0 && d.rect.callee==0 && d.rect.status==0);
        CHECK(ega->gc[5]==0 && ega->gc[8]==255 && ega->seq[2]==15);
        for(plane=0;plane<4u;++plane)for(row=8;row<24u;++row)for(page=0;page<2u;++page) {
            unsigned at=row*40u+2u+page*0x2000u;
            for(x=0;x<3u;++x)CHECK(ega->plane[plane][at+x]==0);
            CHECK(ega->plane[plane][at-1u]==0xA5 && ega->plane[plane][at+3u]==0xA5);
        }
    }
    for(mode=0;mode<=2u;mode+=2) {
        unsigned first=mode?2:4,last=mode?6:12,stride=mode?160:80,left=mode?8:4,width=mode?12:6,banks=mode?4:2;
        c=setup(&bus,mode);CHECK(run(&bus,&c,last-first-1u,0,&d)==PR_R03F1_CLEAR_CPU_OK);
        CHECK(outs==0 && writes==0 && d.callee==0 && d.status==0 && d.rect.callee==-1);
        for(page=0;page<banks;++page)for(row=first;row<last;++row) {
            unsigned seg=0xB800+page*0x200u,at=row*stride+left;
            for(x=0;x<width;++x)CHECK(ram[p(seg,at+x)]==0);
            CHECK(ram[p(seg,at-1u)]==0xA5 && ram[p(seg,at+width)]==0xA5);
        }
        c=setup(&bus,mode);CHECK(run(&bus,&c,0,0,&d)==PR_R03F1_CLEAR_CPU_LIMIT);
        CHECK(c.ip==(mode?0x2E0:0x1D2) && d.callee==0 && outs==0 && writes==0);
        c=setup(&bus,mode);put(ram,c.ss,c.sp+4u,0);
        CHECK(run(&bus,&c,0,0,&d)==PR_R03F1_CLEAR_CPU_OK && d.callee==-1);
    }
    c=setup(&bus,255);CHECK(run(&bus,&c,0,0,NULL)==PR_R03F1_CLEAR_CPU_OK);
    CHECK(outs==0 && writes==0 && (c.flags&0x400u)!=0 && c.sp==0x800C);
    /* A stack alias changes the mode before the caller reads it. */
    c=setup(&bus,1);c.ss=c.ds;c.sp=0x4A20;
    {const unsigned args[]={0x7000,0x9001,2,4,1,2};for(x=0;x<6u;++x)put(ram,c.ss,c.sp+2u*x,args[x]);}
    CHECK(run(&bus,&c,0,0,&d)==PR_R03F1_CLEAR_CPU_OK && outs==0 && writes==0);
    CHECK(ram[p(c.ds,0x4A1E)]==0x77 && d.callee==-1);
    /* A packed output row repeatedly zeroes the live loop counter. */
    c=setup(&bus,0);c.ss=0xB800;c.sp=0x8E;
    {const unsigned args[]={0x7000,0x9001,0,25,0,25};for(x=0;x<6u;++x)put(ram,c.ss,c.sp+2u*x,args[x]);}
    CHECK(run(&bus,&c,4,0,&d)==PR_R03F1_CLEAR_CPU_LIMIT);
    CHECK(c.ip==0x1D2 && word(ram,c.ss,0x82)==0 && d.callee==0);
    /* A wider packed fill overwrites its own far return pair. */
    c=setup(&bus,0);c.ss=0xB800;c.sp=0x50;
    {const unsigned args[]={0x7000,0x9001,0,39,0,0};for(x=0;x<6u;++x)put(ram,c.ss,c.sp+2u*x,args[x]);}
    CHECK(run(&bus,&c,256,0,&d)==PR_R03F1_CLEAR_CPU_TRANSFER);
    CHECK(c.cs==0 && c.ip==0 && c.sp==0x42 && d.callee==0 && d.status==0);
    for(x=0;x<8u;++x) {
        const unsigned sites[]={0x1A5,0x1C7,0x27E,0x29B,0x2B8,0x2D5,0x1F1,0x20C};
        c=setup(&bus,x<2?0:x<6?2:1);ram[p(c.cs,sites[x])]=0x90;
        CHECK(run(&bus,&c,256,65536,&d)==PR_R03F1_CLEAR_CPU_CODE && c.ip==sites[x]);
        if(x==7)CHECK(writes==48 && outs==22 && ega->plane[0][0x142]==0 && ega->plane[0][0x2142]==0xA5);
    }
    c=setup(&bus,1);ram[p(0x8800,0x140)]=0x90;
    CHECK(run(&bus,&c,256,65536,&d)==PR_R03F1_CLEAR_CPU_CALLEE);
    CHECK(c.ip==0x140 && d.callee==1 && d.status==2 && writes==0);
    c=setup(&bus,1);CHECK(run(&bus,&c,256,0,&d)==PR_R03F1_CLEAR_CPU_CALLEE);
    CHECK(c.ip==0x14F && d.callee==1 && d.status==6 && outs==20 && writes==3);
    for(x=0;x<4u;++x) {
        const int refusals[]={0,18,22,40};c=setup(&bus,1);refuse=refusals[x];
        CHECK(run(&bus,&c,256,65536,&d)==PR_R03F1_CLEAR_CPU_CALLEE);
        CHECK(c.ip==(x%2?0x108:8) && d.callee==(x<2?1:2) && d.status==(x%2?4:3));
        CHECK(outs==(unsigned)refusals[x] && writes==(x<2?0u:48u));
    }
    c=setup(&bus,1);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(saved_ega,ega,sizeof *ega);
    CHECK(pr_r03f1_clear_0137_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,0,&d)==PR_R03F1_CLEAR_CPU_ARGUMENT);
    CHECK(pr_r03f1_clear_0137_cpu(&bus,1,&c,0,0,&d)==PR_R03F1_CLEAR_CPU_ARGUMENT);
    CHECK(pr_r03f1_clear_0137_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,0,&d)==PR_R03F1_CLEAR_CPU_ARGUMENT);
    for(x=0;x<5u;++x) {
        c=before;if(x==0)c.ip=0x138;if(x==1)c.sp=0x3D;if(x==2)c.sp=0xFFF5;if(x==3)c.ds=0xA000;if(x==4)c.ss=0x9FFF;
        {pr_r03f1_clear_cpu rejected=c;CHECK(run(&bus,&c,0,0,&d)==PR_R03F1_CLEAR_CPU_ARGUMENT);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.callee==-1 && d.status==-1 && d.rect.callee==-1);}
    }
    CHECK(memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved_ega,ega,sizeof *ega)==0);
    free(saved_ega);free(ega);free(copy);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
