#include "poolrad/replay/system_reset_1410_abi.h"
static uint8_t *span(pr_slot_a_directory_state *s,uint16_t seg,uint16_t off,size_t n)
{
    size_t a=((size_t)seg<<4)+off;
    if ((size_t)off+n>0x10000u || a>s->ram_size || n>s->ram_size-a) return NULL;
    return s->ram+a;
}
static uint16_t word(const uint8_t *p)
{ return (uint16_t)((unsigned)p[0]|((unsigned)p[1]<<8)); }
static void put(uint8_t *p,uint16_t v)
{ p[0]=(uint8_t)v;p[1]=(uint8_t)(v>>8); }
static uint16_t cmp_flags(uint16_t old,unsigned a,unsigned b,unsigned bits)
{
    unsigned mask=(1u<<bits)-1u,sign=1u<<(bits-1u),v=(a-b)&mask,p=v&255u;
    uint16_t f=(uint16_t)(old&(uint16_t)~0x8D5u);
    p^=p>>4;p^=p>>2;p^=p>>1;
    if (!(p&1u)) f|=4u;
    if (!v) f|=0x40u;
    if (v&sign) f|=0x80u;
    if (a<b) f|=1u;
    if ((a^b^v)&0x10u) f|=0x10u;
    if ((a^b)&(a^v)&sign) f|=0x800u;
    return f;
}
pr_system_reset_status pr_system_reset_1410_step(
    pr_slot_a_directory_state *s,pr_system_reset_report *r)
{
    pr_slot_a_directory_cpu *c;uint8_t *p;uint16_t v,next,off,ret;
    if (!s || !r) return PR_SYSTEM_RESET_ARGUMENT;
    *r=(pr_system_reset_report){0};
    if (!s->ram || s->ram_size>0x100000u) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
    c=&s->cpu;
    if (c->cs==0x1026u && c->ip==0x324u) return PR_SYSTEM_RESET_BLOCKED_CALLER;
    if (c->cs==0x1026u && c->ip==0x31Fu) {
        off=(uint16_t)(c->sp-4u);p=span(s,c->ss,off,4u);
        if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
        put(p,0x324u);put(p+2u,c->cs);c->sp=off;c->cs=0x131Cu;c->ip=0x1410u;
        return PR_SYSTEM_RESET_STEPPED;
    }
    if (c->cs!=0x131Cu) return PR_SYSTEM_RESET_BLOCKED_PC;
    next=(uint16_t)(c->ip+1u);
    switch (c->ip) {
    case 0x1410u:
        p=span(s,c->ds,0x442Cu,1u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
        c->ax=(uint16_t)((c->ax&0xFF00u)|*p);c->ip=0x1413u;break;
    case 0x1413u:c->ax=(uint16_t)((c->ax&0xFFu)|0x3D00u);c->ip=0x1415u;break;
    case 0x1415u:case 0x1454u:
        if (c->ip==0x1415u) c->dx=0u;else c->cx=0u;
        c->flags=(uint16_t)((c->flags&(uint16_t)~0x8D5u)|0x44u);c->ip=(uint16_t)(c->ip+2u);break;
    case 0x1417u:c->ip=0x141Fu;break;
    case 0x141Fu:v=c->bp;goto push;
    case 0x1420u:c->bp=c->sp;c->ip=0x1422u;break;
    case 0x1422u:
        p=span(s,c->ss,(uint16_t)(c->bp+8u),4u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
        c->di=word(p);c->es=word(p+2u);c->ip=0x1425u;break;
    case 0x1425u:case 0x142Du:
        p=span(s,c->es,(uint16_t)(c->di+2u),2u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
        c->flags=cmp_flags(c->flags,word(p),c->ip==0x1425u?0xD7B0u:0xD7B3u,16u);
        c->ip=(uint16_t)(c->ip+6u);break;
    case 0x142Bu:c->ip=c->flags&0x40u?0x1447u:0x142Du;break;
    case 0x1433u:c->ip=c->flags&0x40u?0x143Du:0x1435u;break;
    case 0x1435u:
        p=span(s,c->ds,0x4426u,2u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
        put(p,0x66u);c->ip=0x143Bu;break;
    case 0x143Bu:case 0x145Eu:c->ip=0x1471u;break;
    case 0x143Du:v=c->ax;goto push;
    case 0x143Eu:v=c->dx;goto push;
    case 0x143Fu:case 0x1452u:v=c->es;goto push;
    case 0x1440u:v=c->di;goto push;
    case 0x1441u:v=c->cs;goto push;
    case 0x1442u:
        *r=(pr_system_reset_report){1u,0xAF8u,0x1442u,0x1491u,0x1445u,c->ax,c->ds,c->dx};
        return PR_SYSTEM_RESET_BLOCKED_CLOSE;
    case 0x1453u:case 0x1458u:case 0x1471u:
        p=span(s,c->ss,c->sp,2u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;v=word(p);
        if(c->ip==0x1471u)c->bp=v;else c->ds=v;
        c->sp=(uint16_t)(c->sp+2u);c->ip=next;break;
    case 0x1447u:
        p=span(s,c->es,(uint16_t)(c->di+0x30u),1u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
        c->flags=cmp_flags(c->flags,*p,0u,8u);c->ip=0x144Cu;break;
    case 0x144Cu:c->ip=c->flags&0x40u?0x1460u:0x144Eu;break;
    case 0x144Eu:v=c->ds;goto push;
    case 0x144Fu:c->dx=(uint16_t)(c->di+0x30u);c->ip=0x1452u;break;
    case 0x1456u:
        *r=(pr_system_reset_report){1u,0xAF8u,0x1456u,0u,0x1458u,c->ax,c->ds,c->dx};
        return PR_SYSTEM_RESET_BLOCKED_DOS;
    case 0x1459u:c->ip=c->flags&1u?0x145Bu:0x1461u;break;
    case 0x145Bu:
        p=span(s,c->ds,0x4426u,2u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
        put(p,c->ax);c->ip=0x145Eu;break;
    case 0x1460u:v=c->ax;c->ax=c->dx;c->dx=v;c->ip=0x1461u;break;
    case 0x1461u:case 0x1467u:case 0x146Du:
        off=(uint16_t)(c->di+(c->ip==0x1461u?2u:(c->ip==0x146Du?4u:0u)));
        p=span(s,c->es,off,2u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
        put(p,c->ip==0x1461u?0xD7B3u:c->ax);
        c->ip=c->ip==0x1461u?0x1467u:(c->ip==0x1467u?0x146Au:0x1471u);break;
    case 0x146Au:
        p=span(s,c->ss,(uint16_t)(c->bp+6u),2u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
        c->ax=word(p);c->ip=0x146Du;break;
    case 0x1472u:
        p=span(s,c->ss,c->sp,4u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
        ret=word(p);v=word(p+2u);c->sp=(uint16_t)(c->sp+10u);c->ip=ret;c->cs=v;break;
    default:return PR_SYSTEM_RESET_BLOCKED_PC;
    }
    return PR_SYSTEM_RESET_STEPPED;
push:
    off=(uint16_t)(c->sp-2u);p=span(s,c->ss,off,2u);if (!p) return PR_SYSTEM_RESET_BLOCKED_MEMORY;
    put(p,v);c->sp=off;c->ip=next;return PR_SYSTEM_RESET_STEPPED;
}
