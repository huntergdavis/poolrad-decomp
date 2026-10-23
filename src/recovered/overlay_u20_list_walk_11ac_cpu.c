/* U20 list walk, exact [11AC,12A8); see U20_LIST_WALK_11AC_CPU.md.
 * Memory/flag idioms follow merged overlay_u6c_record_unlink_3291_cpu.c; the
 * +3291 and SYSTEM FreeMem models are composed unchanged. */
#include "poolrad/recovered/overlay_u20_list_walk_11ac_cpu.h"
#ifdef PR_U20_11AC_TRACE
#include <stdio.h>
#endif
#define MASK (PR_U20_11AC_RAM_SIZE-1u)
#define BASE 0x11ACu
typedef struct { uint8_t *r; pr_u20_11ac_cpu *c; } machine;
_Static_assert(sizeof(pr_u20_11ac_cpu)==sizeof(pr_u6c_3291_cpu),"scale check: +3291 CPU layout differs");
_Static_assert(sizeof(pr_u20_11ac_cpu)==sizeof(pr_system_freemem_cpu),"scale check: FreeMem CPU layout differs");
static size_t physical(uint16_t s,uint16_t o) { return (((size_t)s<<4)+o)&MASK; }
static uint16_t word_at(const uint8_t *r,size_t p) { return (uint16_t)(r[p]|((uint16_t)r[(p+1)&MASK]<<8)); }
static uint16_t rd(machine *m,uint16_t s,uint16_t o) { return word_at(m->r,physical(s,o)); }
static uint8_t rd8(machine *m,uint16_t s,uint16_t o) { return m->r[physical(s,o)]; }
static void wr(machine *m,uint16_t s,uint16_t o,uint16_t v) {
    size_t p=physical(s,o);m->r[p]=(uint8_t)v;m->r[(p+1)&MASK]=(uint8_t)(v>>8);
}
static uint16_t lr(machine *m,int d) { return rd(m,m->c->ss,(uint16_t)(m->c->bp+d)); }
static void lw(machine *m,int d,uint16_t v) { wr(m,m->c->ss,(uint16_t)(m->c->bp+d),v); }
static uint16_t dsr(machine *m,uint16_t o) { return rd(m,m->c->ds,o); }
static void dsw(machine *m,uint16_t o,uint16_t v) { wr(m,m->c->ds,o,v); }
static void push(machine *m,uint16_t v) { m->c->sp=(uint16_t)(m->c->sp-2u);wr(m,m->c->ss,m->c->sp,v); }
static uint16_t pop(machine *m) { uint16_t v=rd(m,m->c->ss,m->c->sp);m->c->sp=(uint16_t)(m->c->sp+2u);return v; }
static void les(machine *m,uint16_t s,uint16_t o) {
    size_t p=physical(s,o);
    m->c->di=word_at(m->r,p);m->c->es=word_at(m->r,(p+2)&MASK);
}
static uint16_t er(machine *m,unsigned d) { return rd(m,m->c->es,(uint16_t)(m->c->di+d)); }
static void ew(machine *m,unsigned d,uint16_t v) { wr(m,m->c->es,(uint16_t)(m->c->di+d),v); }
static uint8_t er8(machine *m,unsigned d) { return rd8(m,m->c->es,(uint16_t)(m->c->di+d)); }
static unsigned parity(uint8_t v) { v^=v>>4;v^=v>>2;v^=v>>1;return (v&1u)==0; }
static void szp(pr_u20_11ac_cpu *c,uint16_t r,uint16_t sign) {
    if(parity((uint8_t)r))c->flags|=4;
    if(r==0)c->flags|=0x40;
    if(r&sign)c->flags|=0x80;
}
static void sub16(pr_u20_11ac_cpu *c,uint16_t a,uint16_t b) {
    uint16_t r=(uint16_t)(a-b);c->flags&=(uint16_t)~0x08D5u;
    if(a<b)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((a^b)&(a^r)&0x8000)c->flags|=0x800;
    szp(c,r,0x8000);
}
static void logic(pr_u20_11ac_cpu *c,uint16_t r) { c->flags&=(uint16_t)~0x08D5u;szp(c,r,0x8000); }
/* INC leaves CF untouched; the oracle's AF convention is normal/386. */
static void inc16(pr_u20_11ac_cpu *c,uint16_t a,uint16_t r) {
    c->flags&=(uint16_t)~0x08D4u;
    if((a^1u^r)&0x10)c->flags|=0x10;
    if(r==0x8000u)c->flags|=0x800; /* pinned INCw OF = (res == 8000h), flags.cpp:375 */
    szp(c,r,0x8000);
}
static void cmp8(pr_u20_11ac_cpu *c,uint8_t a,uint8_t b) {
    uint8_t r=(uint8_t)(a-b);c->flags&=(uint16_t)~0x08D5u;
    if(a<b)c->flags|=1;
    if((a^b^r)&0x10)c->flags|=0x10;
    if((a^b)&(a^r)&0x80)c->flags|=0x800;
    szp(c,r,0x80);
}
#define COPY_CPU(to,from) do { \
    (to).ax=(from).ax; (to).bx=(from).bx; (to).cx=(from).cx; \
    (to).dx=(from).dx; (to).si=(from).si; (to).di=(from).di; \
    (to).bp=(from).bp; (to).sp=(from).sp; (to).cs=(from).cs; \
    (to).ip=(from).ip; (to).ds=(from).ds; (to).es=(from).es; \
    (to).ss=(from).ss; (to).flags=(from).flags; \
} while(0)
/* LCALL 0078:002A (the loaded stub EA-jumps to U6C +3291, RETF 4): operands are
 * checked before the pushes, the stub after them. */
