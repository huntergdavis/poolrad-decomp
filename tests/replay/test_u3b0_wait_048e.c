#include "poolrad/recovered/overlay_u3b0_entry_01da_cpu.h"
#include "poolrad/replay/u3b0_selection_05c4.h"
#include "poolrad/replay/u3b0_mapped_060c.h"
#include "poolrad/replay/u3b0_text_0634.h"
#include "poolrad/replay/u3b0_prompt_pass.h"
#include "poolrad/replay/u3b0_wait_048e.h"
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
int main(void){
    uint8_t*r=malloc(PR_BUS_RAM_SIZE),*saved=malloc(PR_BUS_RAM_SIZE);pr_ega*ega=malloc(sizeof *ega),*old=malloc(sizeof *old);
    pr_port_env ports={NULL,out,NULL};pr_bus b={r,ega,&ports,write,NULL};pr_crt_input_fifo q;pr_r0899_cpu c,before;unsigned i;
    pr_r0899_key_env ke={{{NULL,vector,portout,portin,timefn,1000u},video,NULL},pr_crt_input_fifo_service,&q,8u};
    pr_u3b0_pass_env env={{0x1a40,0x1899,&ke},{0x4000},{65536,65536},{16,300,{65536,65536}},{0x2e60,0x09d5}};
    pr_u3b0_wait_diagnostic d;CHECK(r&&saved&&ega&&old);
    wait_setup(&b,&c,&q);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_PASS && d.status==PR_U3B0_PASS_RETURN && c.ip==0x09d5 && (c.ax&255u)==0x48 && r[p(0x1800,0x6b2c)]==1);
    wait_setup(&b,&c,&q);CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_PASS && d.status==PR_U3B0_PASS_LOOP && c.ip==0x048e && q.count==0);
    wait_setup(&b,&c,&q);put(r,0x5000,0x13fe,0);r[p(c.ss,c.bp+0x0a)]=0;r[p(c.ds,0x6abf)]=1;CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_PASS && d.status==PR_U3B0_PASS_RETURN && (c.ax&255u)==0x48);
    wait_setup(&b,&c,&q);put(r,0x5000,0x13fe,0);r[p(c.ss,c.bp+0x0a)]=1;r[p(c.ds,0x6abf)]=1;CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_ANIMATION && c.ip==0x04ad && q.count==1);
    wait_setup(&b,&c,&q);r[p(c.ds,0x6abf)]=1;CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    before=c;before.ip=0x04ad;before.es=0x5000;before.di=0x1000;before.flags=(uint16_t)(before.flags&(uint16_t)~0x08d5u);memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_ANIMATION && q.count==1 && memcmp(&c,&before,sizeof c)==0 && memcmp(r,saved,PR_BUS_RAM_SIZE)==0 && memcmp(ega,old,sizeof *ega)==0);
    wait_setup(&b,&c,&q);env.poll.key_env=NULL;CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_PASS && d.status==PR_U3B0_PASS_ARGUMENT && c.ip==0x0566 && c.es==0x5000 && c.di==0x1000 && q.count==1);env.poll.key_env=&ke;
    wait_setup(&b,&c,&q);put(r,c.ds,0x6b30,1);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_CLOCK && c.ip==0x0540 && q.count==1);
    wait_setup(&b,&c,&q);put(r,c.ds,0x6b2e,1);CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_CLOCK && c.ip==0x0540);
    wait_setup(&b,&c,&q);put(r,c.ds,0x6b30,0x8000);put(r,c.ss,c.bp+8,0xa000);CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_PASS && d.status==PR_U3B0_PASS_PROFILE && c.ip==0x0566 && (c.flags&0x08d5u)==0x84u && q.count==1);
    wait_setup(&b,&c,&q);put(r,c.ds,0x49d2,0);put(r,c.ds,0x49d4,0xa000);r[p(c.ds,0x6abf)]=1;
    for(i=0;i<4;i++){ega->plane[i][0x3fe]=(uint8_t)(1+i);ega->plane[i][0x3ff]=(uint8_t)(0x70+i);}
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_ANIMATION && c.ip==0x04ad);
    for(i=0;i<4;i++)CHECK(ega->latch[i]==(uint8_t)(0x70+i));
    wait_setup(&b,&c,&q);put(r,c.ds,0x49d2,0);put(r,c.ds,0x49d4,0xa000);
    ega->plane[0][0x3fe]=1;CHECK(pr_crt_input_fifo_push(&q,0x4800)==PR_CRT_INPUT_OK);
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_PASS && d.status==PR_U3B0_PASS_PROFILE && d.pass.stage==PR_U3B0_PASS_STAGE_RETURN_GUARD && d.pass.status==PR_U3B0_PASS_PROFILE_RETURN_APERTURE && c.ip==0x06cf && q.count==0 && r[p(c.ss,0x4100)]==1);
    wait_setup(&b,&c,&q);put(r,c.ds,0x49d2,0xfc01);put(r,c.ds,0x49d4,0x9000);r[0x9ffff]=1;r[p(c.ds,0x6abf)]=1;
    for(i=0;i<4;i++)ega->plane[i][0]=(uint8_t)(0x30+i);
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_ANIMATION && c.ip==0x04ad);
    for(i=0;i<4;i++)CHECK(ega->latch[i]==(uint8_t)(0x30+i));
    wait_setup(&b,&c,&q);c.bp=0x8f;before=c;memcpy(saved,r,PR_BUS_RAM_SIZE);memcpy(old,ega,sizeof *ega);
    CHECK(pr_u3b0_wait_048e(&b,PR_BUS_RAM_SIZE,&c,&env,&d)==PR_U3B0_WAIT_ARGUMENT && memcmp(&c,&before,sizeof c)==0 && memcmp(r,saved,PR_BUS_RAM_SIZE)==0 && memcmp(ega,old,sizeof *ega)==0);
    free(old);free(ega);free(saved);free(r);printf("3B0 wait: %u checks passed\n",checks);return 0;
}
