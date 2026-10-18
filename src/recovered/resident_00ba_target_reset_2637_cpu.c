#include "poolrad/recovered/resident_00ba_target_reset_2637_cpu.h"
#define MASK (PR_RES_2637_RAM_SIZE-1u)
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static void les(const uint8_t *r,pr_res_2637_cpu *c,uint16_t s,uint16_t o) {
    size_t p=physical(s,o);c->di=word(r,p);c->es=word(r,(p+2)&MASK);
}
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
pr_res_2637_status pr_res_00ba_target_reset_2637_cpu(uint8_t *r,size_t size,pr_res_2637_cpu *c)
{
    static const unsigned fields[]={3,0,7,6};size_t p;uint16_t before,result;
    if(r==NULL || c==NULL || size!=PR_RES_2637_RAM_SIZE || c->ip!=0x2637 || c->sp<4 || c->sp>0xFFF8)return PR_RES_2637_ARGUMENT;
    c->sp=(uint16_t)(c->sp-2u);p=physical(c->ss,c->sp);r[p]=(uint8_t)c->bp;r[(p+1)&MASK]=(uint8_t)(c->bp>>8);c->bp=c->sp;
    before=c->sp;result=(uint16_t)(before-2u);c->flags&=(uint16_t)~0x08D5u;
    if(before<2)c->flags|=1;
    if(parity((uint8_t)result))c->flags|=4;
    if((before^2u^result)&0x10)c->flags|=0x10;
    if(result==0)c->flags|=0x40;
    if(result&0x8000)c->flags|=0x80;
    if((before^2u)&(before^result)&0x8000)c->flags|=0x800;
    c->sp=result;
    for(unsigned i=0;i<4;++i){
        les(r,c,c->ss,(uint16_t)(c->bp+6));les(r,c,c->es,(uint16_t)(c->di+0x108));
        r[physical(c->es,(uint16_t)(c->di+fields[i]))]=0;
    }
    p=physical(c->ss,(uint16_t)(c->bp-1));r[p]=1;c->ax=(uint16_t)((c->ax&0xFF00u)|r[p]);
    c->sp=c->bp;c->bp=word(r,physical(c->ss,c->sp));c->sp=(uint16_t)(c->sp+2);
    p=physical(c->ss,c->sp);c->ip=word(r,p);c->cs=word(r,(p+2)&MASK);c->sp=(uint16_t)(c->sp+8);
    return PR_RES_2637_OK;
}
