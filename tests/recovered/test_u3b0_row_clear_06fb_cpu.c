#include "poolrad/recovered/overlay_u3b0_row_clear_06fb_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,outs,writes;
static int refuse=-1;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned s,unsigned o) { return ((size_t)s*16u+(o&65535u))&1048575u; }
static void put(uint8_t *m,unsigned s,unsigned o,unsigned v) { size_t a=p(s,o);m[a]=(uint8_t)v;m[(a+1u)&1048575u]=(uint8_t)(v>>8); }
static int out(void *ctx,uint16_t port,uint16_t value,unsigned size,uint16_t cs,uint16_t ip)
{
    (void)ctx;(void)port;(void)value;(void)size;(void)cs;(void)ip;
    if(refuse>=0 && outs==(unsigned)refuse)return 1;
    ++outs;return 0;
}
static void written(void *ctx,uint16_t offset,uint8_t value) { (void)ctx;(void)offset;(void)value;++writes; }
static pr_u3b0_06fb_cpu setup(pr_bus *b,unsigned color)
{
    pr_u3b0_06fb_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x4000,0x6FB,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001,3,color,2,3};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<6u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(c.ds,0x4A1E)]=1;
    put(b->ram,c.ds,0x4A16,0xA000);put(b->ram,c.ds,0x4A18,0xA200);
    put(b->ram,c.ds,0x5E20,0x1000);put(b->ram,c.ds,0x5E22,0x7000);
    for(i=0;i<4u;++i)b->ram[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    for(i=0;i<256u;++i)b->ram[p(c.ds,0x25D5+i)]=(uint8_t)(i%4u);
    for(i=0;i<4u;++i)put(b->ram,c.ds,0x25CD+2u*i,i*0x5555u);
    for(i=0;i<8u;++i){put(b->ram,c.ds,0x25E5+2u*i,0xC000u>>(2u*i));b->ram[p(0x7000,0x1100+i)]=(uint8_t)(0x81u^(i*13u));}
    b->ram[p(0x8800,0x207)]=0x9A;put(b->ram,0x8800,0x208,0x182A);put(b->ram,0x8800,0x20A,0x6000);
    b->ram[p(0x8800,0x61F)]=0xE8;put(b->ram,0x8800,0x620,0xFBBD);
    b->ram[p(c.cs,0x70A)]=0x9A;put(b->ram,c.cs,0x70B,0x5EB);put(b->ram,c.cs,0x70D,0x8800);
    outs=writes=0;refuse=-1;return c;
}
static int run(pr_bus *b,pr_u3b0_06fb_cpu *c,size_t limit,pr_u3b0_06fb_diagnostic *d)
{ return pr_u3b0_row_clear_06fb_cpu(b,PR_BUS_RAM_SIZE,c,limit,d); }
static unsigned word(const uint8_t *m,unsigned seg,unsigned off) {
    size_t at=p(seg,off);return m[at]|((unsigned)m[(at+1u)&1048575u]<<8);
}
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*saved=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_u3b0_06fb_cpu c,before;pr_u3b0_06fb_diagnostic d;
    unsigned ah,plane,at,page,x;
    CHECK(ram && copy && ega && saved);
    for(ah=0;ah<256u;ah+=17u) {
        c=setup(&bus,5);c.ax=(uint16_t)((ah<<8)|0xA7);
        CHECK(run(&bus,&c,65536,&d)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8004 && c.bp==0x7777);
        CHECK(outs==1360 && writes==2560 && ega->seq[2]==15);
        CHECK(d.space_status==0 && d.space.callee==0 && d.space.status==0 &&
              d.space.glyph_callee==0 && d.space.glyph_status==0);
        CHECK(word(ram,c.ss,0x7FF6)==((ah<<8)|40u));
        CHECK(word(ram,c.ss,0x7FF8)==(ah<<8));
        CHECK(word(ram,c.ss,0x7FFA)==((ah<<8)|24u));
        CHECK(word(ram,c.ss,0x7FFC)==(ah<<8));
        /* All four planes, both 40-byte x 8-row areas cleared; everything
         * else in the 64KiB planes must remain the synthetic background. */
        for(plane=0;plane<4u;++plane)for(at=0;at<65536u;++at) {
            unsigned relative=at&0x1FFFu;
            int cleared=(at<0x4000u && relative>=7680u && relative<8000u);
            CHECK(ega->plane[plane][at]==(cleared?0:0xA5));
        }
    }
    (void)page;
    c=setup(&bus,5);CHECK(run(&bus,&c,0,&d)==3 && c.ip==0x45B &&
        d.space_status==3 && d.space.status==6 && outs==2 && writes==2);
    c=setup(&bus,5);refuse=1;CHECK(run(&bus,&c,65536,&d)==3 &&
        c.ip==0x3D5 && d.space_status==3 && d.space.status==4 && outs==1);
    for(x=0;x<2u;++x) {
        c=setup(&bus,5);before=c;
        if(x)put(ram,c.cs,0x70B,0);else ram[p(c.cs,0x70A)]=0x90;
        CHECK(run(&bus,&c,65536,&d)==2 && c.ip==0x70A && c.sp==0x7FF6);
        CHECK(c.flags==before.flags && c.bp==0x7FFE && c.ax==0x1228 && outs==0);
        CHECK(d.space_status==-1 && d.space.callee==-1);
    }
    c=setup(&bus,5);ram[p(0x8800,0x61F)]=0x90;
    CHECK(run(&bus,&c,65536,&d)==3 && d.space_status==2 &&
          c.ip==0x61F && c.cs==0x8800 && outs==0);
    c=setup(&bus,5);ram[p(0x8800,0x207)]=0x90;
    CHECK(run(&bus,&c,65536,&d)==3 && d.space_status==3 && d.space.status==2);
    /* Saved caller BP aliases video mode before the renderer reads it. */
    c=setup(&bus,5);c.ss=c.ds;c.sp=0x4A20;
    put(ram,c.ss,c.sp,0x7000);put(ram,c.ss,c.sp+2,0x9001);
    CHECK(run(&bus,&c,65536,&d)==0 && ram[p(c.ds,0x4A1E)]==0x77 &&
          outs==0 && writes==0);
    c=setup(&bus,5);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(saved,ega,sizeof *ega);
    CHECK(pr_u3b0_row_clear_06fb_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,&d)==1);
    CHECK(pr_u3b0_row_clear_06fb_cpu(&bus,1,&c,0,&d)==1);
    CHECK(pr_u3b0_row_clear_06fb_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,&d)==1);
    for(x=0;x<5u;++x) {
        c=before;if(x==0)c.ip=0x6FC;if(x==1)c.sp=0x39;if(x==2)c.sp=0xFFFD;if(x==3)c.ds=0xA000;if(x==4)c.ss=0x9FFF;
        {pr_u3b0_06fb_cpu rejected=c;CHECK(run(&bus,&c,0,&d)==1);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.space_status==-1 && d.space.callee==-1);}
    }
    CHECK(memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved,ega,sizeof *ega)==0);
    c=setup(&bus,5);CHECK(run(&bus,&c,65536,NULL)==0);
    free(saved);free(ega);free(copy);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
