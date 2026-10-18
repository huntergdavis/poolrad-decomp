/* Complete U38 +04BA, composed with U9E +110D and +10E4. */
#include "poolrad/recovered/overlay_u38_pair_04ba.h"
#include "poolrad/recovered/overlay_u9e_entry17_110d.h"
#define MASK (PR_U9E_10E4_RAM_SIZE - 1u)
#define BP(n) ((uint16_t)(c->bp + (n)))
#define COPY_CPU(d,s) do { \
    (d).ax=(s).ax; (d).bx=(s).bx; (d).cx=(s).cx; (d).dx=(s).dx; \
    (d).si=(s).si; (d).di=(s).di; (d).bp=(s).bp; (d).sp=(s).sp; \
    (d).cs=(s).cs; (d).ip=(s).ip; (d).ds=(s).ds; (d).es=(s).es; \
    (d).ss=(s).ss; (d).flags=(s).flags; \
} while (0)
static size_t physical(uint16_t seg, uint16_t off)
{ return (((size_t)seg << 4) + off) & MASK; }
static uint16_t word_at(const uint8_t *ram, size_t at)
{ return (uint16_t)(ram[at] | ((uint16_t)ram[(at+1u)&MASK] << 8)); }
static uint8_t r8(const uint8_t *ram, uint16_t seg, uint16_t off)
{ return ram[physical(seg,off)]; }
static uint16_t r16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ return word_at(ram,physical(seg,off)); }
static void w8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{ ram[physical(seg,off)] = value; }
static void w16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at=physical(seg,off); ram[at]=(uint8_t)value;
    ram[(at+1u)&MASK]=(uint8_t)(value>>8);
}
static void push(uint8_t *ram, pr_u38_04ba_cpu *c, uint16_t value)
{ c->sp=(uint16_t)(c->sp-2u); w16(ram,c->ss,c->sp,value); }
static uint16_t pop(const uint8_t *ram, pr_u38_04ba_cpu *c)
{ uint16_t value=r16(ram,c->ss,c->sp); c->sp=(uint16_t)(c->sp+2u); return value; }
static void les(const uint8_t *ram, pr_u38_04ba_cpu *c, uint16_t offset)
{
    size_t at=physical(c->ss,offset);
    c->di=word_at(ram,at); c->es=word_at(ram,(at+2u)&MASK);
}
static void al(pr_u38_04ba_cpu *c, uint8_t value)
{ c->ax=(uint16_t)((c->ax&0xFF00u)|value); }
static uint16_t parity(uint8_t value)
{
    value^=(uint8_t)(value>>4); value^=(uint8_t)(value>>2);
    value^=(uint8_t)(value>>1); return (value&1u)?0u:4u;
}
static uint16_t sub(pr_u38_04ba_cpu *c, uint16_t a, uint16_t b, unsigned bits)
{
    uint16_t sign=bits==8u?0x80u:0x8000u;
    uint16_t result=(uint16_t)((a-b)&(sign|(sign-1u))),f=parity((uint8_t)result);
    if (a<b) f|=1u;
    if (((a^b^result)&0x10u)!=0u) f|=0x10u;
    if (!result) f|=0x40u;
    if ((result&sign)!=0u) f|=0x80u;
    if (((a^b)&(a^result)&sign)!=0u) f|=0x800u;
    c->flags=(uint16_t)((c->flags&~0x08D5u)|f); return result;
}
static void logic(pr_u38_04ba_cpu *c, uint16_t value)
{
    uint16_t f=parity((uint8_t)value);
    if (!value) f|=0x40u;
    if (value&0x8000u) f|=0x80u;
    c->flags=(uint16_t)((c->flags&~0x08D5u)|f);
}
static int stack_code_alias(const pr_u38_04ba_cpu *c, uint16_t u9e_cs)
{
    unsigned i,k;
    for (i=1; i<=32; ++i) for (k=0; k<2; ++k) {
        size_t at=(physical(c->ss,(uint16_t)(c->sp-i))+k)&MASK;
        if (((at-physical(c->cs,0x04BAu))&MASK)<284u ||
            ((at-physical(u9e_cs,0x10E4u))&MASK)<120u) return 1;
    }
    return 0;
}
static pr_u38_04ba_status decode(uint8_t *ram, size_t size, pr_u38_04ba_cpu *c,
                                uint16_t u9e_cs, uint16_t return_ip)
{
    uint16_t caller_cs=c->cs;
    pr_u9e_110d_cpu callee;
    push(ram,c,caller_cs); push(ram,c,return_ip);
    c->cs=u9e_cs; c->ip=0x110Du;
    COPY_CPU(callee,*c);
    (void)pr_u9e_entry17_110d(ram,size,&callee);
    COPY_CPU(*c,callee);
    return c->cs==caller_cs && c->ip==return_ip ? PR_U38_04BA_OK : PR_U38_04BA_TRANSFER;
}
pr_u38_04ba_status pr_u38_pair_04ba(uint8_t *ram, size_t ram_size,
                                  pr_u38_04ba_cpu *c, uint16_t u9e_cs,
                                  size_t budget)
{
    uint16_t cs;
    pr_u38_04ba_status status;
    if (!ram || !c || ram_size!=PR_U9E_10E4_RAM_SIZE || c->ip!=0x04BAu ||
        stack_code_alias(c,u9e_cs)) return PR_U38_04BA_ARGUMENT;
    cs=c->cs;
    /* +04BA..04CA: reject a source with its high attribute bit set. */
    push(ram,c,c->bp); c->bp=c->sp; c->sp=sub(c,c->sp,12u,16u);
    les(ram,c,BP(8)); (void)sub(c,r8(ram,c->es,(uint16_t)(c->di+3u)),0x7Fu,8u);
    if (!(c->flags&0x41u)) goto done;
    /* +04CD..04E2: decode source into live frame bytes. */
    push(ram,c,r16(ram,c->ss,BP(10))); push(ram,c,r16(ram,c->ss,BP(8)));
    c->di=BP(-1); push(ram,c,c->ss); push(ram,c,c->di);
    c->di=BP(-2); push(ram,c,c->ss); push(ram,c,c->di);
    status=decode(ram,ram_size,c,u9e_cs,0x04E2u);
    if (status!=PR_U38_04BA_OK) return status;
    al(c,r8(ram,c->ss,BP(-1))); les(ram,c,BP(12)); w8(ram,c->es,(uint16_t)(c->di+0x10u),(uint8_t)c->ax);
    al(c,r8(ram,c->ss,BP(-2))); les(ram,c,BP(12)); w8(ram,c->es,(uint16_t)(c->di+0x16u),(uint8_t)c->ax);
    /* +04F6..0515: best pair and iterator live in SS. */
    c->ax=0; logic(c,c->ax); w16(ram,c->ss,BP(-12),c->ax); w16(ram,c->ss,BP(-10),c->ax);
    w8(ram,c->ss,BP(-3),0); w8(ram,c->ss,BP(-4),0);
    les(ram,c,BP(12)); c->ax=r16(ram,c->es,(uint16_t)(c->di+0x7Fu));
    c->dx=r16(ram,c->es,(uint16_t)(c->di+0x81u));
    w16(ram,c->ss,BP(-8),c->ax); w16(ram,c->ss,BP(-6),c->dx);
    for (;;) {
        c->ip=0x0518u;
        if (!budget) return PR_U38_04BA_LIMIT;
        --budget;
        c->ax=r16(ram,c->ss,BP(-8)); c->ax|=r16(ram,c->ss,BP(-6)); logic(c,c->ax);
        if (c->flags&0x40u) break;
        les(ram,c,BP(-8)); (void)sub(c,r8(ram,c->es,c->di),12u,8u);
        if (!(c->flags&0x40u)) {
            les(ram,c,BP(-8)); (void)sub(c,r8(ram,c->es,c->di),0x26u,8u);
            if (!(c->flags&0x40u)) goto next;
        }
        c->ax=r16(ram,c->ss,BP(-8)); c->dx=r16(ram,c->ss,BP(-6));
        (void)sub(c,c->dx,r16(ram,c->ss,BP(10)),16u);
        if (c->flags&0x40u) {
            (void)sub(c,c->ax,r16(ram,c->ss,BP(8)),16u);
            if (c->flags&0x40u) goto next;
        }
        push(ram,c,r16(ram,c->ss,BP(-6))); push(ram,c,r16(ram,c->ss,BP(-8)));
        c->di=BP(-1); push(ram,c,c->ss); push(ram,c,c->di);
        c->di=BP(-2); push(ram,c,c->ss); push(ram,c,c->di);
        status=decode(ram,ram_size,c,u9e_cs,0x0557u);
        if (status!=PR_U38_04BA_OK) return status;
        al(c,r8(ram,c->ss,BP(-1))); (void)sub(c,(uint8_t)c->ax,r8(ram,c->ss,BP(-3)),8u);
        if (c->flags&0x41u) {
            (void)sub(c,r8(ram,c->ss,BP(-3)),0x12u,8u);
            if (!(c->flags&0x40u)) goto next;
            al(c,r8(ram,c->ss,BP(-2))); (void)sub(c,(uint8_t)c->ax,r8(ram,c->ss,BP(-4)),8u);
            if (c->flags&0x41u) goto next;
        }
        al(c,r8(ram,c->ss,BP(-1))); w8(ram,c->ss,BP(-3),(uint8_t)c->ax);
        al(c,r8(ram,c->ss,BP(-2))); w8(ram,c->ss,BP(-4),(uint8_t)c->ax);
        c->ax=r16(ram,c->ss,BP(-8)); c->dx=r16(ram,c->ss,BP(-6));
        w16(ram,c->ss,BP(-12),c->ax); w16(ram,c->ss,BP(-10),c->dx);
next:
        les(ram,c,BP(-8)); c->ax=r16(ram,c->es,(uint16_t)(c->di+5u));
        c->dx=r16(ram,c->es,(uint16_t)(c->di+7u));
        w16(ram,c->ss,BP(-8),c->ax); w16(ram,c->ss,BP(-6),c->dx);
    }
    c->ax=r16(ram,c->ss,BP(-12)); c->ax|=r16(ram,c->ss,BP(-10)); logic(c,c->ax);
    if (c->flags&0x40u) goto done;
    les(ram,c,BP(12)); al(c,r8(ram,c->es,(uint16_t)(c->di+0x10u))); push(ram,c,c->ax);
    les(ram,c,BP(12)); al(c,r8(ram,c->es,(uint16_t)(c->di+0x16u))); push(ram,c,c->ax);
    push(ram,c,cs); push(ram,c,0x05B5u); c->cs=u9e_cs; c->ip=0x10E4u;
    (void)pr_u9e_entry16_10e4(ram,ram_size,c);
    if (c->cs!=cs || c->ip!=0x05B5u) return PR_U38_04BA_TRANSFER;
    les(ram,c,BP(-12)); w8(ram,c->es,(uint16_t)(c->di+3u),(uint8_t)c->ax);
    al(c,r8(ram,c->ss,BP(-3))); les(ram,c,BP(12)); w8(ram,c->es,(uint16_t)(c->di+0x10u),(uint8_t)c->ax);
    al(c,r8(ram,c->ss,BP(-4))); les(ram,c,BP(12)); w8(ram,c->es,(uint16_t)(c->di+0x16u),(uint8_t)c->ax);
done:
    c->sp=c->bp; c->bp=pop(ram,c); c->ip=pop(ram,c); c->cs=pop(ram,c);
    c->sp=(uint16_t)(c->sp+10u); return PR_U38_04BA_OK;
}
