#include "poolrad/recovered/overlay_u3b0_entry_01da_cpu.h"
#include "poolrad/replay/u3b0_selection_05c4.h"
#include "poolrad/replay/u3b0_mapped_060c.h"
#include "poolrad/replay/u3b0_text_0634.h"
#include "poolrad/replay/u3b0_return_06cf.h"
#include "poolrad/replay/u3b0_character_059c.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static unsigned checks,outs,writes;
static int refuse=-1;
#define CHECK(x) do { ++checks;if(!(x)){fprintf(stderr,"line %d: %s\n",__LINE__,#x);exit(1);} } while(0)
static size_t p(unsigned s,unsigned o) { return ((size_t)s*16u+(o&65535u))&1048575u; }
static void put(uint8_t *r,unsigned s,unsigned o,unsigned v) { size_t a=p(s,o);r[a]=(uint8_t)v;r[(a+1)&1048575u]=(uint8_t)(v>>8); }
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
static pr_r0899_cpu selection_setup(pr_bus *b,unsigned length,unsigned column,unsigned mode,unsigned key)
{
    pr_u3b0_01da_cpu x=setup(b,length,column,mode);
    pr_r0899_cpu c={x.ax,x.bx,x.cx,x.dx,x.si,x.di,0x4000,x.sp,x.cs,0x05c4,x.ds,x.es,x.ss,x.flags};
    c.ax=(uint16_t)((c.ax&0xff00u)|key);
    b->ram[p(c.ss,c.bp-0x8e)]=4;
    b->ram[p(c.cs,0x5e2)]=0xe8;put(b->ram,c.cs,0x5e3,(uint16_t)(0x01da-0x05e5));
    b->ram[p(c.cs,0x606)]=0xe8;put(b->ram,c.cs,0x607,(uint16_t)(0x01da-0x0609));
    return c;
}
static void input_setup(pr_bus *b,pr_r0899_cpu *c,unsigned key)
{
    uint8_t *r=b->ram;*c=selection_setup(b,4,2,1,key);c->ip=0x059c;
    r[p(c->ss,c->bp-0x60)]=(uint8_t)key;r[p(c->ss,c->bp-0x63)]=0;
    r[p(c->ss,c->bp+0x0c)]=1;put(r,c->ss,c->bp+6,0x4100);put(r,c->ss,c->bp+8,c->ss);
    put(r,c->ss,c->bp,0x7777);put(r,c->ss,c->bp+2,0x09d5);put(r,c->ss,c->bp+4,0x2e60);
    put(r,c->ds,0x49d2,0x1000);put(r,c->ds,0x49d4,0x5000);
    memset(r+p(c->cs,0x037d),0,32);memset(r+p(c->cs,0x039d),0,32);
    farcall(r,c->cs,0x0610,0x4000,0x182a);farcall(r,c->cs,0x0621,0x4000,0x0dc0);farcall(r,c->cs,0x06a0,0x4000,0x0dc0);
}
int main(void)
{
    uint8_t *r=malloc(PR_BUS_RAM_SIZE),*saved=malloc(PR_BUS_RAM_SIZE);
    pr_ega *ega=malloc(sizeof *ega),*old=malloc(sizeof *old);pr_port_env ports={NULL,out,NULL};pr_bus b={r,ega,&ports,write,NULL};
    pr_u3b0_mapped_env env={0x4000};pr_u3b0_mapped_diagnostic md;pr_u3b0_text_diagnostic d;
    pr_u3b0_text_limits lim={16,300,{65536,65536}};pr_u3b0_return_target target={0x2e60,0x09d5};pr_r0899_cpu c,before;
    CHECK(r && saved && ega && old);
    input_setup(&b,&c,'a');c.ip=0x060c;r[p(c.cs,0x037d+'A'/8)]|=(uint8_t)(1u<<('A'%8));
    r[p(c.ss,c.bp-0x52)]='A';r[p(c.ss,0x4100)]=0;
    r[p(c.cs,0x685)]=0xe8;put(r,c.cs,0x686,(uint16_t)(0x01da-0x0688));
    CHECK(pr_u3b0_mapped_060c(&b,PR_BUS_RAM_SIZE,&c,&env,&md)==PR_U3B0_MAPPED_TEXT_SEARCH);
    CHECK(pr_u3b0_text_0634(&b,PR_BUS_RAM_SIZE,&c,&lim,&d)==PR_U3B0_TEXT_BOUNDARY);
    CHECK(c.ip==0x0691 && c.sp==0x8000 && r[p(c.ds,0x6b2b)]==1 && r[p(c.ss,c.bp-0x63)]==1 && d.outer_jumps==3 && d.inner_jumps==0 && outs>0 && writes>0);
    CHECK(pr_u3b0_mapped_060c(&b,PR_BUS_RAM_SIZE,&c,&env,&md)==PR_U3B0_MAPPED_BOUNDARY && c.ip==0x06cf);
    CHECK(pr_u3b0_return_06cf(r,PR_BUS_RAM_SIZE,&c,&target)==PR_U3B0_RETURN_DONE && c.ip==0x09d5 && (c.ax&255u)=='A');
    input_setup(&b,&c,'Z');c.ip=0x0634;r[p(c.ss,c.bp-0x64)]=0;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    saved[p(c.ss,c.bp-0x90)]=0;before.ax=(uint16_t)((before.ax&0xff00u)|1u);before.ip=0x0691;before.flags=(uint16_t)(before.flags&(uint16_t)~0x08d5u);
    CHECK(pr_u3b0_text_0634(&b,PR_BUS_RAM_SIZE,&c,&lim,&d)==PR_U3B0_TEXT_BOUNDARY && memcmp(&c,&before,sizeof c)==0 && memcmp(r,saved,PR_BUS_RAM_SIZE)==0 && memcmp(ega,old,sizeof *ega)==0);
    input_setup(&b,&c,'Z');c.ip=0x0634;r[p(c.ss,c.bp-0x60)]='Z';
    CHECK(pr_u3b0_text_0634(&b,PR_BUS_RAM_SIZE,&c,&lim,&d)==PR_U3B0_TEXT_BOUNDARY && c.ip==0x0691 && d.outer_jumps==3 && outs==0 && writes==0);
    input_setup(&b,&c,'Z');c.ip=0x0634;r[p(c.ss,c.bp-0x60)]='Z';lim.outer_jumps=0;
    CHECK(pr_u3b0_text_0634(&b,PR_BUS_RAM_SIZE,&c,&lim,&d)==PR_U3B0_TEXT_LIMIT && c.ip==0x068f && r[p(c.ss,c.bp-0x62)]==1 && d.outer_jumps==0);lim.outer_jumps=16;
    input_setup(&b,&c,'A');c.ip=0x0634;r[p(c.ss,c.bp-0x60)]='A';r[p(c.ss,c.bp-0x52)]='A';r[p(c.ss,c.bp-0x8d)]=0;lim.inner_jumps=0;
    CHECK(pr_u3b0_text_0634(&b,PR_BUS_RAM_SIZE,&c,&lim,&d)==PR_U3B0_TEXT_LIMIT && c.ip==0x0677 && r[p(c.ss,c.bp-0x61)]==2 && r[p(c.ss,c.bp-0x63)]==1 && outs==0);lim.inner_jumps=300;
    input_setup(&b,&c,'A');c.ip=0x0634;r[p(c.ss,c.bp-0x60)]='A';r[p(c.ss,c.bp-0x52)]='A';r[p(c.cs,0x685)]=0x90;
    CHECK(pr_u3b0_text_0634(&b,PR_BUS_RAM_SIZE,&c,&lim,&d)==PR_U3B0_TEXT_CODE && c.ip==0x0685 && c.sp==0x7ffa && r[p(c.ds,0x6b2b)]==1 && r[p(c.ss,c.bp-0x63)]==1);
    input_setup(&b,&c,'A');c.ip=0x0634;r[p(c.ss,c.bp-0x60)]='A';r[p(c.ss,c.bp-0x52)]='A';r[p(c.cs,0x685)]=0xe8;put(r,c.cs,0x686,(uint16_t)(0x01da-0x0688));refuse=0;
    CHECK(pr_u3b0_text_0634(&b,PR_BUS_RAM_SIZE,&c,&lim,&d)==PR_U3B0_TEXT_CALLEE && d.status==PR_U3B0_01DA_CALLEE && r[p(0x1800,0x6b2b)]==1 && outs==0);
    input_setup(&b,&c,'A');c.ip=0x0634;c.ds=c.ss;c.bp=0x6bb9;
    r[p(c.ss,c.bp-0x64)]=1;r[p(c.ss,c.bp-0x60)]='A';r[p(c.ss,c.bp-0x52)]='A';
    r[p(c.ss,c.bp-0x8d)]=1;r[p(c.ds,0x6b2b)]=9;r[p(c.cs,0x685)]=0x90;
    CHECK(pr_u3b0_text_0634(&b,PR_BUS_RAM_SIZE,&c,&lim,&d)==PR_U3B0_TEXT_CODE && c.ip==0x0685 && r[p(c.ss,c.bp-0x8e)]==1);
    input_setup(&b,&c,'A');c.ip=0x0634;c.bp=0x8f;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u3b0_text_0634(&b,PR_BUS_RAM_SIZE,&c,&lim,&d)==PR_U3B0_TEXT_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(r,saved,PR_BUS_RAM_SIZE)==0 && memcmp(ega,old,sizeof *ega)==0);
    free(old);free(ega);free(saved);free(r);printf("3B0 text search: %u checks passed\n",checks);return 0;
}
