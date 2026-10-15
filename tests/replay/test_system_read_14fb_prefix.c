#include "poolrad/replay/system_read_14fb_prefix.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[655360u],saved[655360u];
static int blocked(pr_slot_a_directory_state *s,pr_system_read_status status)
{
 pr_slot_a_directory_cpu before=s->cpu;pr_system_read_report r;
 memcpy(saved,ram,sizeof ram);memset(&r,0xA5,sizeof r);
 CHECK(pr_system_read_14fb_step(s,&r)==status);
 CHECK(!memcmp(&before,&s->cpu,sizeof before) && !memcmp(saved,ram,sizeof ram));
 if(status!=PR_SYSTEM_READ_BLOCKED_DOS)CHECK(!r.valid);
 return 0;
}
int main(void)
{
 pr_slot_a_directory_state s={0};pr_system_read_report r,old;
 s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x131Cu;s.cpu.ip=0x150Du;s.cpu.sp=2u;
 CHECK(pr_system_read_14fb_step(&s,&r)==PR_SYSTEM_READ_STEPPED);
 CHECK(s.cpu.ip==0x14B6u && s.cpu.sp==0u && ram[0]==0x10u && ram[1]==0x15u);
 s.cpu.ip=0x14C4u;CHECK(pr_system_read_14fb_step(&s,&r)==PR_SYSTEM_READ_STEPPED);
 CHECK(s.cpu.ip==0x1510u && s.cpu.sp==2u);
 s.cpu.ip=0x1507u;s.cpu.sp=0u;s.cpu.bp=0xABCDu;
 CHECK(pr_system_read_14fb_step(&s,&r)==PR_SYSTEM_READ_STEPPED);
 CHECK(s.cpu.sp==0xFFFEu && ram[0xFFFEu]==0xCDu && ram[0xFFFFu]==0xABu);
 s.cpu.ip=0x1529u;s.cpu.ax=0x3F1Du;s.cpu.bx=6u;s.cpu.cx=0x11Du;s.cpu.ds=0x449Au;s.cpu.dx=8u;
 CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_DOS)==0);
 CHECK(pr_system_read_14fb_step(&s,&r)==PR_SYSTEM_READ_BLOCKED_DOS);
 CHECK(r.valid && r.original_segment==0xAF8u && r.call_pc==0x1529u && r.return_pc==0x152Bu);
 CHECK(r.ax==0x3F1Du && r.bx==6u && r.cx==0x11Du && r.ds==0x449Au && r.dx==8u);
 s.cpu.ip=0x14BEu;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_SIGNATURE_ERROR)==0);
 s.cpu.ip=0x1535u;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_ZERO_TAIL)==0);
 s.cpu.ip=0x1551u;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_ERROR_TAIL)==0);
 s.cpu.ip=0x152Bu;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_PC)==0);
 s.cpu.cs=0xAF8u;s.cpu.ip=0x14FBu;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_PC)==0);
 s.cpu.cs=0x131Cu;s.cpu.ip=0x150Au;s.cpu.bp=0xFFEFu;
 CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x1512u;s.cpu.bp=0xFFF5u;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x1521u;s.cpu.bp=0xFFF2u;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x14B6u;s.cpu.es=0u;s.cpu.di=0xFFFDu;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x151Bu;s.cpu.di=0xFFFBu;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x1526u;s.cpu.di=0xFFFFu;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x1507u;s.cpu.sp=1u;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x14C4u;s.cpu.sp=0xFFFFu;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x151Bu;s.cpu.di=0u;s.cpu.ax=0x11Du;s.cpu.flags=0x7206u;ram[4]=1u;ram[5]=0u;
 CHECK(pr_system_read_14fb_step(&s,&r)==PR_SYSTEM_READ_STEPPED);
 CHECK(s.cpu.ax==0x11Du && s.cpu.dx==0u && s.cpu.flags==0x7202u);
 s.cpu.ip=0x151Bu;s.cpu.ax=0xFFFFu;s.cpu.flags=0x7292u;ram[4]=0xFFu;ram[5]=0xFFu;
 CHECK(pr_system_read_14fb_step(&s,&r)==PR_SYSTEM_READ_STEPPED);
 CHECK(s.cpu.ax==1u && s.cpu.dx==0xFFFEu && s.cpu.flags==0x7A97u);
 s.cpu.ip=0x151Bu;s.cpu.ax=0x100u;s.cpu.flags=0x7202u;ram[4]=0u;ram[5]=1u;
 CHECK(pr_system_read_14fb_step(&s,&r)==PR_SYSTEM_READ_STEPPED);
 CHECK(s.cpu.ax==0u && s.cpu.dx==1u && s.cpu.flags==0x7A43u);
 s.cpu.ip=0x1515u;s.cpu.ax=0u;s.cpu.flags=0x7AD7u;
 CHECK(pr_system_read_14fb_step(&s,&r)==PR_SYSTEM_READ_STEPPED && s.cpu.flags==0x7246u);
 CHECK(pr_system_read_14fb_step(&s,&r)==PR_SYSTEM_READ_STEPPED && s.cpu.ip==0x1535u);
 s.cpu.ip=0x1526u;s.cpu.es=0xFFFFu;s.cpu.di=0xFFFFu;
 CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 s.ram_size=0x100001u;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(&s,PR_SYSTEM_READ_BLOCKED_MEMORY)==0);
 memset(&r,0xA5,sizeof r);old=r;
 CHECK(pr_system_read_14fb_step(NULL,&r)==PR_SYSTEM_READ_ARGUMENT && !memcmp(&r,&old,sizeof r));
 CHECK(pr_system_read_14fb_step(&s,NULL)==PR_SYSTEM_READ_ARGUMENT);
 puts("Read14FB strict real frames, native MUL flags, DOS/tail stops and atomic bounds passed");return 0;
}
