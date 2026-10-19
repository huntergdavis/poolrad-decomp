#include "poolrad/recovered/resident_0709_screen_0cab_cpu.h"
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
static pr_r0709_screen_cpu setup(pr_bus *b,unsigned mode)
{
    pr_r0709_screen_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8800,0xCAB,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<2u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(c.ds,0x4A1A)]=0;
    b->ram[p(0x8800,0x140)]=0x9A;put(b->ram,0x8800,0x141,0x16A8);put(b->ram,0x8800,0x143,0x6000);
    b->ram[p(c.ds,0x4A1E)]=(uint8_t)mode;
    for(i=0;i<8u;++i) {
        const unsigned sites[]={0xCC4,0xCD8,0xD25,0xD39,0xD4D,0xD61,0xCF7,0xD0D};
        b->ram[p(c.cs,sites[i])]=(uint8_t)(i<6?0x9A:0xE8);
        put(b->ram,c.cs,sites[i]+1u,i<6?0x16A8:(0xB7u-sites[i]-3u)&65535u);
        if(i<6)put(b->ram,c.cs,sites[i]+3u,0x6000);

    }
    outs=writes=0;refuse=-1;return c;
}
static int run(pr_bus *b,pr_r0709_screen_cpu *c,size_t rect_limit,pr_r0709_screen_cpu_diagnostic *d)
{ return pr_r0709_screen_0cab_cpu(b,PR_BUS_RAM_SIZE,c,rect_limit,d); }
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE),expected[65536];
    pr_ega *ega=malloc(sizeof *ega),*saved_ega=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_r0709_screen_cpu c,before;pr_r0709_screen_cpu_diagnostic d;
    unsigned mode,active,plane,page,x;
    const unsigned sites[]={0xCC4,0xCD8,0xD25,0xD39,0xD4D,0xD61,0xCF7,0xD0D};
    CHECK(ram && copy && ega && saved_ega);
    memset(expected,0xA5,sizeof expected);
    memset(expected,0,8000);memset(expected+0x2000,0,8000);
    for(active=0;active<8u;++active) {
        c=setup(&bus,1);ram[p(c.ds,0x4A1A)]=(uint8_t)active;
        /* 200 pixel rows per page, each gets a fresh 199-transfer budget. */
        CHECK(run(&bus,&c,199,&d)==PR_R0709_SCREEN_CPU_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8004 && c.bp==0x7777);
        CHECK((c.flags&0x400u)==0 && ram[p(c.ds,0x4A1A)]==active && word(ram,c.ds,0x4A18)==0xA000+active*0x200);
        CHECK(outs==44 && writes==16000 && d.callee==2 && d.status==0 && d.rect.callee==0 && d.rect.status==0);
        CHECK(ega->gc[5]==0 && ega->gc[8]==255 && ega->seq[2]==15);
        for(plane=0;plane<4u;++plane)CHECK(memcmp(ega->plane[plane],expected,sizeof expected)==0);
    }
    for(mode=0;mode<=2u;mode+=2) {
        c=setup(&bus,mode);CHECK(run(&bus,&c,0,&d)==PR_R0709_SCREEN_CPU_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8004 && c.bp==0x7777);
        CHECK(outs==0 && writes==0 && d.callee==0 && d.status==0 && d.rect.callee==-1);
        CHECK((c.flags&0x400u)==0);
        for(page=0;page<4u;++page) {
            unsigned seg=0xB800+page*0x200u;
            for(x=0;x<8192u;++x)CHECK(ram[p(seg,x)]==((x<8000u && (mode==2 || page<2u))?0u:0xA5u));
        }
    }
    c=setup(&bus,255);CHECK(run(&bus,&c,0,NULL)==PR_R0709_SCREEN_CPU_OK);
    CHECK(outs==0 && writes==0 && (c.flags&0x400u)!=0 && c.sp==0x8004);
    /* PUSH BP changes the mode before the single live load. */
    c=setup(&bus,1);c.ss=c.ds;c.sp=0x4A20;
    put(ram,c.ss,c.sp,0x7000);put(ram,c.ss,c.sp+2,0x9001);
    CHECK(run(&bus,&c,0,&d)==PR_R0709_SCREEN_CPU_OK && outs==0 && writes==0);
    CHECK(ram[p(c.ds,0x4A1E)]==0x77 && d.callee==-1);
    /* Packed outputs can overwrite a reached return frame, in either call. */
    for(x=0;x<2u;++x) {
        c=setup(&bus,0);c.ss=(uint16_t)(x?0xB800:0xBA00);c.sp=0x50;
        put(ram,c.ss,c.sp,0x7000);put(ram,c.ss,c.sp+2,0x9001);
        CHECK(run(&bus,&c,0,&d)==PR_R0709_SCREEN_CPU_TRANSFER);
        CHECK(c.cs==0 && c.ip==0 && c.sp==0x4E && d.callee==0 && d.status==0);
    }
    for(x=0;x<8u;++x) {
        c=setup(&bus,x<2?0:x<6?2:1);ram[p(c.cs,sites[x])]=0x90;
        CHECK(run(&bus,&c,199,&d)==PR_R0709_SCREEN_CPU_CODE && c.ip==sites[x]);
        if(x==1)CHECK(ram[p(0xBA00,0)]==0 && ram[p(0xB800,0)]==0xA5);
        if(x==7)CHECK(writes==8000 && outs==22 && ega->plane[0][0]==0 && ega->plane[0][0x2000]==0xA5);
    }
    c=setup(&bus,1);ram[p(0x8800,0x140)]=0x90;
    CHECK(run(&bus,&c,199,&d)==PR_R0709_SCREEN_CPU_CALLEE);
    CHECK(c.ip==0x140 && d.callee==1 && d.status==2 && writes==0);
    c=setup(&bus,1);CHECK(run(&bus,&c,0,&d)==PR_R0709_SCREEN_CPU_CALLEE);
    CHECK(c.ip==0x14F && d.callee==1 && d.status==6 && outs==20 && writes==40);
    for(x=0;x<4u;++x) {
        const int refusals[]={0,18,22,40};c=setup(&bus,1);refuse=refusals[x];
        CHECK(run(&bus,&c,199,&d)==PR_R0709_SCREEN_CPU_CALLEE);
        CHECK(c.ip==(x%2?0x108:8) && d.callee==(x<2?1:2) && d.status==(x%2?4:3));
        CHECK(outs==(unsigned)refusals[x] && writes==(x<2?0u:8000u));
    }
    /* Near-CALL rejection retains the preceding PUSH CS at both sites. */
    for(x=0;x<2u;++x) {
        unsigned at=x?0xD0D:0xCF7;
        c=setup(&bus,1);put(ram,c.cs,at+1u,0);
        CHECK(run(&bus,&c,199,&d)==PR_R0709_SCREEN_CPU_CODE);
        CHECK(c.cs==0x8800 && c.ip==at && c.sp==0x7FF0);
        CHECK(word(ram,c.ss,c.sp)==0x8800 && word(ram,c.ss,c.sp+2u)==(x?0xA000u:0x1200u));
        CHECK((word(ram,c.ss,c.sp+4u)&255u)==x);
        CHECK(outs==(x?22u:0u) && writes==(x?8000u:0u));
        CHECK(d.callee==(x?1:-1));
    }
    c=setup(&bus,1);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(saved_ega,ega,sizeof *ega);
    CHECK(pr_r0709_screen_0cab_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,&d)==PR_R0709_SCREEN_CPU_ARGUMENT);
    CHECK(pr_r0709_screen_0cab_cpu(&bus,1,&c,0,&d)==PR_R0709_SCREEN_CPU_ARGUMENT);
    CHECK(pr_r0709_screen_0cab_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,&d)==PR_R0709_SCREEN_CPU_ARGUMENT);
    for(x=0;x<5u;++x) {
        c=before;if(x==0)c.ip=0xCAC;if(x==1)c.sp=0x31;if(x==2)c.sp=0xFFFD;if(x==3)c.ds=0xA000;if(x==4)c.ss=0x9FFF;
        {pr_r0709_screen_cpu rejected=c;CHECK(run(&bus,&c,0,&d)==PR_R0709_SCREEN_CPU_ARGUMENT);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.callee==-1 && d.status==-1 && d.rect.callee==-1);}
    }
    CHECK(memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved_ega,ega,sizeof *ega)==0);
    free(saved_ega);free(ega);free(copy);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
