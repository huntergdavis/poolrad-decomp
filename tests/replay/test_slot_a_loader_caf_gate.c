#include "poolrad/replay/slot_a_loader_caf_gate.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x100000u],before[0x100000u];
static pr_slot_a_directory_state initial(uint16_t ax)
{
 pr_slot_a_directory_state s={0};s.ram=ram;s.ram_size=sizeof ram;
 s.cpu.cs=0x2A8Eu;s.cpu.ip=0xCAFu;s.cpu.ss=0x1D16u;
 s.cpu.sp=0x33ECu;s.cpu.ax=ax;s.cpu.flags=0x7AD7u;return s;
}
static void blocked(pr_slot_a_directory_state *s,pr_loader_caf_status status)
{
 pr_slot_a_directory_cpu cpu=s->cpu;pr_slot_a_string_0356_report r={0};
 memcpy(before,ram,sizeof ram);
 assert(pr_slot_a_loader_caf_step(s,&r)==status);
 assert(!memcmp(&cpu,&s->cpu,sizeof cpu));assert(!memcmp(before,ram,sizeof ram));
 if(status==PR_LOADER_CAF_BLOCKED_GETMEM){
  assert(r.dependency_valid==1u && r.caller==0x6Fu && r.call_pc==0xCBAu);
  assert(r.callee_segment==0xAF8u && r.callee_pc==0x23Fu);
  assert(r.return_pc==0xCBFu && r.caller_is_overlay==1u);
 }else assert(!r.dependency_valid);
}
int main(void)
{
 pr_slot_a_directory_state s;pr_slot_a_string_0356_report r={0};unsigned i;
 const uint16_t values[]={0,0x100u,0xAB00u,1,0x7Fu,0x80u,0xFFFFu};
 for(i=0;i<sizeof values/sizeof values[0];++i){
  s=initial(values[i]);
  assert(pr_slot_a_loader_caf_step(&s,&r)==PR_LOADER_CAF_STEPPED);
  assert(s.cpu.ax==values[i] && !(s.cpu.flags&0x811u));
  assert((s.cpu.flags&0x40u)==((values[i]&255u)?0u:0x40u));
  assert(pr_slot_a_loader_caf_step(&s,&r)==PR_LOADER_CAF_STEPPED);
  if(!(values[i]&255u)){
   assert(s.cpu.ip==0xCB3u);
   assert(pr_slot_a_loader_caf_step(&s,&r)==PR_LOADER_CAF_STEPPED);
   assert(s.cpu.ip==0xD8Fu && s.cpu.sp==0x33ECu && s.cpu.ax==values[i]);
   blocked(&s,PR_LOADER_CAF_BLOCKED_NEXT_CONTEXT);
  }else{
   assert(s.cpu.ip==0xCB6u);
   assert(pr_slot_a_loader_caf_step(&s,&r)==PR_LOADER_CAF_STEPPED);
   assert(s.cpu.ax==0x3Fu);
   assert(pr_slot_a_loader_caf_step(&s,&r)==PR_LOADER_CAF_STEPPED);
   assert(s.cpu.sp==0x33EAu && ram[0x2054Au]==0x3Fu && !ram[0x2054Bu]);
   blocked(&s,PR_LOADER_CAF_BLOCKED_GETMEM);
  }
 }
 s=initial(0xABCDu);s.cpu.ip=0xCB9u;s.cpu.sp=0;
 assert(pr_slot_a_loader_caf_step(&s,&r)==PR_LOADER_CAF_STEPPED);
 assert(s.cpu.sp==0xFFFEu && ram[0x2D15Eu]==0xCDu && ram[0x2D15Fu]==0xABu);
 s=initial(1);s.cpu.ip=0xCB9u;s.cpu.sp=1;blocked(&s,PR_LOADER_CAF_BLOCKED_MEMORY);
 s=initial(1);s.cpu.ip=0xCB9u;s.ram_size=0x2054Bu;blocked(&s,PR_LOADER_CAF_BLOCKED_MEMORY);
 s=initial(1);s.cpu.ip=0xCB9u;s.cpu.ss=0xFFFFu;s.cpu.sp=0x20u;blocked(&s,PR_LOADER_CAF_BLOCKED_MEMORY);
 s=initial(1);s.ram_size=sizeof ram+1u;blocked(&s,PR_LOADER_CAF_BLOCKED_MEMORY);
 s=initial(1);s.ram=NULL;blocked(&s,PR_LOADER_CAF_BLOCKED_MEMORY);
 s=initial(1);s.cpu.cs=0x1026u;blocked(&s,PR_LOADER_CAF_BLOCKED_PC);
 s=initial(1);s.cpu.ip=0xCBFu;blocked(&s,PR_LOADER_CAF_BLOCKED_PC);
 s=initial(1);r.dependency_valid=7;
 assert(pr_slot_a_loader_caf_step(NULL,&r)==PR_LOADER_CAF_ARGUMENT && r.dependency_valid==7u);
 assert(pr_slot_a_loader_caf_step(&s,NULL)==PR_LOADER_CAF_ARGUMENT && s.cpu.ip==0xCAFu);
 puts("CAF focused branch, typed stop and atomic memory checks passed");return 0;
}
