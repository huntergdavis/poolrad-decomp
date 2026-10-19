#include "poolrad/recovered/overlay_u3b0_entry_01da_cpu.h"
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
static int out(void *ctx,uint16_t port,uint16_t value,unsigned size,uint16_t cs,uint16_t ip) {
    (void)ctx;(void)value;(void)ip;CHECK(size==1 && cs==0x8800);CHECK(port==0x3C4 || port==0x3C5);
    if(refuse>=0 && outs==(unsigned)refuse)return 1;
    ++outs;return 0;
}
static void write(void *ctx,uint16_t off,uint8_t value) { (void)ctx;(void)off;(void)value;++writes; }
static pr_u3b0_01da_cpu setup(pr_bus *b,unsigned length,unsigned column,unsigned mode)
{
    pr_u3b0_01da_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x9000,0x1DA,0x1800,0x1111,0x2800,0x602};
    uint8_t *r=b->ram;memset(r,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    put(r,c.ss,c.sp,0x7000);put(r,c.ss,c.sp+2,0x9001);put(r,c.ss,c.sp+4,0x4000);put(r,c.ss,c.sp+6,1);
    const unsigned sites[]={0x273,0x2CA,0x305,0x372};
    for(unsigned j=0;j<4;++j)farcall(r,c.cs,sites[j],0x8800,0x1DF);
    farcall(r,c.cs,0x28F,0x4000,0xDC0);farcall(r,0x8800,0x207,0x4000,0x182A);
    memset(r+p(c.cs,0x1BA),0,32);r[p(c.cs,0x1BA+'c'/8)]=(uint8_t)(1u<<('c'%8));
    for(unsigned j=0;j<length;++j)r[p(c.ss,0x3FAEu + j)]=(uint8_t)('a'+j%26);
    r[p(c.ss,0x3F74)]=2;r[p(c.ss,0x3F73)]=1;
    r[p(c.ss,0x3F9C)]=(uint8_t)length;r[p(c.ss,0x3F9B)]=(uint8_t)column;r[p(c.ss,0x400E)]=15;r[p(c.ss,0x4010)]=10;
    r[p(c.ds,0x4A1E)]=(uint8_t)mode;put(r,c.ds,0x4A16,0xA000);put(r,c.ds,0x4A18,0xA200);
    put(r,c.ds,0x5E20,0x1000);put(r,c.ds,0x5E22,0x7000);
    for(unsigned i=0;i<4;++i)r[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    memset(r+p(0x7000,0x1000),0xFF,2048);outs=writes=0;refuse=-1;return c;
}
int main(void)
{
    uint8_t *r=malloc(PR_BUS_RAM_SIZE),*saved=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*old=malloc(sizeof *old);pr_port_env env={NULL,out,NULL};pr_bus b={r,ega,&env,write,NULL};
    pr_u3b0_01da_limits l={65536,65536};pr_u3b0_01da_diagnostic d;pr_u3b0_01da_cpu c,before;
    CHECK(r && saved && ega && old);
    for(unsigned ch=0;ch<256;++ch)for(unsigned member=0;member<2;++member)for(unsigned selection=0;selection<3;++selection) {
        c=setup(&b,1,37,3);r[p(c.ss,0x3FAE)]=(uint8_t)ch;memset(r+p(c.cs,0x1BA),0,32);
        if(member)r[p(c.cs,0x1BA+ch/8)]=(uint8_t)(1u<<(ch%8));
        if(selection==0)r[p(c.ss,0x3F74)]=0;
        if(selection==2)r[p(c.ss,0x400E)]=0;
        r[p(c.cs,0x273)]=r[p(c.cs,0x2CA)]=r[p(c.cs,0x305)]=0x90;
        unsigned site=selection==1?0x273u:member?0x2CAu:0x305u;
        unsigned color=selection==1?15u:member?(selection==2?0u:15u):10u;
        CHECK(pr_u3b0_entry_01da_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==2 && c.ip==site && c.sp==0x7FF0);
        const unsigned want[]={1,ch,1,color,24,37};
        for(unsigned j=0;j<6;++j)CHECK((word(r,c.ss,c.sp+2*j)&255u)==want[j]);
        CHECK(outs==0 && writes==0 && (selection==1?d.callee==-1:d.callee==0 && d.status==0));
    }
    for(unsigned column=0;column<256;++column) {
        c=setup(&b,1,column,3);r[p(c.cs,0x372)]=0x90;
        int status=pr_u3b0_entry_01da_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d);
        if(column<38) {
            CHECK(status==2 && c.ip==0x372);
            const unsigned want[]={1,32,39-column,0,24,column+1};
            for(unsigned j=0;j<6;++j)CHECK((word(r,c.ss,c.sp+2*j)&255u)==want[j]);
        } else CHECK(status==0 && c.sp==0x8008 && c.cs==0x9001 && c.ip==0x7000);
        CHECK(outs==0 && writes==0);
        c=setup(&b,0,column,1);
        CHECK(pr_u3b0_entry_01da_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0 && outs==0 && writes==0 && d.callee==-1);
    }
    c=setup(&b,4,2,1);CHECK(pr_u3b0_entry_01da_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0 && outs>0 && writes>0);
    for(unsigned plane=0;plane<4;++plane)for(unsigned page=0;page<2;++page)for(unsigned y=0;y<8;++y) {
        unsigned at=24*320+y*40+page*0x2000;
        for(unsigned x=0;x<=40;++x) {
            unsigned color=x<=4?15:10;
            unsigned expected=x<2 || x==40?0xA5:x>=6?0:(color&(1u<<plane))?255:0;
            CHECK(ega->plane[plane][at+x]==expected);
        }
    }
    for(unsigned column=37;column<=38;++column) {
        c=setup(&b,1,column,1);CHECK(pr_u3b0_entry_01da_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==0);
        for(unsigned plane=0;plane<4;++plane)for(unsigned page=0;page<2;++page)for(unsigned y=0;y<8;++y) {
            unsigned at=24*320+y*40+page*0x2000;
            CHECK(ega->plane[plane][at+column]==255);
            CHECK(ega->plane[plane][at+39]==(column==37?0:0xA5));
        }
    }
    c=setup(&b,4,2,3);l.caller_loopbacks=0;
    CHECK(pr_u3b0_entry_01da_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==5 && c.ip==0x312 && r[p(c.ss,0x7FFD)]==1 && r[p(c.ss,0x7FFC)]==4);l.caller_loopbacks=65536;
    c=setup(&b,4,2,1);l.glyph_loopbacks=0;
    CHECK(pr_u3b0_entry_01da_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.callee==1 && d.status==6);l.glyph_loopbacks=65536;
    c=setup(&b,4,2,1);refuse=0;
    CHECK(pr_u3b0_entry_01da_cpu(&b,PR_BUS_RAM_SIZE,&c,&l,&d)==3 && d.status==4 && outs==0 && writes==0);
    c=setup(&b,4,2,1);before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    for(unsigned n=0;n<7;++n) {
        c=before;if(n==0)c.ip=0;if(n==1)c.sp=0x3F;if(n==2)c.sp=0xFFF5;if(n==3)c.ds=0xA000;if(n==4)c.ss=0x9001;
        pr_u3b0_01da_cpu rejected=c;
        CHECK(pr_u3b0_entry_01da_cpu(n==5?NULL:&b,n==6?1:PR_BUS_RAM_SIZE,&c,&l,&d)==1);
        CHECK(memcmp(&c,&rejected,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    }
    free(old);free(ega);free(saved);free(r);printf("%u semantic checks passed\n",checks);return 0;
}
