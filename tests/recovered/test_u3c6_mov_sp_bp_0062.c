#include "poolrad/recovered/overlay_u3c6_mov_sp_bp_0062.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
_Static_assert(sizeof(pr_u6c_3f31_cpu)==28,"all fourteen CPU words");
static uint8_t ram[PR_U6C_3F31_RAM_SIZE],baseline[PR_U6C_3F31_RAM_SIZE];
static unsigned checks;
static void require(int ok) { if(!ok) { fprintf(stderr,"failed check %u\n",checks);exit(1); } ++checks; }
static pr_u6c_3f31_cpu initial(unsigned v) {
 pr_u6c_3f31_cpu c={0};
 c.ax=(uint16_t)v;c.bx=0x2345;c.cx=0x9876;c.dx=0xFEDC;
 c.si=0x4567;c.di=0x89AB;c.bp=(uint16_t)v;c.sp=(uint16_t)(v^0xFFFFu);
 c.cs=0x1404;c.ip=0x0062;c.ds=0xFFFF;c.es=0x5555;c.ss=0xFFFF;
 c.flags=(uint16_t)((v^0xF202u) );return c;
}
static void run_case(pr_u6c_3f31_cpu c) {
 pr_u6c_3f31_cpu expected=c;
 int accepted=1;
 if(accepted) { expected.sp=expected.bp; expected.ip=0x0064; }
 pr_u3c6_0062_status status=pr_u3c6_mov_sp_bp_0062_fixture(ram,sizeof ram,&c);
 require(status==(accepted?PR_U3C6_0062_NEXT_PENDING:PR_U3C6_0062_ARGUMENT));
 require(memcmp(&c,&expected,sizeof c)==0);
}
int main(void) {
 for(size_t i=0;i<sizeof ram;++i)ram[i]=(uint8_t)(i*37u+(i>>9));
 memcpy(baseline,ram,sizeof ram);
 for(unsigned v=0;v<65536u;++v)run_case(initial(v));
 require(memcmp(ram,baseline,sizeof ram)==0);
 for(unsigned v=0;v<65536u;++v) { pr_u6c_3f31_cpu c=initial(0x8000);c.flags=(uint16_t)v;run_case(c); }
 require(memcmp(ram,baseline,sizeof ram)==0);
 const unsigned edges[4]={0,0x40,0xFFBF,0xFFFF};
 for(unsigned j=0;j<8;++j) { pr_u6c_3f31_cpu c=initial(j&1?0xFFFF:0);c.flags=(uint16_t)edges[j%4];c.ss=j&1?0:0xFFFF;c.sp=j&1?0:0xFFFF;run_case(c);require(memcmp(ram,baseline,sizeof ram)==0); }
 for(unsigned j=0;j<6;++j) {
  pr_u6c_3f31_cpu c=initial(0xA55A);c.flags=0x40;
  if(j==0)c.ip=0x0062+1u;
  pr_u6c_3f31_cpu expected=c;
  size_t size=j==3?0:j==4?sizeof ram-1:j==5?sizeof ram+1:sizeof ram;
  require(pr_u3c6_mov_sp_bp_0062_fixture(j==1?NULL:ram,size,j==2?NULL:&c)==PR_U3C6_0062_ARGUMENT);
  require(memcmp(&c,&expected,sizeof c)==0);require(memcmp(ram,baseline,sizeof ram)==0);
 }
 puts("PASS: 65536 register cases, 65536 FLAGS cases, 8 full RAM snapshots, 6 unchanged argument rejections");return 0;
}
