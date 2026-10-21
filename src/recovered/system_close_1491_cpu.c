/* START.EXE 0AF8:1491, Turbo Pascal SYSTEM Close, raw CPU (docs/re/SYSTEM_CLOSE_1491_CPU.md). */
#include "poolrad/recovered/system_close_1491_cpu.h"
#include "poolrad/recovered/system_filecheck_14b6_cpu.h"
#define MASK (PR_SYSTEM_CLOSE_RAM_SIZE - 1u)
typedef pr_system_close_cpu cpu_t;
static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4) + off) & MASK; }
static uint16_t word(const uint8_t *r, size_t at) { return (uint16_t)(r[at] | ((uint16_t)r[(at + 1u) & MASK] << 8)); }
static void put(uint8_t *r, size_t at, uint16_t v) { r[at] = (uint8_t)v; r[(at + 1u) & MASK] = (uint8_t)(v >> 8); }
static unsigned even(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) == 0; }
/* CMP word a,b: the six arithmetic flags of SUB. */
static void cmp16(cpu_t *c, uint16_t a, uint16_t b) {
    uint16_t r = (uint16_t)(a - b);
    c->flags &= (uint16_t)~0x08D5u;
    if (a < b) c->flags |= 0x01;
    if ((a ^ b ^ r) & 0x10) c->flags |= 0x10;
    if ((a ^ b) & (a ^ r) & 0x8000u) c->flags |= 0x800;
    if (even((uint8_t)r)) c->flags |= 0x04;
    if (r == 0) c->flags |= 0x40;
    if (r & 0x8000u) c->flags |= 0x80;
}
/* The check helper 14B6 is the SHARED model pr_system_filecheck_14b6 (docs/re/SYSTEM_FILECHECK_14B6_CPU.md): one evidence set, composed by every caller. */
static void to_check(pr_system_filecheck_cpu *h, const cpu_t *c) {
    h->ax = c->ax; h->bx = c->bx; h->cx = c->cx; h->dx = c->dx; h->si = c->si; h->di = c->di; h->bp = c->bp; h->sp = c->sp;
    h->cs = c->cs; h->ip = c->ip; h->ds = c->ds; h->es = c->es; h->ss = c->ss; h->flags = c->flags;
}
static void from_check(cpu_t *c, const pr_system_filecheck_cpu *h) {
    c->ax = h->ax; c->bx = h->bx; c->cx = h->cx; c->dx = h->dx; c->si = h->si; c->di = h->di; c->bp = h->bp; c->sp = h->sp;
    c->cs = h->cs; c->ip = h->ip; c->ds = h->ds; c->es = h->es; c->ss = h->ss; c->flags = h->flags;
}
static int same_but_ax_cf(const cpu_t *a, const cpu_t *b) {
    return a->bx == b->bx && a->cx == b->cx && a->dx == b->dx && a->si == b->si && a->di == b->di && a->bp == b->bp && a->sp == b->sp &&
           a->cs == b->cs && a->ds == b->ds && a->es == b->es && a->ss == b->ss && (uint16_t)(a->flags & ~1u) == (uint16_t)(b->flags & ~1u);
}
pr_system_close_status pr_system_close_1491(uint8_t *ram, size_t size, cpu_t *c, pr_system_close_service service, void *context)
{
    cpu_t at_int, after;
    uint16_t entry_sp, return_ip;
    if (!ram || !c || !service || size != PR_SYSTEM_CLOSE_RAM_SIZE || c->ip != 0x1491u || c->sp < 0x0002u || c->sp > 0xFFF8u) return PR_SYSTEM_CLOSE_ARGUMENT;
    entry_sp = c->sp;
    c->bx = c->sp;                                                                         /* 1491 MOV BX,SP */
    c->di = word(ram, physical(c->ss, (uint16_t)(c->bx + 4u)));                            /* 1493 LES DI,SS:[BX+4] */
    c->es = word(ram, physical(c->ss, (uint16_t)(c->bx + 6u)));
    c->sp = (uint16_t)(c->sp - 2u); put(ram, physical(c->ss, c->sp), 0x149Au);             /* 1497 CALL 14B6 */
    {                                                                                      /* 14B6..14C4: the shared check helper (CMP, MOV [4426],67h, RET) */
        pr_system_filecheck_cpu h;
        to_check(&h, c); h.ip = 0x14B6u;
        if (pr_system_filecheck_14b6(ram, size, &h) != PR_SYSTEM_FILECHECK_RETURNED) return PR_SYSTEM_CLOSE_ARGUMENT;   /* unreachable: SP <= FFF6 here */
        from_check(c, &h);
        return_ip = c->ip;                                                                 /* the RET's popped IP, reread after the store */
    }
    if (return_ip != 0x149Au) { c->ip = return_ip; return PR_SYSTEM_CLOSE_TRANSFER; }
    if (!(c->flags & 0x40u)) goto retf4;                                                   /* 149A JNE 14B3 */
    c->bx = word(ram, physical(c->es, c->di));                                             /* 149C MOV BX,ES:[DI] */
    cmp16(c, c->bx, 4u);                                                                   /* 149F CMP BX,4 */
    if (c->flags & 0x41u) goto mark_closed;                                                /* 14A2 JBE 14AD */
    c->ax = (uint16_t)((c->ax & 0x00FFu) | 0x3E00u);                                       /* 14A4 MOV AH,3Eh */
    c->ip = 0x14A6u;                                                                       /* 14A6 INT 21h (declared) */
    at_int = *c;
    if (service(context, &at_int, &after) != 0 || after.ip != 0x14A8u || !same_but_ax_cf(&after, &at_int)) return PR_SYSTEM_CLOSE_SERVICE;
    *c = after;
    if (c->flags & 0x01u) put(ram, physical(c->ds, 0x4426u), c->ax);                       /* 14A8 JAE 14AD not taken; 14AA MOV [4426],AX */
mark_closed:
    put(ram, physical(c->es, (uint16_t)(c->di + 2u)), 0xD7B0u);                            /* 14AD MOV ES:[DI+2],D7B0 */
retf4:
    c->ip = word(ram, physical(c->ss, c->sp)); c->cs = word(ram, physical(c->ss, (uint16_t)(c->sp + 2u)));   /* 14B3 RETF 4 */
    c->sp = (uint16_t)(c->sp + 4u + 4u);
    (void)entry_sp;
    return PR_SYSTEM_CLOSE_RETURNED;
}
