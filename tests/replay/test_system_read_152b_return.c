#include "poolrad/replay/system_read_152b_return.h"
#include <stdio.h>
#include <string.h>
#define CHECK(x) do{if(!(x)){fprintf(stderr,"line%d: %s\n",__LINE__,#x);return 1;}}while(0)
static uint8_t ram[655360u],saved[655360u];
static void put(size_t a,uint16_t v){ram[a]=(uint8_t)v;ram[a+1u]=(uint8_t)(v>>8);}
static int blocked(pr_slot_a_directory_state *s,pr_system_read_return_status status)
{
 pr_slot_a_directory_cpu before=s->cpu;pr_system_read_return_report r;
 memcpy(saved,ram,sizeof ram);memset(&r,0xA5,sizeof r);
 CHECK(pr_system_read_152b_step(s,&r)==status);
 CHECK(!memcmp(&before,&s->cpu,sizeof before) && !memcmp(saved,ram,sizeof ram));
 if(status!=PR_SYSTEM_READ_RETURN_BLOCKED_LOADER && status!=PR_SYSTEM_READ_RETURN_BLOCKED_IO_CALLER)CHECK(!r.valid);
 return 0;
}
int main(void)
{
 pr_slot_a_directory_state s={0};pr_system_read_return_report r,old;unsigned i;
 s.ram=ram;s.ram_size=sizeof ram;s.cpu.cs=0x131Cu;s.cpu.ip=0x152Bu;s.cpu.sp=0xFCu;s.cpu.bp=0x100u;
 s.cpu.es=0x2000u;s.cpu.di=0x40u;s.cpu.ax=0x11Du;s.cpu.dx=8u;s.cpu.flags=0x7202u;
 put(0xFCu,0x64u);put(0xFEu,0u);put(0x100u,0x200u);put(0x102u,0xC49u);put(0x104u,0x2A8Eu);
 put(0x106u,0x20u);put(0x108u,0x1000u);put(0x10Au,0x11Du);put(0x20044u,1u);
 for(i=0;i<13u;i++)CHECK(pr_system_read_152b_step(&s,&r)==PR_SYSTEM_READ_RETURN_STEPPED && !r.valid);
 CHECK(s.cpu.cs==0x2A8Eu && s.cpu.ip==0xC49u && s.cpu.sp==0x114u && s.cpu.bp==0x200u);
 CHECK(s.cpu.ax==0x11Du && s.cpu.ds==0u && s.cpu.cx==0x64u && ram[0x10020u]==0x1Du && ram[0x10021u]==1u);
 CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_LOADER)==0);
 CHECK(pr_system_read_152b_step(&s,&r)==PR_SYSTEM_READ_RETURN_BLOCKED_LOADER && r.valid && r.return_segment==0x2A8Eu && r.return_pc==0xC49u);
 s.cpu.cs=0x131Cu;s.cpu.ip=0x4A2u;s.cpu.sp=0x200u;s.cpu.ax=0xFFFFu;s.cpu.flags=0x7AD7u;
 put(0x200u,0x334u);put(0x202u,0x1026u);put(0x4426u,0xABCDu);
 for(i=0;i<3u;i++)CHECK(pr_system_read_152b_step(&s,&r)==PR_SYSTEM_READ_RETURN_STEPPED);
 CHECK(s.cpu.cs==0x1026u && s.cpu.ip==0x334u && s.cpu.sp==0x204u && s.cpu.ax==0xABCDu && s.cpu.flags==0x7246u);
 CHECK(ram[0x4426u]==0u && ram[0x4427u]==0u && blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_IO_CALLER)==0);
 s.cpu.cs=0x131Cu;s.cpu.ip=0x1531u;s.cpu.es=0x2000u;s.cpu.di=0x40u;s.cpu.ax=0x11Du;s.cpu.dx=0u;s.cpu.flags=0x7246u;
 CHECK(pr_system_read_152b_step(&s,&r)==PR_SYSTEM_READ_RETURN_STEPPED && s.cpu.ax==0x11Du && s.cpu.dx==0u && s.cpu.flags==0x7242u);
 s.cpu.ip=0x1531u;put(0x20044u,0u);CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_DIVIDE)==0);
 put(0x20044u,1u);s.cpu.dx=1u;s.cpu.ax=0u;CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_DIVIDE)==0);
 s.cpu.di=0xFFFBu;CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x1535u;s.cpu.bp=0xFFF9u;CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x152Bu;s.cpu.sp=0xFFFFu;CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x1560u;s.cpu.sp=0xFFFEu;CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x4A4u;s.ram_size=0x4427u;CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.cpu.ip=0x153Eu;s.cpu.es=0u;s.cpu.di=0xFFFFu;CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY)==0);
 s.cpu.ip=0x1529u;CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_PC)==0);
 s.ram_size=0x100001u;CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY)==0);
 s.ram_size=sizeof ram;s.ram=NULL;CHECK(blocked(&s,PR_SYSTEM_READ_RETURN_BLOCKED_MEMORY)==0);
 memset(&r,0xA5,sizeof r);old=r;
 CHECK(pr_system_read_152b_step(NULL,&r)==PR_SYSTEM_READ_RETURN_ARGUMENT && !memcmp(&r,&old,sizeof r));
 CHECK(pr_system_read_152b_step(&s,NULL)==PR_SYSTEM_READ_RETURN_ARGUMENT);
 puts("Read152B/IOResult strict count/frames, full-word exchange, native DIV policy and atomic stops passed");return 0;
}
