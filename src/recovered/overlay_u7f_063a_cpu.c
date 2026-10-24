/* Unit 007F entry 3: party pool distribution; docs/re/OVL_007F_063A_CPU.md. */
#include "poolrad/recovered/overlay_u7f_063a_cpu.h"
#include "poolrad/recovered/overlay_u7f_record_word_add_003a.h"
#include "poolrad/recovered/overlay_u7f_party_count_05e0.h"
#include "poolrad/recovered/system_fillchar_bus_16a8_cpu.h"
#include "poolrad/recovered/system_longdiv_0a76_cpu.h"
#include <string.h>
#define MASK (PR_BUS_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_u7f_063a_cpu cpu_t;
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *m, uint16_t s, uint16_t o) { return m[physical(s,o)]; }
static uint16_t rd16(const uint8_t *m, uint16_t s, uint16_t o)
{ size_t p=physical(s,o); return (uint16_t)(m[p] | ((uint16_t)m[(p+1u)&MASK]<<8)); }
static void wr8(uint8_t *m, uint16_t s, uint16_t o, uint8_t v) { m[physical(s,o)]=v; }
static void wr16(uint8_t *m, uint16_t s, uint16_t o, uint16_t v)
{ size_t p=physical(s,o); m[p]=(uint8_t)v; m[(p+1u)&MASK]=(uint8_t)(v>>8); }
static uint8_t brd8(pr_bus *b, uint16_t s, uint16_t o) { return pr_bus_read8(b,physical(s,o)); }
static uint16_t brd16(pr_bus *b, uint16_t s, uint16_t o)
{ size_t p=physical(s,o); uint16_t lo=pr_bus_read8(b,p); uint16_t hi=pr_bus_read8(b,(p+1u)&MASK); return (uint16_t)(lo|(hi<<8)); }
static void bwr16(pr_bus *b, uint16_t s, uint16_t o, uint16_t v)
{ size_t p=physical(s,o); pr_bus_write8(b,p,(uint8_t)v); pr_bus_write8(b,(p+1u)&MASK,(uint8_t)(v>>8)); }
static void push(uint8_t *m,cpu_t *c,uint16_t v) { c->sp=(uint16_t)(c->sp-2u); wr16(m,c->ss,c->sp,v); }
static uint16_t pop(const uint8_t *m,cpu_t *c) { uint16_t v=rd16(m,c->ss,c->sp); c->sp=(uint16_t)(c->sp+2u); return v; }
static uint16_t parity(uint8_t v) { v^=(uint8_t)(v>>4); v^=(uint8_t)(v>>2); v^=(uint8_t)(v>>1); return (v&1u)?0u:4u; }
static uint16_t szp(uint16_t v,unsigned bits)
{ return (uint16_t)(parity((uint8_t)v)|(v?0u:0x40u)|((v&(bits==8u?0x80u:0x8000u))?0x80u:0u)); }
static uint16_t arith(cpu_t *c,uint16_t a,uint16_t b,unsigned bits,int sub)
{
    uint32_t wide=sub?(uint32_t)a-b:(uint32_t)a+b;
    uint16_t mask=bits==8u?0xFFu:0xFFFFu, sign=bits==8u?0x80u:0x8000u;
    uint16_t r=(uint16_t)(wide&mask),f=szp(r,bits);
    if(sub?a<b:wide>mask)f|=1u;
    if((a^b^r)&0x10u)f|=0x10u;
    if((sub?((a^b)&(a^r)):(~(a^b)&(a^r)))&sign)f|=0x800u;
    c->flags=(uint16_t)((c->flags&~0x8D5u)|f); return r;
}
static uint16_t incdec(cpu_t *c,uint16_t a,unsigned bits,int dec)
{ uint16_t cf=c->flags&1u,r=arith(c,a,1u,bits,dec); c->flags=(uint16_t)((c->flags&~1u)|cf); return r; }
static void logic(cpu_t *c,uint16_t v,unsigned bits)
{ c->flags=(uint16_t)((c->flags&~0x8D5u)|szp(v,bits)); }
static uint16_t shl1(cpu_t *c,uint16_t v)
{
    uint16_t r=(uint16_t)(v<<1),f=(uint16_t)(szp(r,16u)|0x10u); /* pinned SHL AF=1 */
    if(v&0x8000u)f|=1u;
    if((v^r)&0x8000u)f|=0x800u;
    c->flags=(uint16_t)((c->flags&~0x8D5u)|f); return r;
}
static void set_al(cpu_t *c,uint16_t v) { c->ax=(uint16_t)((c->ax&0xFF00u)|(v&0xFFu)); }
static void set_ah(cpu_t *c,uint16_t v) { c->ax=(uint16_t)((c->ax&0x00FFu)|((v&0xFFu)<<8)); }

/* Return frames and relocations are live. Only the original call opcode and
 * fixed offset/displacement are guarded; the far segment is not hard-coded. */
static pr_u7f_063a_status invoke(pr_bus *bus,size_t size,cpu_t *c,size_t *budget,
    pr_u7f_063a_diagnostic *d,uint16_t site,uint16_t next,uint16_t target,int which,int far_call)
{
    uint8_t *ram=bus->ram; uint16_t cs=c->cs,seg=cs; int result=0;
    c->ip=site; if(!*budget)return PR_U7F_063A_LIMIT;
    d->site=site;
    if(rd8(ram,cs,site)!=(far_call?0x9Au:0xE8u) ||
       rd16(ram,cs,(uint16_t)(site+1u))!=(far_call?target:(uint16_t)(target-next)))return PR_U7F_063A_CODE;
    --*budget; d->callee=which; d->callee_status=0;
    if(far_call) { seg=rd16(ram,cs,(uint16_t)(site+3u)); push(ram,c,cs); }
    push(ram,c,next); c->cs=seg; c->ip=target;
    switch(which) {
    case 0: { pr_u7f_05e0_cpu x; COPY(&x,c); result=pr_u7f_party_count_05e0(ram,size,&x); COPY(c,&x); break; }
    case 1: { pr_system_leaf_cpu x; COPY(&x,c); result=pr_system_fillchar_bus_16a8(bus,&x); COPY(c,&x); break; }
    case 2: { pr_system_longdiv_cpu x; COPY(&x,c); result=pr_system_longdiv_0a76(ram,size,&x); COPY(c,&x); break; }
    case 3: result=pr_u7f_0054_cpu(bus,size,c,budget,&d->d0054); break;
    case 4: { pr_u7f_003a_cpu x; COPY(&x,c); result=pr_u7f_record_word_add_003a(ram,size,&x); COPY(c,&x); break; }
    case 5: result=pr_u7f_entry0_0000_cpu(ram,size,c,budget,&d->d0000); break;
    }
    d->callee_status=result;
    if((which==3 && result==PR_U7F_0054_LIMIT)||(which==5 && result==PR_U7F_0000_LIMIT))return PR_U7F_063A_LIMIT;
    if((which==3 && result==PR_U7F_0054_TRANSFER)||(which==5 && result==PR_U7F_0000_TRANSFER))return PR_U7F_063A_TRANSFER;
    if(result)return PR_U7F_063A_CALLEE;
    if(c->cs!=cs || c->ip!=next)return PR_U7F_063A_TRANSFER;
    return PR_U7F_063A_RETURNED;
}

