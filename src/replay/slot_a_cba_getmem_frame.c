#include "poolrad/replay/slot_a_cba_getmem_frame.h"
pr_system_getmem_status pr_slot_a_cba_getmem_step(
 pr_slot_a_directory_state *s,pr_system_getmem_report *r)
{
 pr_slot_a_directory_cpu *c;uint16_t sp;size_t a;
 if(!s || !r)return PR_SYSTEM_GETMEM_ARGUMENT;
 *r=(pr_system_getmem_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
 c=&s->cpu;
 if(c->cs==0x2A8Eu){
  if(c->ip==0xCBFu)return PR_SYSTEM_GETMEM_BLOCKED_CALLER;
  if(c->ip!=0xCBAu)return PR_SYSTEM_GETMEM_BLOCKED_PC;
  sp=(uint16_t)(c->sp-4u);a=((size_t)c->ss<<4)+sp;
  if((size_t)sp+4u>0x10000u || a>s->ram_size || 4u>s->ram_size-a)
   return PR_SYSTEM_GETMEM_BLOCKED_MEMORY;
  s->ram[a]=0xBFu;s->ram[a+1u]=0x0Cu;
  s->ram[a+2u]=(uint8_t)c->cs;s->ram[a+3u]=(uint8_t)(c->cs>>8);
  c->sp=sp;c->cs=0x131Cu;c->ip=0x23Fu;return PR_SYSTEM_GETMEM_STEPPED;
 }
 if(c->cs!=0x131Cu)return PR_SYSTEM_GETMEM_BLOCKED_PC;
 switch(c->ip){
 case 0x23Fu:case 0x240u:case 0x242u:case 0x245u:
 case 0x248u:case 0x249u:case 0x24Bu:case 0x24Eu:case 0x251u:case 0xE2u:
  return pr_system_getmem_23f_step(s,r);
 default:return PR_SYSTEM_GETMEM_BLOCKED_PC;
 }
}
