#include "poolrad/recovered/overlay_u1c_clear_box_2b14.h"
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
static pr_u1c_2b14_cpu setup(pr_bus *b,unsigned mode)
{
    pr_u1c_2b14_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x4000,0x2B14,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001,2,4,1,2};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<6u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(c.ds,0x4A1A)]=0;
    b->ram[p(0x8800,0x140)]=0x9A;put(b->ram,0x8800,0x141,0x16A8);put(b->ram,0x8800,0x143,0x6000);
    b->ram[p(c.ds,0x4A1E)]=(uint8_t)mode;
    for(i=0;i<8u;++i) {
        const unsigned sites[]={0xE41,0xE63,0xF18,0xF35,0xF52,0xF6F,0xE8E,0xEA8};
        b->ram[p(0x8800,sites[i])]=(uint8_t)(i<6?0x9A:0xE8);
        put(b->ram,0x8800,sites[i]+1u,i<6?0x16A8:(0xB7u-sites[i]-3u)&65535u);
        if(i<6)put(b->ram,0x8800,sites[i]+3u,0x6000);

    }
    b->ram[p(c.cs,0x2B27)]=0x9A;put(b->ram,c.cs,0x2B28,0xDD3);put(b->ram,c.cs,0x2B2A,0x8800);
    put(b->ram,c.ds,0x49ED,0xFFFF);b->ram[p(c.ds,0x84DE)]=0xA5;
    outs=writes=0;refuse=-1;return c;
}
static int run(pr_bus *b,pr_u1c_2b14_cpu *c,size_t limit,size_t rect_limit,pr_u1c_2b14_cpu_diagnostic *d)
{ return pr_u1c_clear_box_2b14_cpu(b,PR_BUS_RAM_SIZE,c,limit,rect_limit,d); }
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*saved=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_u1c_2b14_cpu c,before;pr_u1c_2b14_cpu_diagnostic d;
    unsigned active,plane,at,x;
    CHECK(ram && copy && ega && saved);
    for(active=0;active<8u;++active) {
        c=setup(&bus,1);ram[p(c.ds,0x4A1A)]=(uint8_t)active;
        CHECK(run(&bus,&c,0,119,&d)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8004 && c.bp==0x7777);
        CHECK(word(ram,c.ds,0x49ED)==0 && ram[p(c.ds,0x84DE)]==1);
        CHECK(outs==44 && writes==3600 && d.clear_status==0 && d.clear.callee==2 && d.clear.status==0);
        CHECK(word(ram,c.ss,0x7FF6)==0x120F && word(ram,c.ss,0x7FF8)==0x120F &&
              word(ram,c.ss,0x7FFA)==0x1201 && word(ram,c.ss,0x7FFC)==0x1201);
        for(plane=0;plane<4u;++plane)for(at=0;at<65536u;++at) {
            unsigned rel=at&0x1FFFu,row=rel/40u,col=rel%40u;
            int cleared=at<0x4000u && row>=8u && row<128u && col>=1u && col<=15u;
            CHECK(ega->plane[plane][at]==(cleared?0:0xA5));
        }
    }
    c=setup(&bus,3);before=c;ram[p(c.cs,0x2B27)]=0x90;
    /* Exhaustive INC16 results and defined arithmetic flags at call guard,
     * before any renderer can overwrite them. Independent parity bit count. */
    for(x=0;x<65536u;++x) {
        unsigned v=(x+1u)&65535u,bits=0,j,want;
        c=before;c.flags=(uint16_t)(0x602u|(x&1u));put(ram,c.ds,0x49ED,x);
        for(j=0;j<8u;++j)bits+=(v>>j)&1u;
        want=(c.flags&~0x08D4u)|(bits%2u?0u:4u)|(v==0u?0x40u:0u)|
             (v&0x8000u?0x80u:0u)|(x%16u==15u?0x10u:0u)|(x==32767u?0x800u:0u);
        CHECK(run(&bus,&c,0,0,&d)==2 && c.ip==0x2B27 && c.sp==0x7FF6);
        CHECK(word(ram,c.ds,0x49ED)==v && c.flags==want && ram[p(c.ds,0x84DE)]==0xA5);
    }
    for(x=0;x<3u;++x) {
        c=setup(&bus,x);CHECK(run(&bus,&c,0,0,&d)==3 && ram[p(c.ds,0x84DE)]==0xA5);
        CHECK(d.clear_status==(x==1u?3:6) && word(ram,c.ds,0x49ED)==0);
    }
    c=setup(&bus,1);refuse=40;
    CHECK(run(&bus,&c,256,65536,&d)==3 && d.clear_status==3 && d.clear.callee==2 &&
          d.clear.status==4 && outs==40 && writes==1800 && ram[p(c.ds,0x84DE)]==0xA5);
    for(x=0;x<4u;++x) {
        const unsigned sp[]={0x49EF,0x84E0,0x84DE,0x84DC};
        c=setup(&bus,3);c.ss=c.ds;c.sp=(uint16_t)sp[x];
        put(ram,c.ss,c.sp,0x7000);put(ram,c.ss,c.sp+2u,0x9234);
        CHECK(run(&bus,&c,0,0,&d)==0 && outs==0);
        CHECK(c.bp==(x==0u?0x7778:x==1u?0x7701:0x7777));
        CHECK(c.ip==(x==2u?0x7001:0x7000) && c.cs==(x==3u?0x9201:0x9234));
    }
    c=setup(&bus,1);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(saved,ega,sizeof *ega);
    CHECK(pr_u1c_clear_box_2b14_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,0,&d)==1);
    CHECK(pr_u1c_clear_box_2b14_cpu(&bus,1,&c,0,0,&d)==1);
    CHECK(pr_u1c_clear_box_2b14_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,0,&d)==1);
    for(x=0;x<5u;++x) {
        c=before;if(x==0)c.ip=0x2B15;if(x==1)c.sp=0x4B;if(x==2)c.sp=0xFFFD;if(x==3)c.ds=0xA000;if(x==4)c.ss=0x9FFF;
        {pr_u1c_2b14_cpu rejected=c;CHECK(run(&bus,&c,0,0,&d)==1);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.clear_status==-1 && d.clear.callee==-1);}
    }
    CHECK(memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved,ega,sizeof *ega)==0);
    c=setup(&bus,3);CHECK(run(&bus,&c,0,0,NULL)==0 && ram[p(c.ds,0x84DE)]==1);
    free(saved);free(ega);free(copy);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
