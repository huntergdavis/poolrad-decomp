#include "poolrad/recovered/resident_0709_space_05eb_cpu.h"
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
static pr_r0709_space_cpu setup(pr_bus *b,unsigned color)
{
    pr_r0709_space_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8800,0x5EB,0x5000,0x1111,0x2000,0x602};
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
    b->ram[p(c.cs,0x207)]=0x9A;put(b->ram,c.cs,0x208,0x182A);put(b->ram,c.cs,0x20A,0x6000);
    b->ram[p(c.cs,0x61F)]=0xE8;put(b->ram,c.cs,0x620,0xFBBD);
    outs=writes=0;refuse=-1;return c;
}
static int run(pr_bus *b,pr_r0709_space_cpu *c,size_t limit,pr_r0709_space_cpu_diagnostic *d)
{ return pr_r0709_space_05eb_cpu(b,PR_BUS_RAM_SIZE,c,limit,d); }
static unsigned word(const uint8_t *m,unsigned seg,unsigned off) {
    size_t at=p(seg,off);return m[at]|((unsigned)m[(at+1u)&1048575u]<<8);
}
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*saved=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_r0709_space_cpu c,before;pr_r0709_space_cpu_diagnostic d;
    unsigned color,plane,row,page,x;
    CHECK(ram && copy && ega && saved);
    for(color=0;color<16u;++color) {
        c=setup(&bus,color);CHECK(run(&bus,&c,65536,&d)==0);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x800C && c.bp==0x7777);
        CHECK(outs==102 && writes==192 && ega->seq[2]==15);
        CHECK(word(ram,c.ss,0x8004)==3 && word(ram,c.ss,0x8006)==color && word(ram,c.ss,0x800A)==3);
        CHECK(d.callee==0 && d.status==0 && d.glyph_callee==0 && d.glyph_status==0);
        for(plane=0;plane<4u;++plane)for(row=0;row<8u;++row)for(page=0;page<2u;++page) {
            unsigned at=0x283u+row*40u+page*0x2000u;
            for(x=0;x<3u;++x)CHECK(ega->plane[plane][at+x]==((color&(1u<<plane))?(0x81u^(row*13u)):0));
            CHECK(ega->plane[plane][at-1u]==0xA5 && ega->plane[plane][at+3u]==0xA5);
        }
    }
    for(x=0;x<4u;++x) {
        c=setup(&bus,5);put(ram,c.ss,c.sp+(x%2?8u:10u),x<2?(x%2?25:40):255);
        CHECK(run(&bus,&c,0,&d)==0 && outs==0 && writes==0 && d.callee==-1);
        CHECK(c.ax==0x1234 && c.sp==0x800C);
    }
    c=setup(&bus,5);put(ram,c.ss,c.sp+4,0);
    CHECK(run(&bus,&c,0,&d)==0 && outs==0 && writes==0 && d.callee==0 && d.glyph_callee==0);
    c=setup(&bus,5);ram[p(c.ds,0x4A1E)]=3;put(ram,c.ss,c.sp+4,255);
    CHECK(run(&bus,&c,254,NULL)==0 && outs==0 && writes==0);
    /* Count is not clipped at the right edge. */
    c=setup(&bus,5);put(ram,c.ss,c.sp+4,2);put(ram,c.ss,c.sp+10,39);
    CHECK(run(&bus,&c,65536,&d)==0 && outs==68 && writes==128);
    CHECK(ega->plane[0][16u*40u+39u]==0x81 && ega->plane[0][17u*40u]==0x81);
    c=setup(&bus,5);CHECK(run(&bus,&c,0,&d)==3 && c.ip==0x45B && d.status==6 && outs==2 && writes==2);
    c=setup(&bus,5);refuse=1;CHECK(run(&bus,&c,65536,&d)==3 && c.ip==0x3D5 && d.status==4 && outs==1);
    for(x=0;x<2u;++x) {
        c=setup(&bus,5);if(x)put(ram,c.cs,0x620,0);else ram[p(c.cs,0x61F)]=0x90;
        CHECK(run(&bus,&c,65536,&d)==2 && c.ip==0x61F && c.sp==0x7FF0 && d.callee==-1);
        CHECK(word(ram,c.ss,c.sp)==0x8800 && word(ram,c.ss,c.sp+2)==0x1201 && word(ram,c.ss,c.sp+4)==0x1220);
        CHECK(word(ram,c.ss,c.sp+6)==0x1203 && word(ram,c.ss,c.sp+8)==0x1205);
        CHECK(word(ram,c.ss,c.sp+10)==0x1202 && word(ram,c.ss,c.sp+12)==0x1203 && outs==0 && writes==0);
    }
    c=setup(&bus,5);ram[p(c.cs,0x207)]=0x90;
    CHECK(run(&bus,&c,65536,&d)==3 && c.ip==0x207 && d.status==2 && d.glyph_callee==-1);
    /* PUSH BP aliases the downstream video mode before the glyph reads it. */
    c=setup(&bus,5);c.ss=c.ds;c.sp=0x4A20;
    {const unsigned args[]={0x7000,0x9001,3,5,2,3};for(x=0;x<6u;++x)put(ram,c.ss,c.sp+2u*x,args[x]);}
    CHECK(run(&bus,&c,65536,&d)==0 && ram[p(c.ds,0x4A1E)]==0x77 && outs==0 && writes==0);
    c=setup(&bus,5);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(saved,ega,sizeof *ega);
    CHECK(pr_r0709_space_05eb_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,&d)==1);
    CHECK(pr_r0709_space_05eb_cpu(&bus,1,&c,0,&d)==1);
    CHECK(pr_r0709_space_05eb_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,&d)==1);
    for(x=0;x<5u;++x) {
        c=before;if(x==0)c.ip=0x5EC;if(x==1)c.sp=0x2B;if(x==2)c.sp=0xFFF5;if(x==3)c.ds=0xA000;if(x==4)c.ss=0x9FFF;
        {pr_r0709_space_cpu rejected=c;CHECK(run(&bus,&c,0,&d)==1);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.callee==-1 && d.status==-1 && d.glyph_callee==-1);}
    }
    CHECK(memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved,ega,sizeof *ega)==0);
    free(saved);free(ega);free(copy);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
