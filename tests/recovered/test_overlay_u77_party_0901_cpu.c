#include "poolrad/recovered/overlay_u77_party_0901_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,outs,writes;
static int refuse=-1;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned s,unsigned o) { return ((size_t)s*16u+(o&65535u))&1048575u; }
static void put(uint8_t *r,unsigned s,unsigned o,unsigned v) { size_t a=p(s,o);r[a]=(uint8_t)v;r[(a+1)&1048575u]=(uint8_t)(v>>8); }
static void farcall(uint8_t *r,unsigned s,unsigned o,unsigned seg,unsigned ip) { r[p(s,o)]=0x9A;put(r,s,o+1,ip);put(r,s,o+3,seg); }
static void pair(uint8_t *r,unsigned s,unsigned o,unsigned seg,unsigned off) { put(r,s,o,off);put(r,s,o+2,seg); }
static int out(void *ctx,uint16_t port,uint16_t value,unsigned size,uint16_t cs,uint16_t ip) {
    (void)ctx;(void)value;(void)ip;CHECK(size==1 && cs==0x8800);
    CHECK(port>=0x3C0 && port<=0x3DF);
    if(refuse>=0 && outs==(unsigned)refuse)return 1;
    ++outs;return 0;
}
static void write(void *ctx,uint16_t off,uint8_t value) { (void)ctx;(void)off;(void)value;++writes; }
static pr_u77_0901_cpu setup(pr_bus *b,unsigned value,unsigned ext,unsigned row,unsigned mode)
{
    static const unsigned sites[]={0x933,0x971,0x976,0x9AF,0x9BD,0x9D8,0x9E2,0x9F0,0xA0E,0xA28,0xA32,0xA3C,0xA41};
    static const unsigned off[]={0x137,0x1037,0x626,0x1037,0xB25,0xB0B,0xB98,0xB25,0xB25,0xB0B,0xB98,0xB98,0x626};
    static const unsigned seg[]={0x8000,0x8200,0x8800,0x8200,0x4000,0x4000,0x4000,0x4000,0x4000,0x4000,0x4000,0x4000,0x8800};
    static const unsigned fills[]={0x1A5,0x1C7,0x27E,0x29B,0x2B8,0x2D5};
    static const uint8_t literal[]={1,' ',2,'X','X',1,'[',1,']'};
    pr_u77_0901_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x9000,0x901,0x1800,0x1111,0x2800,0x602};
    uint8_t *r=b->ram;memset(r,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    put(r,c.ss,c.sp,0x7000);put(r,c.ss,c.sp+2,0x9001);put(r,c.ss,c.sp+4,0xA500);put(r,c.ss,c.sp+6,0xA500|row);
    for(unsigned i=0;i<13;++i)farcall(r,c.cs,sites[i],seg[i],off[i]);
    for(unsigned i=0;i<6;++i)farcall(r,0x8000,fills[i],0x4000,0x16A8);
    farcall(r,0x8000,0x1F1,0x8800,0xB7);farcall(r,0x8000,0x20C,0x8800,0xB7);
    farcall(r,0x8800,0x140,0x4000,0x16A8);farcall(r,0x8800,0x207,0x4000,0x182A);farcall(r,0x8800,0x63C,0x4000,0xB25);
    r[p(0x8800,0x691)]=0xE8;put(r,0x8800,0x692,0xFB4B);
    farcall(r,0x8200,0x1054,0x4000,0x1366);farcall(r,0x8200,0x1068,0x4000,0xB25);
    for(unsigned i=0,v=1000000000u;i<10;++i,v/=10) { put(r,0x4000,0x12A6+4*i,v);put(r,0x4000,0x12A8+4*i,v>>16); }
    memcpy(r+p(c.cs,0x8F8),literal,sizeof literal);
    pair(r,c.ds,0x5D92,0x6000,0x1000);r[p(0x6000,0x1010+row)]=(uint8_t)value;r[p(0x6000,0x1016)]=(uint8_t)ext;
    r[p(c.ds,0x4A1E)]=(uint8_t)mode;r[p(c.ds,0x4A1A)]=1;
    put(r,c.ds,0x4A16,0xA000);put(r,c.ds,0x4A18,0xA200);pair(r,c.ds,0x5E20,0x7000,0x1000);
    for(unsigned i=0;i<4;++i)r[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    memset(r+p(0x7000,0x1000),0xFF,2048);
    outs=writes=0;refuse=-1;return c;
}
static void string_is(const uint8_t *r,unsigned s,unsigned off,const char *wanted) {
    CHECK(r[p(s,off)]==strlen(wanted));
    for(unsigned i=0;i<strlen(wanted);++i)CHECK(r[p(s,off+1+i)]==(uint8_t)wanted[i]);
}
int main(void)
{
    uint8_t *r=malloc(PR_BUS_RAM_SIZE),*saved=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*old=malloc(sizeof *old);pr_port_env env={NULL,out,NULL};pr_bus b={r,ega,&env,write,NULL};
    pr_u77_0901_limits l={65536,65536,65536,65536};pr_u77_0901_diagnostic d;pr_u77_0901_cpu c,before;char expected[32];
    CHECK(r && saved && ega && old);
    for(unsigned v=0;v<256;++v) {
        c=setup(&b,v,0,0,3);CHECK(pr_u77_party_0901_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8008 && c.bp==0x7777);
        snprintf(expected,sizeof expected,"%u",v);string_is(r,c.ss,0x7ED3,expected);
        CHECK(outs==0 && writes==0 && d.callee==PR_U77_0901_PRINTER && d.status==0);
    }
    for(unsigned ext=1;ext<256;++ext) {
        c=setup(&b,18,ext,0,3);CHECK(pr_u77_party_0901_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
        if(ext==100)strcpy(expected,"[XX]");else snprintf(expected,sizeof expected,ext<10?"[ %u]":"[%u]",ext);
        string_is(r,c.ss,0x7ED3,expected);CHECK(c.sp==0x8008 && outs==0 && writes==0);
    }
    c=setup(&b,18,1,1,3);CHECK(pr_u77_party_0901_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);string_is(r,c.ss,0x7ED3,"18");
    c=setup(&b,8,0,0,1);CHECK(pr_u77_party_0901_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
    CHECK(outs>0 && writes>0);
    for(unsigned plane=0;plane<4;++plane)for(unsigned page=0;page<2;++page)for(unsigned y=0;y<8;++y) {
        unsigned at=7*320+y*40+page*0x2000;
        CHECK(ega->plane[plane][at+6]==((10u&(1u<<plane))?255:0));
        CHECK(ega->plane[plane][at+5]==0 && ega->plane[plane][at+7]==0);
        CHECK(ega->plane[plane][at+4]==0xA5 && ega->plane[plane][at+12]==0xA5);
    }
    c=setup(&b,18,1,0,1);refuse=0;
    CHECK(pr_u77_party_0901_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.callee==0 && d.status==3 && d.clear.rect.status==2 && outs==0);
    c=setup(&b,18,1,0,1);l.printer_loopbacks=0;
    CHECK(pr_u77_party_0901_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.callee==2 && d.status==6 && c.ip==0x69F && writes>0);l.printer_loopbacks=65536;
    c=setup(&b,18,1,0,1);r[p(c.cs,0x971)]=0x90;
    CHECK(pr_u77_party_0901_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==2 && c.ip==0x971 && d.callee==0 && writes>0);
    c=setup(&b,18,1,0,3);r[p(0x4000,0x12A6)]=1;
    CHECK(pr_u77_party_0901_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.callee==1 && d.status==3 && d.decimal_status==1);
    c=setup(&b,18,1,0,1);before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    for(unsigned n=0;n<7;++n) {
        c=before;if(n==0)c.ip=0;if(n==1)c.sp=0x2FF;if(n==2)c.sp=0xFFF5;if(n==3)c.ds=0xA000;if(n==4)c.ss=0x9001;
        pr_u77_0901_cpu rejected=c;
        CHECK(pr_u77_party_0901_cpu(n==5?NULL:&b,n==6?1:PR_BUS_RAM_SIZE,&c,&l,&d)==1);
        CHECK(memcmp(&c,&rejected,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    }
    free(old);free(ega);free(saved);free(r);printf("%u semantic checks passed\n",checks);return 0;
}