#define STEP(ip_) do { c->ip=(ip_); if(!*budget)return PR_U7F_063A_LIMIT; --*budget; } while(0)
pr_u7f_063a_status pr_u7f_063a_run(pr_bus *bus,size_t size,cpu_t *c,size_t *budget,pr_u7f_063a_diagnostic *d)
{
    uint8_t *ram; uint16_t ret_ip,ret_cs,t; pr_u7f_063a_status s;
    if(!bus || !bus->ram || !c || !budget || !d || size!=PR_BUS_RAM_SIZE || c->ip!=0x063Au || c->sp<0x0200u || c->sp>0xFFFCu)return PR_U7F_063A_ARGUMENT;
    ram=bus->ram; ret_ip=rd16(ram,c->ss,c->sp); ret_cs=rd16(ram,c->ss,(uint16_t)(c->sp+2u));
    d->callee=-1; d->callee_status=0; d->site=0; memset(&d->d0054,0,sizeof d->d0054); memset(&d->d0000,0,sizeof d->d0000);
    STEP(0x063Au); push(ram,c,c->bp); /* 063A push bp */
    STEP(0x063Bu); c->bp=c->sp; /* 063B mov bp, sp */
    STEP(0x063Du); c->sp=arith(c,c->sp,0x002Au,16u,1); /* 063D sub sp, 0x2a */
    STEP(0x0640u); c->ax=rd16(ram,c->ds,(uint16_t)(0x5d96)); /* 0640 mov ax, word ptr [0x5d96] */
    STEP(0x0643u); c->dx=rd16(ram,c->ds,(uint16_t)(0x5d98)); /* 0643 mov dx, word ptr [0x5d98] */
    STEP(0x0647u); wr16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->ax)); /* 0647 mov word ptr [bp - 4], ax */
    STEP(0x064Au); wr16(ram,c->ss,(uint16_t)(c->bp - 2),(uint16_t)(c->dx)); /* 064A mov word ptr [bp - 2], dx */
    STEP(0x064Du); wr8(ram,c->ss,(uint16_t)(c->bp - 0xa),(uint8_t)(0x0000u)); /* 064D mov byte ptr [bp - 0xa], 0 */
    STEP(0x0651u); push(ram,c,c->cs); /* 0651 push cs */
    s=invoke(bus,size,c,budget,d,0x0652u,0x0655u,0x05E0u,0,0); /* 0652 call 0x5e0 */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x0655u); wr8(ram,c->ss,(uint16_t)(c->bp - 9),(uint8_t)((uint8_t)c->ax)); /* 0655 mov byte ptr [bp - 9], al */
    STEP(0x0658u); c->di=(uint16_t)(c->bp - 0x1a); /* 0658 lea di, [bp - 0x1a] */
    STEP(0x065Bu); push(ram,c,c->ss); /* 065B push ss */
    STEP(0x065Cu); push(ram,c,c->di); /* 065C push di */
    STEP(0x065Du); c->ax=0x000Eu; /* 065D mov ax, 0xe */
    STEP(0x0660u); push(ram,c,c->ax); /* 0660 push ax */
    STEP(0x0661u); set_al(c,0x0000u); /* 0661 mov al, 0 */
    STEP(0x0663u); push(ram,c,c->ax); /* 0663 push ax */
    s=invoke(bus,size,c,budget,d,0x0664u,0x0669u,0x16A8u,1,1); /* 0664 lcall 0xaf8, 0x16a8 */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x0669u); c->di=(uint16_t)(c->bp - 0x28); /* 0669 lea di, [bp - 0x28] */
    STEP(0x066Cu); push(ram,c,c->ss); /* 066C push ss */
    STEP(0x066Du); push(ram,c,c->di); /* 066D push di */
    STEP(0x066Eu); c->ax=0x000Eu; /* 066E mov ax, 0xe */
    STEP(0x0671u); push(ram,c,c->ax); /* 0671 push ax */
    STEP(0x0672u); set_al(c,0x0000u); /* 0672 mov al, 0 */
    STEP(0x0674u); push(ram,c,c->ax); /* 0674 push ax */
    s=invoke(bus,size,c,budget,d,0x0675u,0x067Au,0x16A8u,1,1); /* 0675 lcall 0xaf8, 0x16a8 */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x067Au); wr8(ram,c->ss,(uint16_t)(c->bp - 0x29),(uint8_t)(0x0000u)); /* 067A mov byte ptr [bp - 0x29], 0 */
    STEP(0x067Eu); goto L_0683; /* 067E jmp 0x683 */
L_0680:
    STEP(0x0680u); wr8(ram,c->ss,(uint16_t)(c->bp - 0x29),(uint8_t)(incdec(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29)),8u,0))); /* 0680 inc byte ptr [bp - 0x29] */