static pr_u20_11ac_status call_unlink(machine *m,uint16_t site,uint16_t resident,
    uint16_t system,const pr_u20_11ac_limits *l,pr_u20_11ac_diagnostic *d)
{
    pr_u20_11ac_cpu *c=m->c;pr_u6c_3291_cpu inner;
    uint16_t caller=c->cs,continuation=(uint16_t)(site+5u),entry_sp=c->sp,stub;
    c->ip=site;d->call_site=site;
    if(rd8(m,caller,site)!=0x9A || rd(m,caller,(uint16_t)(site+1u))!=0x2A)
        return PR_U20_11AC_CODE;
#ifdef PR_U20_11AC_TRACE
    { uint16_t win_o=rd(m,m->c->ds,0x49D6),win_s=rd(m,m->c->ds,0x49D8);
      fprintf(stderr,"CALL b6=%04x b8=%04x win=%04x:%04x w67c=%04x head=%04x:%04x active=%04x:%04x\n",
        rd(m,m->c->ss,(uint16_t)(entry_sp+4u)),rd(m,m->c->ss,(uint16_t)(entry_sp+6u)),
        win_s,win_o,rd(m,win_s,(uint16_t)(win_o+0x67Cu)),
        rd(m,m->c->ds,0x5D98),rd(m,m->c->ds,0x5D96),rd(m,m->c->ds,0x5D94),rd(m,m->c->ds,0x5D92)); }
#endif
    stub=rd(m,caller,(uint16_t)(site+3u));
    push(m,caller);push(m,continuation);
    c->cs=stub;c->ip=0x2A;
    if(rd8(m,stub,0x2A)!=0xEA || rd(m,stub,0x2B)!=0x3291)
        return PR_U20_11AC_CODE;
    c->cs=rd(m,stub,0x2D);c->ip=0x3291;COPY_CPU(inner,*c);
    d->callee=PR_U20_11AC_UNLINK;++d->unlink_calls;
    d->callee_status=(int)pr_u6c_record_unlink_3291_cpu(m->r,PR_U20_11AC_RAM_SIZE,
        &inner,resident,system,&l->unlink,&d->unlink);
    COPY_CPU(*c,inner);
    if(d->callee_status!=(int)PR_U6C_3291_OK)return PR_U20_11AC_CALLEE;
    if(c->cs!=caller || c->ip!=continuation)return PR_U20_11AC_TRANSFER;
    /* RETF 4 pops the 4-byte return frame and the two argument bytes */
    if(c->sp!=(uint16_t)(entry_sp+4u))return PR_U20_11AC_TRANSFER;
    return PR_U20_11AC_OK;
}
/* LCALL 0AF8:0254 FreeMem (RETF 6): size 16h, offset, segment already pushed. */
static pr_u20_11ac_status call_freemem(machine *m,uint16_t site,uint16_t system,
    const pr_u20_11ac_limits *l,pr_u20_11ac_diagnostic *d)
{
    pr_u20_11ac_cpu *c=m->c;pr_system_freemem_cpu inner;
    uint16_t caller=c->cs,continuation=(uint16_t)(site+5u),entry_sp=c->sp;
    c->ip=site;d->call_site=site;
    if(rd8(m,caller,site)!=0x9A || rd(m,caller,(uint16_t)(site+1u))!=0x254 ||
       rd(m,caller,(uint16_t)(site+3u))!=system)return PR_U20_11AC_CODE;
    push(m,caller);push(m,continuation);
    c->cs=system;c->ip=0x254;COPY_CPU(inner,*c);
    d->callee=PR_U20_11AC_FREEMEM;++d->freemem_calls;
    d->freemem_status=pr_system_freemem_0254(m->r,PR_U20_11AC_RAM_SIZE,&inner,l->max_callee_steps);
    d->callee_status=(int)d->freemem_status;
    COPY_CPU(*c,inner);
    if(d->callee_status!=(int)PR_SYSTEM_FREEMEM_RETURNED)return PR_U20_11AC_CALLEE;
    if(c->cs!=caller || c->ip!=continuation)return PR_U20_11AC_TRANSFER;
    /* RETF 6 pops the 4-byte return frame and the three argument words */
    if(c->sp!=(uint16_t)(entry_sp+6u))return PR_U20_11AC_TRANSFER;
    return PR_U20_11AC_OK;
}
pr_u20_11ac_status pr_u20_list_walk_11ac_cpu(uint8_t *ram,size_t size,
    pr_u20_11ac_cpu *c,uint16_t resident,uint16_t system,
    const pr_u20_11ac_limits *l,pr_u20_11ac_diagnostic *d)
{
    pr_u20_11ac_diagnostic scratch;machine m;pr_u20_11ac_status status;unsigned loops=0;
    if(!ram || size!=PR_U20_11AC_RAM_SIZE || !c || !l)return PR_U20_11AC_ARGUMENT;
    if(c->sp<0x0058u || c->sp>0xFFF8u)return PR_U20_11AC_ARGUMENT;
    if(!d)d=&scratch;
    d->callee=PR_U20_11AC_NO_CALLEE;d->callee_status=-1;d->unlink_calls=0;
    d->freemem_calls=0;d->call_site=0;d->freemem_status=PR_SYSTEM_FREEMEM_RETURNED;
    m.r=ram;m.c=c;
    push(&m,c->bp);                       /* 11AC */
    c->bp=c->sp;                          /* 11AD */
    { uint16_t a=c->sp,r=(uint16_t)(a-8u);sub16(c,a,8u);c->sp=r; }  /* 11AF */
    les(&m,c->ds,0x49D6);                 /* 11B2 */
    c->ax=0;logic(c,0);                   /* 11B6 XOR AX,AX */
    ew(&m,0x590,c->ax);                   /* 11B8 */
    c->ax=dsr(&m,0x5D96);                 /* 11BD */
    c->dx=dsr(&m,0x5D98);                 /* 11C0 */
    lw(&m,-4,c->ax);                      /* 11C4 */
    lw(&m,-2,c->dx);                      /* 11C7 */
    for(;;) {                             /* 11CA */
        uint16_t r;
        c->ax=lr(&m,-4);                  /* 11CA */
        r=(uint16_t)(c->ax|lr(&m,-2));c->ax=r;logic(c,r);   /* 11CD OR AX,[BP-2] */
        if((c->flags&0x40)==0) {          /* 11D0 JNE 11D5 */
            les(&m,c->ss,(uint16_t)(c->bp-4));              /* 11D5 */
            les(&m,c->es,(uint16_t)(c->di+0x108));          /* 11D8 */
            { uint8_t a=er8(&m,0x13);cmp8(c,a,1); }         /* 11DD */
            if((c->flags&0x40)!=0)goto type1;               /* 11E2 JE 11EF */
            les(&m,c->ss,(uint16_t)(c->bp-4));              /* 11E4 */
            { uint8_t a=er8(&m,0x10E);cmp8(c,a,1); }        /* 11E7 */
            if((c->flags&0x40)!=0)goto type1;               /* 11ED JNE 124C -> else */
            goto other;
        }
        goto done;                        /* 11D2 JMP 1296 */
type1:
        les(&m,c->ss,(uint16_t)(c->bp-4));                  /* 11EF */
        { uint8_t a=er8(&m,0x10D);cmp8(c,a,1); }            /* 11F2 */
        if((c->flags&0x40)==0) {          /* 11F8 JE 1203 -> skip the increment */
            les(&m,c->ds,0x49D6);                           /* 11FA */
            { uint16_t a=er(&m,0x590),n=(uint16_t)(a+1u);inc16(c,a,n);ew(&m,0x590,n); } /* 11FE */
        }
        les(&m,c->ss,(uint16_t)(c->bp-4));                  /* 1203 */
        c->ax=er(&m,0x104);                                 /* 1206 */
        c->dx=er(&m,0x106);                                 /* 120B */
        lw(&m,-8,c->ax);                                    /* 1210 */
        lw(&m,-6,c->dx);                                    /* 1213 */
        c->ax=lr(&m,-4);                                    /* 1216 */
        c->dx=lr(&m,-2);                                    /* 1219 */
        dsw(&m,0x5D92,c->ax);                               /* 121C */
        dsw(&m,0x5D94,c->dx);                               /* 121F */
        les(&m,c->ss,(uint16_t)(c->bp-4));                  /* 1223 */
#ifdef PR_U20_11AC_TRACE
        fprintf(stderr,"TRACE 1223 cur=%04x:%04x",c->es,c->di);
#endif
        les(&m,c->es,(uint16_t)(c->di+0x108));              /* 1226 */
#ifdef PR_U20_11AC_TRACE
        fprintf(stderr," -> es:di=%04x:%04x",c->es,c->di);
#endif
        { uint8_t a=er8(&m,0x13);cmp8(c,a,1); }             /* 122B */
#ifdef PR_U20_11AC_TRACE
        fprintf(stderr," byte13=%02x zf=%d\n",er8(&m,0x13),(c->flags&0x40)!=0);
#endif
        c->ax=(uint16_t)((c->ax&0xFF00u)|0x00u);            /* 1230 MOV AL,0 */
#ifdef PR_U20_11AC_TRACE
        fprintf(stderr,"AX1230=%04x di=%04x es=%04x\n",c->ax,c->di,c->es);
#endif
        /* 1232 JNE 1235: the increment happens only when the compare was EQUAL */
        if((c->flags&0x40)!=0) {
            uint16_t a=c->ax,n=(uint16_t)(a+1u);inc16(c,a,n);c->ax=n;      /* 1234 INC AX */
        }
        push(&m,c->ax);                                     /* 1235 */
        c->ax=(uint16_t)((c->ax&0xFF00u)|0x01u);            /* 1236 MOV AL,1 */
        push(&m,c->ax);                                     /* 1238 */
        status=call_unlink(&m,0x1239,resident,system,l,d);       /* 1239 */
        if(status!=PR_U20_11AC_OK)return status;
        c->ax=lr(&m,-8);                                    /* 123E */
        c->dx=lr(&m,-6);                                    /* 1241 */
        lw(&m,-4,c->ax);                                    /* 1244 */
        lw(&m,-2,c->dx);                                    /* 1247 */
        goto advance;                                       /* 124A JMP 1293 */
other:
        les(&m,c->ss,(uint16_t)(c->bp-4));                  /* 124C */
        c->ax=er(&m,0x108);                                 /* 124F */
        { uint16_t a=c->ax,b=er(&m,0x10A),r=(uint16_t)(a|b);c->ax=r;logic(c,r); } /* 1254 */
        if((c->flags&0x40)!=0)goto advance;                 /* 1259 JE 1280 */
        les(&m,c->ss,(uint16_t)(c->bp-4));                  /* 125B */
        push(&m,er(&m,0x10A));                              /* 125E */
        push(&m,er(&m,0x108));                              /* 1263 */
        c->ax=0x16;push(&m,c->ax);                          /* 1268..126B */
        status=call_freemem(&m,0x126C,system,l,d);               /* 126C */
        if(status!=PR_U20_11AC_OK)return status;
        les(&m,c->ss,(uint16_t)(c->bp-4));                  /* 1271 */
        c->ax=0;logic(c,0);                                 /* 1274 XOR AX,AX */
        ew(&m,0x108,c->ax);                                 /* 1276 */
        ew(&m,0x10A,c->ax);                                 /* 127B */
advance:
        les(&m,c->ss,(uint16_t)(c->bp-4));                  /* 1280 / 1280.. */
        c->ax=er(&m,0x104);                                 /* 1283 */
        c->dx=er(&m,0x106);                                 /* 1288 */
        lw(&m,-4,c->ax);                                    /* 128D */
        lw(&m,-2,c->dx);                                    /* 1290 */
        if(++loops>l->max_walk_loopbacks) {                 /* 1293 JMP 11CA */
            c->ip=0x11CAu;return PR_U20_11AC_LIMIT;
        }
    }
done:
    c->ax=dsr(&m,0x5D96);                                   /* 1296 */
    c->dx=dsr(&m,0x5D98);                                   /* 1299 */
    dsw(&m,0x5D92,c->ax);                                   /* 129D */
    dsw(&m,0x5D94,c->dx);                                   /* 12A0 */
    c->sp=c->bp;                                            /* 12A4 */
    c->bp=pop(&m);                                          /* 12A6 */
    { uint16_t ip=pop(&m);uint16_t cs=pop(&m);c->ip=ip;c->cs=cs; } /* 12A7 RETF */
    return PR_U20_11AC_OK;
}
