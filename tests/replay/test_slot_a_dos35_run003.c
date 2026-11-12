/* Single observed DOSBox-style AH=35 profile; PSP ownership is a supplied
 * hypothesis, not inferred or verified from the capture. */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define SIZE 655360u
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } cpu;
static void word(uint8_t *r, unsigned p, uint16_t v) { r[p]=(uint8_t)v; r[p+1]=(uint8_t)(v>>8); }
static uint16_t get(const uint8_t *r, unsigned p) { return (uint16_t)(r[p] | (r[p+1]<<8)); }
static int load(const char *p, uint8_t *r) { FILE *f=fopen(p,"rb"); size_t n; int c; if(!f)return 0; n=fread(r,1,SIZE,f); c=fgetc(f); fclose(f); return n==SIZE && c==EOF; }
/* Restricted to this checked state; no mutation on refusal. */
static int service(uint8_t *r, cpu *c, uint16_t psp) {
    unsigned st, stack, i; uint16_t reg[9];
    if (!r || !c || psp!=0x0814 || c->ax!=0x3508 || c->bx!=0x3ed4 || c->cx!=4 || c->dx!=15 || c->si!=0x3e8a || c->di!=0x4f || c->bp!=0x3db6 || c->ds!=0x10d2 || c->es!=0x3da1 || c->cs!=0x10d2 || c->ip!=0x1386 || c->ss!=0x1d16 || c->sp!=0x3dae || c->flags!=0x7202 || r[0x8140]!=0xcd || r[0x8141]!=0x20) return 1;
    st=((unsigned)c->ss<<4); stack=st+c->sp; if(stack>SIZE || stack<24 || ((unsigned)psp<<4)+0x32>SIZE) return 1;
    /* INT frame; DOS handler sees SP minus six and saves registers below it. */
    word(r,stack-2,c->flags); word(r,stack-4,c->cs); word(r,stack-6,0x1388);
    reg[0]=c->ax;reg[1]=c->bx;reg[2]=c->cx;reg[3]=c->dx;reg[4]=c->si;reg[5]=c->di;reg[6]=c->bp;reg[7]=c->ds;reg[8]=c->es;
    for(i=0;i<9;i++) word(r,stack-24+2*i,reg[i]);
    word(r,((unsigned)psp<<4)+0x2e,(uint16_t)(c->sp-24)); word(r,((unsigned)psp<<4)+0x30,c->ss);
    c->bx=get(r,0x20);c->es=get(r,0x22);c->ip=0x1388; return 0;
}
int main(int argc,char **argv) {
    uint8_t *r,*expected,*copy; cpu c={0x3508,0x3ed4,4,15,0x3e8a,0x4f,0x3db6,0x3dae,0x10d2,0x1386,0x10d2,0x3da1,0x1d16,0x7202}, before; unsigned i,diffs=0;
    CHECK(argc==3);r=malloc(SIZE);expected=malloc(SIZE);copy=malloc(SIZE);CHECK(r&&expected&&copy&&load(argv[1],r)&&load(argv[2],expected));
    memcpy(copy,r,SIZE);before=c;CHECK(service(r,&c,0x0815)!=0 && !memcmp(r,copy,SIZE) && !memcmp(&c,&before,sizeof c));
    CHECK(service(r,&c,0x0814)==0);
    for(i=0;i<SIZE;i++) if(r[i]!=expected[i]){if(diffs<8)fprintf(stderr,"diff %05x\n",i);diffs++;}
    CHECK(diffs==0 && c.ax==0x3508 && c.bx==0xfea5 && c.cx==4 && c.dx==15 && c.si==0x3e8a && c.di==0x4f && c.bp==0x3db6 && c.sp==0x3dae && c.cs==0x10d2 && c.ip==0x1388 && c.ds==0x10d2 && c.es==0xf000 && c.ss==0x1d16 && c.flags==0x7202);
    puts("run003 conditional PSP=0814 DOS35 hypothesis: CPU14 and 655360 conventional RAM bytes match; unsupported PSP refused");
    free(r);free(expected);free(copy);return 0;
}