L_0683:
    STEP(0x0683u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0683 mov al, byte ptr [bp - 0x29] */
    STEP(0x0686u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0686 cwde  */
    STEP(0x0687u); c->di=c->ax; /* 0687 mov di, ax */
    STEP(0x0689u); c->di=shl1(c,c->di); /* 0689 shl di, 1 */
    STEP(0x068Bu); c->di=shl1(c,c->di); /* 068B shl di, 1 */
    STEP(0x068Du); (void)arith(c,rd16(ram,c->ds,(uint16_t)(c->di + 0x67f6)),0x0000u,16u,1); /* 068D cmp word ptr [di + 0x67f6], 0 */
    STEP(0x0692u); if(!(c->flags&0x40u) && (!(c->flags&0x80u)==!(c->flags&0x800u)))goto L_069D; /* 0692 jg 0x69d */
    STEP(0x0694u); if((!(c->flags&0x80u)!=!(c->flags&0x800u)))goto L_0701; /* 0694 jl 0x701 */
    STEP(0x0696u); (void)arith(c,rd16(ram,c->ds,(uint16_t)(c->di + 0x67f4)),0x0000u,16u,1); /* 0696 cmp word ptr [di + 0x67f4], 0 */
    STEP(0x069Bu); if((c->flags&0x41u))goto L_0701; /* 069B jbe 0x701 */
L_069D:
    STEP(0x069Du); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 9))); /* 069D mov al, byte ptr [bp - 9] */
    STEP(0x06A0u); t=(uint16_t)((uint8_t)(c->ax>>8)^(uint8_t)(c->ax>>8)); logic(c,t,8u); set_ah(c,t); /* 06A0 xor ah, ah */
    STEP(0x06A2u); t=(uint16_t)(c->dx^c->dx); logic(c,t,16u); c->dx=t; /* 06A2 xor dx, dx */
    STEP(0x06A4u); c->cx=c->ax; /* 06A4 mov cx, ax */
    STEP(0x06A6u); c->bx=c->dx; /* 06A6 mov bx, dx */
    STEP(0x06A8u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 06A8 mov al, byte ptr [bp - 0x29] */
    STEP(0x06ABu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 06AB cwde  */
    STEP(0x06ACu); c->di=c->ax; /* 06AC mov di, ax */
    STEP(0x06AEu); c->di=shl1(c,c->di); /* 06AE shl di, 1 */
    STEP(0x06B0u); c->di=shl1(c,c->di); /* 06B0 shl di, 1 */
    STEP(0x06B2u); c->ax=rd16(ram,c->ds,(uint16_t)(c->di + 0x67f4)); /* 06B2 mov ax, word ptr [di + 0x67f4] */
    STEP(0x06B6u); c->dx=rd16(ram,c->ds,(uint16_t)(c->di + 0x67f6)); /* 06B6 mov dx, word ptr [di + 0x67f6] */
    s=invoke(bus,size,c,budget,d,0x06BAu,0x06BFu,0x0A76u,2,1); /* 06BA lcall 0xaf8, 0xa76 */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x06BFu); c->dx=c->ax; /* 06BF mov dx, ax */
    STEP(0x06C1u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 06C1 mov al, byte ptr [bp - 0x29] */
    STEP(0x06C4u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 06C4 cwde  */
    STEP(0x06C5u); c->di=c->ax; /* 06C5 mov di, ax */
    STEP(0x06C7u); c->di=shl1(c,c->di); /* 06C7 shl di, 1 */
    STEP(0x06C9u); wr16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x1a),(uint16_t)(c->dx)); /* 06C9 mov word ptr [bp + di - 0x1a], dx */
    STEP(0x06CCu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 9))); /* 06CC mov al, byte ptr [bp - 9] */
    STEP(0x06CFu); t=(uint16_t)((uint8_t)(c->ax>>8)^(uint8_t)(c->ax>>8)); logic(c,t,8u); set_ah(c,t); /* 06CF xor ah, ah */
    STEP(0x06D1u); t=(uint16_t)(c->dx^c->dx); logic(c,t,16u); c->dx=t; /* 06D1 xor dx, dx */
    STEP(0x06D3u); c->cx=c->ax; /* 06D3 mov cx, ax */
    STEP(0x06D5u); c->bx=c->dx; /* 06D5 mov bx, dx */
    STEP(0x06D7u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 06D7 mov al, byte ptr [bp - 0x29] */
    STEP(0x06DAu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 06DA cwde  */
    STEP(0x06DBu); c->di=c->ax; /* 06DB mov di, ax */
    STEP(0x06DDu); c->di=shl1(c,c->di); /* 06DD shl di, 1 */
    STEP(0x06DFu); c->di=shl1(c,c->di); /* 06DF shl di, 1 */
    STEP(0x06E1u); c->ax=rd16(ram,c->ds,(uint16_t)(c->di + 0x67f4)); /* 06E1 mov ax, word ptr [di + 0x67f4] */
    STEP(0x06E5u); c->dx=rd16(ram,c->ds,(uint16_t)(c->di + 0x67f6)); /* 06E5 mov dx, word ptr [di + 0x67f6] */
    s=invoke(bus,size,c,budget,d,0x06E9u,0x06EEu,0x0A76u,2,1); /* 06E9 lcall 0xaf8, 0xa76 */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x06EEu); c->ax=c->cx; /* 06EE mov ax, cx */
    STEP(0x06F0u); c->dx=c->bx; /* 06F0 mov dx, bx */
    STEP(0x06F2u); c->dx=c->ax; /* 06F2 mov dx, ax */
    STEP(0x06F4u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 06F4 mov al, byte ptr [bp - 0x29] */
    STEP(0x06F7u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 06F7 cwde  */
    STEP(0x06F8u); c->di=c->ax; /* 06F8 mov di, ax */
    STEP(0x06FAu); c->di=shl1(c,c->di); /* 06FA shl di, 1 */
    STEP(0x06FCu); wr16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28),(uint16_t)(c->dx)); /* 06FC mov word ptr [bp + di - 0x28], dx */
    STEP(0x06FFu); goto L_071B; /* 06FF jmp 0x71b */
L_0701:
    STEP(0x0701u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0701 mov al, byte ptr [bp - 0x29] */
    STEP(0x0704u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0704 cwde  */
    STEP(0x0705u); c->di=c->ax; /* 0705 mov di, ax */
    STEP(0x0707u); c->di=shl1(c,c->di); /* 0707 shl di, 1 */
    STEP(0x0709u); t=(uint16_t)(c->ax^c->ax); logic(c,t,16u); c->ax=t; /* 0709 xor ax, ax */
    STEP(0x070Bu); wr16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x1a),(uint16_t)(c->ax)); /* 070B mov word ptr [bp + di - 0x1a], ax */
    STEP(0x070Eu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 070E mov al, byte ptr [bp - 0x29] */
    STEP(0x0711u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0711 cwde  */
    STEP(0x0712u); c->di=c->ax; /* 0712 mov di, ax */
    STEP(0x0714u); c->di=shl1(c,c->di); /* 0714 shl di, 1 */
    STEP(0x0716u); t=(uint16_t)(c->ax^c->ax); logic(c,t,16u); c->ax=t; /* 0716 xor ax, ax */
    STEP(0x0718u); wr16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28),(uint16_t)(c->ax)); /* 0718 mov word ptr [bp + di - 0x28], ax */
L_071B:
    STEP(0x071Bu); (void)arith(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29)),0x0006u,8u,1); /* 071B cmp byte ptr [bp - 0x29], 6 */
    STEP(0x071Fu); if((c->flags&0x40u))goto L_0724; /* 071F je 0x724 */
    STEP(0x0721u); goto L_0680; /* 0721 jmp 0x680 */
