#include "poolrad/recovered/overlay_u3b0_entry_0969_cpu.h"
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
static void pair(uint8_t *r,unsigned s,unsigned o,unsigned seg,unsigned off) { put(r,s,o,off);put(r,s,o+2,seg); }
static int out(void *ctx,uint16_t port,uint16_t value,unsigned size,uint16_t cs,uint16_t ip) {
    (void)ctx;(void)value;(void)ip;CHECK(size==1 && cs==0x8800);CHECK(port==0x3C4 || port==0x3C5);
    if(refuse>=0 && outs==(unsigned)refuse)return 1;
    ++outs;return 0;
}
static void write(void *ctx,uint16_t off,uint8_t value) { (void)ctx;(void)off;(void)value;++writes; }
static pr_u3b0_0969_cpu setup(pr_bus *b,unsigned length,unsigned style,unsigned mode)
{
    pr_u3b0_0969_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x9000,0x969,0x1800,0x1111,0x2800,0x602};
    uint8_t *r=b->ram;memset(r,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    put(r,c.ss,c.sp,0x7000);put(r,c.ss,c.sp+2,0x9001);put(r,c.ss,c.sp+4,0x2000);put(r,c.ss,c.sp+6,0);
    nearcall(r,c.cs,0x97F,0);nearcall(r,c.cs,0x991,0x8D7);nearcall(r,c.cs,0x9A6,0x923);
    farcall(r,c.cs,0x9F8,0x4000,0xB57);farcall(r,c.cs,0x9FD,0x8800,0x626);
    farcall(r,c.cs,0x8EB,0x4000,0xB25);farcall(r,c.cs,0x937,0x4000,0xB25);
    farcall(r,0x8800,0x207,0x4000,0x182A);farcall(r,0x8800,0x63C,0x4000,0xB25);nearcall(r,0x8800,0x691,0x1DF);
    pair(r,c.ss,0x2014,0x6000,0x1000);r[p(c.ss,0x201C)]=3;r[p(c.ss,0x201E)]=2;r[p(c.ss,0x2020)]=10;put(r,c.ds,0x6B29,0);
    for(unsigned n=0;n<3;++n) {
        unsigned at=0x1000+n*0x200;r[p(0x6000,at)]=(uint8_t)length;
        for(unsigned j=0;j<length;++j)r[p(0x6000,at+1+j)]=(uint8_t)(style==0?' ':style==1?'A':style==2?(j<length/2?' ':'a'):(j<length/2?'a':' '));
        pair(r,0x6000,at+0x2A,n<2?0x6000:0,n<2?at+0x200:0);
    }
    r[0]=0;r[p(c.ds,0x4A1E)]=(uint8_t)mode;put(r,c.ds,0x4A16,0xA000);put(r,c.ds,0x4A18,0xA200);
    pair(r,c.ds,0x5E20,0x7000,0x1000);
    for(unsigned i=0;i<4;++i)r[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    memset(r+p(0x7000,0x1000),0xFF,2048);outs=writes=0;refuse=-1;return c;
}
int main(void)
{
    uint8_t *r=malloc(PR_BUS_RAM_SIZE),*saved=malloc(PR_BUS_RAM_SIZE),expected[256];
    pr_ega *ega=malloc(sizeof *ega),*old=malloc(sizeof *old);pr_port_env env={NULL,out,NULL};pr_bus b={r,ega,&env,write,NULL};
    pr_u3b0_0969_limits l={65536,65536};pr_u3b0_0969_diagnostic d;pr_u3b0_0969_cpu c,before;
    CHECK(r && saved && ega && old);
    for(unsigned length=0;length<256;++length)for(unsigned style=0;style<4;++style) {
        c=setup(&b,length,style,3);
        unsigned n=length<40?length:40,lead=1,trail=n;
        while(lead<n && r[p(0x6000,0x1000+lead)]==' ')++lead;
        while(trail>1 && r[p(0x6000,0x1000+trail)]==' ')--trail;
        unsigned count=(trail-lead+1)&255u;
        if(length<lead)count=0;else if(count>length-lead+1)count=length-lead+1;
        for(unsigned j=0;j<count;++j)expected[j]=r[p(0x6000,0x1000+lead+j)];
        CHECK(pr_u3b0_entry_0969_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8008 && c.bp==0x7777);
        CHECK(r[p(c.ss,0x7EF8)]==count && r[p(c.ss,0x7FF9)]==lead);
        for(unsigned j=0;j<count;++j)CHECK(r[p(c.ss,0x7EF9+j)]==expected[j]);
        CHECK(outs==0 && writes==0 && d.callee==4 && d.status==0 && d.scan_store_status==0);
    }
    for(unsigned index=0;index<5;++index) {
        c=setup(&b,1,1,3);put(r,c.ss,c.sp+6,index);
        CHECK(pr_u3b0_entry_0969_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
        CHECK(word(r,c.ss,0x7FFA)==(index<3?0x1000+index*0x200:0));
        CHECK(word(r,c.ss,0x7FFC)==(index<3?0x6000:0));
        CHECK(r[p(c.ss,0x7EF8)]==(index<3?1:0));
    }
    c=setup(&b,1,1,1);CHECK(pr_u3b0_entry_0969_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
    CHECK(outs==34 && writes==64);
    for(unsigned plane=0;plane<4;++plane)for(unsigned page=0;page<2;++page)for(unsigned y=0;y<8;++y) {
        unsigned at=3*320+2+y*40+page*0x2000;
        CHECK(ega->plane[plane][at]==((10u&(1u<<plane))?255:0));
        CHECK(ega->plane[plane][at-1]==0xA5 && ega->plane[plane][at+1]==0xA5);
    }
    c=setup(&b,2,1,1);l.printer_loopbacks=0;
    CHECK(pr_u3b0_entry_0969_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.status==6 && c.ip==0x69F && writes==64);l.printer_loopbacks=65536;
    c=setup(&b,2,1,1);refuse=0;
    CHECK(pr_u3b0_entry_0969_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.status==3 && d.printer.status==4 && outs==0 && writes==0);
    c=setup(&b,2,1,1);r[p(c.cs,0x97F)]=0x90;
    CHECK(pr_u3b0_entry_0969_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==2 && c.ip==0x97F && c.sp==0x7EF0 && word(r,c.ss,c.sp)==c.cs && d.callee==-1);
    c=setup(&b,2,1,1);r[p(c.cs,0x8EB)]=0x90;
    CHECK(pr_u3b0_entry_0969_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.status==2 && d.callee==1 && d.scan_store_status==-1);
    c=setup(&b,2,1,1);before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    for(unsigned n=0;n<7;++n) {
        c=before;if(n==0)c.ip=0;if(n==1)c.sp=0x2FF;if(n==2)c.sp=0xFFF5;if(n==3)c.ds=0xA000;if(n==4)c.ss=0x9001;
        pr_u3b0_0969_cpu rejected=c;
        CHECK(pr_u3b0_entry_0969_cpu(n==5?NULL:&b,n==6?1:PR_BUS_RAM_SIZE,&c,&l,&d)==1);
        CHECK(memcmp(&c,&rejected,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    }
    free(old);free(ega);free(saved);free(r);printf("%u semantic checks passed\n",checks);return 0;
}
