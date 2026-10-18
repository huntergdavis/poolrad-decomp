/* 0709 +00B7 raw rectangle composition over pr_bus/pr_ega.
 * See docs/re/RES_0709_RECT_FILL_00B7_CPU.md. */
#include "poolrad/recovered/resident_0709_rect_fill_00b7_cpu.h"
#include "poolrad/recovered/resident_0709_page_select_00a0_cpu.h"
#include "poolrad/recovered/resident_0709_ega_reset_0000_cpu.h"
#include "poolrad/recovered/system_fillchar_bus_16a8_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4u) + off) & MASK;
}
static uint16_t word_at(pr_bus *ram, size_t at)
{
    return (uint16_t)(pr_bus_read8(ram,at) | ((uint16_t)pr_bus_read8(ram,(at+1u)&MASK) << 8u));
}
static uint16_t read_word(pr_bus *ram, uint16_t seg, uint16_t off)
{
    return word_at(ram,physical(seg,off));
}
static void put_word(pr_bus *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at=physical(seg,off);
    pr_bus_write8(ram,at,(uint8_t)value);pr_bus_write8(ram,(at+1u)&MASK,(uint8_t)(value >> 8u));
}
static void push(pr_bus *ram, pr_r0709_rect_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);put_word(ram,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *ram, pr_r0709_rect_cpu *c)
{
    uint16_t value=read_word(ram,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);return value;
}
static void al(pr_r0709_rect_cpu *c, uint8_t value)
{
    c->ax=(uint16_t)((c->ax&0xFF00u)|value);
}
static void result_flags(pr_r0709_rect_cpu *c, uint16_t result, uint16_t sign)
{
    uint8_t parity=(uint8_t)result;
    parity ^= (uint8_t)(parity >> 4u);parity ^= (uint8_t)(parity >> 2u);
    parity ^= (uint8_t)(parity >> 1u);
    if ((parity&1u)==0u)c->flags|=4u;
    if (result==0u)c->flags|=0x40u;
    if (result&sign)c->flags|=0x80u;
}
static uint16_t arithmetic(pr_r0709_rect_cpu *c, uint16_t left, uint16_t right,
                           unsigned carry, unsigned bits, unsigned subtract)
{
    uint32_t mask=bits==8u ? 255u : 65535u;
    uint16_t sign=bits==8u ? 0x80u : 0x8000u;
    uint32_t wide=subtract ? (uint32_t)left-right-carry : (uint32_t)left+right+carry;
    uint16_t result=(uint16_t)(wide&mask);
    c->flags&=(uint16_t)~0x08D5u;
    if (subtract ? (uint32_t)left<(uint32_t)right+carry : wide>mask)c->flags|=1u;
    if ((left^right^result)&0x10u)c->flags|=0x10u;
    if ((subtract ? (left^right)&(left^result) : (uint16_t)~(left^right)&(left^result))&sign)
        c->flags|=0x800u;
    result_flags(c,result,sign);return result;
}