L_0724:
    STEP(0x0724u); c->ax=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); /* 0724 mov ax, word ptr [bp - 4] */
    STEP(0x0727u); t=(uint16_t)(c->ax|rd16(ram,c->ss,(uint16_t)(c->bp - 2))); logic(c,t,16u); c->ax=t; /* 0727 or ax, word ptr [bp - 2] */
    STEP(0x072Au); if(!(c->flags&0x40u))goto L_072F; /* 072A jne 0x72f */
    STEP(0x072Cu); goto L_088E; /* 072C jmp 0x88e */
L_072F:
    STEP(0x072Fu); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 072F les di, ptr [bp - 4] */
    STEP(0x0732u); (void)arith(c,brd8(bus,c->es,(uint16_t)(c->di + 0x84)),0x0080u,8u,1); /* 0732 cmp byte ptr es:[di + 0x84], 0x80 */
    STEP(0x0738u); if((c->flags&1u))goto L_073D; /* 0738 jb 0x73d */
    STEP(0x073Au); goto L_0878; /* 073A jmp 0x878 */
L_073D:
    STEP(0x073Du); wr8(ram,c->ss,(uint16_t)(c->bp - 0x29),(uint8_t)(0x0006u)); /* 073D mov byte ptr [bp - 0x29], 6 */
    STEP(0x0741u); goto L_0746; /* 0741 jmp 0x746 */
L_0743:
    STEP(0x0743u); wr8(ram,c->ss,(uint16_t)(c->bp - 0x29),(uint8_t)(incdec(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29)),8u,1))); /* 0743 dec byte ptr [bp - 0x29] */
L_0746:
    STEP(0x0746u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 2))); /* 0746 push word ptr [bp - 2] */
    STEP(0x0749u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 4))); /* 0749 push word ptr [bp - 4] */
    STEP(0x074Cu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 074C mov al, byte ptr [bp - 0x29] */
    STEP(0x074Fu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 074F cwde  */
    STEP(0x0750u); c->di=c->ax; /* 0750 mov di, ax */
    STEP(0x0752u); c->di=shl1(c,c->di); /* 0752 shl di, 1 */
    STEP(0x0754u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x1a))); /* 0754 push word ptr [bp + di - 0x1a] */
    STEP(0x0757u); c->di=(uint16_t)(c->bp - 0xc); /* 0757 lea di, [bp - 0xc] */
    STEP(0x075Au); push(ram,c,c->ss); /* 075A push ss */
    STEP(0x075Bu); push(ram,c,c->di); /* 075B push di */
    STEP(0x075Cu); push(ram,c,c->cs); /* 075C push cs */
    s=invoke(bus,size,c,budget,d,0x075Du,0x0760u,0x0054u,3,0); /* 075D call 0x54 */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x0760u); t=(uint16_t)((uint8_t)c->ax|(uint8_t)c->ax); logic(c,t,8u); set_al(c,t); /* 0760 or al, al */
    STEP(0x0762u); if((c->flags&0x40u))goto L_0767; /* 0762 je 0x767 */
    STEP(0x0764u); goto L_0815; /* 0764 jmp 0x815 */
L_0767:
    STEP(0x0767u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0767 mov al, byte ptr [bp - 0x29] */
    STEP(0x076Au); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 076A cwde  */
    STEP(0x076Bu); c->ax=shl1(c,c->ax); /* 076B shl ax, 1 */
    STEP(0x076Du); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 076D les di, ptr [bp - 4] */
    STEP(0x0770u); c->di=arith(c,c->di,c->ax,16u,0); /* 0770 add di, ax */
    STEP(0x0772u); c->dx=brd16(bus,c->es,(uint16_t)(c->di + 0x88)); /* 0772 mov dx, word ptr es:[di + 0x88] */
    STEP(0x0777u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0777 mov al, byte ptr [bp - 0x29] */
    STEP(0x077Au); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 077A cwde  */
    STEP(0x077Bu); c->di=c->ax; /* 077B mov di, ax */
    STEP(0x077Du); c->di=shl1(c,c->di); /* 077D shl di, 1 */
    STEP(0x077Fu); c->ax=rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x1a)); /* 077F mov ax, word ptr [bp + di - 0x1a] */
    STEP(0x0782u); c->ax=arith(c,c->ax,c->dx,16u,0); /* 0782 add ax, dx */
    STEP(0x0784u); c->dx=c->ax; /* 0784 mov dx, ax */
    STEP(0x0786u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0786 mov al, byte ptr [bp - 0x29] */
    STEP(0x0789u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0789 cwde  */
    STEP(0x078Au); c->ax=shl1(c,c->ax); /* 078A shl ax, 1 */
    STEP(0x078Cu); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 078C les di, ptr [bp - 4] */
    STEP(0x078Fu); c->di=arith(c,c->di,c->ax,16u,0); /* 078F add di, ax */
    STEP(0x0791u); bwr16(bus,c->es,(uint16_t)(c->di + 0x88),(uint16_t)(c->dx)); /* 0791 mov word ptr es:[di + 0x88], dx */
    STEP(0x0796u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 2))); /* 0796 push word ptr [bp - 2] */
    STEP(0x0799u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 4))); /* 0799 push word ptr [bp - 4] */
    STEP(0x079Cu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 079C mov al, byte ptr [bp - 0x29] */
    STEP(0x079Fu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 079F cwde  */
    STEP(0x07A0u); c->di=c->ax; /* 07A0 mov di, ax */
    STEP(0x07A2u); c->di=shl1(c,c->di); /* 07A2 shl di, 1 */
    STEP(0x07A4u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x1a))); /* 07A4 push word ptr [bp + di - 0x1a] */
    STEP(0x07A7u); push(ram,c,c->cs); /* 07A7 push cs */
    s=invoke(bus,size,c,budget,d,0x07A8u,0x07ABu,0x003Au,4,0); /* 07A8 call 0x3a */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x07ABu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 07AB mov al, byte ptr [bp - 0x29] */
    STEP(0x07AEu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 07AE cwde  */
    STEP(0x07AFu); c->di=c->ax; /* 07AF mov di, ax */
    STEP(0x07B1u); c->di=shl1(c,c->di); /* 07B1 shl di, 1 */
    STEP(0x07B3u); (void)arith(c,rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28)),0x0000u,16u,1); /* 07B3 cmp word ptr [bp + di - 0x28], 0 */
    STEP(0x07B7u); if((c->flags&0x41u))goto L_0813; /* 07B7 jbe 0x813 */
    STEP(0x07B9u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 2))); /* 07B9 push word ptr [bp - 2] */
    STEP(0x07BCu); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 4))); /* 07BC push word ptr [bp - 4] */
    STEP(0x07BFu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 07BF mov al, byte ptr [bp - 0x29] */
    STEP(0x07C2u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 07C2 cwde  */
    STEP(0x07C3u); c->di=c->ax; /* 07C3 mov di, ax */
    STEP(0x07C5u); c->di=shl1(c,c->di); /* 07C5 shl di, 1 */
    STEP(0x07C7u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28))); /* 07C7 push word ptr [bp + di - 0x28] */
    STEP(0x07CAu); c->di=(uint16_t)(c->bp - 0xc); /* 07CA lea di, [bp - 0xc] */
    STEP(0x07CDu); push(ram,c,c->ss); /* 07CD push ss */
    STEP(0x07CEu); push(ram,c,c->di); /* 07CE push di */
    STEP(0x07CFu); push(ram,c,c->cs); /* 07CF push cs */
    s=invoke(bus,size,c,budget,d,0x07D0u,0x07D3u,0x0054u,3,0); /* 07D0 call 0x54 */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x07D3u); t=(uint16_t)((uint8_t)c->ax|(uint8_t)c->ax); logic(c,t,8u); set_al(c,t); /* 07D3 or al, al */
    STEP(0x07D5u); if(!(c->flags&0x40u))goto L_0813; /* 07D5 jne 0x813 */
    STEP(0x07D7u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 07D7 mov al, byte ptr [bp - 0x29] */
    STEP(0x07DAu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 07DA cwde  */
    STEP(0x07DBu); c->ax=shl1(c,c->ax); /* 07DB shl ax, 1 */
    STEP(0x07DDu); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 07DD les di, ptr [bp - 4] */
    STEP(0x07E0u); c->di=arith(c,c->di,c->ax,16u,0); /* 07E0 add di, ax */
    STEP(0x07E2u); c->ax=brd16(bus,c->es,(uint16_t)(c->di + 0x88)); /* 07E2 mov ax, word ptr es:[di + 0x88] */
    STEP(0x07E7u); c->ax=incdec(c,c->ax,16u,0); /* 07E7 inc ax */
    STEP(0x07E8u); c->dx=c->ax; /* 07E8 mov dx, ax */
    STEP(0x07EAu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 07EA mov al, byte ptr [bp - 0x29] */
    STEP(0x07EDu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 07ED cwde  */
    STEP(0x07EEu); c->ax=shl1(c,c->ax); /* 07EE shl ax, 1 */
    STEP(0x07F0u); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 07F0 les di, ptr [bp - 4] */
    STEP(0x07F3u); c->di=arith(c,c->di,c->ax,16u,0); /* 07F3 add di, ax */
    STEP(0x07F5u); bwr16(bus,c->es,(uint16_t)(c->di + 0x88),(uint16_t)(c->dx)); /* 07F5 mov word ptr es:[di + 0x88], dx */
    STEP(0x07FAu); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 2))); /* 07FA push word ptr [bp - 2] */
    STEP(0x07FDu); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 4))); /* 07FD push word ptr [bp - 4] */
    STEP(0x0800u); c->ax=0x0001u; /* 0800 mov ax, 1 */
    STEP(0x0803u); push(ram,c,c->ax); /* 0803 push ax */
    STEP(0x0804u); push(ram,c,c->cs); /* 0804 push cs */
    s=invoke(bus,size,c,budget,d,0x0805u,0x0808u,0x003Au,4,0); /* 0805 call 0x3a */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x0808u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0808 mov al, byte ptr [bp - 0x29] */
    STEP(0x080Bu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 080B cwde  */
    STEP(0x080Cu); c->di=c->ax; /* 080C mov di, ax */
    STEP(0x080Eu); c->di=shl1(c,c->di); /* 080E shl di, 1 */
    STEP(0x0810u); wr16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28),(uint16_t)(incdec(c,rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28)),16u,1))); /* 0810 dec word ptr [bp + di - 0x28] */
