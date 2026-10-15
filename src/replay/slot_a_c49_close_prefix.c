#include "poolrad/replay/slot_a_c49_close_prefix.h"
static uint8_t *span(pr_slot_a_directory_state *s,uint16_t segment,uint16_t offset,size_t size)
{
 size_t address=((size_t)segment<<4)+offset;
 if(size>0x10000u-(size_t)offset || address>s->ram_size || size>s->ram_size-address)return NULL;
 return s->ram+address;
}
static uint16_t load16(const uint8_t *p)
{return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8));}
static void store16(uint8_t *p,uint16_t value)
{p[0]=(uint8_t)value;p[1]=(uint8_t)(value>>8);}
static uint16_t compare(uint16_t prior,unsigned lhs,unsigned rhs)
{
 unsigned difference=(lhs-rhs)&65535u,parity=difference&255u;
 uint16_t flags=(uint16_t)(prior&(uint16_t)~0x8D5u);
 parity^=parity>>4;parity^=parity>>2;parity^=parity>>1;
 if(!(parity&1u))flags|=4u;
 if(!difference)flags|=0x40u;
 if(difference&0x8000u)flags|=0x80u;
 if(lhs<rhs)flags|=1u;
 if((lhs^rhs^difference)&0x10u)flags|=0x10u;
 if((lhs^rhs)&(lhs^difference)&0x8000u)flags|=0x800u;
 return flags;
}
pr_c49_close_status pr_slot_a_c49_close_step(
 pr_slot_a_directory_state *s,pr_c49_close_report *r)
{
 pr_slot_a_directory_cpu *c;uint8_t *p;uint16_t offset,value,next;
 if(!s || !r)return PR_C49_CLOSE_ARGUMENT;
 *r=(pr_c49_close_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_C49_CLOSE_BLOCKED_MEMORY;
 c=&s->cpu;
 if(c->cs==0x2A8Eu){
  switch(c->ip){
  case 0xC49u:c->di=(uint16_t)(c->bp-0x180u);c->ip=0xC4Du;return PR_C49_CLOSE_STEPPED;
  case 0xC4Du:value=c->ss;next=0xC4Eu;goto push;
  case 0xC4Eu:value=c->di;next=0xC4Fu;goto push;
  case 0xC4Fu:
   offset=(uint16_t)(c->sp-4u);p=span(s,c->ss,offset,4u);if(!p)return PR_C49_CLOSE_BLOCKED_MEMORY;
   store16(p,0xC54u);store16(p+2u,c->cs);c->sp=offset;c->cs=0x131Cu;c->ip=0x1491u;
   return PR_C49_CLOSE_STEPPED;
  default:return PR_C49_CLOSE_BLOCKED_PC;
  }
 }
 if(c->cs!=0x131Cu)return PR_C49_CLOSE_BLOCKED_PC;
 switch(c->ip){
 case 0x1491u:c->bx=c->sp;c->ip=0x1493u;break;
 case 0x1493u:
  p=span(s,c->ss,(uint16_t)(c->bx+4u),4u);if(!p)return PR_C49_CLOSE_BLOCKED_MEMORY;
  c->di=load16(p);c->es=load16(p+2u);c->ip=0x1497u;break;
 case 0x1497u:value=0x149Au;next=0x14B6u;goto push;
 case 0x14B6u:
  p=span(s,c->es,(uint16_t)(c->di+2u),2u);if(!p)return PR_C49_CLOSE_BLOCKED_MEMORY;
  c->flags=compare(c->flags,load16(p),0xD7B3u);c->ip=0x14BCu;break;
 case 0x14BCu:c->ip=c->flags&0x40u?0x14C4u:0x14BEu;break;
 case 0x14C4u:
  p=span(s,c->ss,c->sp,2u);if(!p)return PR_C49_CLOSE_BLOCKED_MEMORY;
  value=load16(p);c->sp=(uint16_t)(c->sp+2u);c->ip=value;break;
 case 0x149Au:c->ip=c->flags&0x40u?0x149Cu:0x14B3u;break;
 case 0x149Cu:
  p=span(s,c->es,c->di,2u);if(!p)return PR_C49_CLOSE_BLOCKED_MEMORY;
  c->bx=load16(p);c->ip=0x149Fu;break;
 case 0x149Fu:c->flags=compare(c->flags,c->bx,4u);c->ip=0x14A2u;break;
 case 0x14A2u:c->ip=c->flags&0x41u?0x14ADu:0x14A4u;break;
 case 0x14A4u:c->ax=(uint16_t)((c->ax&255u)|0x3E00u);c->ip=0x14A6u;break;
 case 0x14A6u:
  *r=(pr_c49_close_report){1u,0xAF8u,0x14A6u,0x14A8u,c->ax,c->bx};return PR_C49_CLOSE_BLOCKED_DOS;
 case 0x14ADu:return PR_C49_CLOSE_BLOCKED_RESERVED_TAIL;
 case 0x14BEu:return PR_C49_CLOSE_BLOCKED_SIGNATURE_ERROR;
 case 0x14B3u:return PR_C49_CLOSE_BLOCKED_RETURN_TAIL;
 default:return PR_C49_CLOSE_BLOCKED_PC;
 }
 return PR_C49_CLOSE_STEPPED;
push:
 offset=(uint16_t)(c->sp-2u);p=span(s,c->ss,offset,2u);if(!p)return PR_C49_CLOSE_BLOCKED_MEMORY;
 store16(p,value);c->sp=offset;c->ip=next;return PR_C49_CLOSE_STEPPED;
}
