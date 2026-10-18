#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
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
static pr_r0709_printer_cpu setup(pr_bus *b,unsigned color)
{
    pr_r0709_printer_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8800,0x626,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001,0x800,0x4000,color,2,3};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<7u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(c.ds,0x4A1E)]=1;
    put(b->ram,c.ds,0x4A16,0xA000);put(b->ram,c.ds,0x4A18,0xA200);
    put(b->ram,c.ds,0x5E20,0x1000);put(b->ram,c.ds,0x5E22,0x7000);
    for(i=0;i<4u;++i)b->ram[p(c.ds,0x25C9+i)]=(uint8_t)(1u<<i);
    for(i=0;i<256u;++i)b->ram[p(c.ds,0x25D5+i)]=(uint8_t)(i%4u);
    for(i=0;i<4u;++i)put(b->ram,c.ds,0x25CD+2u*i,i*0x5555u);
    for(i=0;i<8u;++i){put(b->ram,c.ds,0x25E5+2u*i,0xC000u>>(2u*i));b->ram[p(0x7000,0x1008+i)]=(uint8_t)(0x81u^(i*13u));}
    b->ram[p(c.cs,0x207)]=0x9A;put(b->ram,c.cs,0x208,0x182A);put(b->ram,c.cs,0x20A,0x6000);
    b->ram[p(0x4000,0x800)]=3;memset(b->ram+p(0x4000,0x801),'a',3);
    b->ram[p(c.cs,0x63C)]=0x9A;put(b->ram,c.cs,0x63D,0xB25);put(b->ram,c.cs,0x63F,0x6000);
    b->ram[p(c.cs,0x691)]=0xE8;put(b->ram,c.cs,0x692,0xFB4B);
    outs=writes=0;refuse=-1;return c;
}
static int run(pr_bus *b,pr_r0709_printer_cpu *c,size_t limit,size_t glyph_limit,pr_r0709_printer_cpu_diagnostic *d)
{ return pr_r0709_printer_0626_cpu(b,PR_BUS_RAM_SIZE,c,limit,glyph_limit,d); }
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*saved_ega=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_r0709_printer_cpu c,before;pr_r0709_printer_cpu_diagnostic d;
    unsigned color,plane,row,page,x;
    CHECK(ram && copy && ega && saved_ega);
    for(color=0;color<16u;++color) {
        c=setup(&bus,color);CHECK(run(&bus,&c,256,65536,&d)==PR_R0709_PRINTER_CPU_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x800E && c.bp==0x7777);
        CHECK((c.flags&0x600u)==0x200u && ram[p(c.ss,0x800C)]==6);
        CHECK(ram[p(c.ss,0x7EFE)]==3 && memcmp(ram+p(c.ss,0x7EFF),"aaa",3)==0);
        CHECK(outs==102 && writes==192 && ega->seq[2]==15);
        CHECK(d.callee==1 && d.status==0 && d.glyph_callee==0 && d.glyph_status==0);
        for(plane=0;plane<4u;++plane)for(row=0;row<8u;++row)for(page=0;page<2u;++page) {
            unsigned at=0x283u+row*40u+page*0x2000u;
            for(x=0;x<3u;++x)CHECK(ega->plane[plane][at+x]==((color&(1u<<plane))?(0x81u^(row*13u)):0));
            CHECK(ega->plane[plane][at-1u]==0xA5 && ega->plane[plane][at+3u]==0xA5);
        }
    }
    /* Copy happens even when the outer x/y guard rejects rendering. */
    for(x=0;x<2u;++x) {
        c=setup(&bus,5);put(ram,c.ss,c.sp+(x?10u:12u),40);
        CHECK(run(&bus,&c,256,65536,&d)==PR_R0709_PRINTER_CPU_OK && outs==0 && writes==0);
        CHECK(ram[p(c.ss,0x7EFE)]==3 && memcmp(ram+p(c.ss,0x7EFF),"aaa",3)==0);
        CHECK(d.callee==0 && d.status==0 && ram[p(c.ss,0x800C)]==(x?3:40));
    }
    /* y25..39 passes the outer guard but each glyph rejects it. */
    for(x=25;x<=39;x+=14) {
        c=setup(&bus,5);put(ram,c.ss,c.sp+10u,x);
        CHECK(run(&bus,&c,256,65536,&d)==PR_R0709_PRINTER_CPU_OK && outs==0 && writes==0);
        CHECK(ram[p(c.ss,0x800C)]==6 && d.callee==1 && d.glyph_callee==-1);
    }
    c=setup(&bus,5);ram[p(0x4000,0x800)]=0;
    CHECK(run(&bus,&c,0,0,NULL)==PR_R0709_PRINTER_CPU_OK && outs==0 && writes==0);
    CHECK(ram[p(c.ss,0x7EFE)]==0 && ram[p(c.ss,0x800C)]==3);
    c=setup(&bus,5);put(ram,c.ss,c.sp+12u,39);ram[p(0x4000,0x800)]=220;
    memset(ram+p(0x4000,0x801),'a',220);
    CHECK(run(&bus,&c,256,65536,&d)==PR_R0709_PRINTER_CPU_OK);
    CHECK(ram[p(c.ss,0x800C)]==3 && outs==136 && writes==256);
    /* Forward overlap propagates the original length byte through the copy. */
    c=setup(&bus,5);ram[p(c.ds,0x4A1E)]=3;
    put(ram,c.ss,c.sp+4u,0x7EFD);put(ram,c.ss,c.sp+6u,c.ss);
    ram[p(c.ss,0x7EFD)]=3;memcpy(ram+p(c.ss,0x7EFE),"abc",3);
    CHECK(run(&bus,&c,256,65536,&d)==PR_R0709_PRINTER_CPU_OK);
    for(x=0;x<4u;++x)CHECK(ram[p(c.ss,0x7EFEu+x)]==3);
    c=setup(&bus,5);CHECK(run(&bus,&c,0,65536,&d)==PR_R0709_PRINTER_CPU_LIMIT);
    CHECK(c.ip==0x69F && outs==34 && writes==64 && ram[p(c.ss,0x800C)]==4);
    c=setup(&bus,5);CHECK(run(&bus,&c,256,0,&d)==PR_R0709_PRINTER_CPU_CALLEE);
    CHECK(c.ip==0x45B && outs==2 && writes==2 && d.callee==1 && d.status==6);
    c=setup(&bus,5);refuse=1;CHECK(run(&bus,&c,256,65536,&d)==PR_R0709_PRINTER_CPU_CALLEE);
    CHECK(c.ip==0x3D5 && outs==1 && d.status==4 && d.glyph_callee==0 && d.glyph_status==0);
    for(x=0;x<3u;++x) {
        const unsigned sites[]={0x63C,0x691,0x207};
        c=setup(&bus,5);ram[p(c.cs,sites[x])]=0x90;
        CHECK(run(&bus,&c,256,65536,&d)==(x==2?PR_R0709_PRINTER_CPU_CALLEE:PR_R0709_PRINTER_CPU_CODE));
        CHECK(c.ip==sites[x] && outs==0 && writes==0);
        if(x==2)CHECK(d.callee==1 && d.status==2 && d.glyph_callee==-1);
    }
    c=setup(&bus,5);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(saved_ega,ega,sizeof *ega);
    CHECK(pr_r0709_printer_0626_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,0,&d)==PR_R0709_PRINTER_CPU_ARGUMENT);
    CHECK(pr_r0709_printer_0626_cpu(&bus,1,&c,0,0,&d)==PR_R0709_PRINTER_CPU_ARGUMENT);
    CHECK(pr_r0709_printer_0626_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,0,&d)==PR_R0709_PRINTER_CPU_ARGUMENT);
    for(x=0;x<5u;++x) {
        c=before;if(x==0)c.ip=0x627;if(x==1)c.sp=0x12F;if(x==2)c.sp=0xFFF3;if(x==3)c.ds=0xA000;if(x==4)c.ss=0x9FFF;
        {pr_r0709_printer_cpu rejected=c;CHECK(run(&bus,&c,0,0,&d)==PR_R0709_PRINTER_CPU_ARGUMENT);
         CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.callee==-1 && d.status==-1 && d.glyph_callee==-1);}
    }
    CHECK(memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved_ega,ega,sizeof *ega)==0);
    c=before;put(ram,c.ss,c.sp+6u,0xA000);memcpy(copy,ram,PR_BUS_RAM_SIZE);
    CHECK(run(&bus,&c,0,0,&d)==PR_R0709_PRINTER_CPU_ARGUMENT);
    CHECK(memcmp(&c,&before,sizeof c)==0 && memcmp(copy,ram,PR_BUS_RAM_SIZE)==0 && memcmp(saved_ega,ega,sizeof *ega)==0);
    free(saved_ega);free(ega);free(copy);free(ram);printf("%u semantic checks passed\n",checks);return 0;
}