L_0813:
    STEP(0x0813u); goto L_086F; /* 0813 jmp 0x86f */
L_0815:
    STEP(0x0815u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0815 mov al, byte ptr [bp - 0x29] */
    STEP(0x0818u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0818 cwde  */
    STEP(0x0819u); c->ax=shl1(c,c->ax); /* 0819 shl ax, 1 */
    STEP(0x081Bu); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 081B les di, ptr [bp - 4] */
    STEP(0x081Eu); c->di=arith(c,c->di,c->ax,16u,0); /* 081E add di, ax */
    STEP(0x0820u); c->ax=brd16(bus,c->es,(uint16_t)(c->di + 0x88)); /* 0820 mov ax, word ptr es:[di + 0x88] */
    STEP(0x0825u); c->ax=arith(c,c->ax,rd16(ram,c->ss,(uint16_t)(c->bp - 0xc)),16u,0); /* 0825 add ax, word ptr [bp - 0xc] */
    STEP(0x0828u); c->dx=c->ax; /* 0828 mov dx, ax */
    STEP(0x082Au); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 082A mov al, byte ptr [bp - 0x29] */
    STEP(0x082Du); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 082D cwde  */
    STEP(0x082Eu); c->ax=shl1(c,c->ax); /* 082E shl ax, 1 */
    STEP(0x0830u); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 0830 les di, ptr [bp - 4] */
    STEP(0x0833u); c->di=arith(c,c->di,c->ax,16u,0); /* 0833 add di, ax */
    STEP(0x0835u); bwr16(bus,c->es,(uint16_t)(c->di + 0x88),(uint16_t)(c->dx)); /* 0835 mov word ptr es:[di + 0x88], dx */
    STEP(0x083Au); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 083A mov al, byte ptr [bp - 0x29] */
    STEP(0x083Du); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 083D cwde  */
    STEP(0x083Eu); c->di=c->ax; /* 083E mov di, ax */
    STEP(0x0840u); c->di=shl1(c,c->di); /* 0840 shl di, 1 */
    STEP(0x0842u); c->dx=rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x1a)); /* 0842 mov dx, word ptr [bp + di - 0x1a] */
    STEP(0x0845u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0845 mov al, byte ptr [bp - 0x29] */
    STEP(0x0848u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0848 cwde  */
    STEP(0x0849u); c->di=c->ax; /* 0849 mov di, ax */
    STEP(0x084Bu); c->di=shl1(c,c->di); /* 084B shl di, 1 */
    STEP(0x084Du); c->ax=rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28)); /* 084D mov ax, word ptr [bp + di - 0x28] */
    STEP(0x0850u); c->ax=arith(c,c->ax,c->dx,16u,0); /* 0850 add ax, dx */
    STEP(0x0852u); c->ax=arith(c,c->ax,rd16(ram,c->ss,(uint16_t)(c->bp - 0xc)),16u,1); /* 0852 sub ax, word ptr [bp - 0xc] */
    STEP(0x0855u); c->dx=c->ax; /* 0855 mov dx, ax */
    STEP(0x0857u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0857 mov al, byte ptr [bp - 0x29] */
    STEP(0x085Au); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 085A cwde  */
    STEP(0x085Bu); c->di=c->ax; /* 085B mov di, ax */
    STEP(0x085Du); c->di=shl1(c,c->di); /* 085D shl di, 1 */
    STEP(0x085Fu); wr16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28),(uint16_t)(c->dx)); /* 085F mov word ptr [bp + di - 0x28], dx */
    STEP(0x0862u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 2))); /* 0862 push word ptr [bp - 2] */
    STEP(0x0865u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 4))); /* 0865 push word ptr [bp - 4] */
    STEP(0x0868u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 0xc))); /* 0868 push word ptr [bp - 0xc] */
    STEP(0x086Bu); push(ram,c,c->cs); /* 086B push cs */
    s=invoke(bus,size,c,budget,d,0x086Cu,0x086Fu,0x003Au,4,0); /* 086C call 0x3a */
    if(s!=PR_U7F_063A_RETURNED)return s;
