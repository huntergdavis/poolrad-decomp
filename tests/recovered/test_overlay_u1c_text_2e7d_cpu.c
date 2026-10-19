#include "poolrad/recovered/overlay_u1c_text_2e7d_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,outs,writes;
static int refuse=-1;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned s,unsigned o) { return ((size_t)s*16u+(o&65535u))&1048575u; }
static void put(uint8_t *r,unsigned s,unsigned o,unsigned v) { size_t a=p(s,o);r[a]=(uint8_t)v;r[(a+1)&1048575u]=(uint8_t)(v>>8); }
static void pair(uint8_t *r,unsigned s,unsigned o,unsigned seg,unsigned off) { put(r,s,o,off);put(r,s,o+2,seg); }
static unsigned word(const uint8_t *r,unsigned s,unsigned o) { size_t a=p(s,o);return r[a]|((unsigned)r[(a+1)&1048575u]<<8); }
static int out(void *ctx,uint16_t port,uint16_t value,unsigned size,uint16_t cs,uint16_t ip) {
    (void)ctx;(void)value;CHECK(size==1 && cs==0x8800 && ip>=0x1DF && ip<0x5EB);
    CHECK(port==0x3C4 || port==0x3C5);
    if(refuse>=0 && outs==(unsigned)refuse)return 1;
    ++outs;return 0;
}
static void write(void *ctx,uint16_t offset,uint8_t value) { (void)ctx;(void)offset;(void)value;++writes; }
static pr_u1c_2e7d_cpu setup(pr_bus *b,unsigned value,unsigned term)
{
    static const unsigned sites[]={0x2E8F,0x2E9C,0x2EA9,0x2EE9,0x2F0D};
    static const unsigned off[]={0x16A8,0x2A,0x4D,0x57,0x1DF},seg[]={0xAF8,0x2B,0x2B,0x2B,0x8800};
    static const unsigned stub[]={0x2A,0x4D,0x57},entry[]={0,0x6AF,0xE0C};
    pr_u1c_2e7d_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x9000,0x2E7D,0x1800,0x1111,0x2800,0x602};
    uint8_t *r=b->ram;memset(r,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    put(r,c.ss,c.sp,0x7000);put(r,c.ss,c.sp+2,0x9001);
    for(unsigned i=0;i<5;++i) { r[p(c.cs,sites[i])]=0x9A;put(r,c.cs,sites[i]+1,off[i]);put(r,c.cs,sites[i]+3,seg[i]); }
    for(unsigned i=0;i<3;++i) { r[p(0x2B,stub[i])]=0xEA;put(r,0x2B,stub[i]+1,entry[i]);put(r,0x2B,stub[i]+3,0x4000); }
    pair(r,c.ds,0x49D2,0x6000,0x1000);pair(r,c.ds,0x49DE,0x6300,0x1000);put(r,c.ds,0x49ED,0x9900);
    r[p(0x6300,0x1001)]=2;r[p(0x6300,0x1002)]=0;r[p(0x6300,0x1003)]=0x49;r[p(0x6300,0x1004)]=0x7E;
    put(r,0x6000,0x1000,value);put(r,0x6000,0x1002,term);
    r[p(c.ds,0x4A1E)]=1;put(r,c.ds,0x4A16,0xA000);put(r,c.ds,0x4A18,0xA200);
    pair(r,c.ds,0x5E20,0x7000,0x1000);r[p(c.ds,0x5E24)]=2;r[p(c.ds,0x5E25)]=3;r[p(c.ds,0x84DE)]=0;
    for(unsigned i=0;i<4;++i)r[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    for(unsigned i=0;i<2048;++i)r[p(0x7000,0x1000+i)]=(uint8_t)((i*37u+(i>>3)*13u)^0xA6u);
    outs=writes=0;refuse=-1;return c;
}
int main(void)
{
    uint8_t *r=malloc(PR_BUS_RAM_SIZE),*saved=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*old=malloc(sizeof *old);
    pr_port_env env={NULL,out,NULL};pr_bus b={r,ega,&env,write,NULL};
    pr_u1c_2e7d_limits l={0xAF8,64,4000,512,65536};pr_u1c_2e7d_diagnostic d;pr_u1c_2e7d_cpu c,before;
    CHECK(r && saved && ega && old);
    for(unsigned v=0;v<256;++v) {
        c=setup(&b,v,0);
        CHECK(pr_u1c_text_2e7d_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
        unsigned drawn=v>0 && v<255;
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8004 && c.bp==0x7777);
        CHECK(r[p(c.ds,0x5E24)]==2+drawn && r[p(c.ds,0x5E25)]==4);
        CHECK(outs==34*drawn && writes==64*drawn);
        CHECK(word(r,c.ds,0x84C8)==0 && r[p(c.ds,0x84E0)]==0 && r[p(c.ds,0x84DF)]==1);
        CHECK(d.callee==PR_U1C_2E7D_LOAD_WORD && d.status==0 && d.glyph_callee==-1);
        if(drawn && v%8==1)for(unsigned plane=0;plane<4;++plane)for(unsigned row=0;row<8;++row)for(unsigned page=0;page<2;++page) {
            unsigned at=3*320+2+row*40+page*0x2000;
            unsigned fi=8*(v+1)+row;
            CHECK(ega->plane[plane][at]==(uint8_t)((fi*37u+(fi>>3)*13u)^0xA6u));
            CHECK(ega->plane[plane][at-1]==0xA5 && ega->plane[plane][at+1]==0xA5);
        }
    }
    c=setup(&b,64,0);r[p(c.ds,0x84DE)]=1;r[p(c.ds,0x5E25)]=14;r[p(c.ds,0x5E24)]=20;
    CHECK(pr_u1c_text_2e7d_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
    CHECK(r[p(c.ds,0x5E24)]==3 && r[p(c.ds,0x5E25)]==6 && r[p(c.ds,0x84E2)]==1);
    c=setup(&b,64,0);l.max_loopbacks=0;
    CHECK(pr_u1c_text_2e7d_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==PR_U1C_2E7D_LIMIT);
    CHECK(c.cs==0x9000 && c.ip==0x2F23 && outs==34 && writes==64);
    CHECK(r[p(c.ds,0x5E24)]==3 && r[p(c.ds,0x5E25)]==3 && r[p(c.ss,c.bp-5)]==1);
    l.max_loopbacks=512;
    c=setup(&b,64,0);refuse=1;
    CHECK(pr_u1c_text_2e7d_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==PR_U1C_2E7D_CALLEE);
    CHECK(d.callee==4 && d.status==4 && c.cs==0x8800 && c.ip==0x3D5 && outs==1);
    CHECK(r[p(c.ds,0x5E24)]==2 && r[p(c.ds,0x5E25)]==3);
    c=setup(&b,64,0);l.max_glyph_loopbacks=0;
    CHECK(pr_u1c_text_2e7d_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.status==6 && c.ip==0x45B && writes==2);
    l.max_glyph_loopbacks=65536;
    c=setup(&b,64,0);r[p(c.cs,0x2EE9)]=0x90;
    CHECK(pr_u1c_text_2e7d_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==2 && c.ip==0x2EE9 && outs==0);
    CHECK(d.callee==2 && d.status==0);
    /* A changed video mode via the saved-BP write must be observed live. */
    c=setup(&b,64,0);c.ss=c.ds;c.sp=0x4A20;put(r,c.ss,c.sp,0x7000);put(r,c.ss,c.sp+2,0x9001);
    CHECK(pr_u1c_text_2e7d_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0 && outs==0 && writes==0);
    CHECK(r[p(c.ds,0x4A1E)]==0x77 && r[p(c.ds,0x5E24)]==3);
    c=setup(&b,64,0);before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    for(unsigned n=0;n<7;++n) {
        c=before;if(n==0)c.ip=0;if(n==1)c.sp=0x239;if(n==2)c.sp=0xFFFD;if(n==3)c.ds=0xA000;if(n==4)c.ss=0x9001;
        pr_u1c_2e7d_cpu rejected=c;
        CHECK(pr_u1c_text_2e7d_cpu(n==5?NULL:&b,n==6?1:PR_BUS_RAM_SIZE,&c,&l,&d)==1);
        CHECK(memcmp(&c,&rejected,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    }
    free(old);free(ega);free(saved);free(r);printf("%u semantic checks passed\n",checks);return 0;
}
