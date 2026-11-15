#include "poolrad/replay/slot_a_loadword_classifier_call.h"
#include <string.h>
static size_t phys(uint16_t s,uint16_t o){return (((size_t)s<<4)+o)&0xfffffu;}
static void push(uint8_t *r,pr_u1c_3171_cpu *c,uint16_t v){size_t p;c->sp=(uint16_t)(c->sp-2u);p=phys(c->ss,c->sp);r[p]=(uint8_t)v;r[(p+1u)&0xfffffu]=(uint8_t)(v>>8);}
int pr_slot_a_loadword_classifier_call(uint8_t *r,size_t n,const uint8_t *code,pr_u1c_3171_cpu *c)
{
    size_t p,a;uint16_t left,result,flags,rel,arg;uint8_t parity;unsigned i,j;
    if(!r||n!=0x100000u||!code||!c||c->ip!=0x0e0c||c->sp<14u||c->sp>0xfffau)return 1;
    p=phys(c->cs,0x0e0c);if(p>n-13u||memcmp(r+p,code,13u)||code[9]!=0x0e||code[10]!=0xe8)return 1;
    rel=(uint16_t)(code[11]|((uint16_t)code[12]<<8));if((uint16_t)(0x0e19u+rel)!=0x06fb)return 1;
    for(i=0;i<4;i++){a=phys(c->ss,(uint16_t)(c->sp-(i==0?2u:8u+2u*i)));for(j=0;j<13;j++)if(a==p+j||((a+1u)&0xfffffu)==p+j)return 1;}
    push(r,c,c->bp);c->bp=c->sp;left=c->sp;result=(uint16_t)(left-6u);
    parity=(uint8_t)result;parity^=(uint8_t)(parity>>4);parity^=(uint8_t)(parity>>2);parity^=(uint8_t)(parity>>1);
    flags=(uint16_t)((left<6u?1u:0u)|((parity&1u)==0u?4u:0u)|(((left^6u^result)&0x10u)?0x10u:0u)|(result==0?0x40u:0u)|(result&0x8000u?0x80u:0u)|(((left^6u)&(left^result)&0x8000u)?0x800u:0u));
    c->flags=(uint16_t)((c->flags&(uint16_t)~0x08d5u)|flags);c->sp=result;a=phys(c->ss,(uint16_t)(c->bp+6u));arg=(uint16_t)(r[a]|((uint16_t)r[(a+1u)&0xfffffu]<<8));push(r,c,arg);push(r,c,c->cs);push(r,c,0x0e19);c->ip=0x06fb;return 0;
}