L_086F:
    STEP(0x086Fu); (void)arith(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29)),0x0000u,8u,1); /* 086F cmp byte ptr [bp - 0x29], 0 */
    STEP(0x0873u); if((c->flags&0x40u))goto L_0878; /* 0873 je 0x878 */
    STEP(0x0875u); goto L_0743; /* 0875 jmp 0x743 */
L_0878:
    STEP(0x0878u); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 0878 les di, ptr [bp - 4] */
    STEP(0x087Bu); c->ax=brd16(bus,c->es,(uint16_t)(c->di + 0x104)); /* 087B mov ax, word ptr es:[di + 0x104] */
    STEP(0x0880u); c->dx=brd16(bus,c->es,(uint16_t)(c->di + 0x106)); /* 0880 mov dx, word ptr es:[di + 0x106] */
    STEP(0x0885u); wr16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->ax)); /* 0885 mov word ptr [bp - 4], ax */
    STEP(0x0888u); wr16(ram,c->ss,(uint16_t)(c->bp - 2),(uint16_t)(c->dx)); /* 0888 mov word ptr [bp - 2], dx */
    STEP(0x088Bu); goto L_0724; /* 088B jmp 0x724 */
L_088E:
    STEP(0x088Eu); wr8(ram,c->ss,(uint16_t)(c->bp - 0x29),(uint8_t)(0x0006u)); /* 088E mov byte ptr [bp - 0x29], 6 */
    STEP(0x0892u); goto L_0897; /* 0892 jmp 0x897 */
L_0894:
    STEP(0x0894u); wr8(ram,c->ss,(uint16_t)(c->bp - 0x29),(uint8_t)(incdec(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29)),8u,1))); /* 0894 dec byte ptr [bp - 0x29] */
L_0897:
    STEP(0x0897u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0897 mov al, byte ptr [bp - 0x29] */
    STEP(0x089Au); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 089A cwde  */
    STEP(0x089Bu); c->di=c->ax; /* 089B mov di, ax */
    STEP(0x089Du); c->di=shl1(c,c->di); /* 089D shl di, 1 */
    STEP(0x089Fu); (void)arith(c,rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28)),0x0000u,16u,1); /* 089F cmp word ptr [bp + di - 0x28], 0 */
    STEP(0x08A3u); if(!(c->flags&0x41u))goto L_08A8; /* 08A3 ja 0x8a8 */
    STEP(0x08A5u); goto L_09A4; /* 08A5 jmp 0x9a4 */
L_08A8:
    STEP(0x08A8u); c->ax=rd16(ram,c->ds,(uint16_t)(0x5d96)); /* 08A8 mov ax, word ptr [0x5d96] */
    STEP(0x08ABu); c->dx=rd16(ram,c->ds,(uint16_t)(0x5d98)); /* 08AB mov dx, word ptr [0x5d98] */
    STEP(0x08AFu); wr16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->ax)); /* 08AF mov word ptr [bp - 4], ax */
    STEP(0x08B2u); wr16(ram,c->ss,(uint16_t)(c->bp - 2),(uint16_t)(c->dx)); /* 08B2 mov word ptr [bp - 2], dx */
L_08B5:
    STEP(0x08B5u); c->ax=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); /* 08B5 mov ax, word ptr [bp - 4] */
    STEP(0x08B8u); t=(uint16_t)(c->ax|rd16(ram,c->ss,(uint16_t)(c->bp - 2))); logic(c,t,16u); c->ax=t; /* 08B8 or ax, word ptr [bp - 2] */
    STEP(0x08BBu); if(!(c->flags&0x40u))goto L_08C0; /* 08BB jne 0x8c0 */
    STEP(0x08BDu); goto L_09A4; /* 08BD jmp 0x9a4 */
L_08C0:
    STEP(0x08C0u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 2))); /* 08C0 push word ptr [bp - 2] */
    STEP(0x08C3u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 4))); /* 08C3 push word ptr [bp - 4] */
    STEP(0x08C6u); push(ram,c,c->cs); /* 08C6 push cs */
    s=invoke(bus,size,c,budget,d,0x08C7u,0x08CAu,0x0000u,5,0); /* 08C7 call 0 */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x08CAu); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 08CA les di, ptr [bp - 4] */
    STEP(0x08CDu); c->ax=arith(c,c->ax,brd16(bus,c->es,(uint16_t)(c->di + 0x102)),16u,1); /* 08CD sub ax, word ptr es:[di + 0x102] */
    STEP(0x08D2u); wr16(ram,c->ss,(uint16_t)(c->bp - 0xc),(uint16_t)(c->ax)); /* 08D2 mov word ptr [bp - 0xc], ax */
    STEP(0x08D5u); (void)arith(c,rd16(ram,c->ss,(uint16_t)(c->bp - 0xc)),0x0000u,16u,1); /* 08D5 cmp word ptr [bp - 0xc], 0 */
    STEP(0x08D9u); if(!(c->flags&0x41u))goto L_08DE; /* 08D9 ja 0x8de */
    STEP(0x08DBu); goto L_098E; /* 08DB jmp 0x98e */
