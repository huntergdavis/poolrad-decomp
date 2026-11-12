/* Conditional run003 DOS35 binding. PSP ownership is supplied, not proven. */
#include "poolrad/recovered/resident_08ae_vector_137c_cpu.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIZE 655360u
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
static void word(uint8_t *r, unsigned p, uint16_t v) { r[p]=(uint8_t)v; r[p+1]=(uint8_t)(v>>8); }
static uint16_t get(const uint8_t *r, unsigned p) { return (uint16_t)(r[p] | ((unsigned)r[p+1]<<8)); }
static int load(const char *p, uint8_t *r) { FILE *f=fopen(p,"rb"); size_t n; int c; if(!f)return 0; n=fread(r,1,SIZE,f); c=fgetc(f); fclose(f); return n==SIZE && c==EOF; }
static int service(void *ctx, uint8_t *r, pr_r08ae_137c_cpu *c, uint16_t *flags) {
    unsigned stack, i; uint16_t reg[9];
    if (!ctx || *(const uint16_t *)ctx!=0x0814 || c->ax!=0x3508 || c->bx!=0x3ed4 || c->cx!=4 || c->dx!=15 || c->si!=0x3e8a || c->di!=0x4f || c->bp!=0x3db6 || c->ds!=0x10d2 || c->es!=0x3da1 || c->cs!=0x10d2 || c->ip!=0x1386 || c->ss!=0x1d16 || c->sp!=0x3da8 || c->flags!=0x7002 || r[0x8140]!=0xcd || r[0x8141]!=0x20) return 1;
    stack=((unsigned)c->ss<<4)+c->sp;
    if (stack<18 || stack+6>SIZE || get(r,stack)!=0x1388 || get(r,stack+2)!=c->cs || get(r,stack+4)!=0x7202) return 1;
    reg[0]=c->ax;reg[1]=c->bx;reg[2]=c->cx;reg[3]=c->dx;reg[4]=c->si;reg[5]=c->di;reg[6]=c->bp;reg[7]=c->ds;reg[8]=c->es;
    for(i=0;i<9;i++) word(r,stack-18+2*i,reg[i]);
    word(r,0x8140+0x2e,(uint16_t)(c->sp-18)); word(r,0x8140+0x30,c->ss);
    c->bx=get(r,0x20); c->es=get(r,0x22); *flags=get(r,stack+4); return 0;
}
int main(int argc,char **argv) {
    uint8_t *r,*expected; unsigned i,diffs=0; uint16_t psp=0x0815;
    pr_r08ae_137c_cpu initial={0x3508,0x3ed4,4,15,0x3e8a,0x4f,0x3db6,0x3dae,0x10d2,0x1386,0x10d2,0x3da1,0x1d16,0x7202}, c;
    const pr_r08ae_137c_cpu post={0x3508,0xfea5,4,15,0x3e8a,0x4f,0x3db6,0x3dae,0x10d2,0x1388,0x10d2,0xf000,0x1d16,0x7202};
    pr_r08ae_137c_env env={&psp,service,NULL};
    CHECK(argc==3); r=calloc(1,PR_R08AE_137C_RAM_SIZE); expected=malloc(SIZE); CHECK(r&&expected&&load(argv[1],r)&&load(argv[2],expected));
    c=initial; CHECK(pr_r08ae_vector_137c_from_1386(r,PR_R08AE_137C_RAM_SIZE,&c,&env)==PR_R08AE_137C_ENV_DOS); CHECK(c.ip==0x1386 && c.sp==0x3da8 && c.flags==0x7002);
    CHECK(load(argv[1],r)); c=initial; CHECK(pr_r08ae_vector_137c_from_1386(r,PR_R08AE_137C_RAM_SIZE,&c,NULL)==PR_R08AE_137C_ENV_DOS); CHECK(c.ip==0x1386 && c.sp==0x3da8);
    CHECK(load(argv[1],r)); c=initial; psp=0x0814;
    CHECK(pr_r08ae_vector_137c_from_1386(r,PR_R08AE_137C_RAM_SIZE,&c,&env)==PR_R08AE_137C_STOP_1388);
    for(i=0;i<SIZE;i++) if(r[i]!=expected[i]) { if(diffs<8) fprintf(stderr,"diff %05x\n",i); diffs++; }
    CHECK(diffs==0); CHECK(c.ax==post.ax && c.bx==post.bx && c.cx==post.cx && c.dx==post.dx && c.si==post.si && c.di==post.di && c.bp==post.bp && c.sp==post.sp && c.cs==post.cs && c.ip==post.ip && c.ds==post.ds && c.es==post.es && c.ss==post.ss && c.flags==post.flags);
    puts("run003 1386->1388: conditional PSP=0814 binding matches CPU14 and 655360 RAM bytes; missing/unsupported binding refuses");
    free(r); free(expected); return 0;
}
