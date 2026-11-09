#include "poolrad/replay/u25_operand_return.h"
#include "poolrad/recovered/overlay_u3b0_entry_01da_cpu.h"
#include "poolrad/replay/u3b0_selection_05c4.h"
#include "poolrad/replay/u3b0_mapped_060c.h"
#include "poolrad/replay/u3b0_text_0634.h"
#include "poolrad/replay/u3b0_prompt_pass.h"
#include "poolrad/replay/u3b0_wait_048e.h"
#include "poolrad/replay/u3b0_draw_wait_047a.h"
#include "poolrad/replay/u3b0_clock_draw_0432.h"
#include "poolrad/replay/u3b0_entry_03bd.h"
#include "poolrad/replay/u3b0_prompt_session.h"
#include "poolrad/replay/u68_prompt_098f.h"
#include "poolrad/replay/u68_290b_first_field.h"
#include "poolrad/replay/u25_operand_loop.h"
#include "poolrad/replay/crt_input_fifo.h"
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
    (void)ctx;(void)value;(void)ip;CHECK(size==1 && cs==0x8800);CHECK(port==0x3C4 || port==0x3C5 || port==0x3CE || port==0x3CF);
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
static int vector(void*x,uint8_t*r,pr_r08ae_driver_cpu*c,uint16_t*f){(void)x;(void)r;(void)c;(void)f;return 1;}
static int portout(void*x,uint16_t p0,uint8_t v){(void)x;(void)p0;(void)v;return 1;}
static int portin(void*x,uint16_t p0,uint8_t*v){(void)x;(void)p0;(void)v;return 1;}
static int timefn(void*x,const pr_dos_gettime_cpu*b,pr_dos_gettime_cpu*a){(void)x;(void)b;(void)a;return 1;}
static int video(void*x,uint8_t*r,const pr_dos_intr_cpu*b,pr_dos_intr_cpu*a){(void)x;(void)r;(void)b;(void)a;return 1;}
static void pass_setup(pr_bus*b,pr_r0899_cpu*c,pr_crt_input_fifo*q){
    static const uint16_t sites[8][2]={{0x63,0x1a40},{0x82,0x18ae},{0x97,0x1af8},{0xa4,0x18ae},{0xb3,0x1af8},{0xbd,0x1af8},{0xe7,0x1a40},{0xf0,0x1a40}};
    uint8_t*r=b->ram;unsigned i;input_setup(b,c,'a');c->ip=0x0566;pr_crt_input_fifo_init(q);
    farcall(r,c->cs,0x566,0x1a40,0x02fd);farcall(r,c->cs,0x572,0x1899,0x0059);farcall(r,c->cs,0x580,0x1899,0x0059);
    for(i=0;i<8;i++)put(r,0x1899,sites[i][0],sites[i][1]);
    put(r,0x1899,0x10,0x18ae);put(r,0x1899,0x28,0x1a32);put(r,0x1899,0x2f,0x1af8);
    memset(r+p(0x1899,0x33),0,32);r[p(0x1899,0x33)]=3;memcpy(r+p(0x1899,0x53),"\x05" "HELLO",6);
    put(r,c->ds,0x4422,0x6800);r[p(0x6800,0x80)]=0;r[p(c->ds,0x8501)]=0;r[p(c->ds,0x8502)]=0;
    r[p(c->ds,0x5372)]=1;put(r,c->ds,0x2612,1);r[p(c->ss,0x4100)]=0;
    r[p(c->cs,0x685)]=0xe8;put(r,c->cs,0x686,(uint16_t)(0x01da-0x0688));
}
static void wait_setup(pr_bus*b,pr_r0899_cpu*c,pr_crt_input_fifo*q){
    pass_setup(b,c,q);c->ip=0x048e;put(b->ram,0x5000,0x13fe,3);
    b->ram[p(c->ds,0x6abf)]=0;put(b->ram,c->ds,0x6b30,0);put(b->ram,c->ds,0x6b2e,0);
}
static void draw_setup(pr_bus*b,pr_r0899_cpu*c,pr_crt_input_fifo*q){
    wait_setup(b,c,q);c->ip=0x047a;b->ram[p(c->ss,c->bp-0x2a)]=0;b->ram[p(c->ds,0x6b2b)]=1;
    b->ram[p(c->cs,0x48b)]=0xe8;put(b->ram,c->cs,0x48c,(uint16_t)(0x01da-0x048e));
}
typedef struct { unsigned calls, refuse_on; } clock_state;
static int clock_service(void *ctx,const pr_dos_gettime_cpu *before,pr_dos_gettime_cpu *after){
    clock_state *s=ctx;++s->calls;if(s->calls==s->refuse_on)return 1;
    *after=*before;after->cx=(uint16_t)(s->calls==1?0:0x000a);after->dx=(uint16_t)(s->calls==1?5:0x3706);after->ip=0x00a9;return 0;
}
static void clock_draw_setup(pr_bus*b,pr_r0899_cpu*c,pr_crt_input_fifo*q,unsigned label){
    uint8_t*r=b->ram;unsigned i;draw_setup(b,c,q);c->ip=0x0432;
    farcall(r,c->cs,0x0432,0x8800,0x0c3f);farcall(r,c->cs,0x043d,0x8800,0x0c3f);
    farcall(r,c->cs,0x0475,0x8800,0x0626);put(r,0x8800,0x0c5c,0x6000);put(r,0x8800,0x0c6c,0x6800);
    farcall(r,0x8800,0x063c,0x4000,0x0b25);r[p(0x8800,0x0691)]=0xe8;put(r,0x8800,0x0692,0xfb4b);
    for(i=0;i<256;i++)r[p(c->ds,0x25d5+i)]=(uint8_t)(i%4u);
    for(i=0;i<4;i++)put(r,c->ds,0x25cd+2u*i,i*0x5555u);
    for(i=0;i<8;i++)put(r,c->ds,0x25e5+2u*i,0xc000u>>(2u*i));
    r[p(c->ss,c->bp-0x2a)]=(uint8_t)label;r[p(c->ss,c->bp-0x29)]='a';r[p(c->ss,c->bp+0x12)]=5;
}
static void entry_setup(pr_bus*b,pr_r0899_cpu*c,pr_crt_input_fifo*q,unsigned label){
    uint8_t *r=b->ram;clock_draw_setup(b,c,q,0);memcpy(r+p(0x6000,0),r+p(c->cs,0),65536);c->cs=0x6000;c->ip=0x03bd;c->sp=0x4002;c->bp=0x7777;
    put(r,c->ss,0x4002,0x09d5);put(r,c->ss,0x4004,0x2e60);
    put(r,c->ss,0x4018,0x0800);put(r,c->ss,0x401a,0x4000);
    put(r,c->ss,0x4014,0x0900);put(r,c->ss,0x4016,0x4000);
    r[p(0x4000,0x0800)]=(uint8_t)label;r[p(0x4000,0x0801)]='a';
    r[p(0x4000,0x0900)]=1;r[p(0x4000,0x0901)]='a';
    r[p(c->ss,0x4010)]=0;r[p(c->ss,0x400e)]=0;r[p(c->ss,0x4012)]=5;
    farcall(r,c->cs,0x03d2,0x4000,0x0b25);farcall(r,c->cs,0x03e5,0x4000,0x0b25);
    r[p(c->cs,0x040c)]=0xe8;put(r,c->cs,0x040d,(uint16_t)(0x00cb-0x040f));
    farcall(r,c->cs,0x00df,0x4000,0x0b25);farcall(r,c->cs,0x00f4,0x4000,0x16a8);farcall(r,c->cs,0x012b,0x4000,0x0dc0);
    memset(r+p(c->cs,0x00ab),0,32);r[p(c->cs,0x00ab+'a'/8)]|=(uint8_t)(1u<<('a'%8));
}
static void u68_setup(pr_bus*b,pr_r0899_cpu*c,pr_crt_input_fifo*q){
    uint8_t *r=b->ram;entry_setup(b,c,q,0);c->cs=0x7600;c->ip=0x098f;c->bp=0x4058;c->sp=0x401c;
    put(r,c->ds,0x49d6,0x3000);put(r,c->ds,0x49d8,0x5000);put(r,0x5000,0x3592,0xa5a5);put(r,0x5000,0x35aa,0xa5a5);
    r[p(c->ds,0x49fa)]=1;r[p(c->ds,0x04b6)]=1;r[p(c->ds,0x04b7)]='a';r[p(c->cs,0x097a)]=1;r[p(c->cs,0x097b)]='a';
    farcall(r,c->cs,0x09b2,0x4000,0x0b0b);farcall(r,c->cs,0x09d0,0x5000,0x002a);
    r[p(0x5000,0x002a)]=0xea;put(r,0x5000,0x002b,0x03bd);put(r,0x5000,0x002d,0x6000);
    r[p(c->cs,0x0ab2)]=0xe8;put(r,c->cs,0x0ab3,(uint16_t)(0x068d-0x0ab5));
    put(r,c->cs,0x06c1,0x002f);put(r,c->cs,0x06c3,0x5100);r[p(0x5100,0x002f)]=0xea;
    put(r,0x5100,0x0030,0x0350);put(r,0x5100,0x0032,0x7000);
    r[p(c->ds,0x6aad)]=5;r[p(c->ds,0x6aae)]=3;r[p(c->ds,0x6aaf)]=2;
    r[p(c->ds,0x2738)]=1;r[p(c->ds,0x2741)]=0;r[p(c->ds,0x84dc)]=1;
    put(r,c->ds,0x6a5c,0x2000);put(r,c->ds,0x6a5e,0x4000);memset(r+p(0x4000,0x2000),0,0x402);
}
static void display_setup(pr_bus*b,pr_r0899_cpu*c,pr_crt_input_fifo*q,unsigned value){
    uint8_t*r=b->ram;uint32_t power=1000000000u;unsigned i;u68_setup(b,c,q);
    farcall(r,c->cs,0x0b9b,0x6800,0x290b);r[p(0x6800,0x2967)]=0xe8;put(r,0x6800,0x2968,(uint16_t)(0x1073-0x296a));
    farcall(r,0x6800,0x2973,0x0af8,0x0b25);farcall(r,0x6800,0x2989,0x0af8,0x0b0b);
    farcall(r,0x6800,0x2993,0x0af8,0x0b98);farcall(r,0x6800,0x29a1,0x0af8,0x0b25);
    farcall(r,0x6800,0x108e,0x0af8,0x1366);farcall(r,0x6800,0x10a2,0x0af8,0x0b25);
    for(i=0;i<10;i++,power/=10){put(r,0x0af8,0x12a6+4*i,power);put(r,0x0af8,0x12a8+4*i,power>>16);}
    r[p(0x6800,0x29c3)]=0xe8;put(r,0x6800,0x29c4,(uint16_t)(0x1073-0x29c6));
    farcall(r,0x6800,0x29cf,0x0af8,0x0b25);farcall(r,0x6800,0x29e5,0x0af8,0x0b0b);
    farcall(r,0x6800,0x29ef,0x0af8,0x0b98);farcall(r,0x6800,0x29fd,0x0af8,0x0b25);
    r[p(0x6800,0x2a19)]=0xe8;put(r,0x6800,0x2a1a,(uint16_t)(0x1037-0x2a1c));
    r[p(0x6800,0x2a31)]=0xe8;put(r,0x6800,0x2a32,(uint16_t)(0x1037-0x2a34));
    farcall(r,0x6800,0x1054,0x0af8,0x1366);farcall(r,0x6800,0x1068,0x0af8,0x0b25);
    farcall(r,0x6800,0x2a21,0x0af8,0x0b98);farcall(r,0x6800,0x2a34,0x0af8,0x0b98);
    farcall(r,0x6800,0x2a3e,0x0af8,0x0b98);farcall(r,0x6800,0x2a4c,0x0af8,0x0b25);
    r[p(0x6800,0x28f4)]=1;r[p(0x6800,0x28f5)]='/';r[p(0x6800,0x28f6)]=1;r[p(0x6800,0x28f7)]='!';
    farcall(r,0x6800,0x2a5c,0x0af8,0x0b0b);farcall(r,0x6800,0x2a74,0x0af8,0x0b98);
    farcall(r,0x6800,0x2a7e,0x0af8,0x0b98);farcall(r,0x6800,0x2a8c,0x0af8,0x0b25);
    farcall(r,0x6800,0x2a9c,0x0af8,0x0b0b);farcall(r,0x6800,0x2aa6,0x0af8,0x0b98);
    farcall(r,0x6800,0x2ab0,0x0af8,0x0b98);farcall(r,0x6800,0x2aba,0x0af8,0x0b98);
    farcall(r,0x6800,0x2ac8,0x0af8,0x0b25);r[p(0x6800,0x28f8)]=1;r[p(0x6800,0x28f9)]='|';
    r[p(c->ds,0x25ae)]=1;r[p(c->ds,0x25af)]='A';
    farcall(r,0x6800,0x2adf,0x0af8,0x0b0b);farcall(r,0x6800,0x2ae9,0x0af8,0x0b98);farcall(r,0x6800,0x2af7,0x0af8,0x0b25);
    farcall(r,0x6800,0x2b19,0x0af8,0x0b0b);farcall(r,0x6800,0x2b23,0x0af8,0x0b98);farcall(r,0x6800,0x2b31,0x0af8,0x0b25);
    r[p(0x6800,0x28fa)]=1;r[p(0x6800,0x28fb)]='M';r[p(0x6800,0x2903)]=1;r[p(0x6800,0x2904)]='F';
    {const uint16_t sites[]={0x2f38,0x2f43,0x2f4e,0x2f59,0x2f70,0x2f7b,0x2f86},targets[]={1,0xf4,0x111,0x126,0x1a3,0x234,0x292};
    for(i=0;i<7;i++){r[p(0x7900,sites[i])]=0xe8;put(r,0x7900,sites[i]+1,(uint16_t)(targets[i]-sites[i]-3));}}
    farcall(r,0x7900,0x029b,0x5100,0x002a);r[p(0x5100,0x002a)]=0xea;put(r,0x5100,0x002b,0);put(r,0x5100,0x002d,0x7100);
    r[p(0x7900,0x31a5)]=0xe8;put(r,0x7900,0x31a6,(uint16_t)(0x2f2d-0x31a8));
    r[p(0x7900,0x3458)]=0xe8;put(r,0x7900,0x3459,(uint16_t)(0x3171-0x345b));
    farcall(r,0x7900,0x33c9,0x4000,0x182a);farcall(r,0x7900,0x33dd,0x4000,0x182a);
    farcall(r,0x7600,0x0d35,0x7400,0x0137);
    farcall(r,0x6800,0x2b55,0x8800,0x0626);
    farcall(r,0x6800,0x2b42,0x7400,0x0137);farcall(r,0x7400,0x01f1,0x8800,0x00b7);farcall(r,0x7400,0x020c,0x8800,0x00b7);
    farcall(r,0x8800,0x0140,0x4000,0x16a8);
    r[p(0x6800,0x28f2)]=1;r[p(0x6800,0x28f3)]='X';put(r,0x5000,0x1192,value);
    put(r,0x5000,0x1190,0);put(r,0x5000,0x118e,5);
}
static void enter_290b(pr_bus*b,pr_r0899_cpu*c){c->cs=0x6800;c->ip=0x290b;c->sp=0x4018;c->bp=0x4058;
    put(b->ram,c->ss,0x4018,0x0ba0);put(b->ram,c->ss,0x401a,0x7600);}