L_08DE:
    STEP(0x08DEu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 08DE mov al, byte ptr [bp - 0x29] */
    STEP(0x08E1u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 08E1 cwde  */
    STEP(0x08E2u); c->di=c->ax; /* 08E2 mov di, ax */
    STEP(0x08E4u); c->di=shl1(c,c->di); /* 08E4 shl di, 1 */
    STEP(0x08E6u); c->ax=rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28)); /* 08E6 mov ax, word ptr [bp + di - 0x28] */
    STEP(0x08E9u); (void)arith(c,c->ax,rd16(ram,c->ss,(uint16_t)(c->bp - 0xc)),16u,1); /* 08E9 cmp ax, word ptr [bp - 0xc] */
    STEP(0x08ECu); if((c->flags&0x41u))goto L_093D; /* 08EC jbe 0x93d */
    STEP(0x08EEu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 08EE mov al, byte ptr [bp - 0x29] */
    STEP(0x08F1u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 08F1 cwde  */
    STEP(0x08F2u); c->ax=shl1(c,c->ax); /* 08F2 shl ax, 1 */
    STEP(0x08F4u); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 08F4 les di, ptr [bp - 4] */
    STEP(0x08F7u); c->di=arith(c,c->di,c->ax,16u,0); /* 08F7 add di, ax */
    STEP(0x08F9u); c->ax=brd16(bus,c->es,(uint16_t)(c->di + 0x88)); /* 08F9 mov ax, word ptr es:[di + 0x88] */
    STEP(0x08FEu); c->ax=arith(c,c->ax,rd16(ram,c->ss,(uint16_t)(c->bp - 0xc)),16u,0); /* 08FE add ax, word ptr [bp - 0xc] */
    STEP(0x0901u); c->dx=c->ax; /* 0901 mov dx, ax */
    STEP(0x0903u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0903 mov al, byte ptr [bp - 0x29] */
    STEP(0x0906u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0906 cwde  */
    STEP(0x0907u); c->ax=shl1(c,c->ax); /* 0907 shl ax, 1 */
    STEP(0x0909u); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 0909 les di, ptr [bp - 4] */
    STEP(0x090Cu); c->di=arith(c,c->di,c->ax,16u,0); /* 090C add di, ax */
    STEP(0x090Eu); bwr16(bus,c->es,(uint16_t)(c->di + 0x88),(uint16_t)(c->dx)); /* 090E mov word ptr es:[di + 0x88], dx */
    STEP(0x0913u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 2))); /* 0913 push word ptr [bp - 2] */
    STEP(0x0916u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 4))); /* 0916 push word ptr [bp - 4] */
    STEP(0x0919u); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 0xc))); /* 0919 push word ptr [bp - 0xc] */
    STEP(0x091Cu); push(ram,c,c->cs); /* 091C push cs */
    s=invoke(bus,size,c,budget,d,0x091Du,0x0920u,0x003Au,4,0); /* 091D call 0x3a */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x0920u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0920 mov al, byte ptr [bp - 0x29] */
    STEP(0x0923u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0923 cwde  */
    STEP(0x0924u); c->di=c->ax; /* 0924 mov di, ax */
    STEP(0x0926u); c->di=shl1(c,c->di); /* 0926 shl di, 1 */
    STEP(0x0928u); c->ax=rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28)); /* 0928 mov ax, word ptr [bp + di - 0x28] */
    STEP(0x092Bu); c->ax=arith(c,c->ax,rd16(ram,c->ss,(uint16_t)(c->bp - 0xc)),16u,1); /* 092B sub ax, word ptr [bp - 0xc] */
    STEP(0x092Eu); c->dx=c->ax; /* 092E mov dx, ax */
    STEP(0x0930u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0930 mov al, byte ptr [bp - 0x29] */
    STEP(0x0933u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0933 cwde  */
    STEP(0x0934u); c->di=c->ax; /* 0934 mov di, ax */
    STEP(0x0936u); c->di=shl1(c,c->di); /* 0936 shl di, 1 */
    STEP(0x0938u); wr16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28),(uint16_t)(c->dx)); /* 0938 mov word ptr [bp + di - 0x28], dx */
    STEP(0x093Bu); goto L_098E; /* 093B jmp 0x98e */
L_093D:
    STEP(0x093Du); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 093D mov al, byte ptr [bp - 0x29] */
    STEP(0x0940u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0940 cwde  */
    STEP(0x0941u); c->di=c->ax; /* 0941 mov di, ax */
    STEP(0x0943u); c->di=shl1(c,c->di); /* 0943 shl di, 1 */
    STEP(0x0945u); c->dx=rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28)); /* 0945 mov dx, word ptr [bp + di - 0x28] */
    STEP(0x0948u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0948 mov al, byte ptr [bp - 0x29] */
    STEP(0x094Bu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 094B cwde  */
    STEP(0x094Cu); c->ax=shl1(c,c->ax); /* 094C shl ax, 1 */
    STEP(0x094Eu); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 094E les di, ptr [bp - 4] */
    STEP(0x0951u); c->di=arith(c,c->di,c->ax,16u,0); /* 0951 add di, ax */
    STEP(0x0953u); c->ax=brd16(bus,c->es,(uint16_t)(c->di + 0x88)); /* 0953 mov ax, word ptr es:[di + 0x88] */
    STEP(0x0958u); c->ax=arith(c,c->ax,c->dx,16u,0); /* 0958 add ax, dx */
    STEP(0x095Au); c->dx=c->ax; /* 095A mov dx, ax */
    STEP(0x095Cu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 095C mov al, byte ptr [bp - 0x29] */
    STEP(0x095Fu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 095F cwde  */
    STEP(0x0960u); c->ax=shl1(c,c->ax); /* 0960 shl ax, 1 */
    STEP(0x0962u); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 0962 les di, ptr [bp - 4] */
    STEP(0x0965u); c->di=arith(c,c->di,c->ax,16u,0); /* 0965 add di, ax */
    STEP(0x0967u); bwr16(bus,c->es,(uint16_t)(c->di + 0x88),(uint16_t)(c->dx)); /* 0967 mov word ptr es:[di + 0x88], dx */
    STEP(0x096Cu); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 2))); /* 096C push word ptr [bp - 2] */
    STEP(0x096Fu); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp - 4))); /* 096F push word ptr [bp - 4] */
    STEP(0x0972u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0972 mov al, byte ptr [bp - 0x29] */
    STEP(0x0975u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0975 cwde  */
    STEP(0x0976u); c->di=c->ax; /* 0976 mov di, ax */
    STEP(0x0978u); c->di=shl1(c,c->di); /* 0978 shl di, 1 */
    STEP(0x097Au); push(ram,c,rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28))); /* 097A push word ptr [bp + di - 0x28] */
    STEP(0x097Du); push(ram,c,c->cs); /* 097D push cs */
    s=invoke(bus,size,c,budget,d,0x097Eu,0x0981u,0x003Au,4,0); /* 097E call 0x3a */
    if(s!=PR_U7F_063A_RETURNED)return s;
    STEP(0x0981u); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 0981 mov al, byte ptr [bp - 0x29] */
    STEP(0x0984u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 0984 cwde  */
    STEP(0x0985u); c->di=c->ax; /* 0985 mov di, ax */
    STEP(0x0987u); c->di=shl1(c,c->di); /* 0987 shl di, 1 */
    STEP(0x0989u); t=(uint16_t)(c->ax^c->ax); logic(c,t,16u); c->ax=t; /* 0989 xor ax, ax */
    STEP(0x098Bu); wr16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28),(uint16_t)(c->ax)); /* 098B mov word ptr [bp + di - 0x28], ax */
