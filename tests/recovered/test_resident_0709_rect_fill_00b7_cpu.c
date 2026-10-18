#include "poolrad/recovered/resident_0709_rect_fill_00b7_cpu.h"
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
static pr_r0709_rect_cpu setup(pr_bus *b,unsigned color)
{
    pr_r0709_rect_cpu c={0x1234,0x3456,0x5678,0x789A,0x9ABC,0xBCDE,0x7777,0x8000,0x8800,0xB7,0x5000,0x1111,0x2000,0x602};
    unsigned i;const unsigned args[]={0x7000,0x9001,color,1,1,3,1,1};
    memset(b->ram,0xA5,PR_BUS_RAM_SIZE);pr_ega_reset(b->ega);
    memset(b->ega->plane,0xA5,sizeof b->ega->plane);
    for(i=0;i<8u;++i)put(b->ram,c.ss,c.sp+2u*i,args[i]);
    b->ram[p(c.ds,0x4A1A)]=0;
    b->ram[p(c.cs,0x140)]=0x9A;put(b->ram,c.cs,0x141,0x16A8);put(b->ram,c.cs,0x143,0x6000);
    outs=writes=0;refuse=-1;return c;
}
int main(void)
{
    uint8_t *ram=malloc(PR_BUS_RAM_SIZE),*copy=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*before_ega=malloc(sizeof *ega);
    pr_port_env env={NULL,out,NULL};pr_bus bus={ram,ega,&env,written,NULL};
    pr_r0709_rect_cpu c,before;pr_r0709_rect_cpu_diagnostic d;
    const unsigned row[]={0x2141,0x2169,0x2191,0x21B9,0x21E1,0x2209,0x2231,0x2259};
    unsigned color,plane,r,x;
    CHECK(ram && copy && ega && before_ega);
    for(color=0;color<16u;++color) {
        c=setup(&bus,color);
        CHECK(pr_r0709_rect_fill_00b7_cpu(&bus,PR_BUS_RAM_SIZE,&c,65536,&d)==PR_R0709_RECT_CPU_OK);
        CHECK(c.cs==0x9001 && c.ip==0x7000 && c.sp==0x8010 && c.bp==0x7777);
        CHECK((c.flags&0x400u)==0 && word(ram,c.ds,0x4A18)==0xA000 && ram[p(c.ds,0x4A1A)]==0);
        CHECK(outs==22 && writes==24 && ega->gc[5]==0 && ega->gc[8]==255 && ega->seq[2]==15);
        CHECK(d.callee==0 && d.status==0);
        for(plane=0;plane<4u;++plane)for(r=0;r<8u;++r) {
            for(x=0;x<3u;++x)CHECK(ega->plane[plane][row[r]+x]==((color&(1u<<plane))?255u:0u));
            CHECK(ega->plane[plane][row[r]-1u]==0xA5 && ega->plane[plane][row[r]+3u]==0xA5);
        }
        CHECK(ega->plane[0][0x141]==0xA5);
    }
    c=setup(&bus,5);
    CHECK(pr_r0709_rect_fill_00b7_cpu(&bus,PR_BUS_RAM_SIZE,&c,0,&d)==PR_R0709_RECT_CPU_LIMIT);
    CHECK(c.ip==0x14F && writes==3 && outs==20 && ega->gc[5]==2);
    CHECK(ega->plane[0][0x2141]==255 && ega->plane[0][0x2169]==0xA5);
    c=setup(&bus,5);refuse=18;
    CHECK(pr_r0709_rect_fill_00b7_cpu(&bus,PR_BUS_RAM_SIZE,&c,0,&d)==PR_R0709_RECT_CPU_PORT);
    CHECK(c.ip==0x108 && writes==0 && outs==18 && ega->gc[5]==0);
    c=setup(&bus,5);refuse=0;ega->gc_index=8;
    CHECK(pr_r0709_rect_fill_00b7_cpu(&bus,PR_BUS_RAM_SIZE,&c,0,&d)==PR_R0709_RECT_CPU_CALLEE);
    CHECK(d.callee==1 && d.status==2 && writes==0 && outs==0 && ega->gc_index==8);
    c=setup(&bus,5);ram[p(c.cs,0x140)]=0x90;
    CHECK(pr_r0709_rect_fill_00b7_cpu(&bus,PR_BUS_RAM_SIZE,&c,0,&d)==PR_R0709_RECT_CPU_CODE);
    CHECK(c.ip==0x140 && writes==0);
    c=setup(&bus,5);before=c;memcpy(copy,ram,PR_BUS_RAM_SIZE);memcpy(before_ega,ega,sizeof *ega);
    CHECK(pr_r0709_rect_fill_00b7_cpu(NULL,PR_BUS_RAM_SIZE,&c,0,&d)==PR_R0709_RECT_CPU_ARGUMENT);
    CHECK(pr_r0709_rect_fill_00b7_cpu(&bus,1,&c,0,&d)==PR_R0709_RECT_CPU_ARGUMENT);
    CHECK(pr_r0709_rect_fill_00b7_cpu(&bus,PR_BUS_RAM_SIZE,NULL,0,&d)==PR_R0709_RECT_CPU_ARGUMENT);
    for(x=0;x<5u;++x) {
        c=before;
        if(x==0)c.ip=0xB8;
        if(x==1)c.sp=0x1F;
        if(x==2)c.sp=0xFFF1;
        if(x==3)c.ds=0xA000;
        if(x==4)c.ss=0x9FFF;
        { pr_r0709_rect_cpu rejected=c;
          CHECK(pr_r0709_rect_fill_00b7_cpu(&bus,PR_BUS_RAM_SIZE,&c,0,&d)==PR_R0709_RECT_CPU_ARGUMENT);
          CHECK(memcmp(&c,&rejected,sizeof c)==0 && d.callee==-1 && d.status==-1); }
    }
    CHECK(memcmp(ram,copy,PR_BUS_RAM_SIZE)==0 && memcmp(ega,before_ega,sizeof *ega)==0);
    free(before_ega);free(ega);free(copy);free(ram);
    printf("%u semantic checks passed\n",checks);return 0;
}
