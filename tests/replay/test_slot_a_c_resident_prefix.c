#define _DEFAULT_SOURCE
#include "poolrad/recovered/resident_08ae_dispatch_00dc_cpu.h"
#include "poolrad/recovered/resident_08ae_vector_137c_cpu.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#define CONV 0xa0000u
#define TOTAL 0x100000u
#define CHECK(x) do { if (!(x)) { fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while (0)
static unsigned get16(const uint8_t *m,unsigned p) { return m[p] | ((unsigned)m[p+1]<<8); }
static void put16(uint8_t *m,unsigned p,unsigned v) { m[p]=(uint8_t)v; m[p+1]=(uint8_t)(v>>8); }
static int refuse(void *ctx,uint8_t *ram,pr_r08ae_137c_cpu *cpu,uint16_t *flags) {
    pr_r08ae_137c_cpu *capture=ctx; (void)ram; (void)flags; *capture=*cpu; return 1;
}
static int load(const char *name,uint8_t *m) { FILE *f=fopen(name,"rb"); size_t n; int end; if(!f)return 0; n=fread(m,1,CONV,f); end=fgetc(f); fclose(f); return n==CONV && end==EOF; }
int main(int argc,char **argv) {
    uint8_t *m,*target; pr_r08ae_00dc_cpu d={0}; pr_r08ae_137c_cpu v,seen={0},pre;
    pr_r08ae_137c_env env={&seen,refuse,NULL};
    unsigned i, base=0x1d160u, sp; const uint8_t instruction[]={0x9a,0xdc,0x00,0xd2,0x10};
    CHECK(argc==3);
    m=mmap(NULL,TOTAL,PROT_NONE,MAP_PRIVATE|MAP_ANONYMOUS,-1,0); CHECK(m!=MAP_FAILED);
    CHECK(mprotect(m,CONV,PROT_READ|PROT_WRITE)==0);
    target=malloc(CONV); CHECK(target && load(argv[1],m) && load(argv[2],target));
    CHECK(!memcmp(m+0x2e600+0x742,instruction,5));
    /* CPU0742 transcription; far CALL derives return frame, conditional on no interrupt. */
    d=(pr_r08ae_00dc_cpu){4,0x3ed4,0,0,0x3e8a,0,0x3dbe,0x3dbc,0x10d2,0x00dc,0x14a0,0x3da1,0x1d16,0x7202};
    /* Correct captured registers at 0742. */
    d.ax=1; d.cx=4; d.dx=15; d.di=0x4f;
    put16(m,base+0x3dba,0x2e60); put16(m,base+0x3db8,0x0747); d.sp=0x3db8;
    CHECK(pr_r08ae_dispatch_00dc_cpu(m,TOTAL,&d)==PR_R08AE_00DC_CALL_137C);
    memcpy(&v,&d,sizeof v);
    CHECK(pr_r08ae_vector_137c_cpu(m,TOTAL,&v,&env)==PR_R08AE_137C_ENV_DOS);
    CHECK(seen.ip==0x1386 && seen.sp==0x3da8 && seen.ax==0x3508);
    sp=base+seen.sp; CHECK(get16(m,sp)==0x1388 && get16(m,sp+2)==0x10d2);
    pre=seen; pre.sp=(uint16_t)(seen.sp+6); pre.flags=(uint16_t)get16(m,sp+4);
    CHECK(pre.ax==0x3508 && pre.bx==0x3ed4 && pre.cx==4 && pre.dx==15 && pre.si==0x3e8a && pre.di==0x4f);
    CHECK(pre.bp==0x3db6 && pre.sp==0x3dae && pre.cs==0x10d2 && pre.ip==0x1386);
    CHECK(pre.ds==0x10d2 && pre.es==0x3da1 && pre.ss==0x1d16 && pre.flags==0x7202);
    for(i=0x3dae;i<0x3dbc;i++) CHECK(m[base+i]==target[base+i]);
    printf("C dispatch + 137C: pre-INT CPU14 and 14 stack bytes match; DOS refused after INT frame\n");
    free(target); CHECK(munmap(m,TOTAL)==0); return 0;
}
