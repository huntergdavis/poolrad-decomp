#include "poolrad/replay/slot_a_cba_getmem_frame.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x100000u],before[0x100000u];
static void put(size_t a,uint16_t v){ram[a]=(uint8_t)v;ram[a+1u]=(uint8_t)(v>>8);}
static pr_slot_a_directory_state initial(void)
{
 pr_slot_a_directory_state s={0};s.ram=ram;s.ram_size=sizeof ram;
 s.cpu.cs=0x2A8Eu;s.cpu.ip=0xCBAu;s.cpu.ss=0x1D16u;
 s.cpu.sp=0x33EAu;s.cpu.bp=0x37A6u;s.cpu.ax=0xABCDu;s.cpu.flags=0x7602u;
 put(0x2054Au,0x3Fu);return s;
}
static void blocked(pr_slot_a_directory_state *s,pr_system_getmem_status status)
{
 pr_slot_a_directory_cpu c=s->cpu;pr_system_getmem_report r={0};memcpy(before,ram,sizeof ram);
 assert(pr_slot_a_cba_getmem_step(s,&r)==status);
 assert(!memcmp(&c,&s->cpu,sizeof c) && !memcmp(before,ram,sizeof ram));
 if(status==PR_SYSTEM_GETMEM_BLOCKED_CORE)
  assert(r.dependency_valid && r.call_is_near && r.call_pc==0x245u && r.callee_segment==0xAF8u && r.callee_pc==0x308u && r.return_pc==0x248u);
 else assert(!r.dependency_valid);
}
int main(void)
{
 pr_slot_a_directory_state s=initial();pr_system_getmem_report r={0};unsigned i;
 for(i=0;i<4u;++i)assert(pr_slot_a_cba_getmem_step(&s,&r)==PR_SYSTEM_GETMEM_STEPPED);
 assert(s.cpu.cs==0x131Cu && s.cpu.ip==0x245u && s.cpu.bp==0x33E4u && s.cpu.sp==0x33E4u);
 assert(s.cpu.ax==0x3Fu && s.cpu.flags==0x7602u);
 assert(!memcmp(ram+0x20544u,"\xA6\x37\xBF\x0C\x8E\x2A\x3F\0",8));
 blocked(&s,PR_SYSTEM_GETMEM_BLOCKED_CORE);
 /* A separate return input supplies the actual post-core state, never a bridge. */
 s.cpu.ip=0x248u;s.cpu.ax=0x1234u;s.cpu.dx=0x5678u;
 for(i=0;i<3u;++i)assert(pr_slot_a_cba_getmem_step(&s,&r)==PR_SYSTEM_GETMEM_STEPPED);
 assert(s.cpu.cs==0x2A8Eu && s.cpu.ip==0xCBFu && s.cpu.sp==0x33ECu && s.cpu.bp==0x37A6u);
 assert(s.cpu.ax==0x1234u && s.cpu.dx==0x5678u);blocked(&s,PR_SYSTEM_GETMEM_BLOCKED_CALLER);
 s=initial();s.cpu.cs=0x131Cu;s.cpu.ip=0x248u;s.cpu.sp=0x33E4u;s.cpu.flags|=1u;
 for(i=0;i<4u;++i)assert(pr_slot_a_cba_getmem_step(&s,&r)==PR_SYSTEM_GETMEM_STEPPED);
 assert(s.cpu.ip==0xE2u && s.cpu.ax==0xCBu);blocked(&s,PR_SYSTEM_GETMEM_BLOCKED_RUNTIME_ERROR);
 s=initial();s.cpu.sp=2;blocked(&s,PR_SYSTEM_GETMEM_BLOCKED_MEMORY);
 s=initial();s.ram_size=0x20549u;blocked(&s,PR_SYSTEM_GETMEM_BLOCKED_MEMORY);
 s=initial();s.cpu.ss=0xFFFFu;s.cpu.sp=0x20u;blocked(&s,PR_SYSTEM_GETMEM_BLOCKED_MEMORY);
 s=initial();s.ram=NULL;blocked(&s,PR_SYSTEM_GETMEM_BLOCKED_MEMORY);
 s=initial();s.ram_size=sizeof ram+1u;blocked(&s,PR_SYSTEM_GETMEM_BLOCKED_MEMORY);
 s=initial();s.cpu.cs=0x131Cu;s.cpu.ip=0x308u;blocked(&s,PR_SYSTEM_GETMEM_BLOCKED_PC);
 s=initial();s.cpu.cs=0x1026u;blocked(&s,PR_SYSTEM_GETMEM_BLOCKED_PC);
 r.dependency_valid=9;assert(pr_slot_a_cba_getmem_step(NULL,&r)==PR_SYSTEM_GETMEM_ARGUMENT && r.dependency_valid==9u);
 assert(pr_slot_a_cba_getmem_step(&s,NULL)==PR_SYSTEM_GETMEM_ARGUMENT);
 puts("CBA frame, independent wrapper return and atomic blockers passed");return 0;
}
