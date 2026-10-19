#include "poolrad/recovered/overlay_u3b0_entry_0c33_cpu.h"
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
    (void)ctx;(void)value;(void)ip;CHECK(size==1 && cs==0x8800);CHECK(port==0x3C4 || port==0x3C5 || port==0x3CE || port==0x3CF || port==0x3D8 || port==0x3D9);
    if(refuse>=0 && outs==(unsigned)refuse)return 1;
    ++outs;return 0;
}
static void write(void *ctx,uint16_t off,uint8_t value) { (void)ctx;(void)off;(void)value;++writes; }
static pr_u3b0_0c33_cpu setup(pr_bus *b,unsigned length,unsigned style,unsigned mode)
{
    pr_u3b0_0c33_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x9000,0xC33,0x1800,0x1111,0x2800,0x602};
    uint8_t *r=b->ram;memset(r,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    put(r,c.ss,c.sp,0x7000);put(r,c.ss,c.sp+2,0x9001);put(r,c.ss,c.sp+4,0x2000);put(r,c.ss,c.sp+6,0);
    farcall(r,c.cs,0x7CA,0x8400,0x137);nearcall(r,c.cs,0x7DE,0);nearcall(r,c.cs,0x7F9,0x711);
    farcall(r,c.cs,0x849,0x8800,0x626);farcall(r,c.cs,0x869,0x8800,0x626);farcall(r,c.cs,0x8B5,0x8800,0x1DF);
    const unsigned fill[]={0x1A5,0x1C7,0x27E,0x29B,0x2B8,0x2D5};
    for(unsigned j=0;j<6;++j)farcall(r,0x8400,fill[j],0x4000,0x16A8);
    farcall(r,0x8400,0x1F1,0x8800,0xB7);farcall(r,0x8400,0x20C,0x8800,0xB7);
    farcall(r,0x8800,0x140,0x4000,0x16A8);
    farcall(r,0x8800,0x207,0x4000,0x182A);farcall(r,0x8800,0x63C,0x4000,0xB25);nearcall(r,0x8800,0x691,0x1DF);
    pair(r,c.ss,0x2014,0x6000,0x1000);r[p(c.ss,0x201C)]=3;r[p(c.ss,0x201E)]=2;r[p(c.ss,0x2022)]=10;r[p(c.ss,0x2024)]=5;r[p(c.ss,0x2018)]=5;r[p(c.ss,0x201A)]=9;r[p(c.ss,0x1FAC)]=8;r[p(c.ds,0x4A1A)]=1;
    for(unsigned n=0;n<6;++n) {
        unsigned at=0x1000+n*0x200;r[p(0x6000,at)]=(uint8_t)length;
        for(unsigned j=0;j<length;++j)r[p(0x6000,at+1+j)]=(uint8_t)(style==0?' ':style==1?'A':style==2?(j<length/2?' ':'a'):(j<length/2?'a':' '));
        r[p(0x6000,at+0x29)]=0;
        pair(r,0x6000,at+0x2A,n<5?0x6000:0,n<5?at+0x200:0);
    }
    r[0]=0;r[p(c.ds,0x4A1E)]=(uint8_t)mode;put(r,c.ds,0x4A16,0xA000);put(r,c.ds,0x4A18,0xA200);
    pair(r,c.ds,0x5E20,0x7000,0x1000);
    for(unsigned i=0;i<4;++i)r[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    nearcall(r,c.cs,0xCB1,0xB03);nearcall(r,c.cs,0xCBC,0x7A4);
    pair(r,c.ss,0x200A,0x6000,0x8000);put(r,0x6000,0x8000,0);put(r,c.ds,0x6B29,0);
    put(r,c.ss,0x1FA7,2);put(r,c.ss,0x1FAA,6);put(r,c.ss,c.sp+6,1);
    r[p(c.ss,0x2018)]=4;r[p(c.ss,0x1FAC)]=3;
    memset(r+p(0x7000,0x1000),0xFF,2048);outs=writes=0;refuse=-1;return c;
}
static unsigned expected_base(unsigned top,unsigned page,unsigned total,unsigned direction) {
    unsigned moved=(direction?(top+page):(top-page))&65535u;
    if(direction) {
        unsigned ceiling=(total-page)&65535u;
        if((int16_t)moved>(int16_t)ceiling)moved=ceiling;
    } else if((int16_t)moved<0)moved=0;
    return moved;
}
int main(void)
{
    uint8_t *r=malloc(PR_BUS_RAM_SIZE),*saved=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*old=malloc(sizeof *old);pr_port_env env={NULL,out,NULL};pr_bus b={r,ega,&env,write,NULL};
    pr_u3b0_0c33_limits l={65536,{65536,65536,65536,65536,65536}};pr_u3b0_0c33_diagnostic d;pr_u3b0_0c33_cpu c,before;
    CHECK(r && saved && ega && old);
    const unsigned words[]={0,1,2,5,6,0x7FFE,0x7FFF,0x8000,0x8001,0xFFFE,0xFFFF};
    for(unsigned i=0;i<11;++i)for(unsigned j=0;j<11;++j)for(unsigned k=0;k<11;++k)for(unsigned direction=0;direction<2;++direction) {
        unsigned top=words[i],page=words[j],total=words[k],cursor=(top+3)&65535u;
        c=setup(&b,2,1,3);put(r,c.ds,0x6B29,top);put(r,c.ss,0x1FA7,page);put(r,c.ss,0x1FAA,total);put(r,0x6000,0x8000,cursor);put(r,c.ss,c.sp+6,direction);
        r[p(c.cs,0xCB1)]=0x90;
        CHECK(pr_u3b0_entry_0c33_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==2 && c.ip==0xCB1);
        unsigned want=expected_base(top,page,total,direction);
        CHECK(word(r,c.ds,0x6B29)==want && word(r,0x6000,0x8000)==((want+3)&65535u));
        CHECK(word(r,c.ss,0x7FFC)==3 && c.sp==0x7FF6 && word(r,c.ss,c.sp)==c.cs);
        CHECK(d.callee==-1 && outs==0 && writes==0);
    }
    for(unsigned direction=0;direction<256;++direction) {
        c=setup(&b,2,1,3);put(r,c.ds,0x6B29,3);put(r,0x6000,0x8000,4);put(r,c.ss,c.sp+6,0xFF00|direction);
        CHECK(pr_u3b0_entry_0c33_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
        CHECK(word(r,c.ds,0x6B29)==(direction?4u:1u) && word(r,0x6000,0x8000)==(direction?5u:2u));
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8008 && c.bp==0x7777 && outs==0 && writes==0);
    }
    c=setup(&b,2,1,3);pair(r,c.ss,0x200A,c.ds,0x6B29);put(r,c.ds,0x6B29,3);r[p(c.cs,0xCB1)]=0x90;
    CHECK(pr_u3b0_entry_0c33_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==2 && word(r,c.ds,0x6B29)==4 && word(r,c.ss,0x7FFC)==0);
    c=setup(&b,2,1,3);pair(r,c.ss,0x200A,c.ss,0x7FFC);put(r,c.ss,0x7FFC,5);put(r,c.ds,0x6B29,1);r[p(c.cs,0xCB1)]=0x90;
    CHECK(pr_u3b0_entry_0c33_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==2 && word(r,c.ds,0x6B29)==3 && word(r,c.ss,0x7FFC)==7);
    c=setup(&b,1,1,1);r[p(0x6000,0x1629)]=1;
    CHECK(pr_u3b0_entry_0c33_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0 && outs>0 && writes>0);
    CHECK(word(r,c.ds,0x6B29)==2 && word(r,0x6000,0x8000)==2);
    for(unsigned plane=0;plane<4;++plane)for(unsigned page=0;page<2;++page)
    for(unsigned row=3;row<=4;++row)for(unsigned y=0;y<8;++y) {
        unsigned at=row*320+y*40+page*0x2000,color=row==4?5:10;
        CHECK(ega->plane[plane][at+2]==((color&(1u<<plane))?255:0));
        for(unsigned x=3;x<=9;++x)CHECK(ega->plane[plane][at+x]==0);
        CHECK(ega->plane[plane][at+1]==0xA5 && ega->plane[plane][at+10]==0xA5);
    }
    c=setup(&b,2,1,3);r[p(0x6000,0x1429)]=1;l.step_loopbacks=0;
    CHECK(pr_u3b0_entry_0c33_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.callee==0 && d.status==4 && d.step_status==0 && c.ip==0xB17);
    CHECK(word(r,c.ds,0x6B29)==2 && word(r,0x6000,0x8000)==3);l.step_loopbacks=65536;
    c=setup(&b,2,1,3);l.display.row_loopbacks=0;
    CHECK(pr_u3b0_entry_0c33_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.callee==1 && d.status==5 && c.ip==0x8CE);l.display.row_loopbacks=65536;
    c=setup(&b,2,1,3);r[p(c.cs,0xCBC)]=0x90;
    CHECK(pr_u3b0_entry_0c33_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==2 && c.ip==0xCBC && d.callee==0 && d.status==0 && word(r,c.ss,c.sp)==c.cs);
    c=setup(&b,2,1,1);refuse=0;
    CHECK(pr_u3b0_entry_0c33_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.status==3 && d.display.status==3 && d.display.clear.status==3 && d.display.clear.rect.status==2 && outs==0 && writes==0);
    c=setup(&b,2,1,1);before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    for(unsigned n=0;n<7;++n) {
        c=before;if(n==0)c.ip=0;if(n==1)c.sp=0x31F;if(n==2)c.sp=0xFFF5;if(n==3)c.ds=0xA000;if(n==4)c.ss=0x9001;
        pr_u3b0_0c33_cpu rejected=c;
        CHECK(pr_u3b0_entry_0c33_cpu(n==5?NULL:&b,n==6?1:PR_BUS_RAM_SIZE,&c,&l,&d)==1);
        CHECK(memcmp(&c,&rejected,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    }
    free(old);free(ega);free(saved);free(r);printf("%u semantic checks passed\n",checks);return 0;
}
