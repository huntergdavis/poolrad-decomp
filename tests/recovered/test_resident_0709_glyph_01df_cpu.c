#include "poolrad/recovered/resident_0709_glyph_01df_cpu.h"
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
static pr_r0709_glyph_cpu setup(pr_bus *b,unsigned color)
{
    pr_r0709_glyph_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8800,0x1DF,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001,1,97,2,color,2,3};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<8u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(c.ds,0x4A1E)]=1;
    put(b->ram,c.ds,0x4A16,0xA000);put(b->ram,c.ds,0x4A18,0xA200);
    put(b->ram,c.ds,0x5E20,0x1000);put(b->ram,c.ds,0x5E22,0x7000);
    for(i=0;i<4u;++i)b->ram[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    for(i=0;i<256u;++i)b->ram[p(c.ds,0x25D5+i)]=(uint8_t)(i%4u);
    for(i=0;i<4u;++i)put(b->ram,c.ds,0x25CD+2u*i,i*0x5555u);
    for(i=0;i<8u;++i){put(b->ram,c.ds,0x25E5+2u*i,0xC000u>>(2u*i));b->ram[p(0x7000,0x1008+i)]=(uint8_t)(0x81u^(i*13u));}
    b->ram[p(c.cs,0x207)]=0x9A;put(b->ram,c.cs,0x208,0x182A);put(b->ram,c.cs,0x20A,0x6000);
    outs=writes=0;refuse=-1;return c;
}
static int run(pr_bus *b,pr_r0709_glyph_cpu *c,size_t limit,pr_r0709_glyph_cpu_diagnostic *d)
{ return pr_r0709_glyph_01df_cpu(b,PR_BUS_RAM_SIZE,c,limit,d); }
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*saved_ega=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_r0709_glyph_cpu c,before;pr_r0709_glyph_cpu_diagnostic d;
    unsigned color,plane,row,page,x;
    CHECK(ram && copy && ega && saved_ega);
    for(color=0;color<16u;++color) {
        c=setup(&bus,color);CHECK(run(&bus,&c,65536,&d)==PR_R0709_GLYPH_CPU_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8010 && c.bp==0x7777);
        CHECK((c.flags&0x600u)==0x600u && ram[p(c.ss,0x800E)]==5);
        CHECK(outs==68 && writes==128 && ega->seq[2]==15 && d.callee==0 && d.status==0);
        for(plane=0;plane<4u;++plane)for(row=0;row<8u;++row)for(page=0;page<2u;++page) {
            unsigned at=0x283u+row*40u+page*0x2000u;
            for(x=0;x<2u;++x)CHECK(ega->plane[plane][at+x]==((color&(1u<<plane))?(0x81u^(row*13u)):0));
            CHECK(ega->plane[plane][at-1u]==0xA5 && ega->plane[plane][at+2u]==0xA5);
        }
    }
    /* Packed modes use ordinary bus memory; these are not hardware claims. */
    c=setup(&bus,3);ram[p(c.ds,0x4A1E)]=0;put(ram,c.ss,c.sp+8u,1);
    memset(ram+p(0x7000,0x1008),0xAA,8);
    CHECK(run(&bus,&c,65536,&d)==PR_R0709_GLYPH_CPU_OK && outs==0 && writes==0);
    for(row=0;row<4u;++row)for(page=0;page<2u;++page)CHECK(word(ram,0xB800+page*0x200u,80u*(8u+row)+6u)==0xCCCC);
    c=setup(&bus,10);ram[p(c.ds,0x4A1E)]=2;put(ram,c.ss,c.sp+8u,1);
    memset(ram+p(0x7000,0x1008),0xAA,8);
    CHECK(run(&bus,&c,65536,&d)==PR_R0709_GLYPH_CPU_OK && outs==0 && writes==0);
    for(row=0;row<2u;++row)for(page=0;page<4u;++page)for(x=0;x<4u;++x)
        CHECK(ram[p(0xB800+page*0x200u,160u*(4u+row)+12u+x)]==0xA0);
    c=setup(&bus,5);CHECK(run(&bus,&c,0,&d)==PR_R0709_GLYPH_CPU_LIMIT);
    CHECK(c.ip==0x45B && outs==2 && writes==2 && ega->seq[2]==8);
    c=setup(&bus,5);CHECK(run(&bus,&c,31,&d)==PR_R0709_GLYPH_CPU_LIMIT);
    CHECK(c.ip==0x5E2 && outs==34 && writes==64);
    for(x=0;x<3u;++x) {
        const int refusal[]={0,1,33};const unsigned ips[]={0x3AD,0x3D5,0x470};
        c=setup(&bus,5);refuse=refusal[x];
        CHECK(run(&bus,&c,65536,&d)==PR_R0709_GLYPH_CPU_PORT);
        CHECK(c.ip==ips[x] && outs==(unsigned)refusal[x]);
    }
    c=setup(&bus,5);ram[p(c.cs,0x207)]=0x90;
    CHECK(run(&bus,&c,65536,&d)==PR_R0709_GLYPH_CPU_CODE && c.ip==0x207 && c.sp==0x7FEA && d.callee==-1);
    c=setup(&bus,5);put(ram,c.ss,c.sp+4u,0);ram[p(c.ds,0x4A1E)]=255;
    CHECK(run(&bus,&c,65536,NULL)==PR_R0709_GLYPH_CPU_OK && outs==0 && writes==0);
    c=setup(&bus,5);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(saved_ega,ega,sizeof *ega);
    CHECK(pr_r0709_glyph_01df_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,&d)==PR_R0709_GLYPH_CPU_ARGUMENT);
    CHECK(pr_r0709_glyph_01df_cpu(&bus,1,&c,0,&d)==PR_R0709_GLYPH_CPU_ARGUMENT);
    CHECK(pr_r0709_glyph_01df_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,&d)==PR_R0709_GLYPH_CPU_ARGUMENT);
    for(x=0;x<5u;++x) {
        c=before;if(x==0)c.ip=0x1E0;if(x==1)c.sp=0x19;if(x==2)c.sp=0xFFF1;if(x==3)c.ds=0xA000;if(x==4)c.ss=0x9FFF;
        {pr_r0709_glyph_cpu rejected=c;CHECK(run(&bus,&c,0,&d)==PR_R0709_GLYPH_CPU_ARGUMENT);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.callee==-1 && d.status==-1);}
    }
    CHECK(memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved_ega,ega,sizeof *ega)==0);
    free(saved_ega);free(ega);free(copy);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
