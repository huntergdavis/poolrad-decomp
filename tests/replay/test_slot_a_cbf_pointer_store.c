#include "poolrad/replay/slot_a_cbf_pointer_store.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static uint8_t ram[0x100000u],before[0x100000u];
static void put(size_t a,uint16_t v){ram[a]=(uint8_t)v;ram[a+1u]=(uint8_t)(v>>8);}
static pr_slot_a_directory_state initial(uint16_t bp,uint16_t di,uint16_t es)
{
 pr_slot_a_directory_state s={0};s.ram=ram;s.ram_size=sizeof ram;
 memset(ram,0xA5u,sizeof ram);s.cpu.cs=0x2A8Eu;s.cpu.ip=0xCBFu;
 s.cpu.ss=0x2000u;s.cpu.bp=bp;s.cpu.di=0x7777u;s.cpu.es=0x1111u;
 s.cpu.ax=0xABCDu;s.cpu.dx=0xEF01u;s.cpu.flags=0x7AD7u;
 put(0x20000u+(uint16_t)(bp+0xAu),di);
 put(0x20000u+(uint16_t)(bp+0xCu),es);return s;
}
static void blocked(pr_slot_a_directory_state *s,pr_cbf_store_status status)
{
 pr_slot_a_directory_cpu c=s->cpu;pr_cbf_store_report r={0};memcpy(before,ram,sizeof ram);
 assert(pr_slot_a_cbf_pointer_store_step(s,&r)==status);
 assert(!memcmp(&c,&s->cpu,sizeof c) && !memcmp(before,ram,sizeof ram));
 if(status==PR_CBF_STORE_BLOCKED_CONTINUATION)
  assert(r.valid && r.continuation_segment==0x2A8Eu && r.continuation_pc==0xCCCu);
 else assert(!r.valid);
}
int main(void)
{
 pr_slot_a_directory_state s=initial(0x1000u,0x200u,0x3000u);pr_cbf_store_report r={0};unsigned i;
 for(i=0;i<3u;++i)assert(pr_slot_a_cbf_pointer_store_step(&s,&r)==PR_CBF_STORE_STEPPED);
 assert(s.cpu.di==0x200u && s.cpu.es==0x3000u && s.cpu.flags==0x7AD7u);
 assert(!memcmp(ram+0x302C8u,"\xCD\xAB\x01\xEF",4));
 assert(ram[0x302C7u]==0xA5u && ram[0x302CCu]==0xA5u);
 blocked(&s,PR_CBF_STORE_BLOCKED_CONTINUATION);
 s=initial(0x1000u,0xF42u,0x2000u);
 for(i=0;i<3u;++i)assert(pr_slot_a_cbf_pointer_store_step(&s,&r)==PR_CBF_STORE_STEPPED);
 assert(!memcmp(ram+0x2100Au,"\xCD\xAB\x01\xEF",4));
 assert(s.cpu.es==0x2000u && s.cpu.di==0xF42u); /* Writes alias original pointer, no reread. */
 s=initial(0xFFF6u,0xFF38u,0x3000u);
 for(i=0;i<3u;++i)assert(pr_slot_a_cbf_pointer_store_step(&s,&r)==PR_CBF_STORE_STEPPED);
 assert(!memcmp(ram+0x30000u,"\xCD\xAB\x01\xEF",4));
 s=initial(0xFFF4u,0x200u,0x3000u);blocked(&s,PR_CBF_STORE_BLOCKED_MEMORY);
 s=initial(0x1000u,0xFF37u,0x3000u);
 assert(pr_slot_a_cbf_pointer_store_step(&s,&r)==PR_CBF_STORE_STEPPED);blocked(&s,PR_CBF_STORE_BLOCKED_MEMORY);
 s=initial(0x1000u,0xFF35u,0x3000u);
 for(i=0;i<2u;++i)assert(pr_slot_a_cbf_pointer_store_step(&s,&r)==PR_CBF_STORE_STEPPED);
 assert(ram[0x3FFFDu]==0xCDu && ram[0x3FFFEu]==0xABu);blocked(&s,PR_CBF_STORE_BLOCKED_MEMORY);
 s=initial(0x1000u,0x200u,0x3000u);s.ram_size=0x2100Du;blocked(&s,PR_CBF_STORE_BLOCKED_MEMORY);
 s=initial(0x1000u,0x200u,0x3000u);s.cpu.ss=0xFFFFu;blocked(&s,PR_CBF_STORE_BLOCKED_MEMORY);
 s=initial(0x1000u,0x200u,0x3000u);s.ram=NULL;blocked(&s,PR_CBF_STORE_BLOCKED_MEMORY);
 s=initial(0x1000u,0x200u,0x3000u);s.ram_size=sizeof ram+1u;blocked(&s,PR_CBF_STORE_BLOCKED_MEMORY);
 s=initial(0x1000u,0x200u,0x3000u);s.cpu.cs=0x131Cu;blocked(&s,PR_CBF_STORE_BLOCKED_PC);
 s=initial(0x1000u,0x200u,0x3000u);s.cpu.ip=0xCCFu;blocked(&s,PR_CBF_STORE_BLOCKED_PC);
 r.valid=9;assert(pr_slot_a_cbf_pointer_store_step(NULL,&r)==PR_CBF_STORE_ARGUMENT && r.valid==9u);
 assert(pr_slot_a_cbf_pointer_store_step(&s,NULL)==PR_CBF_STORE_ARGUMENT);
 puts("CBF full pointer stores, source alias, wrapped offsets and atomic blockers passed");return 0;
}