int main(void){
    uint8_t*r=malloc(PR_BUS_RAM_SIZE),*saved=malloc(PR_BUS_RAM_SIZE);pr_ega*ega=malloc(sizeof *ega),*old=malloc(sizeof *old);
    pr_port_env ports={NULL,out,NULL};pr_bus b={r,ega,&ports,write,NULL};pr_crt_input_fifo q;pr_r0899_cpu c,before;
    pr_r0899_key_env ke={{{NULL,vector,portout,portin,timefn,1000u},video,NULL},pr_crt_input_fifo_service,&q,8u};
    pr_u3b0_draw_wait_env env={{65536,65536},{{0x1a40,0x1899,&ke},{0x4000},{65536,65536},{16,300,{65536,65536}},{0x2e60,0x09d5}}};
    pr_u3b0_draw_wait_diagnostic d;CHECK(r&&saved&&ega&&old);
    draw_setup(&b,&c,&q);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_draw_wait_047a(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_DRAW_WAIT_REACHED && d.drawer_status==PR_U3B0_01DA_OK && d.wait_status==PR_U3B0_WAIT_PASS && d.wait.status==PR_U3B0_PASS_RETURN && c.ip==0x09d5 && (c.ax&255u)==0x48 && r[p(0x1800,0x6b2c)]==1 && outs>0 && writes>0);
    draw_setup(&b,&c,&q);CHECK(pr_u3b0_draw_wait_047a(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_DRAW_WAIT_REACHED && d.wait_status==PR_U3B0_WAIT_PASS && d.wait.status==PR_U3B0_PASS_LOOP && c.ip==0x048e && outs>0 && writes>0);
    draw_setup(&b,&c,&q);r[p(c.ds,0x6abf)]=1;CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_draw_wait_047a(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_DRAW_WAIT_REACHED && d.wait_status==PR_U3B0_WAIT_ANIMATION && c.ip==0x04ad && q.count==1 && outs>0 && writes>0);
    draw_setup(&b,&c,&q);refuse=0;CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_draw_wait_047a(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_DRAW_WAIT_CALLEE && d.drawer_status==PR_U3B0_01DA_CALLEE && q.count==1 && r[p(0x2800,0x3f9b)]==0 && outs==0);
    draw_setup(&b,&c,&q);r[p(c.cs,0x48b)]=0x90;CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    saved[p(c.ss,c.bp-0x65)]=0;put(saved,c.ss,0x7ffe,1);put(saved,c.ss,0x7ffc,c.bp);put(saved,c.ss,0x7ffa,c.cs);
    before.ax=1;before.sp=0x7ffa;before.ip=0x048b;before.flags=(uint16_t)((before.flags&(uint16_t)~0x08d5u)|0x44u);
    CHECK(pr_u3b0_draw_wait_047a(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_DRAW_WAIT_CODE && q.count==1 && memcmp(&c,&before,sizeof c)==0 && memcmp(r,saved,PR_BUS_RAM_SIZE)==0 && memcmp(ega,old,sizeof *ega)==0);
    draw_setup(&b,&c,&q);c.ds=c.ss;c.bp=0x6b90;r[p(c.ss,c.bp-0x2a)]=7;r[p(c.cs,0x48b)]=0x90;
    CHECK(pr_u3b0_draw_wait_047a(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_DRAW_WAIT_CODE && c.ip==0x048b && r[p(c.ds,0x6b2b)]==7 && r[p(c.ss,0x7ffe)]==7 && r[p(c.ss,0x7fff)]==0);
    draw_setup(&b,&c,&q);env.pass.poll.key_env=NULL;CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_draw_wait_047a(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_DRAW_WAIT_REACHED && d.wait_status==PR_U3B0_WAIT_PASS && d.wait.status==PR_U3B0_PASS_ARGUMENT && c.ip==0x0566 && q.count==1 && outs>0 && writes>0);env.pass.poll.key_env=&ke;
    draw_setup(&b,&c,&q);c.bp=0x8f;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u3b0_draw_wait_047a(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_DRAW_WAIT_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(r,saved,PR_BUS_RAM_SIZE)==0 && memcmp(ega,old,sizeof *ega)==0);
    /* Real clocks, optional real label printer, then the existing drawer/input path. */
    {clock_state ts={0,0};pr_u3b0_clock_draw_env ce={clock_service,&ts,65536,65536,env};pr_u3b0_clock_draw_diagnostic cd;
    clock_draw_setup(&b,&c,&q,0);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_clock_draw_0432(&b,PR_BUS_RAM_SIZE,&c,&ce,&cd)==PR_U3B0_CLOCK_DRAW_REACHED);
    CHECK(ts.calls==2 && cd.phase==3 && cd.draw_status==PR_U3B0_DRAW_WAIT_REACHED && cd.draw.wait_status==PR_U3B0_WAIT_PASS && c.ip==0x09d5);
    CHECK(r[p(0x2800,0x3fa9)]==5 && r[p(0x2800,0x3faa)]==0); /* first sample = 5 */
    CHECK(r[p(0x2800,0x3fa5)]==0 && r[p(0x2800,0x3fa6)]==0 && r[p(0x2800,0x3fa7)]==1); /* +30 carries */
    CHECK(r[p(0x2800,0x3fa1)]==50 && r[p(0x2800,0x3fa3)]==1); /* +50 */
    ts.calls=0;clock_draw_setup(&b,&c,&q,1);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_clock_draw_0432(&b,PR_BUS_RAM_SIZE,&c,&ce,&cd)==PR_U3B0_CLOCK_DRAW_REACHED && cd.printer_status==PR_R0709_PRINTER_CPU_OK && cd.printer.callee==1 && cd.printer.status==0 && cd.phase==3 && ts.calls==2 && outs>0 && writes>0);
    ts.calls=0;ts.refuse_on=1;clock_draw_setup(&b,&c,&q,1);
    CHECK(pr_u3b0_clock_draw_0432(&b,PR_BUS_RAM_SIZE,&c,&ce,&cd)==PR_U3B0_CLOCK_DRAW_CALLEE && cd.phase==0 && cd.clock.gettime_status==PR_DOS_GETTIME_SERVICE && c.cs==0x6000 && c.ip==0x00a7 && ts.calls==1 && outs==0);
    ts.calls=0;ts.refuse_on=2;clock_draw_setup(&b,&c,&q,1);
    CHECK(pr_u3b0_clock_draw_0432(&b,PR_BUS_RAM_SIZE,&c,&ce,&cd)==PR_U3B0_CLOCK_DRAW_CALLEE && cd.phase==1 && cd.clock.gettime_status==PR_DOS_GETTIME_SERVICE && c.cs==0x6000 && c.ip==0x00a7 && ts.calls==2);
    CHECK(r[p(0x2800,0x3fa9)]==5 && r[p(0x2800,0x3faa)]==0 && outs==0);
    ts.calls=0;ts.refuse_on=0;clock_draw_setup(&b,&c,&q,1);refuse=0;
    CHECK(pr_u3b0_clock_draw_0432(&b,PR_BUS_RAM_SIZE,&c,&ce,&cd)==PR_U3B0_CLOCK_DRAW_CALLEE && cd.phase==2 && cd.printer_status==PR_R0709_PRINTER_CPU_CALLEE && ts.calls==2);
    ts.calls=0;clock_draw_setup(&b,&c,&q,1);r[p(c.cs,0x0475)]=0x90;
    CHECK(pr_u3b0_clock_draw_0432(&b,PR_BUS_RAM_SIZE,&c,&ce,&cd)==PR_U3B0_CLOCK_DRAW_CODE && cd.phase==2 && c.ip==0x0475 && c.sp==0x7ff6 && ts.calls==2 && outs==0);
    ts.calls=0;clock_draw_setup(&b,&c,&q,0);c.sp=0x013f;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u3b0_clock_draw_0432(&b,PR_BUS_RAM_SIZE,&c,&ce,&cd)==PR_U3B0_CLOCK_DRAW_ARGUMENT && memcmp(&before,&c,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0 && ts.calls==0);
    ts.calls=0;clock_draw_setup(&b,&c,&q,0);r[p(c.cs,0x043d)]=0x90;
    CHECK(pr_u3b0_clock_draw_0432(&b,PR_BUS_RAM_SIZE,&c,&ce,&cd)==PR_U3B0_CLOCK_DRAW_CODE && cd.phase==1 && c.ip==0x043d && ts.calls==1 && r[p(0x2800,0x3fa9)]==5);
    }
    /* Real entry setup, string stores and split precede real clocks/drawer/input. */
    {clock_state ts={0,0};pr_u3b0_clock_draw_env ce={clock_service,&ts,65536,65536,env};pr_u3b0_entry_diagnostic ed;
    entry_setup(&b,&c,&q,1);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_entry_03bd(&b,PR_BUS_RAM_SIZE,&c,&ce,&ed)==PR_U3B0_ENTRY_REACHED);
    CHECK(ed.phase==3 && ed.split_status==PR_U3B0_00CB_OK && ed.clock_draw_status==PR_U3B0_CLOCK_DRAW_REACHED && ed.clock_draw.printer_status==PR_R0709_PRINTER_CPU_OK && ed.clock_draw.printer.callee==1 && ed.clock_draw.draw.wait_status==PR_U3B0_WAIT_PASS && c.ip==0x09d5 && ts.calls==2);
    CHECK(r[p(0x2800,0x3fd6)]==1 && r[p(0x2800,0x3fd7)]=='a' && r[p(0x2800,0x3fad)]==1 && r[p(0x2800,0x3fae)]=='a');
    entry_setup(&b,&c,&q,0);put(r,c.ss,0x401a,0xa000);before=c;memcpy(old,ega,sizeof *ega);
    CHECK(pr_u3b0_entry_03bd(&b,PR_BUS_RAM_SIZE,&c,&ce,&ed)==PR_U3B0_ENTRY_PROFILE && ed.phase==0 && c.ip==0x03d2 && c.sp==0x3f66 && memcmp(old,ega,sizeof *ega)==0);
    entry_setup(&b,&c,&q,1);put(r,c.ss,0x4016,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u3b0_entry_03bd(&b,PR_BUS_RAM_SIZE,&c,&ce,&ed)==PR_U3B0_ENTRY_PROFILE && ed.phase==1 && c.ip==0x03e5 && r[p(0x2800,0x3fd6)]==1 && r[p(0x2800,0x3fd7)]=='a' && memcmp(old,ega,sizeof *ega)==0);
    entry_setup(&b,&c,&q,0);r[p(c.cs,0x03e5)]=0x90;
    CHECK(pr_u3b0_entry_03bd(&b,PR_BUS_RAM_SIZE,&c,&ce,&ed)==PR_U3B0_ENTRY_CODE && ed.phase==1 && c.ip==0x03e5 && r[p(0x2800,0x3fd6)]==0);
    entry_setup(&b,&c,&q,0);r[p(c.cs,0x040c)]=0x90;
    CHECK(pr_u3b0_entry_03bd(&b,PR_BUS_RAM_SIZE,&c,&ce,&ed)==PR_U3B0_ENTRY_CODE && ed.phase==2 && c.ip==0x040c && r[p(0x2800,0x3fad)]==1);
    entry_setup(&b,&c,&q,0);r[p(c.cs,0x00df)]=0x90;
    CHECK(pr_u3b0_entry_03bd(&b,PR_BUS_RAM_SIZE,&c,&ce,&ed)==PR_U3B0_ENTRY_CALLEE && ed.phase==2 && ed.split_status==PR_U3B0_00CB_CODE && c.ip==0x00df);
    entry_setup(&b,&c,&q,0);c.sp=0x01d1;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u3b0_entry_03bd(&b,PR_BUS_RAM_SIZE,&c,&ce,&ed)==PR_U3B0_ENTRY_ARGUMENT && memcmp(&before,&c,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    }
    /* A session yields only at its own no-event loop, then consumes real FIFO input. */
    {clock_state ts={0,0};pr_u3b0_clock_draw_env ce={clock_service,&ts,65536,65536,env};pr_u3b0_prompt_session session;pr_u3b0_session_diagnostic sd;const pr_port_env *saved_ports;
    entry_setup(&b,&c,&q,1);pr_u3b0_prompt_session_init(&session);
    CHECK(pr_u3b0_prompt_session_start(&session,&b,PR_BUS_RAM_SIZE,&c,&ce,&sd)==PR_U3B0_SESSION_YIELD && session.phase==PR_U3B0_SESSION_WAITING && c.ip==0x048e && ts.calls==2 && q.count==0);
    memcpy(old,ega,sizeof *ega);before=c;
    CHECK(pr_u3b0_prompt_session_resume(&session,&sd)==PR_U3B0_SESSION_YIELD && c.ip==0x048e && ts.calls==2 && q.count==0 && memcmp(old,ega,sizeof *ega)==0);
    before=c;c.ax^=1;
    CHECK(pr_u3b0_prompt_session_resume(&session,&sd)==PR_U3B0_SESSION_ARGUMENT && session.phase==PR_U3B0_SESSION_WAITING && ts.calls==2);c=before;
    saved_ports=b.ports;b.ports=NULL;
    CHECK(pr_u3b0_prompt_session_resume(&session,&sd)==PR_U3B0_SESSION_ARGUMENT && session.phase==PR_U3B0_SESSION_WAITING);b.ports=saved_ports;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_prompt_session_resume(&session,&sd)==PR_U3B0_SESSION_RETURN && session.phase==PR_U3B0_SESSION_FINISHED && c.ip==0x09d5 && (c.ax&255u)==0x48 && q.count==0 && ts.calls==2);
    CHECK(pr_u3b0_prompt_session_resume(&session,&sd)==PR_U3B0_SESSION_ARGUMENT);
    ts.calls=0;entry_setup(&b,&c,&q,0);pr_u3b0_prompt_session_init(&session);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_prompt_session_start(&session,&b,PR_BUS_RAM_SIZE,&c,&ce,&sd)==PR_U3B0_SESSION_RETURN && session.phase==PR_U3B0_SESSION_FINISHED && (c.ax&255u)==0x48 && ts.calls==2);
    entry_setup(&b,&c,&q,0);r[p(c.ds,0x6abf)]=1;pr_u3b0_prompt_session_init(&session);
    CHECK(pr_u3b0_prompt_session_start(&session,&b,PR_BUS_RAM_SIZE,&c,&ce,&sd)==PR_U3B0_SESSION_STOP && session.phase==PR_U3B0_SESSION_FINISHED && c.ip==0x04ad);
    CHECK(pr_u3b0_prompt_session_resume(&session,&sd)==PR_U3B0_SESSION_ARGUMENT);
    }
    /* U68 real preparation, suspended prompt and Up preflight share one bus. */
    {clock_state ts={0,0};pr_u3b0_clock_draw_env ce={clock_service,&ts,65536,65536,env};pr_u68_prompt us;pr_u68_prompt_diagnostic ud;
    ce.draw.pass.target.cs=0x7600;ce.draw.pass.target.ip=0x09d5;u68_setup(&b,&c,&q);pr_u68_prompt_init(&us);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_YIELD && us.phase==PR_U68_PROMPT_WAITING && c.ip==0x048e && ts.calls==2);
    CHECK(r[p(0x5000,0x3592)]==0 && r[p(0x5000,0x3593)]==0 && r[p(0x5000,0x35aa)]==0xa5 && r[p(0x2800,0x4025)]==1 && r[p(0x2800,0x4026)]=='a');
    before=c;c.ax^=1;CHECK(pr_u68_prompt_resume(&us,&ud)==PR_U68_PROMPT_ARGUMENT && us.phase==PR_U68_PROMPT_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_prompt_resume(&us,&ud)==PR_U68_PROMPT_DISPLAY && c.cs==0x7600 && c.ip==0x0b9b && c.bp==0x4058 && c.sp==0x401c && ts.calls==2 && q.count==0);
    CHECK(r[p(0x2800,0x4054)]==1 && r[p(0x2800,0x4055)]==0x48 && r[p(0x2800,0x4056)]==1 && r[p(0x5000,0x35aa)]==0 && ud.preflight_status==PR_U68_068D_RETURNED);
    CHECK(pr_u68_prompt_resume(&us,&ud)==PR_U68_PROMPT_ARGUMENT);
    u68_setup(&b,&c,&q);c.bp++;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);pr_u68_prompt_init(&us);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_ARGUMENT && memcmp(&before,&c,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    u68_setup(&b,&c,&q);r[p(c.ds,0x49fa)]=2;pr_u68_prompt_init(&us);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_OUTSIDE && c.ip==0x0bac && r[p(0x5000,0x3592)]==0);
    u68_setup(&b,&c,&q);put(r,c.ds,0x49d6,0x4468);put(r,c.ds,0x49d8,c.ds);pr_u68_prompt_init(&us);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_OUTSIDE && c.ip==0x0bac && r[p(c.ds,0x49fa)]==0);
    u68_setup(&b,&c,&q);put(r,c.ds,0x49d6,0);put(r,c.ds,0x49d8,0xa000);pr_u68_prompt_init(&us);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_YIELD && ega->plane[0][0x592]==0 && ega->plane[0][0x593]==0 && r[p(0xa000,0x592)]==0xa5);
    u68_setup(&b,&c,&q);put(r,c.ds,0x49d6,0);put(r,c.ds,0x49d8,0xa000);pr_u68_prompt_init(&us);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_DISPLAY && c.ip==0x0b9b && ega->plane[0][0x5aa]==0 && r[p(0xa000,0x5aa)]==0xa5);
    u68_setup(&b,&c,&q);put(r,c.ds,0x49d6,0xfa6d);put(r,c.ds,0x49d8,0xa000);memcpy(old,ega,sizeof *ega);pr_u68_prompt_init(&us);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_PROFILE && c.ip==0x0995 && memcmp(old,ega,sizeof *ega)==0);
    u68_setup(&b,&c,&q);r[p(c.cs,0x09b2)]=0x90;pr_u68_prompt_init(&us);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_CODE && c.ip==0x09b2 && r[p(0x5000,0x3592)]==0);
    u68_setup(&b,&c,&q);r[p(0x5000,0x002a)]=0xcd;pr_u68_prompt_init(&us);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_OVERLAY && c.cs==0x5000 && c.ip==0x002a && r[p(0x2800,0x4025)]==1);
    u68_setup(&b,&c,&q);pr_u68_prompt_init(&us);CHECK(pr_crt_input_fifo_push(&q,0x5000)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_OUTSIDE && c.ip==0x0abc && r[p(0x2800,0x4054)]==1 && r[p(0x2800,0x4055)]==0x50 && r[p(0x5000,0x35aa)]==0xa5);
    u68_setup(&b,&c,&q);r[p(0x5100,0x002f)]=0xcd;pr_u68_prompt_init(&us);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_CALLEE && ud.preflight_status==PR_U68_068D_OVERLAY && c.cs==0x5100 && c.ip==0x002f && r[p(0x5000,0x35aa)]==0);
    u68_setup(&b,&c,&q);put(r,c.ds,0x49d6,0x64b2);put(r,c.ds,0x49d8,c.ds);pr_u68_prompt_init(&us);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_PROFILE && c.ip==0x0ab2 && r[p(c.ds,0x6a5e)]==0 && r[p(c.ds,0x6a5f)]==0x40);
    u68_setup(&b,&c,&q);put(r,c.ds,0x6a5e,0xa000);pr_u68_prompt_init(&us);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_prompt_start(&us,&b,PR_BUS_RAM_SIZE,&c,&ce,&ud)==PR_U68_PROMPT_PROFILE && c.ip==0x0ab2 && r[p(0x2800,0x4054)]==1 && r[p(0x5000,0x35aa)]==0xa5);
    }
    /* Source-bound U68 -> 290B first field, preserving idle continuation. */
    {clock_state ts={0,0};pr_u3b0_clock_draw_env ce={clock_service,&ts,65536,65536,env};pr_u68_290b bridge;pr_u68_290b_diagnostic bd;pr_290b_first_diagnostic fd;
    ce.draw.pass.target.cs=0x7600;ce.draw.pass.target.ip=0x09d5;
    display_setup(&b,&c,&q,7);pr_u68_290b_init(&bridge);
    CHECK(pr_u68_290b_start(&bridge,&b,PR_BUS_RAM_SIZE,&c,&ce,&bd)==PR_U68_290B_YIELD && c.ip==0x048e && ts.calls==2);
    before=c;c.ax^=1;CHECK(pr_u68_290b_resume(&bridge,&bd)==PR_U68_290B_ARGUMENT && bridge.phase==PR_U68_290B_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_resume(&bridge,&bd)==PR_U68_290B_FIRST_FIELD && c.cs==0x6800 && c.ip==0x29a6 && c.bp==0x4016 && c.sp==0x3de4 && ts.calls==2);
    CHECK(bd.first.stage==5 && r[p(0x2800,0x4011)]==2 && r[p(0x2800,0x4012)]=='X' && r[p(0x2800,0x4013)]=='7');
    CHECK(pr_u68_290b_resume(&bridge,&bd)==PR_U68_290B_ARGUMENT);
    ts.calls=0;display_setup(&b,&c,&q,42);pr_u68_290b_init(&bridge);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_start(&bridge,&b,PR_BUS_RAM_SIZE,&c,&ce,&bd)==PR_U68_290B_FIRST_FIELD && bd.first.stage==2 && c.ip==0x29a6);
    CHECK(r[p(0x2800,0x4011)]==2 && r[p(0x2800,0x4012)]=='4' && r[p(0x2800,0x4013)]=='2');
    display_setup(&b,&c,&q,256);enter_290b(&b,&c);put(r,c.ds,0x49d2,0x3e83);put(r,c.ds,0x49d4,c.ss);
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE && c.ip==0x29a6 && r[p(c.ss,0x4012)]=='2' && r[p(c.ss,0x4013)]=='2');
    display_setup(&b,&c,&q,7);enter_290b(&b,&c);put(r,c.ds,0x49d2,0);put(r,c.ds,0x49d4,0xa000);
    ega->plane[0][0x192]=7;ega->plane[0][0x193]=0;ega->plane[1][0x193]=0x77;
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE && c.ip==0x29a6 && r[p(c.ss,0x4012)]=='X' && r[p(c.ss,0x4013)]=='7' && ega->latch[1]==0x77);
    display_setup(&b,&c,&q,7);enter_290b(&b,&c);put(r,c.ds,0x49d2,0xfe6d);put(r,c.ds,0x49d4,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_PROFILE && c.ip==0x2961 && memcmp(old,ega,sizeof *ega)==0);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);r[p(c.cs,0x2973)]=0x90;
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CODE && c.ip==0x2973 && fd.stage==2 && r[p(c.ss,0x3ee5)]==2);
    ts.calls=0;display_setup(&b,&c,&q,42);r[p(c.cs,0x0b9b)]=0x90;pr_u68_290b_init(&bridge);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_start(&bridge,&b,PR_BUS_RAM_SIZE,&c,&ce,&bd)==PR_U68_290B_CODE && c.ip==0x0b9b && r[p(0x2800,0x4054)]==1);
    display_setup(&b,&c,&q,7);enter_290b(&b,&c);r[p(c.cs,0x2989)]=0x90;
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CODE && c.ip==0x2989 && fd.stage==3 && r[p(c.ss,0x4011)]==1 && r[p(c.ss,0x4012)]=='7');
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);put(r,c.cs,0x2976,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_PROFILE && c.cs==0xa000 && c.ip==0x0b25 && fd.stage==2 && memcmp(old,ega,sizeof *ega)==0);
    display_setup(&b,&c,&q,7);enter_290b(&b,&c);put(r,c.cs,0x2996,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_PROFILE && c.cs==0xa000 && c.ip==0x0b98 && fd.stage==4 && r[p(c.ss,0x4012)]=='7' && memcmp(old,ega,sizeof *ega)==0);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);put(r,c.cs,0x1091,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CALLEE && c.cs==0xa000 && c.ip==0x1366 && fd.stage==1 && fd.status==PR_RES_1073_PROFILE && memcmp(old,ega,sizeof *ega)==0);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);c.sp=0x043d;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_ARGUMENT && memcmp(&before,&c,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);put(r,0x0af8,0x12ca,2);
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CALLEE && c.ip==0x1366 && fd.stage==1 && fd.status==PR_RES_1073_CALLEE);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);r[p(c.ds,0x49fa)]=2;
    CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_OUTSIDE && c.ip==0x2929 && r[p(c.ss,0x4015)]==5);
    }
    /* U68 idle-to-Up composes both source fields and never repeats setup. */
    {clock_state ts={0,0};pr_u3b0_clock_draw_env ce={clock_service,&ts,65536,65536,env};pr_u68_290b_two bridge;pr_u68_290b_two_diagnostic td;pr_290b_first_diagnostic fd;
    ce.draw.pass.target.cs=0x7600;ce.draw.pass.target.ip=0x09d5;
    display_setup(&b,&c,&q,7);pr_u68_290b_two_init(&bridge);
    CHECK(pr_u68_290b_two_start(&bridge,&b,PR_BUS_RAM_SIZE,&c,&ce,&td)==PR_U68_290B_TWO_YIELD && c.ip==0x048e && ts.calls==2);
    before=c;c.ax^=1;CHECK(pr_u68_290b_two_resume(&bridge,&td)==PR_U68_290B_TWO_ARGUMENT && bridge.phase==PR_U68_290B_TWO_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_two_resume(&bridge,&td)==PR_U68_290B_TWO_FIELDS && c.cs==0x6800 && c.ip==0x2a02 && ts.calls==2 && td.second.stage==5);
    CHECK(r[p(c.ss,0x4011)]==2 && r[p(c.ss,0x4012)]=='X' && r[p(c.ss,0x4013)]=='7' && r[p(c.ss,0x400e)]==2 && r[p(c.ss,0x400f)]=='X' && r[p(c.ss,0x4010)]=='5');
    {pr_u68_290b_branch joined;pr_u68_290b_branch_diagnostic jd;clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);pr_u68_290b_branch_init(&joined);
    CHECK(pr_u68_290b_branch_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&jd)==PR_U68_290B_BRANCH_YIELD);
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_branch_resume(&joined,&jd)==PR_U68_290B_BRANCH_REACHED && c.ip==0x2a51 && jd.branch.stage==6);
    CHECK(r[p(c.ss,c.bp-0x31)]==4 && memcmp(r+p(c.ss,c.bp-0x30),"5/3!",4)==0);
    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u25_loop_session joined;pr_u25_loop_diagnostic ld;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x339a,0x405e,0x4060,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x7934)]=9;r[p(0x5000,0x7935)]=2;r[p(0x5000,0x7936)]=0x42;r[p(0x5000,0x7937)]=0x77;r[p(0x5000,0x7938)]=1;r[p(0x5000,0x7939)]=0x22;r[p(0x5000,0x793a)]=0x33;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u25_loop_session_init(&joined);
    CHECK(pr_u25_loop_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,1,&ld)==PR_U25_LOOP_YIELD);
    target.ip=0x1234;before=c;c.ax^=1;CHECK(pr_u25_loop_resume(&joined,&ld)==PR_U25_LOOP_ARGUMENT && joined.phase==PR_U25_LOOP_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u25_loop_resume(&joined,&ld)==PR_U25_LOOP_REACHED && c.cs==0x7100 && c.ip==0x012c && ld.loop.iterations==1 && (c.flags&0x08d5u)==0x44);
    CHECK(r[p(c.ds,0x7006)]==2 && r[p(c.ds,0x7086)]==0x42 && r[p(c.ds,0x7046)]==0x77 && r[p(c.ds,0x7007)]==1 && r[p(c.ds,0x7087)]==0x22 && r[p(c.ds,0x7047)]==0x33 && r[p(c.ds,0x49ed)]==0x3a);
    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u25_return_session joined;pr_u25_return_diagnostic ld;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x339a,0x405e,0x4060,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x7934)]=9;r[p(0x5000,0x7935)]=2;r[p(0x5000,0x7936)]=0x42;r[p(0x5000,0x7937)]=0x77;r[p(0x5000,0x7938)]=1;r[p(0x5000,0x7939)]=0x22;r[p(0x5000,0x793a)]=0x33;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u25_return_session_init(&joined);
    CHECK(pr_u25_return_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,1,&ld)==PR_U25_RETURN_YIELD);
    target.ip=0x1234;before=c;c.ax^=1;CHECK(pr_u25_return_resume(&joined,&ld)==PR_U25_RETURN_ARGUMENT && joined.phase==PR_U25_RETURN_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u25_return_resume(&joined,&ld)==PR_U25_RETURN_REACHED && c.cs==0x7900 && c.ip==0x02a0 && c.sp==0x4046 && c.bp==0x404a && ld.loop.loop.iterations==1 && (c.flags&0x08d5u)==0);
    CHECK(r[p(c.ds,0x7006)]==2 && r[p(c.ds,0x7086)]==0x42 && r[p(c.ds,0x7046)]==0x77 && r[p(c.ds,0x7007)]==1 && r[p(c.ds,0x7087)]==0x22 && r[p(c.ds,0x7047)]==0x33 && r[p(c.ds,0x49ed)]==0x3b);
    {pr_290b_return_target x={0x7900,0x02a0,0x49f3,0x4444,c.ds};
    c.ss=c.ds;c.bp=0x49eb;c.sp=0x49e3;c.ip=0x012c;put(r,c.ss,c.bp,0x4444);put(r,c.ss,c.bp+2,0x02a0);put(r,c.ss,c.bp+4,0x7900);put(r,c.ss,c.bp+6,2);
    CHECK(pr_u25_operand_return(&b,PR_BUS_RAM_SIZE,&c,&x)==PR_290B_FIRST_TRANSFER && c.ip==0x02a1 && c.sp==0x49f3 && c.bp==0x4444);
    c.bp=0x3000;c.sp=0x2ff8;c.ip=0x012c;put(r,c.ss,c.bp,0x4444);put(r,c.ss,c.bp+2,0x02a0);put(r,c.ss,c.bp+4,0x7900);put(r,c.ds,0x49ed,0xffff);c.flags|=1;x.sp=0x3008;
    CHECK(pr_u25_operand_return(&b,PR_BUS_RAM_SIZE,&c,&x)==PR_290B_FIRST_DONE && r[p(c.ds,0x49ed)]==0 && r[p(c.ds,0x49ee)]==0 && (c.flags&0x08d5u)==0x55);
    c.ip=0x0000;c.sp=0x3002;c.bp=0x4444;put(r,c.ss,0x3002,0x02a0);put(r,c.ss,0x3004,0x7900);put(r,c.ss,0x3006,0);put(r,c.ds,0x49ed,0x1234);x.sp=0x3008;
    CHECK(pr_u25_operand_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_OUTSIDE && c.ip==0x012c);
    CHECK(pr_u25_operand_return(&b,PR_BUS_RAM_SIZE,&c,&x)==PR_290B_FIRST_DONE && c.ip==0x02a0 && r[p(c.ds,0x49ed)]==0x35);
    c.ip=0x012c;c.bp=7;c.sp=0xffff;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u25_operand_return(&b,PR_BUS_RAM_SIZE,&c,&x)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);}
    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u25_tag_session joined;pr_u25_tag_diagnostic td;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x339a,0x405e,0x4060,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x7934)]=9;r[p(0x5000,0x7935)]=2;r[p(0x5000,0x7936)]=0x42;r[p(0x5000,0x7937)]=0x77;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u25_tag_session_init(&joined);
    CHECK(pr_u25_tag_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&td)==PR_U25_TAG_YIELD);
    target.ip=0x1234;before=c;c.ax^=1;CHECK(pr_u25_tag_resume(&joined,&td)==PR_U25_TAG_ARGUMENT && joined.phase==PR_U25_TAG_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u25_tag_resume(&joined,&td)==PR_U25_TAG_REACHED && c.cs==0x7100 && c.ip==0x0121 && c.ax==1 && c.di==1 && (c.dx&255u)==0x77 && (c.flags&0x08d5u)==0x44);
    CHECK(r[p(c.ds,0x7006)]==2 && r[p(c.ds,0x7086)]==0x42 && r[p(c.ds,0x7046)]==0x77 && r[p(c.ds,0x49ed)]==0x37 && r[p(c.ds,0x49ee)]==0x12);
    {pr_r0899_cpu state=c;pr_u25_loop_step_diagnostic step;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    CHECK(pr_u25_numeric_loop(&b,PR_BUS_RAM_SIZE,&c,0,&step)==PR_U25_LOOP_STEP_LIMIT && c.ip==0x001f && step.iterations==0 && r[p(c.ss,c.bp-1)]==1 && (c.flags&0x08d5u)==0x95);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(0x5000,0x7938)]=1;r[p(0x5000,0x7939)]=0x22;r[p(0x5000,0x793a)]=0x33;
    CHECK(pr_u25_numeric_loop(&b,PR_BUS_RAM_SIZE,&c,1,&step)==PR_U25_LOOP_STEP_DONE && c.ip==0x012c && step.iterations==1 && r[p(c.ds,0x7047)]==0x33 && r[p(c.ds,0x49ed)]==0x3a);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(0x5000,0x7938)]=0x80;r[p(0x5000,0x7939)]=0x22;
    CHECK(pr_u25_numeric_loop(&b,PR_BUS_RAM_SIZE,&c,1,&step)==PR_U25_LOOP_STEP_STRING && c.ip==0x009a && step.iterations==1 && r[p(c.ss,c.bp-2)]==0 && r[p(c.ds,0x49ed)]==0x39);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0xa000);ega->plane[0][0x7938]=1;ega->plane[0][0x7939]=0x22;ega->plane[0][0x793a]=0x33;ega->plane[1][0x793a]=0x6a;
    CHECK(pr_u25_numeric_loop(&b,PR_BUS_RAM_SIZE,&c,1,&step)==PR_U25_LOOP_STEP_DONE && c.ip==0x012c && r[p(c.ds,0x7047)]==0x33 && ega->latch[1]==0x6a);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.ss=c.ds;c.bp=0x49ee;c.sp=0x49e6;put(r,c.ds,0x49ed,1);r[p(c.ss,c.bp-7)]=2;put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x6703)]=4;r[p(0x5000,0x6704)]=0x33;
    CHECK(pr_u25_numeric_loop(&b,PR_BUS_RAM_SIZE,&c,1,&step)==PR_U25_LOOP_STEP_LIMIT && c.ip==0x001f && step.iterations==1 && r[p(c.ds,0x49ed)]==4 && r[p(c.ds,0x7007)]==4);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ss,c.bp-1)]=0xff;r[p(c.ss,c.bp-7)]=0;put(r,c.ds,0x49ed,0);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x6701)]=4;r[p(0x5000,0x6702)]=0x33;
    CHECK(pr_u25_numeric_loop(&b,PR_BUS_RAM_SIZE,&c,1,&step)==PR_U25_LOOP_STEP_DONE && c.ip==0x012c && step.iterations==1 && r[p(c.ss,c.bp-1)]==0 && r[p(c.ds,0x7005)]==4 && r[p(c.ds,0x7085)]==0x33);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ss,c.bp-7)]=1;
    CHECK(pr_u25_numeric_loop(&b,PR_BUS_RAM_SIZE,&c,0,&step)==PR_U25_LOOP_STEP_DONE && c.ip==0x012c && step.iterations==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.sp--;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u25_numeric_loop(&b,PR_BUS_RAM_SIZE,&c,1,&step)==PR_U25_LOOP_STEP_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;free(base);free(ebase);}

    jt.calls=0;target.ip=0x339a;display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x7934)]=9;r[p(0x5000,0x7935)]=0x80;r[p(0x5000,0x7936)]=0x42;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u25_tag_session_init(&joined);
    CHECK(pr_u25_tag_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&td)==PR_U25_TAG_YIELD);
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u25_tag_resume(&joined,&td)==PR_U25_TAG_STRING && c.ip==0x009a && r[p(c.ss,c.bp-2)]==0 && r[p(c.ds,0x49ed)]==0x36);

    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u25_operand_session joined;pr_u25_operand_diagnostic od;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x339a,0x405e,0x4060,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x7934)]=9;r[p(0x5000,0x7935)]=2;r[p(0x5000,0x7936)]=0x42;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u25_operand_session_init(&joined);
    CHECK(pr_u25_operand_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&od)==PR_U25_OPERAND_YIELD);
    target.ip=0x1234;before=c;c.ax^=1;CHECK(pr_u25_operand_resume(&joined,&od)==PR_U25_OPERAND_ARGUMENT && joined.phase==PR_U25_OPERAND_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u25_operand_resume(&joined,&od)==PR_U25_OPERAND_REACHED && c.cs==0x7100 && c.ip==0x005f && c.bp==0x403e && c.sp==0x4036 && c.ax==0x1236 && c.di==1 && (c.dx&255u)==0x42 && (c.flags&0x08d5u)==4);
    CHECK(r[p(c.ds,0x7006)]==2 && r[p(c.ds,0x7086)]==0x42 && r[p(c.ds,0x49ed)]==0x36 && r[p(c.ds,0x49ee)]==0x12 && r[p(c.ss,c.bp-1)]==1 && r[p(c.ss,c.bp-7)]==2);
    {pr_r0899_cpu state=c;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);unsigned j;CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    for(j=1;j<=3;j++){memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x7006)]=(uint8_t)j;r[p(0x5000,0x7937)]=0x77;
        CHECK(pr_u25_operand_tag(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x0121 && c.ax==1 && c.di==1 && (c.dx&255u)==0x77 && (c.flags&0x08d5u)==0x44 && r[p(c.ds,0x7046)]==0x77 && r[p(c.ds,0x49ed)]==0x37);}
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x7006)]=0x80;
    CHECK(pr_u25_operand_tag(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_OUTSIDE && c.ip==0x009a && c.ax==0x80 && (c.flags&0x08d5u)==0x44);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x7006)]=0x81;
    CHECK(pr_u25_operand_tag(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_OUTSIDE && c.ip==0x00d7 && c.ax==0x81 && (c.flags&0x08d5u)==0x44);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x7006)]=4;
    CHECK(pr_u25_operand_tag(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x0121 && c.ax==4 && (c.flags&0x08d5u)==0x881);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x7006)]=2;put(r,c.ds,0x49ed,0xfffe);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0xa000);ega->plane[0][0x66ff]=0x77;ega->plane[1][0x66ff]=0x6a;
    CHECK(pr_u25_operand_tag(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x0121 && r[p(c.ds,0x7046)]==0x77 && r[p(c.ds,0x49ed)]==0xff && ega->latch[1]==0x6a);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.ss=c.ds;c.bp=0x49ee;c.sp=0x49e6;put(r,c.ds,0x49ed,1);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(c.ds,0x7006)]=2;r[p(0x5000,0x6702)]=0x66;
    CHECK(pr_u25_operand_tag(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x0121 && c.di==2 && r[p(c.ds,0x7047)]==0x66 && r[p(c.ds,0x49ed)]==2);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.sp--;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u25_operand_tag(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;free(base);free(ebase);}

    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u1c_save_session joined;pr_u1c_save_diagnostic sd;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x339a,0x405e,0x4060,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x7934)]=9;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u1c_save_session_init(&joined);
    CHECK(pr_u1c_save_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&sd)==PR_U1C_SAVE_YIELD);
    target.ip=0x1234;before=c;c.ax^=1;CHECK(pr_u1c_save_resume(&joined,&sd)==PR_U1C_SAVE_ARGUMENT && joined.phase==PR_U1C_SAVE_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u1c_save_resume(&joined,&sd)==PR_U1C_SAVE_LOADER && c.cs==0x7100 && c.ip==0 && c.bp==0x404a && c.sp==0x4040 && c.ax==0x1202 && sd.entry.stage==3 && (c.flags&0x08d5u)==0);
    CHECK(r[p(c.ss,c.sp)]==0xa0 && r[p(c.ss,c.sp+1)]==2 && r[p(c.ss,c.sp+2)]==0 && r[p(c.ss,c.sp+3)]==0x79 && r[p(c.ss,c.sp+4)]==2 && r[p(c.ss,c.sp+5)]==0x12);
    {pr_r0899_cpu state=c;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    r[p(c.ss,c.sp+4)]=0;CHECK(pr_u25_operand_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_OUTSIDE && c.ip==0x012c && (c.ax&255u)==1 && (c.flags&0x08d5u)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,c.ds,0x49ed,0xfffe);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0xa000);
    ega->plane[0][0x66ff]=3;ega->plane[0][0x6700]=0x42;ega->plane[1][0x6700]=0x6a;
    CHECK(pr_u25_operand_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x005f && c.ax==0 && (c.flags&0x08d5u)==0x54 && r[p(c.ds,0x7006)]==3 && r[p(c.ds,0x7086)]==0x42 && ega->latch[1]==0x6a);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.ss=c.ds;c.sp=0x7009;put(r,c.ss,c.sp+4,2);put(r,c.ds,0x49ed,0);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x6701)]=3;r[p(0x5000,0x6702)]=0x42;
    CHECK(pr_u25_operand_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.bp==0x7007 && c.di==3 && r[p(c.ds,0x7006)]==3 && r[p(c.ds,0x7088)]==0x42);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,c.ds,0x49ed,0);put(r,c.ds,0x49de,0x0904);put(r,c.ds,0x49e0,c.ds);r[p(c.ds,0x7005)]=0x33;r[p(c.ds,0x7006)]=0x44;
    CHECK(pr_u25_operand_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && r[p(c.ds,0x7006)]==0x33 && r[p(c.ds,0x7086)]==0x33);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.sp=9;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u25_operand_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;free(base);free(ebase);}

    jt.calls=0;target.ip=0x339a;display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x7934)]=0;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u1c_save_session_init(&joined);
    CHECK(pr_u1c_save_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&sd)==PR_U1C_SAVE_YIELD);
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u1c_save_resume(&joined,&sd)==PR_U1C_SAVE_OTHER && c.cs==0x7900 && c.ip==1);
    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u1c_ecl_session joined;pr_u1c_ecl_diagnostic ed;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x339a,0x405e,0x4060,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x7934)]=9;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u1c_ecl_session_init(&joined);
    CHECK(pr_u1c_ecl_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&ed)==PR_U1C_ECL_YIELD);
    target.ip=0x1234;before=c;c.ax^=1;CHECK(pr_u1c_ecl_resume(&joined,&ed)==PR_U1C_ECL_ARGUMENT && joined.phase==PR_U1C_ECL_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u1c_ecl_resume(&joined,&ed)==PR_U1C_ECL_HANDLER && c.cs==0x7900 && c.ip==0x0292 && c.bp==0x4050 && c.sp==0x404c && (c.ax&255u)==9 && (c.flags&0x08d5u)==0x44);
    CHECK(r[p(c.ss,c.sp)]==0x89 && r[p(c.ss,c.sp+1)]==0x2f && r[p(c.ss,c.sp+2)]==0 && r[p(c.ss,c.sp+3)]==0x79 && r[p(c.ds,0x49ed)]==0x34);
    {pr_r0899_cpu state=c;pr_290b_first_diagnostic fd2;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    CHECK(pr_u1c_save_loader_entry(&b,PR_BUS_RAM_SIZE,&c,&fd2)==PR_290B_FIRST_DONE && c.cs==0x7100 && c.ip==0 && c.bp==(uint16_t)(state.sp-2) && c.sp==(uint16_t)(state.sp-12) && fd2.stage==3);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.cs,0x029b)]=0x90;
    CHECK(pr_u1c_save_loader_entry(&b,PR_BUS_RAM_SIZE,&c,&fd2)==PR_290B_FIRST_CODE && c.ip==0x029b && c.sp==(uint16_t)(state.sp-8) && fd2.stage==1);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,c.cs,0x029e,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_save_loader_entry(&b,PR_BUS_RAM_SIZE,&c,&fd2)==PR_290B_FIRST_PROFILE && c.cs==0xa000 && c.ip==0x002a && fd2.stage==2 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(0x5100,0x002a)]=0x90;
    CHECK(pr_u1c_save_loader_entry(&b,PR_BUS_RAM_SIZE,&c,&fd2)==PR_290B_FIRST_CODE && c.cs==0x5100 && c.ip==0x002a && fd2.stage==2);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,0x5100,0x002b,1);
    CHECK(pr_u1c_save_loader_entry(&b,PR_BUS_RAM_SIZE,&c,&fd2)==PR_290B_FIRST_CODE && c.cs==0x5100 && c.ip==0x002a && fd2.stage==2);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,0x5100,0x002d,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_save_loader_entry(&b,PR_BUS_RAM_SIZE,&c,&fd2)==PR_290B_FIRST_PROFILE && c.cs==0xa000 && c.ip==0 && fd2.stage==3 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.sp=0x8002;
    CHECK(pr_u1c_save_loader_entry(&b,PR_BUS_RAM_SIZE,&c,&fd2)==PR_290B_FIRST_DONE && c.ip==0 && (c.flags&0x08d5u)==0x814);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.ss=c.ds;c.sp=0x6f81;c.bp=0x2222;
    CHECK(pr_u1c_save_loader_entry(&b,PR_BUS_RAM_SIZE,&c,&fd2)==PR_290B_FIRST_DONE && c.ip==0 && r[p(c.ds,0x6f7f)]==0x22 && r[p(c.ds,0x6f80)]==0x22);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.sp=11;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_save_loader_entry(&b,PR_BUS_RAM_SIZE,&c,&fd2)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;free(base);free(ebase);}

    jt.calls=0;target.ip=0x339a;display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x7934)]=0x2f;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u1c_ecl_session_init(&joined);
    CHECK(pr_u1c_ecl_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&ed)==PR_U1C_ECL_YIELD);
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u1c_ecl_resume(&joined,&ed)==PR_U1C_ECL_OTHER && c.ip==0x2f8c && (c.ax&255u)==0x2f && ed.prefix_status==PR_290B_FIRST_OUTSIDE);

    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u1c_runner_session joined;pr_u1c_runner_diagnostic rd;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x339a,0x405e,0x4060,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(c.ds,0x6f7f)]=0x55;r[p(0x5000,0x7934)]=0x2f;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u1c_runner_session_init(&joined);
    CHECK(pr_u1c_runner_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&rd)==PR_U1C_RUNNER_YIELD);
    target.ip=0x1234;before=c;c.ax^=1;CHECK(pr_u1c_runner_resume(&joined,&rd)==PR_U1C_RUNNER_ARGUMENT && joined.phase==PR_U1C_RUNNER_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u1c_runner_resume(&joined,&rd)==PR_U1C_RUNNER_DISPATCH && c.cs==0x7900 && c.ip==0x2f2d && c.bp==0x4056 && c.sp==0x4052 && c.ax==0x122f && c.di==0x1234);
    CHECK(r[p(c.ds,0x49ed)]==0x34 && r[p(c.ds,0x49ee)]==0x12 && r[p(c.ds,0x6f7e)]==0x55 && r[p(c.ds,0x6f7f)]==0x2f && r[p(c.ss,c.sp)]==0xa8 && r[p(c.ss,c.sp+1)]==0x31);
    {const uint16_t targets[]={1,0xf4,0x111,0x126,0x1a3,0x1a3,0x1a3,0x1a3,0x234,0x292};pr_r0899_cpu state=c;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);unsigned j;CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    for(j=0;j<10;j++){memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x6f7f)]=(uint8_t)j;
        CHECK(pr_u1c_ecl_dispatch_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==targets[j] && c.bp==(uint16_t)(state.sp-2) && c.sp==(uint16_t)(state.sp-6) && (c.ax&255u)==j && (c.flags&0x08d5u)==0x44);}
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x6f7f)]=0x80;
    CHECK(pr_u1c_ecl_dispatch_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_OUTSIDE && c.ip==0x2f8c && c.sp==(uint16_t)(state.sp-2) && (c.flags&0x08d5u)==0x814);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x6f7f)]=9;r[p(c.cs,0x2f86)]=0x90;
    CHECK(pr_u1c_ecl_dispatch_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_CODE && c.ip==0x2f86 && c.sp==(uint16_t)(state.sp-4) && (c.flags&0x08d5u)==0x44);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x6f7f)]=9;put(r,c.cs,0x2f87,0);
    CHECK(pr_u1c_ecl_dispatch_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_CODE && c.ip==0x2f86 && c.sp==(uint16_t)(state.sp-4));
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.ss=c.ds;c.sp=0x6f81;c.bp=0x1209;r[p(c.ds,0x6f7f)]=0x88;
    CHECK(pr_u1c_ecl_dispatch_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x0292 && (c.ax&255u)==9 && r[p(c.ds,0x6f7f)]==9);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.sp=5;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_ecl_dispatch_prefix(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;free(base);free(ebase);}

    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u1c_script_session joined;pr_u1c_script_diagnostic sd;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x339a,0x405e,0x4060,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;put(r,c.ds,0x49e3,0x1234);put(r,c.ds,0x49ed,0xbeef);
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u1c_script_session_init(&joined);
    CHECK(pr_u1c_script_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&sd)==PR_U1C_SCRIPT_YIELD);
    before=c;c.ax^=1;CHECK(pr_u1c_script_resume(&joined,&sd)==PR_U1C_SCRIPT_ARGUMENT && joined.phase==PR_U1C_SCRIPT_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u1c_script_resume(&joined,&sd)==PR_U1C_SCRIPT_REACHED && c.cs==0x7900 && c.ip==0x3171 && c.sp==0x4058 && c.bp==0x4060);
    CHECK(r[p(c.ss,0x4058)]==0x5b && r[p(c.ss,0x4059)]==0x34 && r[p(c.ss,0x405a)]==0 && r[p(c.ss,0x405b)]==0x79 && r[p(c.ss,0x405c)]==0x34 && r[p(c.ss,0x405d)]==0x12 && r[p(c.ds,0x49ed)]==0xef && r[p(c.ds,0x49ee)]==0xbe);
    {pr_r0899_cpu state=c;pr_290b_return_target ret={0x7900,0x345b,0x405e,0x4060,c.ss};uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0xa000);r[p(c.ds,0x6f7f)]=0x55;ega->plane[0][0x7934]=0x2f;ega->plane[1][0x7934]=0x6a;
    CHECK(pr_u1c_runner_prefix(&b,PR_BUS_RAM_SIZE,&c,&ret)==PR_290B_FIRST_DONE && c.ip==0x2f2d && c.ax==0x122f && r[p(c.ds,0x6f7e)]==0x55 && r[p(c.ds,0x6f7f)]==0x2f && ega->latch[1]==0x6a);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x7934)]=0x2f;r[p(c.cs,0x31a5)]=0x90;
    CHECK(pr_u1c_runner_prefix(&b,PR_BUS_RAM_SIZE,&c,&ret)==PR_290B_FIRST_CODE && c.ip==0x31a5 && c.sp==(uint16_t)(state.sp-4) && r[p(c.ds,0x6f7f)]==0x2f);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,c.ds,0x49de,0xffff);put(r,c.ds,0x49e0,0xa000);ega->plane[0][0x7933]=0x3d;ega->plane[1][0x7933]=0x6a;r[p(c.cs,0x31a5)]=0x90;
    CHECK(pr_u1c_runner_prefix(&b,PR_BUS_RAM_SIZE,&c,&ret)==PR_290B_FIRST_CODE && c.ip==0x31a5 && c.di==0x1233 && (c.flags&0x08d5u)==0x15 && r[p(c.ds,0x6f7f)]==0x3d && ega->latch[1]==0x6a);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.ss=c.ds;c.bp=0x4a09;c.sp=0x4a07;c.ip=0x344c;r[p(c.ds,0x49ff)]=0;put(r,c.ds,0x49e3,0x1234);
    CHECK(pr_u1c_script_entry(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x3171);ret=(pr_290b_return_target){0x7900,0x345b,0x4a07,0x4a09,c.ss};
    CHECK(pr_u1c_runner_prefix(&b,PR_BUS_RAM_SIZE,&c,&ret)==PR_290B_FIRST_DONE && c.ip==0x345b && c.bp==0x4a09 && c.sp==0x4a07 && r[p(c.ds,0x49ff)]==9);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.ss=c.ds;c.bp=0x4a09;c.sp=0x4a07;c.ip=0x344c;r[p(c.ds,0x49ff)]=0;put(r,c.ds,0x49e3,0x1234);
    CHECK(pr_u1c_script_entry(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x3171);ret=(pr_290b_return_target){0x7900,0x1234,0x4a07,0x4a09,c.ss};
    CHECK(pr_u1c_runner_prefix(&b,PR_BUS_RAM_SIZE,&c,&ret)==PR_290B_FIRST_TRANSFER && c.ip==0x345b && c.cs==0x7900 && c.sp==0x4a07);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.ss=c.ds;c.bp=0x6f88;c.sp=0x6f80;put(r,c.ss,c.sp,0x345b);put(r,c.ss,c.sp+2,0x7900);put(r,c.ss,c.sp+4,0x0010);put(r,c.ds,0x49de,0);put(r,c.ds,0x49e0,0x5000);r[p(0x5000,0x6710)]=0x2f;
    CHECK(pr_u1c_runner_prefix(&b,PR_BUS_RAM_SIZE,&c,&ret)==PR_290B_FIRST_DONE && c.ip==0x2f2d && r[p(c.ss,c.bp)]==0x6f && r[p(c.ss,c.bp+1)]==0x2f);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.sp=5;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_runner_prefix(&b,PR_BUS_RAM_SIZE,&c,&ret)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;free(base);free(ebase);}

    jt.calls=0;display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,2);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x49ff)]=0;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u1c_script_session_init(&joined);
    CHECK(pr_u1c_script_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&sd)==PR_U1C_SCRIPT_YIELD);
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u1c_script_resume(&joined,&sd)==PR_U1C_SCRIPT_OTHER && c.ip==0x33ec);

    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u1c_dispatch_session joined;pr_u1c_dispatch_diagnostic dd;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x339a,0x405e,0x4060,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);put(r,0x5000,0x3594,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;
    put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u1c_dispatch_session_init(&joined);
    CHECK(pr_u1c_dispatch_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&dd)==PR_U1C_DISPATCH_YIELD);
    target.ip=0x1234;before=c;c.ax^=1;CHECK(pr_u1c_dispatch_resume(&joined,&dd)==PR_U1C_DISPATCH_ARGUMENT && joined.phase==PR_U1C_DISPATCH_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u1c_dispatch_resume(&joined,&dd)==PR_U1C_DISPATCH_REACHED && c.cs==0x7900 && c.ip==0x344c && (c.ax&255u)==0x48 && dd.dispatch.stage==1);
    {pr_r0899_cpu state=c;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    r[p(c.ds,0x49ff)]=0;put(r,c.ds,0x49e3,0x1234);CHECK(pr_u1c_script_entry(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x3171 && c.sp==(uint16_t)(state.sp-6) && r[p(c.ss,c.sp)]==0x5b && r[p(c.ss,c.sp+4)]==0x34 && (c.flags&0x08d5u)==0x44);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x49ff)]=1;r[p(c.ds,0x442f)]=1;CHECK(pr_u1c_script_entry(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x3462 && c.sp==state.sp);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x49ff)]=1;r[p(c.ds,0x442f)]=0;CHECK(pr_u1c_script_entry(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x34f5 && c.sp==state.sp);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x49ff)]=0;put(r,c.ds,0x49e3,0x1234);r[p(c.cs,0x3458)]=0x90;
    CHECK(pr_u1c_script_entry(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_CODE && c.ip==0x3458 && c.sp==(uint16_t)(state.sp-4) && r[p(c.ss,c.sp+2)]==0x34);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.ss=c.ds;c.bp=0x49e9;c.sp=0x49e7;r[p(c.ds,0x49ff)]=0;put(r,c.ds,0x49e3,0x1234);
    CHECK(pr_u1c_script_entry(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.ip==0x3171 && r[p(c.ss,c.sp+4)]==0x34 && r[p(c.ds,0x49e3)]==(uint8_t)c.cs);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.sp--;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_script_entry(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;free(base);free(ebase);}

    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u1c_up_session joined;pr_u1c_up_diagnostic ud;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x339a,0x405e,0x4060,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;r[p(c.ds,0x84dc)]=0x21;
    put(r,c.ds,0x5d92,0x1111);put(r,c.ds,0x5d94,0x2222);put(r,c.ss,c.bp,0x4060);put(r,c.ss,c.bp+2,0x339a);put(r,c.ss,c.bp+4,0x7900);pr_u1c_up_session_init(&joined);
    CHECK(pr_u1c_up_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&ud)==PR_U1C_UP_YIELD);
    target.ip=0x1234;before=c;c.ax^=1;CHECK(pr_u1c_up_resume(&joined,&ud)==PR_U1C_UP_ARGUMENT && joined.phase==PR_U1C_UP_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u1c_up_resume(&joined,&ud)==PR_U1C_UP_REACHED && c.cs==0x7900 && c.ip==0x33bd && c.bp==0x4060 && c.sp==0x405e);
    CHECK(r[p(c.ss,0x405f)]==0x48 && r[p(c.ds,0x4436)]==0x11 && r[p(c.ds,0x4438)]==0x22 && r[p(0x5000,0x11e4)]==0x21 && c.ax==0x21 && c.dx==0x2222 && c.di==0x3000 && c.es==0x5000 && (c.flags&0x08d5u)==0x44);
    {pr_r0899_cpu state=c;pr_290b_first_diagnostic dd;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    put(r,0x5000,0x3594,2);r[p(c.ds,0x49ff)]=1;CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_DONE && c.ip==0x344c && dd.stage==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,0x5000,0x3594,2);r[p(c.ds,0x49ff)]=0;r[p(c.ss,c.bp-1)]='x';
    CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_DONE && c.ip==0x33ec && (c.ax&255u)=='X' && dd.stage==2);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,0x5000,0x3594,1);r[p(c.ds,0x49ff)]=0;r[p(c.ss,c.bp-1)]='e';
    CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_DONE && c.ip==0x33e6 && (c.ax&255u)=='E' && dd.stage==2);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,0x5000,0x3594,1);c.ss=c.ds;c.bp=0x4a07;c.sp=0x4a05;r[p(c.ss,c.bp-1)]='e';r[p(c.ds,0x49ff)]=0;
    CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_DONE && c.ip==0x344c && dd.stage==1 && r[p(c.ds,0x49ff)]==0xce);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.di=0xfa6b;c.es=0xa000;memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_PROFILE && c.ip==0x33bd && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.di=0;c.es=0xa000;ega->plane[0][0x594]=2;ega->plane[0][0x595]=0;ega->plane[1][0x595]=0x6a;r[p(c.ds,0x49ff)]=1;
    CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_DONE && c.ip==0x344c && ega->latch[1]==0x6a);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,0x5000,0x3594,0);r[p(c.cs,0x33c9)]=0x90;
    CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_CODE && c.ip==0x33c9 && dd.stage==1 && c.sp==(uint16_t)(state.sp-2) && (c.flags&0x08d5u)==0x95);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,0x5000,0x3594,2);r[p(c.ds,0x49ff)]=0;r[p(c.cs,0x33dd)]=0x90;
    CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_CODE && c.ip==0x33dd && dd.stage==2 && (c.flags&0x08d5u)==0x44);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,0x5000,0x3594,0);put(r,c.cs,0x33cc,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_PROFILE && c.cs==0xa000 && c.ip==0x182a && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;put(r,0x5000,0x3594,2);r[p(c.ds,0x49ff)]=0;put(r,c.cs,0x33e0,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_PROFILE && c.cs==0xa000 && c.ip==0x182a && dd.stage==2 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;c.sp--;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_dispatch(&b,PR_BUS_RAM_SIZE,&c,&dd)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;free(base);free(ebase);}

    {pr_r0899_cpu initial=c;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    c.ip=0x339a;c.ax=0x1248;c.ss=c.ds;c.bp=0x5d93;c.sp=0x5d91;put(r,c.ds,0x5d92,0xabcd);put(r,c.ds,0x5d94,0x1234);
    CHECK(pr_u1c_post_up(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && r[p(c.ds,0x4436)]==0x48 && r[p(c.ds,0x4437)]==0xab && r[p(c.ds,0x4438)]==0x34);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;c.ip=0x339a;c.ax=0x48;put(r,c.ds,0x49d2,0x47f2);put(r,c.ds,0x49d4,c.ds);r[p(c.ds,0x84dc)]=0x7a;put(r,c.ds,0x49d8,0x5000);
    CHECK(pr_u1c_post_up(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && c.di==0x7a && c.es==0x5000);
    memcpy(r,base,PR_BUS_RAM_SIZE);c=initial;c.ip=0x339a;c.ax=0x48;pr_ega_reset(ega);put(r,c.ds,0x49d2,0);put(r,c.ds,0x49d4,0xa000);r[p(c.ds,0x84dc)]=0x6e;
    CHECK(pr_u1c_post_up(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_DONE && ega->plane[0][0x1e4]==0x6e && ega->plane[3][0x1e4]==0x6e && ega->plane[0][0x1e5]==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;c.ip=0x339a;c.ax=0x48;put(r,c.ds,0x49d2,0xfe1b);put(r,c.ds,0x49d4,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_post_up(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_PROFILE && c.ip==0x33b4 && c.ax==0x21 && (c.flags&0x08d5u)==0x44 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;c.ip=0x339a;c.sp--;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u1c_post_up(&b,PR_BUS_RAM_SIZE,&c)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    free(base);free(ebase);}
    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u68_290b_exit joined;pr_u68_290b_exit_diagnostic ed;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};pr_290b_return_target target={0x7900,0x1234,0x405e,0x8888,0x2800};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);r[p(c.ds,0x49f3)]=2;r[p(c.ds,0x84e4)]=0;
    put(r,c.ss,c.bp,0x8888);put(r,c.ss,c.bp+2,0x1234);put(r,c.ss,c.bp+4,0x7900);pr_u68_290b_exit_init(&joined);
    CHECK(pr_u68_290b_exit_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&target,&ed)==PR_U68_290B_EXIT_YIELD);
    target.ip=0x5678;before=c;c.ax^=1;CHECK(pr_u68_290b_exit_resume(&joined,&ed)==PR_U68_290B_EXIT_ARGUMENT && joined.phase==PR_U68_290B_EXIT_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_exit_resume(&joined,&ed)==PR_U68_290B_EXIT_RETURNED && c.cs==target.cs && c.ip==0x1234 && c.sp==target.sp && c.bp==target.bp && (c.ax&255u)==0x48 && ed.exit.status==PR_R03F1_CLEAR_CPU_OK);
    CHECK(r[p(c.ds,0x49f9)]==1 && r[p(c.ds,0x84e4)]==1);
    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u68_290b_print joined;pr_u68_290b_print_diagnostic pd;pr_290b_clear_limits cl={65536,65536};pr_290b_print_limits pl={65536,65536};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);r[p(c.ds,0x49f3)]=2;pr_u68_290b_print_init(&joined);
    CHECK(pr_u68_290b_print_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&cl,&pl,&pd)==PR_U68_290B_PRINT_YIELD);
    pl.printer_loopbacks=0;before=c;c.ax^=1;CHECK(pr_u68_290b_print_resume(&joined,&pd)==PR_U68_290B_PRINT_ARGUMENT && joined.phase==PR_U68_290B_PRINT_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_print_resume(&joined,&pd)==PR_U68_290B_PRINT_RETURNED && c.cs==0x7600 && c.ip==0x0ba0 && c.sp==0x401c && c.bp==0x4058 && pd.print.status==PR_R0709_PRINTER_CPU_OK);
    {pr_290b_clear_limits lim={65536,65536};pr_290b_return_target target={0x7900,0x1234,0x405e,0x8888,c.ss};pr_290b_clear_diagnostic xd;pr_r0899_cpu initial=c;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    r[p(c.ss,c.bp-2)]=0;CHECK(pr_u68_post_display_exit(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&xd)==PR_290B_FIRST_OUTSIDE && c.ip==0x09a8 && (c.flags&0x08d5u)==0x44);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;put(r,c.ss,c.bp,0x8888);put(r,c.ss,c.bp+2,0x1234);put(r,c.ss,c.bp+4,0x7900);r[p(c.ds,0x49fa)]=7;r[p(c.ds,0x84e4)]=1;
    CHECK(pr_u68_post_display_exit(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&xd)==PR_290B_FIRST_DONE && c.cs==target.cs && c.ip==target.ip && (c.ax&255u)==0x48 && r[p(c.ds,0x49f9)]==7 && xd.status==-1);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;put(r,c.ss,c.bp,0x8888);put(r,c.ss,c.bp+2,0x2222);put(r,c.ss,c.bp+4,0x7900);r[p(c.ds,0x84e4)]=1;
    CHECK(pr_u68_post_display_exit(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&xd)==PR_290B_FIRST_TRANSFER && c.ip==0x2222);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;r[p(c.ds,0x84e4)]=0;r[p(c.cs,0x0d35)]=0x90;
    CHECK(pr_u68_post_display_exit(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&xd)==PR_290B_FIRST_CODE && c.ip==0x0d35 && r[p(c.ds,0x84e4)]==0 && r[p(c.ds,0x49f9)]==1);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;r[p(c.ds,0x84e4)]=0;lim.rect_loopbacks=0;
    CHECK(pr_u68_post_display_exit(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&xd)==PR_290B_FIRST_CALLEE && xd.status==PR_R03F1_CLEAR_CPU_CALLEE && r[p(c.ds,0x84e4)]==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;r[p(c.ds,0x84e4)]=0;put(r,c.cs,0x0d38,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u68_post_display_exit(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&xd)==PR_290B_FIRST_PROFILE && c.cs==0xa000 && c.ip==0x0137 && r[p(c.ds,0x84e4)]==0 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;c.sp--;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u68_post_display_exit(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&xd)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;c.ss=c.ds;c.bp=0x49fc;c.sp=(uint16_t)(c.bp-0x3c);target.ss=c.ss;target.sp=0x4a02;
    r[p(c.ds,0x49fa)]=7;r[p(c.ds,0x84e4)]=1;put(r,c.ss,c.bp,0x8888);put(r,c.ss,c.bp+2,0x1234);put(r,c.ss,c.bp+4,0x7900);
    CHECK(pr_u68_post_display_exit(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&xd)==PR_290B_FIRST_DONE && (c.ax&255u)==7 && r[p(c.ds,0x49f9)]==7);
    free(base);free(ebase);}

    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u68_290b_clear joined;pr_u68_290b_clear_diagnostic cd;pr_290b_clear_limits lim={65536,65536};clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);r[p(c.ds,0x49f3)]=2;pr_u68_290b_clear_init(&joined);
    CHECK(pr_u68_290b_clear_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&lim,&cd)==PR_U68_290B_CLEAR_YIELD);
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_clear_resume(&joined,&cd)==PR_U68_290B_CLEAR_REACHED && c.ip==0x2b47 && cd.clear.status==PR_R03F1_CLEAR_CPU_OK);
    CHECK(r[p(c.ss,c.bp-0x31)]==12 && memcmp(r+p(c.ss,c.bp-0x30),"5/3!A!X7|X5M",12)==0);
    {pr_290b_print_limits lim={65536,65536};pr_290b_return_target target={0x7600,0x0ba0,0x401c,0x4058,c.ss};pr_290b_print_diagnostic pd;pr_r0899_cpu initial=c;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);unsigned written_before;CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);written_before=writes;
    CHECK(pr_290b_print_return(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&pd)==PR_290B_FIRST_DONE && c.cs==target.cs && c.ip==target.ip && c.sp==target.sp && c.bp==target.bp && writes>written_before);
    CHECK(ega->plane[1][0x12d1]!=0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;put(r,c.ss,(uint16_t)(c.bp+2u),0x1234);
    CHECK(pr_290b_print_return(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&pd)==PR_290B_FIRST_TRANSFER && c.ip==0x1234 && c.cs==target.cs && c.sp==target.sp);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;r[p(c.cs,0x2b55)]=0x90;
    CHECK(pr_290b_print_return(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&pd)==PR_290B_FIRST_CODE && c.ip==0x2b55 && c.sp==(uint16_t)(initial.sp-10));
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;put(r,c.cs,0x2b58,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_print_return(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&pd)==PR_290B_FIRST_PROFILE && c.cs==0xa000 && c.ip==0x0626 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;lim.printer_loopbacks=0;
    CHECK(pr_290b_print_return(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&pd)==PR_290B_FIRST_CALLEE && pd.status==PR_R0709_PRINTER_CPU_LIMIT);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;c.sp--;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_print_return(&b,PR_BUS_RAM_SIZE,&c,&lim,&target,&pd)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    free(base);free(ebase);}
    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u68_290b_mode joined;pr_u68_290b_mode_diagnostic md;clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);r[p(c.ds,0x49f3)]=2;pr_u68_290b_mode_init(&joined);
    CHECK(pr_u68_290b_mode_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&md)==PR_U68_290B_MODE_YIELD);
    before=c;c.ax^=1;CHECK(pr_u68_290b_mode_resume(&joined,&md)==PR_U68_290B_MODE_ARGUMENT && joined.phase==PR_U68_290B_MODE_WAITING);c=before;
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_mode_resume(&joined,&md)==PR_U68_290B_MODE_REACHED && c.ip==0x2b36 && md.mode.stage==3);
    CHECK(r[p(c.ss,c.bp-0x31)]==12 && memcmp(r+p(c.ss,c.bp-0x30),"5/3!A!X7|X5M",12)==0);
    {pr_290b_clear_limits lim={65536,65536};pr_290b_clear_diagnostic cl;pr_r0899_cpu initial=c;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    CHECK(pr_290b_clear(&b,PR_BUS_RAM_SIZE,&c,&lim,&cl)==PR_290B_FIRST_DONE && c.cs==0x6800 && c.ip==0x2b47 && c.sp==initial.sp && cl.status==PR_R03F1_CLEAR_CPU_OK && cl.clear.callee==2);
    CHECK(ega->plane[0][0x12d1]==0 && ega->plane[0][0x32d1]==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;lim.rect_loopbacks=0;
    CHECK(pr_290b_clear(&b,PR_BUS_RAM_SIZE,&c,&lim,&cl)==PR_290B_FIRST_CALLEE && cl.status==PR_R03F1_CLEAR_CPU_CALLEE && cl.clear.callee==1);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;lim.rect_loopbacks=65536;r[p(c.cs,0x2b42)]=0x90;
    CHECK(pr_290b_clear(&b,PR_BUS_RAM_SIZE,&c,&lim,&cl)==PR_290B_FIRST_CODE && c.ip==0x2b42 && c.sp==(uint16_t)(initial.sp-8));
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;put(r,c.cs,0x2b45,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_clear(&b,PR_BUS_RAM_SIZE,&c,&lim,&cl)==PR_290B_FIRST_PROFILE && c.cs==0xa000 && c.ip==0x0137 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=initial;c.sp--;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_clear(&b,PR_BUS_RAM_SIZE,&c,&lim,&cl)==PR_290B_FIRST_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    free(base);free(ebase);}
    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    {pr_u68_290b_assembly joined;pr_u68_290b_assembly_diagnostic ad;clock_state jt={0,0};pr_u3b0_clock_draw_env je={clock_service,&jt,65536,65536,env};
    pr_r0899_cpu prior=c;uint8_t *snapshot=malloc(PR_BUS_RAM_SIZE);pr_ega *esnapshot=malloc(sizeof *ega);CHECK(snapshot&&esnapshot);
    je.draw.pass.target=ce.draw.pass.target;memcpy(snapshot,r,PR_BUS_RAM_SIZE);memcpy(esnapshot,ega,sizeof *ega);
    display_setup(&b,&c,&q,7);put(r,0x5000,0x11f6,0);pr_u68_290b_assembly_init(&joined);
    CHECK(pr_u68_290b_assembly_start(&joined,&b,PR_BUS_RAM_SIZE,&c,&je,&ad)==PR_U68_290B_ASSEMBLY_YIELD);
    CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_assembly_resume(&joined,&ad)==PR_U68_290B_ASSEMBLY_REACHED && c.ip==0x2acd && ad.assembly.stage==9);
    CHECK(r[p(c.ss,c.bp-0x31)]==11 && memcmp(r+p(c.ss,c.bp-0x30),"5/3!A!X7|X5",11)==0);
    {pr_r0899_cpu state=c;uint8_t *base=malloc(PR_BUS_RAM_SIZE);pr_ega *ebase=malloc(sizeof *ega);CHECK(base&&ebase);memcpy(base,r,PR_BUS_RAM_SIZE);memcpy(ebase,ega,sizeof *ega);
    r[p(c.ds,0x49f3)]=2;CHECK(pr_290b_mode(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE && c.ip==0x2b36 && fd.stage==3);
    CHECK(r[p(c.ss,c.bp-0x31)]==12 && memcmp(r+p(c.ss,c.bp-0x30),"5/3!A!X7|X5M",12)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x49f3)]=1;put(r,0x5000,0x3594,0xfffe);
    CHECK(pr_290b_mode(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE && c.ip==0x2b36 && c.ax==0 && (c.flags&0x08d5u)==0x44 && fd.stage==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x49f3)]=1;put(r,c.ds,0x49d6,0);put(r,c.ds,0x49d8,0xa000);
    ega->plane[0][0x594]=1;ega->plane[0][0x595]=0;ega->plane[1][0x595]=0x6a;
    CHECK(pr_290b_mode(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE && c.ip==0x2b36 && fd.stage==3 && ega->latch[1]==0x6a);
    CHECK(r[p(c.ss,c.bp-0x31)]==12 && memcmp(r+p(c.ss,c.bp-0x30),"5/3!A!X7|X5F",12)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x49f3)]=1;put(r,c.ds,0x49d6,0xfa6b);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_mode(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_PROFILE && c.ip==0x2b02 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x49f3)]=2;put(r,c.ds,0x49d6,0xfa6b);put(r,c.ds,0x49d8,0xa000);r[p(c.cs,0x2adf)]=0x90;memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_mode(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CODE && c.ip==0x2adf && fd.stage==1 && (c.flags&0x08d5u)==0x44 && memcmp(old,ega,sizeof *ega)==0);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;r[p(c.ds,0x49f3)]=2;r[p(c.cs,0x2ae9)]=0x90;
    CHECK(pr_290b_mode(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CODE && c.ip==0x2ae9 && fd.stage==2);
    memcpy(r,base,PR_BUS_RAM_SIZE);memcpy(ega,ebase,sizeof *ega);c=state;before=c;c.sp--;memcpy(saved,r,PR_BUS_RAM_SIZE);
    CHECK(pr_290b_mode(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_ARGUMENT && c.sp==(uint16_t)(before.sp-1) && memcmp(saved,r,PR_BUS_RAM_SIZE)==0);
    free(base);free(ebase);}

    memcpy(r,snapshot,PR_BUS_RAM_SIZE);memcpy(ega,esnapshot,sizeof *ega);c=prior;free(snapshot);free(esnapshot);}
    put(r,0x5000,0x11f6,0);CHECK(pr_290b_record_branch(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE && c.ip==0x2a51 && fd.stage==6);
    CHECK(r[p(c.ss,c.bp-0x31)]==4 && memcmp(r+p(c.ss,c.bp-0x30),"5/3!",4)==0);
    before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);r[p(c.cs,0x2aa6)]=0x90;
    CHECK(pr_290b_assembly(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CODE && c.ip==0x2aa6 && fd.stage==6);
    CHECK(r[p(c.ss,c.bp-0x31)]==6 && memcmp(r+p(c.ss,c.bp-0x30),"5/3!A!",6)==0);
    memcpy(r,saved,PR_BUS_RAM_SIZE);memcpy(ega,old,sizeof *ega);c=before;
    before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    c.ds=c.ss;c.bp=0x6be0;c.sp=(uint16_t)(c.bp-0x232);r[p(c.ss,c.bp-0x31)]=2;r[p(c.ss,c.bp-0x30)]='q';r[p(c.ss,c.bp-0x2f)]='r';r[p(c.cs,0x2a74)]=0x90;
    CHECK(pr_290b_assembly(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CODE && c.ip==0x2a74 && c.ax==2 && c.si==2 && c.di==0x25ae && r[p(c.ds,0x6aaf)]==2);
    memcpy(r,saved,PR_BUS_RAM_SIZE);memcpy(ega,old,sizeof *ega);c=before;
    before=c;c.sp--;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_assembly(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_ARGUMENT && c.sp==(uint16_t)(before.sp-1) && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);c=before;
    r[p(c.cs,0x2a74)]=0x90;r[p(c.ds,0x6aaf)]=255;
    CHECK(pr_290b_assembly(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CODE && c.ip==0x2a74 && c.ax==255 && c.si==255 && c.di==0x28a5 && fd.stage==2 && (c.flags&0x08d5u)==0x14u);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);put(r,0x5000,0x11f6,0x8001);
    CHECK(pr_290b_record_branch(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE && c.ip==0x2a51 && (c.flags&0x08d5u)==0x0080u && fd.stage==0);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);put(r,c.ds,0x49d2,0);put(r,c.ds,0x49d4,0xa000);
    ega->plane[0][0x1f6]=1;ega->plane[0][0x1f7]=0;ega->plane[1][0x1f7]=0x6a;
    CHECK(pr_290b_record_branch(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE && c.ip==0x2a51 && ega->latch[1]==0x6a);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);put(r,c.ds,0x49d2,0xfe09);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_record_branch(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_PROFILE && c.ip==0x2a06 && memcmp(old,ega,sizeof *ega)==0);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);put(r,0x5000,0x11f6,0);put(r,c.cs,0x1057,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_record_branch(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_PROFILE && c.cs==0xa000 && c.ip==0x1366 && fd.stage==1 && memcmp(old,ega,sizeof *ega)==0);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);put(r,0x5000,0x11f6,0);r[p(c.cs,0x2a31)]=0x90;
    CHECK(pr_290b_record_branch(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CODE && c.ip==0x2a31 && fd.stage==3);
    ts.calls=0;display_setup(&b,&c,&q,42);put(r,0x5000,0x1190,12);put(r,0x5000,0x118e,3);pr_u68_290b_two_init(&bridge);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u68_290b_two_start(&bridge,&b,PR_BUS_RAM_SIZE,&c,&ce,&td)==PR_U68_290B_TWO_FIELDS && td.second.stage==2 && c.ip==0x2a02);
    CHECK(r[p(c.ss,0x4012)]=='4' && r[p(c.ss,0x4013)]=='2' && r[p(c.ss,0x400f)]=='1' && r[p(c.ss,0x4010)]=='2');
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    put(r,0x5000,0x1190,0xffff);put(r,0x5000,0x118e,12);r[p(c.cs,0x29c3)]=0x90;
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_CODE && c.ip==0x29c3 && c.ax==2 && c.dx==9 && (c.flags&0x08d5u)==0x0011u && fd.stage==1);
    display_setup(&b,&c,&q,7);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    put(r,c.ds,0x49d2,0x3e81);put(r,c.ds,0x49d4,c.ss);
    {unsigned a=(unsigned)r[p(c.ss,0x4011)]|((unsigned)r[p(c.ss,0x4012)]<<8);unsigned z=(unsigned)r[p(c.ss,0x400f)]|((unsigned)r[p(c.ss,0x4010)]<<8);char expect[8];
    snprintf(expect,sizeof expect,"%u",(10u*a+z)&0xffffu);
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE && r[p(c.ss,0x400f)]==(uint8_t)expect[0] && r[p(c.ss,0x4010)]==(uint8_t)expect[1]);}
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    put(r,c.ds,0x49d2,0);put(r,c.ds,0x49d4,0xa000);ega->plane[0][0x190]=0x34;ega->plane[0][0x191]=0x12;
    ega->plane[0][0x18e]=5;ega->plane[0][0x18f]=0;ega->plane[1][0x18f]=0x77;
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE && c.ip==0x2a02 && r[p(c.ss,0x400f)]=='4' && r[p(c.ss,0x4010)]=='6' && ega->latch[1]==0x77);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    put(r,c.ds,0x49d2,0xfe6f);put(r,c.ds,0x49d4,0xa000);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_PROFILE && c.ip==0x29b3 && c.ax==10 && memcmp(old,ega,sizeof *ega)==0);
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    put(r,c.ds,0x49d2,0xfe71);put(r,c.ds,0x49d4,0xa000);ega->plane[0][1]=0xff;ega->plane[0][2]=0xff;before=c;
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_PROFILE && c.ip==0x29bc && c.ax==0xfff6 && c.dx==9 && (c.flags&0x0845u)==0x0805u && (c.flags&0x0090u)==(before.flags&0x0090u));
    display_setup(&b,&c,&q,42);enter_290b(&b,&c);CHECK(pr_290b_first_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_DONE);
    c.sp--;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_290b_second_field(&b,PR_BUS_RAM_SIZE,&c,&fd)==PR_290B_FIRST_ARGUMENT && memcmp(&before,&c,sizeof c)==0 && memcmp(saved,r,PR_BUS_RAM_SIZE)==0 && memcmp(old,ega,sizeof *ega)==0);
    }
    free(old);free(ega);free(saved);free(r);printf("3B0 drawer/wait: %u checks passed\n",checks);return 0;
}