L_098E:
    STEP(0x098Eu); t=rd16(ram,c->ss,(uint16_t)(c->bp - 4)); c->es=rd16(ram,c->ss,(uint16_t)((uint16_t)(c->bp - 4)+2u)); c->di=t; /* 098E les di, ptr [bp - 4] */
    STEP(0x0991u); c->ax=brd16(bus,c->es,(uint16_t)(c->di + 0x104)); /* 0991 mov ax, word ptr es:[di + 0x104] */
    STEP(0x0996u); c->dx=brd16(bus,c->es,(uint16_t)(c->di + 0x106)); /* 0996 mov dx, word ptr es:[di + 0x106] */
    STEP(0x099Bu); wr16(ram,c->ss,(uint16_t)(c->bp - 4),(uint16_t)(c->ax)); /* 099B mov word ptr [bp - 4], ax */
    STEP(0x099Eu); wr16(ram,c->ss,(uint16_t)(c->bp - 2),(uint16_t)(c->dx)); /* 099E mov word ptr [bp - 2], dx */
    STEP(0x09A1u); goto L_08B5; /* 09A1 jmp 0x8b5 */
L_09A4:
    STEP(0x09A4u); (void)arith(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29)),0x0000u,8u,1); /* 09A4 cmp byte ptr [bp - 0x29], 0 */
    STEP(0x09A8u); if((c->flags&0x40u))goto L_09AD; /* 09A8 je 0x9ad */
    STEP(0x09AAu); goto L_0894; /* 09AA jmp 0x894 */
L_09AD:
    STEP(0x09ADu); wr8(ram,c->ds,(uint16_t)(0x6dc0),(uint8_t)(0x0000u)); /* 09AD mov byte ptr [0x6dc0], 0 */
    STEP(0x09B2u); wr8(ram,c->ss,(uint16_t)(c->bp - 0x29),(uint8_t)(0x0000u)); /* 09B2 mov byte ptr [bp - 0x29], 0 */
    STEP(0x09B6u); goto L_09BB; /* 09B6 jmp 0x9bb */
L_09B8:
    STEP(0x09B8u); wr8(ram,c->ss,(uint16_t)(c->bp - 0x29),(uint8_t)(incdec(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29)),8u,0))); /* 09B8 inc byte ptr [bp - 0x29] */
L_09BB:
    STEP(0x09BBu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 09BB mov al, byte ptr [bp - 0x29] */
    STEP(0x09BEu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 09BE cwde  */
    STEP(0x09BFu); c->di=c->ax; /* 09BF mov di, ax */
    STEP(0x09C1u); c->di=shl1(c,c->di); /* 09C1 shl di, 1 */
    STEP(0x09C3u); c->ax=rd16(ram,c->ss,(uint16_t)(c->bp + c->di - 0x28)); /* 09C3 mov ax, word ptr [bp + di - 0x28] */
    STEP(0x09C6u); t=(uint16_t)(c->dx^c->dx); logic(c,t,16u); c->dx=t; /* 09C6 xor dx, dx */
    STEP(0x09C8u); c->cx=c->ax; /* 09C8 mov cx, ax */
    STEP(0x09CAu); c->bx=c->dx; /* 09CA mov bx, dx */
    STEP(0x09CCu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 09CC mov al, byte ptr [bp - 0x29] */
    STEP(0x09CFu); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 09CF cwde  */
    STEP(0x09D0u); c->di=c->ax; /* 09D0 mov di, ax */
    STEP(0x09D2u); c->di=shl1(c,c->di); /* 09D2 shl di, 1 */
    STEP(0x09D4u); c->di=shl1(c,c->di); /* 09D4 shl di, 1 */
    STEP(0x09D6u); wr16(ram,c->ds,(uint16_t)(c->di + 0x67f4),(uint16_t)(c->cx)); /* 09D6 mov word ptr [di + 0x67f4], cx */
    STEP(0x09DAu); wr16(ram,c->ds,(uint16_t)(c->di + 0x67f6),(uint16_t)(c->bx)); /* 09DA mov word ptr [di + 0x67f6], bx */
    STEP(0x09DEu); set_al(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29))); /* 09DE mov al, byte ptr [bp - 0x29] */
    STEP(0x09E1u); c->ax=(uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; /* 09E1 cwde  */
    STEP(0x09E2u); c->di=c->ax; /* 09E2 mov di, ax */
    STEP(0x09E4u); c->di=shl1(c,c->di); /* 09E4 shl di, 1 */
    STEP(0x09E6u); c->di=shl1(c,c->di); /* 09E6 shl di, 1 */
    STEP(0x09E8u); c->ax=rd16(ram,c->ds,(uint16_t)(c->di + 0x67f4)); /* 09E8 mov ax, word ptr [di + 0x67f4] */
    STEP(0x09ECu); t=(uint16_t)(c->ax|rd16(ram,c->ds,(uint16_t)(c->di + 0x67f6))); logic(c,t,16u); c->ax=t; /* 09EC or ax, word ptr [di + 0x67f6] */
    STEP(0x09F0u); if((c->flags&0x40u))goto L_09F7; /* 09F0 je 0x9f7 */
    STEP(0x09F2u); wr8(ram,c->ds,(uint16_t)(0x6dc0),(uint8_t)(0x0001u)); /* 09F2 mov byte ptr [0x6dc0], 1 */
L_09F7:
    STEP(0x09F7u); (void)arith(c,rd8(ram,c->ss,(uint16_t)(c->bp - 0x29)),0x0006u,8u,1); /* 09F7 cmp byte ptr [bp - 0x29], 6 */
    STEP(0x09FBu); if(!(c->flags&0x40u))goto L_09B8; /* 09FB jne 0x9b8 */
    STEP(0x09FDu); c->sp=c->bp; /* 09FD mov sp, bp */
    STEP(0x09FFu); c->bp=pop(ram,c); /* 09FF pop bp */
    STEP(0x0A00u); c->ip=pop(ram,c); c->cs=pop(ram,c); return (c->ip==ret_ip && c->cs==ret_cs)?PR_U7F_063A_RETURNED:PR_U7F_063A_TRANSFER; /* 0A00 retf  */
}
#undef STEP
#undef COPY
