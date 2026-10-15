#include "poolrad/replay/slot_a_cbf_pointer_store.h"
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
pr_cbf_store_status pr_slot_a_cbf_pointer_store_step(
 pr_slot_a_directory_state *s,pr_cbf_store_report *r)
{
 pr_slot_a_directory_cpu *c;uint8_t *p;uint16_t di,es;
 if(!s || !r)return PR_CBF_STORE_ARGUMENT;
 *r=(pr_cbf_store_report){0};
 if(!s->ram || s->ram_size>0x100000u)return PR_CBF_STORE_BLOCKED_MEMORY;
 c=&s->cpu;if(c->cs!=0x2A8Eu)return PR_CBF_STORE_BLOCKED_PC;
 switch(c->ip){
 case 0xCBFu:
  p=span(s,c->ss,(uint16_t)(c->bp+0xAu),4u);if(!p)return PR_CBF_STORE_BLOCKED_MEMORY;
  di=word(p);es=word(p+2u);c->di=di;c->es=es;c->ip=0xCC2u;break;
 case 0xCC2u:
  p=span(s,c->es,(uint16_t)(c->di+0xC8u),2u);if(!p)return PR_CBF_STORE_BLOCKED_MEMORY;
  put(p,c->ax);c->ip=0xCC7u;break;
 case 0xCC7u:
  p=span(s,c->es,(uint16_t)(c->di+0xCAu),2u);if(!p)return PR_CBF_STORE_BLOCKED_MEMORY;
  put(p,c->dx);c->ip=0xCCCu;break;
 case 0xCCCu:
  *r=(pr_cbf_store_report){1u,c->cs,c->ip};return PR_CBF_STORE_BLOCKED_CONTINUATION;
 default:return PR_CBF_STORE_BLOCKED_PC;
 }
 return PR_CBF_STORE_STEPPED;
}