static void logic_zero(pr_r0709_rect_cpu *c) {
    c->flags=(uint16_t)((c->flags&~0x08D5u)|0x44u);
}
static uint16_t incdec(pr_r0709_rect_cpu *c,uint16_t v,unsigned sub) {
    uint16_t cf=c->flags&1u;v=arithmetic(c,v,1u,0u,16u,sub);
    c->flags=(uint16_t)((c->flags&~1u)|cf);return v;
}
static int ordinary_segment(uint16_t s) {
    size_t start=(size_t)s<<4u;
    return start>0xAFFFFu || start+0xFFFFu<0xA0000u;
}
static int bus_out(void *ctx,uint16_t p,uint16_t v,unsigned n,uint16_t cs,uint16_t ip) {
    return pr_bus_out(ctx,p,v,n,cs,ip);
}
static int dependency(pr_bus *b,pr_r0709_rect_cpu *c,int which) {
    int status;pr_port_env env={b,bus_out,NULL};
#define ADAPT(type,call) do { type inner;_Static_assert(sizeof inner==sizeof *c,"CPU layout");     memcpy(&inner,c,sizeof inner);status=(int)(call);memcpy(c,&inner,sizeof inner); } while(0)
    if(which==0) { ADAPT(pr_r0709_page_cpu,pr_r0709_page_select_00a0_cpu(b->ram,PR_BUS_RAM_SIZE,&inner)); }
    else if(which==1) { ADAPT(pr_r0709_ega_reset_cpu,pr_r0709_ega_reset_0000_cpu(b->ram,PR_BUS_RAM_SIZE,&inner,&env)); }
    else { ADAPT(pr_system_leaf_cpu,pr_system_fillchar_bus_16a8(b,&inner)); }
#undef ADAPT
    return status;
}
pr_r0709_rect_cpu_status pr_r0709_rect_fill_00b7_cpu(pr_bus *ram,size_t size,
    pr_r0709_rect_cpu *c,size_t max_loopbacks,pr_r0709_rect_cpu_diagnostic *d)
{
    pr_r0709_rect_cpu_diagnostic fallback={-1,-1};uint16_t caller;size_t loops=0;
    if(d==NULL)d=&fallback;
    *d=fallback;
    if(ram==NULL || ram->ram==NULL || ram->ega==NULL || c==NULL ||
       size!=PR_BUS_RAM_SIZE || c->ip!=0xB7u || c->sp<0x20u || c->sp>0xFFF0u ||
       !ordinary_segment(c->ss) || !ordinary_segment(c->ds))
        return PR_R0709_RECT_CPU_ARGUMENT;
    caller=c->cs;
    for(;;) {
        if(c->cs!=caller)return PR_R0709_RECT_CPU_TRANSFER;
        switch(c->ip) {
        case 0x00B7: push(ram,c,c->bp);c->ip=0x00B8u;break;
        case 0x00B8: c->bp=c->sp;c->ip=0x00BAu;break;
        case 0x00BA: c->sp=arithmetic(c,c->sp,0x000Au,0u,16u,1u);c->ip=0x00BDu;break;
        case 0x00BD: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(18970)))));c->ip=0x00C0u;break;
        case 0x00C0: pr_bus_write8(ram,physical(c->ss,(uint16_t)(c->bp+-1)),(uint8_t)((uint8_t)c->ax));c->ip=0x00C3u;break;
        case 0x00C3: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+16)))));c->ip=0x00C6u;break;
        case 0x00C6: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x00C7u;break;
        case 0x00C7: c->dx=(c->ax&0x8000u)?0xFFFFu:0;c->ip=0x00C8u;break;
        case 0x00C8: push(ram,c,c->dx);c->ip=0x00C9u;break;
        case 0x00C9: push(ram,c,c->ax);c->ip=0x00CAu;break;
        case 0x00CA: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+14)))));c->ip=0x00CDu;break;
        case 0x00CD: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x00CEu;break;
        case 0x00CE: c->dx=0x0028u;c->ip=0x00D1u;break;
        case 0x00D1: {uint32_t v=(uint32_t)c->ax*c->dx;c->ax=(uint16_t)v;c->dx=(uint16_t)(v>>16u);}c->ip=0x00D3u;break;
        case 0x00D3: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x0003u));c->ip=0x00D5u;break;
        case 0x00D5: c->ax=(uint16_t)(c->ax<<3u);c->ip=0x00D7u;break;
        case 0x00D7: c->dx=0;logic_zero(c);c->ip=0x00D9u;break;
        case 0x00D9: c->cx=pop(ram,c);c->ip=0x00DAu;break;
        case 0x00DA: c->bx=pop(ram,c);c->ip=0x00DBu;break;
        case 0x00DB: c->ax=arithmetic(c,c->ax,c->cx,0u,16u,0u);c->ip=0x00DDu;break;
        case 0x00DD: c->dx=arithmetic(c,c->dx,c->bx,c->flags&1u,16u,0u);c->ip=0x00DFu;break;
        case 0x00DF: put_word(ram,c->ss,(uint16_t)(c->bp+-3),c->ax);c->ip=0x00E2u;break;
        case 0x00E2: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+14)))));c->ip=0x00E5u;break;
        case 0x00E5: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x00E6u;break;
        case 0x00E6: c->dx=c->ax;c->ip=0x00E8u;break;
        case 0x00E8: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+10)))));c->ip=0x00EBu;break;
        case 0x00EB: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x00ECu;break;
        case 0x00EC: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,1u);c->ip=0x00EEu;break;
        case 0x00EE: c->ax=incdec(c,c->ax,0u);c->ip=0x00EFu;break;
        case 0x00EF: c->cx=(uint16_t)((c->cx&0xFF00u)|(uint8_t)(0x0003u));c->ip=0x00F1u;break;
        case 0x00F1: c->ax=(uint16_t)(c->ax<<3u);c->ip=0x00F3u;break;
        case 0x00F3: c->ax=incdec(c,c->ax,1u);c->ip=0x00F4u;break;
        case 0x00F4: put_word(ram,c->ss,(uint16_t)(c->bp+-5),c->ax);c->ip=0x00F7u;break;
        case 0x00F7: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+8)))));c->ip=0x00FAu;break;
        case 0x00FA: push(ram,c,c->ax);c->ip=0x00FBu;break;
        case 0x00FB: push(ram,c,c->cs);c->ip=0x00FCu;break;
        case 0x00FC: push(ram,c,0x00FFu);c->ip=0x00A0u;d->callee=0;d->status=dependency(ram,c,0);if(d->status!=0)return PR_R0709_RECT_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x00FFu)return PR_R0709_RECT_CPU_TRANSFER;break;
        case 0x00FF: push(ram,c,c->cs);c->ip=0x0100u;break;
        case 0x0100: push(ram,c,0x0103u);c->ip=0x0000u;d->callee=1;d->status=dependency(ram,c,1);if(d->status!=0)return PR_R0709_RECT_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0103u)return PR_R0709_RECT_CPU_TRANSFER;break;
        case 0x0103: al(c,(uint8_t)(0x0005u));c->ip=0x0105u;break;
        case 0x0105: c->dx=0x03CEu;c->ip=0x0108u;break;
        case 0x0108: if(pr_bus_out(ram,c->dx,(uint8_t)c->ax,1u,c->cs,c->ip))return PR_R0709_RECT_CPU_PORT;c->ip=0x0109u;break;
        case 0x0109: al(c,(uint8_t)(0x0002u));c->ip=0x010Bu;break;
        case 0x010B: c->dx=0x03CFu;c->ip=0x010Eu;break;
        case 0x010E: if(pr_bus_out(ram,c->dx,(uint8_t)c->ax,1u,c->cs,c->ip))return PR_R0709_RECT_CPU_PORT;c->ip=0x010Fu;break;
        case 0x010F: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-5));c->ip=0x0112u;break;
        case 0x0112: put_word(ram,c->ss,(uint16_t)(c->bp+-9),c->ax);c->ip=0x0115u;break;
        case 0x0115: c->ax=0;logic_zero(c);c->ip=0x0117u;break;
        case 0x0117: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-9)),0u,16u,1u);c->ip=0x011Au;break;
        case 0x011A: c->ip=((c->flags&0x41u)==0)?0x0151u:0x011Cu;break;
        case 0x011C: put_word(ram,c->ss,(uint16_t)(c->bp+-7),c->ax);c->ip=0x011Fu;break;
        case 0x011F: c->ip=0x0124u;break;
        case 0x0121: put_word(ram,c->ss,(uint16_t)(c->bp+-7),incdec(c,read_word(ram,c->ss,(uint16_t)(c->bp+-7)),0u));c->ip=0x0124u;break;
        case 0x0124: c->ax=read_word(ram,c->ds,(uint16_t)(18968));c->ip=0x0127u;break;
        case 0x0127: push(ram,c,c->ax);c->ip=0x0128u;break;
        case 0x0128: c->di=read_word(ram,c->ss,(uint16_t)(c->bp+-3));c->ip=0x012Bu;break;
        case 0x012B: c->es=pop(ram,c);c->ip=0x012Cu;break;
        case 0x012C: push(ram,c,c->es);c->ip=0x012Du;break;
        case 0x012D: push(ram,c,c->di);c->ip=0x012Eu;break;
        case 0x012E: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+16)))));c->ip=0x0131u;break;
        case 0x0131: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0132u;break;
        case 0x0132: c->dx=c->ax;c->ip=0x0134u;break;
        case 0x0134: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+12)))));c->ip=0x0137u;break;
        case 0x0137: c->ax=(uint16_t)((c->ax&0x80u)?0xFF00u|(c->ax&255u):c->ax&255u);c->ip=0x0138u;break;
        case 0x0138: c->ax=arithmetic(c,c->ax,c->dx,0u,16u,1u);c->ip=0x013Au;break;
        case 0x013A: c->ax=incdec(c,c->ax,0u);c->ip=0x013Bu;break;
        case 0x013B: push(ram,c,c->ax);c->ip=0x013Cu;break;
        case 0x013C: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+6)))));c->ip=0x013Fu;break;
        case 0x013F: push(ram,c,c->ax);c->ip=0x0140u;break;
        case 0x0140: if(pr_bus_read8(ram,physical(c->cs,c->ip))!=0x9A || read_word(ram,c->cs,0x141u)!=0x16A8u)return PR_R0709_RECT_CPU_CODE;push(ram,c,c->cs);push(ram,c,0x0145u);c->cs=read_word(ram,c->cs,0x143u);c->ip=0x16A8u;d->callee=2;d->status=dependency(ram,c,2);if(d->status!=0)return PR_R0709_RECT_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x0145u)return PR_R0709_RECT_CPU_TRANSFER;break;
        case 0x0145: put_word(ram,c->ss,(uint16_t)(c->bp+-3),arithmetic(c,read_word(ram,c->ss,(uint16_t)(c->bp+-3)),0x0028u,0u,16u,0u));c->ip=0x0149u;break;
        case 0x0149: c->ax=read_word(ram,c->ss,(uint16_t)(c->bp+-7));c->ip=0x014Cu;break;
        case 0x014C: (void)arithmetic(c,c->ax,read_word(ram,c->ss,(uint16_t)(c->bp+-9)),0u,16u,1u);c->ip=0x014Fu;break;
        case 0x014F: if((c->flags&0x40u)==0){if(loops==max_loopbacks)return PR_R0709_RECT_CPU_LIMIT;++loops;c->ip=0x0121u;}else c->ip=0x0151u;break;
        case 0x0151: al(c,(uint8_t)(0x0005u));c->ip=0x0153u;break;
        case 0x0153: c->dx=0x03CEu;c->ip=0x0156u;break;
        case 0x0156: if(pr_bus_out(ram,c->dx,(uint8_t)c->ax,1u,c->cs,c->ip))return PR_R0709_RECT_CPU_PORT;c->ip=0x0157u;break;
        case 0x0157: al(c,(uint8_t)(0x0000u));c->ip=0x0159u;break;
        case 0x0159: c->dx=0x03CFu;c->ip=0x015Cu;break;
        case 0x015C: if(pr_bus_out(ram,c->dx,(uint8_t)c->ax,1u,c->cs,c->ip))return PR_R0709_RECT_CPU_PORT;c->ip=0x015Du;break;
        case 0x015D: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ss,(uint16_t)(c->bp+-1)))));c->ip=0x0160u;break;
        case 0x0160: pr_bus_write8(ram,physical(c->ds,(uint16_t)(18970)),(uint8_t)((uint8_t)c->ax));c->ip=0x0163u;break;
        case 0x0163: al(c,(uint8_t)(pr_bus_read8(ram,physical(c->ds,(uint16_t)(18970)))));c->ip=0x0166u;break;
        case 0x0166: push(ram,c,c->ax);c->ip=0x0167u;break;
        case 0x0167: push(ram,c,c->cs);c->ip=0x0168u;break;
        case 0x0168: push(ram,c,0x016Bu);c->ip=0x00A0u;d->callee=0;d->status=dependency(ram,c,0);if(d->status!=0)return PR_R0709_RECT_CPU_CALLEE;if(c->cs!=caller || c->ip!=0x016Bu)return PR_R0709_RECT_CPU_TRANSFER;break;
        case 0x016B: c->sp=c->bp;c->ip=0x016Du;break;
        case 0x016D: c->bp=pop(ram,c);c->ip=0x016Eu;break;
        case 0x016E: c->ip=pop(ram,c);c->cs=pop(ram,c);c->sp=(uint16_t)(c->sp+12u);return PR_R0709_RECT_CPU_OK;
        default:return PR_R0709_RECT_CPU_TRANSFER;
        }
    }
}
