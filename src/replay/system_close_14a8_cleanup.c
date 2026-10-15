#include "poolrad/replay/system_close_14a8_cleanup.h"
static uint8_t *span(pr_slot_a_directory_state *s,uint16_t seg,uint16_t off,size_t n)
{
 size_t a=((size_t)seg<<4)+off;
 if((size_t)off+n>0x10000u || a>s->ram_size || n>s->ram_size-a)return NULL;
 return s->ram+a;
}
static uint16_t word(const uint8_t *p)
{return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));}
static void put(uint8_t *p,uint16_t v)
{p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8);}
pr_system_close_cleanup_status pr_system_close_14a8_step(
 pr_slot_a_directory_state *s,pr_system_close_cleanup_report *r)
{
 pr_slot_a_directory_cpu *c;uint8_t *p;uint16_t ip,cs;
 if(!s || !r)return PR_SYSTEM_CLOSE_CLEANUP_ARGUMENT;
 *r=(pr_system_close_cleanup_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY;
 c=&s->cpu;
 if(c->cs==0x2A8Eu && c->ip==0xC54u){
  *r=(pr_system_close_cleanup_report){1u,c->cs,c->ip};
  return PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_LOADER;
 }
 if(c->cs!=0x131Cu)return PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_PC;
 switch(c->ip){
 case 0x14A8u:c->ip=c->flags&1u?0x14AAu:0x14ADu;break;
 case 0x14AAu:
  p=span(s,c->ds,0x4426u,2u);if(!p)return PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY;
  put(p,c->ax);c->ip=0x14ADu;break;
 case 0x14ADu:
  p=span(s,c->es,(uint16_t)(c->di+2u),2u);if(!p)return PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY;
  put(p,0xD7B0u);c->ip=0x14B3u;break;
 case 0x14B3u:
  p=span(s,c->ss,c->sp,4u);if(!p)return PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_MEMORY;
  ip=word(p);cs=word(p+2u);c->sp=(uint16_t)(c->sp+8u);c->ip=ip;c->cs=cs;break;
 default:return PR_SYSTEM_CLOSE_CLEANUP_BLOCKED_PC;
 }
 return PR_SYSTEM_CLOSE_CLEANUP_STEPPED;
}
