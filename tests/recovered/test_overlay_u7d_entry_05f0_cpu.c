#include "poolrad/recovered/overlay_u7d_entry_05f0_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,outs,writes;
static int refuse=-1;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned s,unsigned o) { return ((size_t)s*16u+(o&65535u))&1048575u; }
static void put(uint8_t *r,unsigned s,unsigned o,unsigned v) { size_t a=p(s,o);r[a]=(uint8_t)v;r[(a+1)&1048575u]=(uint8_t)(v>>8); }
static unsigned word(const uint8_t *r,unsigned s,unsigned o) { size_t a=p(s,o);return r[a]|((unsigned)r[(a+1)&1048575u]<<8); }
static void farcall(uint8_t *r,unsigned s,unsigned o,unsigned seg,unsigned ip) { r[p(s,o)]=0x9A;put(r,s,o+1,ip);put(r,s,o+3,seg); }
static void nearcall(uint8_t *r,unsigned s,unsigned o,unsigned ip) { r[p(s,o)]=0xE8;put(r,s,o+1,ip-o-3u); }
static int out(void *ctx,uint16_t port,uint16_t value,unsigned size,uint16_t cs,uint16_t ip) {
    (void)ctx;(void)value;(void)ip;CHECK(size==1 && cs==0x8800);CHECK(port==0x3C4 || port==0x3C5);
    if(refuse>=0 && outs==(unsigned)refuse)return 1;
    ++outs;return 0;
}
static void write(void *ctx,uint16_t off,uint8_t value) { (void)ctx;(void)off;(void)value;++writes; }
static pr_u7d_05f0_cpu setup(pr_bus *b,unsigned value,unsigned selection,unsigned mode)
{
    pr_u7d_05f0_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x9000,0x5F0,0x1800,0x1111,0x2800,0x602};
    uint8_t *r=b->ram;memset(r,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    put(r,c.ss,c.sp,0x7000);put(r,c.ss,c.sp+2,0x9001);put(r,c.ss,c.sp+4,selection);
    nearcall(r,c.cs,0x674,0x565);nearcall(r,c.cs,0x6CA,0x565);
    farcall(r,c.cs,0x633,0x4000,0xB0B);farcall(r,c.cs,0x695,0x4000,0xB0B);
    const unsigned printing[]={0x638,0x677,0x69A,0x6CD};
    for(unsigned j=0;j<4;++j)farcall(r,c.cs,printing[j],0x8800,0x626);
    const unsigned sites[]={0x576,0x584,0x59A,0x5A4,0x5B2,0x5CA,0x5D8},targets[]={0x1037,0xB25,0xB0B,0xB98,0xB25,0xB57,0xB25};
    for(unsigned j=0;j<7;++j)farcall(r,c.cs,sites[j],j==0?0x8400:0x4000,targets[j]);
    farcall(r,0x8400,0x1054,0x4000,0x1366);farcall(r,0x8400,0x1068,0x4000,0xB25);
    farcall(r,0x8800,0x207,0x4000,0x182A);farcall(r,0x8800,0x63C,0x4000,0xB25);nearcall(r,0x8800,0x691,0x1DF);
    r[p(c.cs,0x563)]=1;r[p(c.cs,0x564)]='0';r[p(c.cs,0x5E3)]=10;memcpy(r+p(c.cs,0x5E4),"SYNTHETIC!",10);
    r[p(c.cs,0x5EE)]=1;r[p(c.cs,0x5EF)]=':';
    uint32_t power=1000000000;
    for(unsigned j=0;j<10;++j) { put(r,0x4000,0x12A6+4*j,power&65535u);put(r,0x4000,0x12A8+4*j,power>>16);power/=10; }
    put(r,c.ds,0x6DC4,value);put(r,c.ds,0x6DC6,0);put(r,c.ds,0x6DC8,value);put(r,c.ds,0x6DCA,value);
    r[p(c.ds,0x4A1E)]=(uint8_t)mode;put(r,c.ds,0x4A16,0xA000);put(r,c.ds,0x4A18,0xA200);
    put(r,c.ds,0x5E20,0x1000);put(r,c.ds,0x5E22,0x7000);
    for(unsigned i=0;i<4;++i)r[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    memset(r+p(0x7000,0x1000),0xFF,2048);outs=writes=0;refuse=-1;return c;
}
int main(void)
{
    uint8_t *r=malloc(PR_BUS_RAM_SIZE),*saved=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*old=malloc(sizeof *old);pr_port_env env={NULL,out,NULL};pr_bus b={r,ega,&env,write,NULL};
    pr_u7d_05f0_limits l={65536,65536,65536};pr_u7d_05f0_diagnostic d;pr_u7d_05f0_cpu c,before;
    CHECK(r && saved && ega && old);
    for(unsigned value=0;value<256;++value)for(unsigned selection=0;selection<7;++selection) {
        c=setup(&b,value,selection,3);
        CHECK(pr_u7d_entry_05f0_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8006 && c.bp==0x7777);
        unsigned a=value<10?0:value<100?value/10:value/100,bv=value<100?value%10:(value/10)%10;
        CHECK(r[p(c.ss,0x7EEE)]==2 && r[p(c.ss,0x7EEF)]=='0'+a && r[p(c.ss,0x7EF0)]=='0'+bv);
        for(unsigned j=0;j<7;++j)CHECK(word(r,c.ss,0x7FEF+2*j)==(j==selection?15:10));
        CHECK(r[p(c.ss,0x7FFD)]==3 && r[p(c.ss,0x7FEE)]==17 && outs==0 && writes==0);
    }
    for(unsigned selection=0;selection<256;++selection) {
        c=setup(&b,12,selection,3);r[p(c.cs,0x633)]=0x90;
        CHECK(pr_u7d_entry_05f0_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==2 && c.ip==0x633);
        int index=selection<128?(int)selection:(int)selection-256;
        CHECK(word(r,c.ss,(unsigned)(0x7FFE + 2*index - 15))==15);
    }
    for(unsigned selection=2;selection<=4;++selection) {
        c=setup(&b,12,selection,1);CHECK(pr_u7d_entry_05f0_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
        CHECK(outs==18*34 && writes==18*64);
        for(unsigned plane=0;plane<4;++plane)for(unsigned page=0;page<2;++page)for(unsigned y=0;y<8;++y) {
            unsigned at=17*320+y*40+page*0x2000;
            for(unsigned x=0;x<=20;++x) {
                unsigned color=((selection==4 && (x==12 || x==13)) || (selection==3 && (x==15 || x==16)) || (selection==2 && (x==18 || x==19)))?15:10;
                unsigned expected=(x==0 || x==11 || x==20)?0xA5:(color&(1u<<plane))?255:0;
                CHECK(ega->plane[plane][at+x]==expected);
            }
        }
    }
    c=setup(&b,12,2,3);l.caller_loopbacks=0;
    CHECK(pr_u7d_entry_05f0_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==5 && c.ip==0x611 && word(r,c.ss,0x7FEF)==10 && r[p(c.ss,0x7FFD)]==0);l.caller_loopbacks=6;
    c=setup(&b,12,2,3);CHECK(pr_u7d_entry_05f0_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==5 && c.ip==0x6A7 && r[p(c.ss,0x7FFD)]==4);l.caller_loopbacks=65536;
    c=setup(&b,12,2,3);r[p(0x4000,0x12A7)]=0;
    CHECK(pr_u7d_entry_05f0_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.callee==1 && d.status==3 && d.format_callee==0 && d.format_status==3);
    c=setup(&b,12,2,1);refuse=0;
    CHECK(pr_u7d_entry_05f0_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.status==3 && d.printer.status==4 && outs==0 && writes==0);
    c=setup(&b,12,2,1);before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    for(unsigned n=0;n<7;++n) {
        c=before;if(n==0)c.ip=0;if(n==1)c.sp=0x4FF;if(n==2)c.sp=0xFFF7;if(n==3)c.ds=0xA000;if(n==4)c.ss=0x9001;
        pr_u7d_05f0_cpu rejected=c;
        CHECK(pr_u7d_entry_05f0_cpu(n==5?NULL:&b,n==6?1:PR_BUS_RAM_SIZE,&c,&l,&d)==1);
        CHECK(memcmp(&c,&rejected,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    }
    free(old);free(ega);free(saved);free(r);printf("%u semantic checks passed\n",checks);return 0;
}
