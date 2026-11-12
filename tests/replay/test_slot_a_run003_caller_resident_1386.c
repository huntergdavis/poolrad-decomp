/* Run003 captured path from caller 348D to first resident DOS refusal. */
#define _DEFAULT_SOURCE
#include "poolrad/recovered/overlay_u3bd_lookup_fullstate.h"
#include "poolrad/recovered/resident_08ae_dispatch_00dc_cpu.h"
#include "poolrad/recovered/resident_08ae_vector_137c_cpu.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#define C 0xa0000u
#define T 0x100000u
#define OK(x) do {if(!(x)){fprintf(stderr,"fail line %d: %s\n",__LINE__,#x);return 1;}}while(0)
static unsigned a(unsigned s,unsigned o){return (s<<4)+o;}
static unsigned w(uint8_t*m,unsigned p){return m[p]|(m[p+1]<<8);}
static void put(uint8_t*m,unsigned p,unsigned v){m[p]=v;m[p+1]=v>>8;}
static void push(uint8_t*m,pr_u3bd_lookup_cpu*c,unsigned v){c->sp-=2;put(m,a(c->ss,c->sp),v);}
static unsigned flags_sub(unsigned f,unsigned x,unsigned y,unsigned bits){unsigned mask=bits==8?255:65535,sign=bits==8?128:32768,r=(x-y)&mask,z=r&255;f&=~0x8d5u;if(x<y)f|=1;if((x^y^r)&16)f|=16;if(!r)f|=64;if(r&sign)f|=128;if((x^y)&(x^r)&sign)f|=0x800;z^=z>>4;z^=z>>2;z^=z>>1;if(!(z&1))f|=4;return f;}
static int load(char *p, uint8_t *m) {
    FILE *f = fopen(p, "rb"); size_t n; int extra;
    if (!f) return 0;
    n = fread(m, 1, C, f); extra = fgetc(f); fclose(f);
    return n == C && extra == EOF;
}
static int refuse_dos(void *ctx, uint8_t *ram, pr_r08ae_137c_cpu *cpu, uint16_t *flags) {
    (void)ram; (void)flags; *(pr_r08ae_137c_cpu *)ctx = *cpu; return 1;
}
int main(int n,char**v){uint8_t*m,*r;pr_u3bd_lookup_cpu c={4,0x3ed4,0,0,0x3e8a,0,0x3ef8,0x3ef6,0x3582,0x348d,0x14a0,0x3996,0x1d16,0x7246};unsigned stub,seg,p,i;
OK(n==3);m=mmap(0,T,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0);OK(m!=MAP_FAILED);OK(!mprotect(m,C,PROT_READ|PROT_WRITE));r=malloc(C);OK(r&&load(v[1],m));
OK(m[a(c.cs,c.ip)]==0x9a && w(m,a(c.cs,c.ip+1))==0x25 && w(m,a(c.cs,c.ip+3))==0x0896);
push(m,&c,c.cs); push(m,&c,0x3492); c.cs=0x0896; c.ip=0x25;
OK(m[a(c.cs,c.ip)]==0xea && w(m,a(c.cs,c.ip+1))==0x0d6d && w(m,a(c.cs,c.ip+3))==0x2e60); c.cs=0x2e60; c.ip=0x0d6d;
push(m,&c,c.bp);c.bp=c.sp;c.flags=flags_sub(c.flags,c.sp,0x12c,16);c.sp-=0x12c;m[a(c.ss,c.bp-1)]=0;
c.ax=(c.ax&0xff00)|m[a(c.ds,0x49fa)];c.flags=flags_sub(c.flags,c.ax&255,1,8);OK((c.ax&255)==1);
c.di=w(m,a(c.ds,0x49d6));c.es=w(m,a(c.ds,0x49d8));p=w(m,a(c.es,c.di+0x592));c.flags=flags_sub(c.flags,p,255,16);OK(p<255);
for(i=0;i<3;i++){c.ax=(c.ax&0xff00)|m[a(c.ds,0x6aad+i)];push(m,&c,c.ax);}
stub=w(m,a(c.cs,0x0d9f));seg=w(m,a(c.cs,0x0da1));OK(stub==0x2f&&seg==0xbf9);push(m,&c,c.cs);push(m,&c,0x0da3);OK(m[a(seg,stub)]==0xea&&w(m,a(seg,stub+1))==0x350);c.cs=w(m,a(seg,stub+3));c.ip=0x350;
OK(pr_u3bd_field_0350_fullstate(m,T,&c)==PR_U3BD_LOOKUP_OK);OK(c.cs==0x2e60&&c.ip==0x0da3&&(c.ax&255)==1);c.flags=flags_sub(c.flags,c.ax&255,1,8);m[a(c.ss,c.bp-1)]=1;c.flags=flags_sub(c.flags,1,0,8);
push(m,&c,c.cs);push(m,&c,0x1031);c.ip=0x073b;push(m,&c,c.bp);c.bp=c.sp;push(m,&c,w(m,a(c.ds,0x2628)));c.ip=0x0742;
printf("CPU AX=%04x BX=%04x CX=%04x DX=%04x SI=%04x DI=%04x BP=%04x SP=%04x CS=%04x IP=%04x DS=%04x ES=%04x SS=%04x FLAGS=%04x\n",c.ax,c.bx,c.cx,c.dx,c.si,c.di,c.bp,c.sp,c.cs,c.ip,c.ds,c.es,c.ss,c.flags);
/* Continue the C-produced state in the same mapping; no 0742 reload. */
{ pr_r08ae_00dc_cpu d; pr_r08ae_137c_cpu x, observed = {0}, before;
  pr_r08ae_137c_env env = {&observed, refuse_dos, NULL};
  unsigned ssbase = a(c.ss, 0), sp; uint8_t frame_before[6]; unsigned differences = 0;
  OK(sizeof d == sizeof c && sizeof x == sizeof c);
  OK(m[a(c.cs,c.ip)] == 0x9a && w(m,a(c.cs,c.ip+1)) == 0x00dc && w(m,a(c.cs,c.ip+3)) == 0x10d2);
  push(m,&c,c.cs); push(m,&c,0x0747); c.cs=0x10d2; c.ip=0x00dc;
  memcpy(&d,&c,sizeof d);
  OK(pr_r08ae_dispatch_00dc_cpu(m,T,&d)==PR_R08AE_00DC_CALL_137C);
  memcpy(&x,&d,sizeof x);
  /* 137C pushes DS and ES above this range before the INT frame is written. */
  memcpy(frame_before, m+ssbase+0x3da8, sizeof frame_before);
  OK(pr_r08ae_vector_137c_cpu(m,T,&x,&env)==PR_R08AE_137C_ENV_DOS);
  OK(observed.ip==0x1386 && observed.sp==0x3da8 && observed.ax==0x3508);
  sp=ssbase+observed.sp;
  OK(w(m,sp)==0x1388 && w(m,sp+2)==0x10d2);
  before=observed; before.sp+=6; before.flags=w(m,sp+4);
  OK(before.ax==0x3508 && before.bx==0x3ed4 && before.cx==4 && before.dx==15 && before.si==0x3e8a && before.di==0x4f);
  OK(before.bp==0x3db6 && before.sp==0x3dae && before.cs==0x10d2 && before.ip==0x1386);
  OK(before.ds==0x10d2 && before.es==0x3da1 && before.ss==0x1d16 && before.flags==0x7202);
  OK(load(v[2],r));
  for(i=0x3dae;i<0x3dbc;i++) OK(m[ssbase+i]==r[ssbase+i]);
  for(i=0;i<C;i++) { unsigned byte=(i>=ssbase+0x3da8 && i<ssbase+0x3dae)?frame_before[i-(ssbase+0x3da8)]:m[i]; if(byte!=r[i]) {if(differences<12)printf("pre-INT diff %05x %02x %02x\n",i,byte,r[i]);differences++;} }
  printf("pre-INT conventional RAM differences: %u\n", differences);
  OK(differences == 0);
  printf("run003 348D C -> pre-INT 1386: CPU14 and 14 prior stack bytes match; DOS refused after frame\n");
}
free(r);OK(munmap(m,T)==0);return 0;}
