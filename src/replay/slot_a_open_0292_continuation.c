#include "poolrad/replay/slot_a_open_0292_continuation.h"
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
static uint16_t cmp_zero(uint16_t old,unsigned v)
{
 unsigned p=v;uint16_t f=(uint16_t)(old&(uint16_t)~0x8D5u);
 p^=p>>4;p^=p>>2;p^=p>>1;
 if(!(p&1u))f|=4u;
 if(!v)f|=0x40u;
 if(v&0x80u)f|=0x80u;
 return f;
}
pr_slot_a_open_0292_status pr_slot_a_open_0292_step(
 pr_slot_a_directory_state *s,pr_slot_a_open_0292_report *r)
{
 pr_slot_a_directory_cpu *c;uint8_t *p;uint16_t off,value,next,ret;
 if(!s || !r)return PR_SLOT_A_OPEN_0292_ARGUMENT;
 *r=(pr_slot_a_open_0292_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY;
 c=&s->cpu;
 if(c->cs==0x2A8Eu && c->ip==0xC2Bu)return PR_SLOT_A_OPEN_0292_BLOCKED_CALLER;
 if(c->cs!=0x1026u)return PR_SLOT_A_OPEN_0292_BLOCKED_PC;
 next=(uint16_t)(c->ip+1u);
 switch(c->ip){
 case 0x289u:return PR_SLOT_A_OPEN_0292_BLOCKED_RETRY;
 case 0x292u:case 0x2DFu:
  off=(uint16_t)(c->bp-(c->ip==0x292u?0x54u:1u));p=span(s,c->ss,off,1u);
  if(!p)return PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY;
  *p=(uint8_t)c->ax;c->ip=(uint16_t)(c->ip+3u);break;
 case 0x295u:case 0x2ADu:case 0x2B9u:case 0x29Bu:case 0x2B3u:
  off=(uint16_t)(c->bp+((c->ip==0x29Bu || c->ip==0x2B3u)?0xAu:0xFFACu));
  p=span(s,c->ss,off,1u);if(!p)return PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY;
  c->flags=cmp_zero(c->flags,*p);c->ip=(uint16_t)(c->ip+4u);break;
 case 0x299u:c->ip=c->flags&0x40u?0x29Bu:0x2ADu;break;
 case 0x29Fu:c->ip=c->flags&0x40u?0x2A1u:0x2ADu;break;
 case 0x2B1u:c->ip=c->flags&0x40u?0x2B3u:0x2B9u;break;
 case 0x2B7u:c->ip=c->flags&0x40u?0x289u:0x2B9u;break;
 case 0x2BDu:c->ip=c->flags&0x40u?0x2DCu:0x2BFu;break;
 case 0x2A1u:c->ax=(uint16_t)((c->ax&0xFF00u)|0xEu);c->ip=0x2A3u;break;
 case 0x2A3u:case 0x2D6u:value=c->ax;goto push;
 case 0x2A4u:case 0x2C4u:c->di=(uint16_t)(c->bp-(c->ip==0x2A4u?0x53u:0x2Au));c->ip=(uint16_t)(c->ip+3u);break;
 case 0x2A7u:case 0x2C7u:value=c->ss;goto push;
 case 0x2A8u:case 0x2C3u:case 0x2C8u:case 0x2D2u:value=c->di;goto push;
 case 0x2A9u:value=c->cs;goto push;
 case 0x2AAu:
  *r=(pr_slot_a_open_0292_report){1u,0x802u,0x2AAu,0x802u,0x193u,0x2ADu,0u};
  return PR_SLOT_A_OPEN_0292_BLOCKED_MESSAGE;
 case 0x2BFu:case 0x2CEu:
  p=span(s,c->ss,(uint16_t)(c->bp+6u),4u);if(!p)return PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY;
  c->di=word(p);c->es=word(p+2u);c->ip=(uint16_t)(c->ip+3u);break;
 case 0x2C2u:case 0x2D1u:value=c->es;goto push;
 case 0x2C9u:case 0x2D7u:
  *r=(pr_slot_a_open_0292_report){1u,0x802u,c->ip,0xAF8u,(uint16_t)(c->ip==0x2C9u?0x13E2u:0x1410u),(uint16_t)(c->ip+5u),0u};
  return c->ip==0x2C9u?PR_SLOT_A_OPEN_0292_BLOCKED_ASSIGN:PR_SLOT_A_OPEN_0292_BLOCKED_RESET;
 case 0x2D3u:c->ax=1u;c->ip=0x2D6u;break;
 case 0x2DCu:case 0x2E2u:
  off=(uint16_t)(c->bp-(c->ip==0x2DCu?0x54u:1u));p=span(s,c->ss,off,1u);
  if(!p)return PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY;
  c->ax=(uint16_t)((c->ax&0xFF00u)|*p);c->ip=(uint16_t)(c->ip+3u);break;
 case 0x2E5u:c->sp=c->bp;c->ip=0x2E7u;break;
 case 0x2E7u:
  p=span(s,c->ss,c->sp,2u);if(!p)return PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY;
  value=word(p);c->sp=(uint16_t)(c->sp+2u);c->bp=value;c->ip=0x2E8u;break;
 case 0x2E8u:
  p=span(s,c->ss,c->sp,4u);if(!p)return PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY;
  ret=word(p);value=word(p+2u);c->sp=(uint16_t)(c->sp+18u);c->ip=ret;c->cs=value;break;
 default:return PR_SLOT_A_OPEN_0292_BLOCKED_PC;
 }
 return PR_SLOT_A_OPEN_0292_STEPPED;
push:
 off=(uint16_t)(c->sp-2u);p=span(s,c->ss,off,2u);if(!p)return PR_SLOT_A_OPEN_0292_BLOCKED_MEMORY;
 put(p,value);c->sp=off;c->ip=next;return PR_SLOT_A_OPEN_0292_STEPPED;
}
