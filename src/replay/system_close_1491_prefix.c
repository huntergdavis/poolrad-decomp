#include "poolrad/replay/system_close_1491_prefix.h"
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
static uint16_t cmp_flags(uint16_t old,unsigned a,unsigned b)
{
 unsigned v=(a-b)&65535u,p=v&255u;uint16_t f=(uint16_t)(old&(uint16_t)~0x8D5u);
 p^=p>>4;p^=p>>2;p^=p>>1;
 if(!(p&1u))f|=4u;
 if(!v)f|=0x40u;
 if(v&0x8000u)f|=0x80u;
 if(a<b)f|=1u;
 if((a^b^v)&0x10u)f|=0x10u;
 if((a^b)&(a^v)&0x8000u)f|=0x800u;
 return f;
}
pr_system_close_status pr_system_close_1491_step(
 pr_slot_a_directory_state *s,pr_system_close_report *r)
{
 pr_slot_a_directory_cpu *c;uint8_t *p;uint16_t off,v;
 if(!s || !r)return PR_SYSTEM_CLOSE_ARGUMENT;
 *r=(pr_system_close_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_SYSTEM_CLOSE_BLOCKED_MEMORY;
 c=&s->cpu;if(c->cs!=0x131Cu)return PR_SYSTEM_CLOSE_BLOCKED_PC;
 switch(c->ip){
 case 0x1491u:c->bx=c->sp;c->ip=0x1493u;break;
 case 0x1493u:
  p=span(s,c->ss,(uint16_t)(c->bx+4u),4u);if(!p)return PR_SYSTEM_CLOSE_BLOCKED_MEMORY;
  c->di=word(p);c->es=word(p+2u);c->ip=0x1497u;break;
 case 0x1497u:
  off=(uint16_t)(c->sp-2u);p=span(s,c->ss,off,2u);if(!p)return PR_SYSTEM_CLOSE_BLOCKED_MEMORY;
  put(p,0x149Au);c->sp=off;c->ip=0x14B6u;break;
 case 0x14B6u:
  p=span(s,c->es,(uint16_t)(c->di+2u),2u);if(!p)return PR_SYSTEM_CLOSE_BLOCKED_MEMORY;
  c->flags=cmp_flags(c->flags,word(p),0xD7B3u);c->ip=0x14BCu;break;
 case 0x14BCu:c->ip=c->flags&0x40u?0x14C4u:0x14BEu;break;
 case 0x14C4u:
  p=span(s,c->ss,c->sp,2u);if(!p)return PR_SYSTEM_CLOSE_BLOCKED_MEMORY;
  v=word(p);c->sp=(uint16_t)(c->sp+2u);c->ip=v;break;
 case 0x149Au:c->ip=c->flags&0x40u?0x149Cu:0x14B3u;break;
 case 0x149Cu:
  p=span(s,c->es,c->di,2u);if(!p)return PR_SYSTEM_CLOSE_BLOCKED_MEMORY;
  c->bx=word(p);c->ip=0x149Fu;break;
 case 0x149Fu:c->flags=cmp_flags(c->flags,c->bx,4u);c->ip=0x14A2u;break;
 case 0x14A2u:c->ip=c->flags&0x41u?0x14ADu:0x14A4u;break;
 case 0x14A4u:c->ax=(uint16_t)((c->ax&0xFFu)|0x3E00u);c->ip=0x14A6u;break;
 case 0x14A6u:
  *r=(pr_system_close_report){1u,0xAF8u,0x14A6u,0x14A8u,c->ax,c->bx};return PR_SYSTEM_CLOSE_BLOCKED_DOS;
 case 0x14ADu:return PR_SYSTEM_CLOSE_BLOCKED_RESERVED_TAIL;
 case 0x14BEu:return PR_SYSTEM_CLOSE_BLOCKED_SIGNATURE_ERROR;
 case 0x14B3u:return PR_SYSTEM_CLOSE_BLOCKED_RETURN_TAIL;
 default:return PR_SYSTEM_CLOSE_BLOCKED_PC;
 }
 return PR_SYSTEM_CLOSE_STEPPED;
}
