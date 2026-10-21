/* U23 +0239: original 278-byte inventory append, composed with the capacity check 275B, StrLoad 0B0B, the message display
 * 1693, GetMem, Move and the character update 0BB8. See docs/re/OVL_0023_0239_CPU.md. */
#include "poolrad/recovered/overlay_u23_append_0239_cpu.h"
#include "poolrad/recovered/resident_00ba_character_0bb8_cpu.h"
#define MASK (PR_BUS_RAM_SIZE-1u)
#define COPY(to,from) do { \
    (to).ax=(from).ax; (to).bx=(from).bx; (to).cx=(from).cx; (to).dx=(from).dx; \
    (to).si=(from).si; (to).di=(from).di; (to).bp=(from).bp; (to).sp=(from).sp; \
    (to).cs=(from).cs; (to).ip=(from).ip; (to).ds=(from).ds; (to).es=(from).es; \
    (to).ss=(from).ss; (to).flags=(from).flags; } while(0)
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg<<4u)+off)&MASK; }
static uint8_t byte_at(pr_bus *bus, uint16_t seg, uint16_t off) { return pr_bus_read8(bus,physical(seg,off)); }
static uint16_t word(pr_bus *bus, uint16_t seg, uint16_t off)
{
    size_t a=physical(seg,off);
    return (uint16_t)(pr_bus_read8(bus,a)|((uint16_t)pr_bus_read8(bus,(a+1u)&MASK)<<8u));
}
static void put_word(pr_bus *bus, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a=physical(seg,off);
    pr_bus_write8(bus,a,(uint8_t)value);
    pr_bus_write8(bus,(a+1u)&MASK,(uint8_t)(value>>8u));
}
static void push(pr_bus *bus, pr_u23_0239_cpu *c, uint16_t value)
{
    c->sp=(uint16_t)(c->sp-2u);
    put_word(bus,c->ss,c->sp,value);
}
static uint16_t pop(pr_bus *bus, pr_u23_0239_cpu *c)
{
    uint16_t value=word(bus,c->ss,c->sp);
    c->sp=(uint16_t)(c->sp+2u);
    return value;
}
static uint16_t local(const pr_u23_0239_cpu *c, int displacement) { return (uint16_t)(c->bp+displacement); }
static unsigned parity(uint8_t v)
{
    v^=(uint8_t)(v>>4u);v^=(uint8_t)(v>>2u);v^=(uint8_t)(v>>1u);
    return (v&1u)==0u;
}
/* SUB flags for the one SUB SP,14h; every defined flag is written. */
static void subtract16(pr_u23_0239_cpu *c, uint16_t a, uint16_t b)
{
    uint32_t r=((uint32_t)a-b)&0xFFFFu;
    c->flags&=(uint16_t)~0x08D5u;
    if(a<b)c->flags|=0x0001u;
    if(parity((uint8_t)r))c->flags|=0x0004u;
    if((a^b^r)&0x10u)c->flags|=0x0010u;
    if(r==0u)c->flags|=0x0040u;
    if(r&0x8000u)c->flags|=0x0080u;
    if(((a^b)&(a^r))&0x8000u)c->flags|=0x0800u;
}
/* OR/XOR set SZP, clear CF and OF; AF is cleared as the Unicorn oracle and DOSBox-X do for these opcodes. */
static uint16_t logic(pr_u23_0239_cpu *c, uint16_t v, unsigned bits)
{
    c->flags&=(uint16_t)~0x08D5u;
    if(parity((uint8_t)v))c->flags|=0x0004u;
    if((bits==8u?(uint8_t)v:v)==0u)c->flags|=0x0040u;
    if(v&(bits==8u?0x80u:0x8000u))c->flags|=0x0080u;
    return v;
}
static int ordinary(uint16_t seg)
{
    size_t a=(size_t)seg<<4u;
    return a>0xAFFFFu || a+0xFFFFu<0xA0000u;
}
static void les(pr_bus *bus, pr_u23_0239_cpu *c, uint16_t seg, uint16_t off)
{
    c->di=word(bus,seg,off);c->es=word(bus,seg,(uint16_t)(off+2u));
}
/* One composed far call. `next` is the instruction after the 5-byte call. */
static pr_u23_0239_status far_call(pr_bus *bus, pr_u23_0239_cpu *c, uint16_t site, uint16_t target, pr_u23_0239_callee which,
    const pr_u23_0239_limits *l, pr_u23_0239_diagnostic *d)
{
    uint16_t caller=c->cs,next=(uint16_t)(site+5u),segment,entry_cs;
    c->ip=site;
    if(byte_at(bus,caller,site)!=0x9Au || word(bus,caller,(uint16_t)(site+1u))!=target)return PR_U23_0239_CODE;
    segment=word(bus,caller,(uint16_t)(site+3u));entry_cs=segment;
    if(which==PR_U23_0239_CHECK) {   /* the overlay stub resolves to 275B in the segment its far-jump names */
        if(byte_at(bus,segment,target)!=0xEAu || word(bus,segment,(uint16_t)(target+1u))!=0x275Bu)return PR_U23_0239_RESOLVE;
        entry_cs=word(bus,segment,(uint16_t)(target+3u));
    }
    push(bus,c,caller);push(bus,c,next);
    c->cs=entry_cs;c->ip=which==PR_U23_0239_CHECK?0x275Bu:target;d->callee=(int)which;
    switch(which) {
    case PR_U23_0239_CHECK: {
        pr_ovl0077_275b_state inner;size_t budget=l->resident_budget;
        COPY(inner,*c);
        d->status=(int)pr_ovl0077_275b_cpu(bus->ram,PR_BUS_RAM_SIZE,&inner,&budget,&d->check);
        COPY(*c,inner);break; }
    case PR_U23_0239_STRLOAD: {
        pr_system_leaf_cpu inner;
        COPY(inner,*c);
        d->status=(int)pr_system_strload_0b0b(bus->ram,PR_BUS_RAM_SIZE,&inner);
        COPY(*c,inner);break; }
    case PR_U23_0239_DISPLAY: {
        pr_r00ba_1693_cpu inner;
        COPY(inner,*c);
        d->status=(int)pr_r00ba_display_1693_cpu(bus,PR_BUS_RAM_SIZE,&inner,l->clear_loopbacks,l->rect_loopbacks,
            l->printer_loopbacks,l->glyph_loopbacks,l->delay_loopbacks,&d->display);
        COPY(*c,inner);break; }
    case PR_U23_0239_GETMEM_HEAD: case PR_U23_0239_GETMEM_TAIL: {
        pr_system_getmem_cpu inner;
        COPY(inner,*c);
        d->status=(int)pr_system_getmem_023f(bus->ram,PR_BUS_RAM_SIZE,&inner,l->heap_steps);
        COPY(*c,inner);break; }
    case PR_U23_0239_MOVE_HEAD: case PR_U23_0239_MOVE_TAIL: {
        pr_system_leaf_cpu inner;
        COPY(inner,*c);
        d->status=(int)pr_system_move_0a3f(bus->ram,PR_BUS_RAM_SIZE,&inner);
        COPY(*c,inner);break; }
    default: {
        pr_res00ba_cpu inner;size_t budget=l->resident_budget;
        COPY(inner,*c);
        d->status=(int)pr_res00ba_0bb8_cpu(bus->ram,PR_BUS_RAM_SIZE,&inner,&budget);
        COPY(*c,inner);break; }
    }
    if(d->status!=0)return PR_U23_0239_CALLEE;
    if(c->cs!=caller || c->ip!=next)return PR_U23_0239_TRANSFER;
    return PR_U23_0239_OK;
}
pr_u23_0239_status pr_u23_append_0239_cpu(pr_bus *bus, size_t size, pr_u23_0239_cpu *c, const pr_u23_0239_limits *l,
    pr_u23_0239_diagnostic *d)
{
    pr_u23_0239_diagnostic local_diag={-1,-1,{-1,-1,0},{-1,-1,-1,{-1,-1,{-1,-1}},{-1,-1,-1,-1}},0};
    pr_u23_0239_status status;
    if(d==NULL)d=&local_diag;
    *d=local_diag;
    if(bus==NULL || bus->ram==NULL || bus->ega==NULL || c==NULL || l==NULL || size!=PR_BUS_RAM_SIZE || c->ip!=0x0239u ||
       c->sp<0x0200u || c->sp>0xFFE0u || !ordinary(c->cs) || !ordinary(c->ds) || !ordinary(c->ss))
        return PR_U23_0239_ARGUMENT;
    push(bus,c,c->bp);c->bp=c->sp;                                          /* 0239..023A */
    subtract16(c,c->sp,0x14u);c->sp=(uint16_t)(c->sp-0x14u);                /* 023C */
    push(bus,c,word(bus,c->ds,0x5D94u));push(bus,c,word(bus,c->ds,0x5D92u)); /* 023F..0243 */
    push(bus,c,word(bus,c->ss,local(c,0x0C)));push(bus,c,word(bus,c->ss,local(c,0x0A))); /* 0247..024A */
    status=far_call(bus,c,0x024Du,0x004Du,PR_U23_0239_CHECK,l,d);
    if(status!=PR_U23_0239_OK)return status;
    logic(c,(uint16_t)((uint8_t)c->ax|(uint8_t)c->ax),8u);                  /* 0252 OR AL,AL (the byte result) */
    if(!(c->flags&0x0040u)) {                                               /* 0254 JE 0274 not taken */
        c->di=local(c,-0x13);push(bus,c,c->ss);push(bus,c,c->di);           /* 0256..025A */
        c->di=0x022Eu;push(bus,c,c->cs);push(bus,c,c->di);                  /* 025B..025F */
        status=far_call(bus,c,0x0260u,0x0B0Bu,PR_U23_0239_STRLOAD,l,d);
        if(status!=PR_U23_0239_OK)return status;
        status=far_call(bus,c,0x0265u,0x1693u,PR_U23_0239_DISPLAY,l,d);
        if(status!=PR_U23_0239_OK)return status;
        les(bus,c,c->ss,local(c,6));                                        /* 026A */
        pr_bus_write8(bus,physical(c->es,c->di),1u);                        /* 026D */
        goto done;                                                          /* 0271 */
    }
    les(bus,c,c->ss,local(c,6));pr_bus_write8(bus,physical(c->es,c->di),0u); /* 0274..0277 */
    les(bus,c,c->ds,0x5D92u);                                               /* 027B */
    c->ax=word(bus,c->es,(uint16_t)(c->di+0xC8u));c->dx=word(bus,c->es,(uint16_t)(c->di+0xCAu)); /* 027F..0284 */
    put_word(bus,c->ss,local(c,-4),c->ax);put_word(bus,c->ss,local(c,-2),c->dx); /* 0289..028C */
    c->ax=word(bus,c->ss,local(c,-4));                                      /* 028F */
    c->ax=logic(c,(uint16_t)(c->ax|word(bus,c->ss,local(c,-2))),16u);       /* 0292 */
    if(c->flags&0x0040u) {                                                  /* 0295 JNE 02DC not taken: empty list */
        c->ax=0x003Fu;push(bus,c,c->ax);                                    /* 0297..029A */
        status=far_call(bus,c,0x029Bu,0x023Fu,PR_U23_0239_GETMEM_HEAD,l,d);
        if(status!=PR_U23_0239_OK)return status;
        put_word(bus,c->ss,local(c,-4),c->ax);put_word(bus,c->ss,local(c,-2),c->dx); /* 02A0..02A3 */
        les(bus,c,c->ss,local(c,0x0A));push(bus,c,c->es);push(bus,c,c->di); /* 02A6..02AA */
        les(bus,c,c->ss,local(c,-4));push(bus,c,c->es);push(bus,c,c->di);   /* 02AB..02AF */
        c->ax=0x003Fu;push(bus,c,c->ax);                                    /* 02B0..02B3 */
        status=far_call(bus,c,0x02B4u,0x0A3Fu,PR_U23_0239_MOVE_HEAD,l,d);
        if(status!=PR_U23_0239_OK)return status;
        les(bus,c,c->ss,local(c,-4));                                       /* 02B9 */
        c->ax=logic(c,(uint16_t)(c->ax^c->ax),16u);                         /* 02BC */
        put_word(bus,c->es,(uint16_t)(c->di+0x2Au),c->ax);put_word(bus,c->es,(uint16_t)(c->di+0x2Cu),c->ax); /* 02BE..02C5 */
        c->ax=word(bus,c->ss,local(c,-4));c->dx=word(bus,c->ss,local(c,-2)); /* 02C6..02CC */
        les(bus,c,c->ds,0x5D92u);                                           /* 02CC */
        put_word(bus,c->es,(uint16_t)(c->di+0xC8u),c->ax);put_word(bus,c->es,(uint16_t)(c->di+0xCAu),c->dx); /* 02D0..02D9 */
        goto redraw;                                                        /* 02DA */
    }
    for(;;) {                                                               /* 02DC: walk to the tail */
        les(bus,c,c->ss,local(c,-4));
        c->ax=word(bus,c->es,(uint16_t)(c->di+0x2Au));
        c->ax=logic(c,(uint16_t)(c->ax|word(bus,c->es,(uint16_t)(c->di+0x2Cu))),16u);
        if(c->flags&0x0040u)break;                                          /* 02E7 JE 02FC */
        les(bus,c,c->ss,local(c,-4));
        c->ax=word(bus,c->es,(uint16_t)(c->di+0x2Au));c->dx=word(bus,c->es,(uint16_t)(c->di+0x2Cu));
        put_word(bus,c->ss,local(c,-4),c->ax);put_word(bus,c->ss,local(c,-2),c->dx);
        if(d->walked>=l->walk_limit) {c->ip=0x02FAu;return PR_U23_0239_LIMIT;}
        ++d->walked;                                                        /* 02FA JMP 02DC */
    }
    c->ax=0x003Fu;push(bus,c,c->ax);                                        /* 02FC..02FF */
    status=far_call(bus,c,0x0300u,0x023Fu,PR_U23_0239_GETMEM_TAIL,l,d);
    if(status!=PR_U23_0239_OK)return status;
    put_word(bus,c->ss,local(c,-8),c->ax);put_word(bus,c->ss,local(c,-6),c->dx); /* 0305..0308 */
    les(bus,c,c->ss,local(c,0x0A));push(bus,c,c->es);push(bus,c,c->di);     /* 030B..030F */
    les(bus,c,c->ss,local(c,-8));push(bus,c,c->es);push(bus,c,c->di);       /* 0310..0314 */
    c->ax=0x003Fu;push(bus,c,c->ax);                                        /* 0315..0318 */
    status=far_call(bus,c,0x0319u,0x0A3Fu,PR_U23_0239_MOVE_TAIL,l,d);
    if(status!=PR_U23_0239_OK)return status;
    les(bus,c,c->ss,local(c,-8));                                           /* 031E */
    c->ax=logic(c,(uint16_t)(c->ax^c->ax),16u);                             /* 0321 */
    put_word(bus,c->es,(uint16_t)(c->di+0x2Au),c->ax);put_word(bus,c->es,(uint16_t)(c->di+0x2Cu),c->ax); /* 0323..032A */
    c->ax=word(bus,c->ss,local(c,-8));c->dx=word(bus,c->ss,local(c,-6));   /* 032B..0330 */
    les(bus,c,c->ss,local(c,-4));                                           /* 0331 */
    put_word(bus,c->es,(uint16_t)(c->di+0x2Au),c->ax);put_word(bus,c->es,(uint16_t)(c->di+0x2Cu),c->dx); /* 0334..033B */
redraw:
    push(bus,c,word(bus,c->ds,0x5D94u));push(bus,c,word(bus,c->ds,0x5D92u)); /* 033C..0343 */
    status=far_call(bus,c,0x0344u,0x0BB8u,PR_U23_0239_REDRAW,l,d);
    if(status!=PR_U23_0239_OK)return status;
done:
    c->sp=c->bp;c->bp=pop(bus,c);                                           /* 0349..034B */
    c->ip=pop(bus,c);c->cs=pop(bus,c);c->sp=(uint16_t)(c->sp+8u);            /* 034C RETF 8 */
    return PR_U23_0239_OK;
}
