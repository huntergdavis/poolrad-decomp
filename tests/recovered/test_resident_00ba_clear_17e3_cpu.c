#include "poolrad/recovered/resident_00ba_clear_17e3_cpu.h"
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
static pr_r00ba_clear_cpu setup(pr_bus *b,unsigned mode)
{
    pr_r00ba_clear_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x7800,0x17E3,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001,2,4,1,2};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<6u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(c.ds,0x4A1A)]=0;
    b->ram[p(0x8800,0x140)]=0x9A;put(b->ram,0x8800,0x141,0x16A8);put(b->ram,0x8800,0x143,0x6000);
    b->ram[p(c.ds,0x4A1E)]=(uint8_t)mode;
    for(i=0;i<8u;++i) {
        const unsigned sites[]={0x1A5,0x1C7,0x27E,0x29B,0x2B8,0x2D5,0x1F1,0x20C};
        b->ram[p(0x8000,sites[i])]=0x9A;put(b->ram,0x8000,sites[i]+1u,i<6?0x16A8:0xB7);
        put(b->ram,0x8000,sites[i]+3u,i<6?0x6000:0x8800);
    }
    for(i=0;i<2u;++i) {
        unsigned at=i?0x180C:0x17F9;b->ram[p(c.cs,at)]=0x9A;
        put(b->ram,c.cs,at+1u,0x137);put(b->ram,c.cs,at+3u,0x8000);
    }
    b->ram[p(c.ds,0x49F3)]=5;
    outs=writes=0;refuse=-1;return c;
}
static int run(pr_bus *b,pr_r00ba_clear_cpu *c,size_t limit,size_t rect_limit,pr_r00ba_clear_cpu_diagnostic *d)
{ return pr_r00ba_clear_17e3_cpu(b,PR_BUS_RAM_SIZE,c,limit,rect_limit,d); }
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*saved=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_r00ba_clear_cpu c,before;pr_r00ba_clear_cpu_diagnostic d;
    unsigned selector,active,plane,row,page,x,mode;
    CHECK(ram && copy && ega && saved);
    for(selector=0;selector<2u;++selector)for(active=0;active<8u;++active) {
        unsigned top=selector?10:18,bottom=selector?21:22,left=selector?23:1;
        c=setup(&bus,1);ram[p(c.ds,0x49F3)]=selector?5:0;ram[p(c.ds,0x4A1A)]=(uint8_t)active;
        CHECK(run(&bus,&c,0,(bottom-top+1u)*8u-1u,&d)==PR_R00BA_CLEAR_CPU_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8004 && c.bp==0x7777);
        CHECK(outs==44 && writes==2u*(bottom-top+1u)*8u*(39u-left));
        CHECK(d.callee==(selector?0:1) && d.status==0 && d.clear.callee==2 && d.clear.rect.status==0);
        CHECK(ram[p(c.ds,0x4A1A)]==active && word(ram,c.ds,0x4A18)==0xA000+active*0x200u);
        for(plane=0;plane<4u;++plane)for(page=0;page<2u;++page)for(row=0;row<200u;++row) {
            uint8_t expected[40];memset(expected,0xA5,sizeof expected);
            if(row>=top*8u && row<(bottom+1u)*8u)memset(expected+left,0,39u-left);
            CHECK(memcmp(ega->plane[plane]+page*0x2000u+row*40u,expected,40)==0);
        }
    }
    for(selector=0;selector<2u;++selector)for(mode=0;mode<=2u;mode+=2) {
        unsigned top=selector?10:18,bottom=selector?21:22,left=selector?23:1;
        unsigned scale=mode?2:4,stride=mode?160:80,width=(39u-left)*(mode?4:2),banks=mode?4:2;
        c=setup(&bus,mode);ram[p(c.ds,0x49F3)]=selector?5:0;
        CHECK(run(&bus,&c,256,0,&d)==0 && outs==0 && writes==0);
        for(page=0;page<banks;++page)for(row=top*scale;row<(bottom+1u)*scale;++row) {
            unsigned at=row*stride+left*(mode?4:2),seg=0xB800+page*0x200;
            for(x=0;x<width;++x)CHECK(ram[p(seg,at+x)]==0);
            CHECK(ram[p(seg,at-1u)]==0xA5 && ram[p(seg,at+width)]==0xA5);
        }
        c=setup(&bus,mode);CHECK(run(&bus,&c,0,0,&d)==PR_R00BA_CLEAR_CPU_CALLEE);
        CHECK(d.status==PR_R03F1_CLEAR_CPU_LIMIT && c.cs==0x8000 && c.ip==(mode?0x2E0:0x1D2));
    }
    for(selector=0;selector<2u;++selector) {
        unsigned at=selector?0x17F9:0x180C;
        c=setup(&bus,1);ram[p(c.ds,0x49F3)]=selector?5:0;ram[p(c.cs,at)]=0x90;c.ax=0xEF12;
        CHECK(run(&bus,&c,0,0,&d)==PR_R00BA_CLEAR_CPU_CODE && c.ip==at && c.sp==0x7FF6);
        CHECK(word(ram,c.ss,c.sp)==(selector?0xEF15:0xEF16) && word(ram,c.ss,c.sp+2)==0xEF26);
        CHECK(word(ram,c.ss,c.sp+4)==(selector?0xEF0A:0xEF12) && word(ram,c.ss,c.sp+6)==(selector?0xEF17:0xEF01));
        CHECK(d.callee==-1 && outs==0 && writes==0);
    }
    /* PUSH BP changes the selector before it is read. */
    c=setup(&bus,1);c.ss=c.ds;c.sp=0x49F5;put(ram,c.ss,c.sp,0x7000);put(ram,c.ss,c.sp+2,0x9001);
    CHECK(run(&bus,&c,256,65536,&d)==0 && d.callee==1 && ram[p(c.ds,0x49F3)]==0x77);
    /* The same live alias can change the downstream mode. */
    c=setup(&bus,1);c.ss=c.ds;c.sp=0x4A20;put(ram,c.ss,c.sp,0x7000);put(ram,c.ss,c.sp+2,0x9001);
    CHECK(run(&bus,&c,0,0,&d)==0 && outs==0 && writes==0 && ram[p(c.ds,0x4A1E)]==0x77);
    c=setup(&bus,255);CHECK(run(&bus,&c,0,0,NULL)==0 && outs==0 && writes==0 && c.sp==0x8004);
    c=setup(&bus,1);ram[p(0x8000,0x1F1)]=0x90;
    CHECK(run(&bus,&c,0,0,&d)==3 && d.status==2 && c.cs==0x8000 && c.ip==0x1F1);
    c=setup(&bus,1);CHECK(run(&bus,&c,0,0,&d)==3 && d.status==3 && d.clear.status==6 && c.ip==0x14F);
    for(x=0;x<4u;++x) {
        const int refusals[]={0,18,22,40};c=setup(&bus,1);refuse=refusals[x];
        CHECK(run(&bus,&c,256,65536,&d)==3 && d.status==3 && d.clear.status==(x%2?4:3));
        CHECK(outs==(unsigned)refusals[x] && writes==(x<2?0u:1536u));
    }
    c=setup(&bus,1);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(saved,ega,sizeof *ega);
    CHECK(pr_r00ba_clear_17e3_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,0,&d)==1);
    CHECK(pr_r00ba_clear_17e3_cpu(&bus,1,&c,0,0,&d)==1);
    CHECK(pr_r00ba_clear_17e3_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,0,&d)==1);
    for(x=0;x<5u;++x) {
        c=before;if(x==0)c.ip=0x17E4;if(x==1)c.sp=0x4B;if(x==2)c.sp=0xFFFD;if(x==3)c.ds=0xA000;if(x==4)c.ss=0x9FFF;
        {pr_r00ba_clear_cpu rejected=c;CHECK(run(&bus,&c,0,0,&d)==1);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.callee==-1 && d.status==-1 && d.clear.callee==-1 && d.clear.rect.callee==-1);}
    }
    CHECK(memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved,ega,sizeof *ega)==0);
    free(saved);free(ega);free(copy);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
