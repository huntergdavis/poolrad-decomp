/* START.EXE resident 0802:025D, open with the "insert disk" prompt loop, raw CPU (docs/re/RES_0802_025D_CPU.md). */
#include "poolrad/recovered/resident_0802_025d_cpu.h"
#include "poolrad/recovered/system_ioresult_assign_cpu.h"
#include "poolrad/recovered/system_reset_1410_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#define MASK (PR_R0802_025D_RAM_SIZE - 1u)
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u
#define ARITH (CF | PF | AF | ZF | SF | OF)
#define CONTINUE (-1)
#define COPY_CPU(d, s) do { (d)->ax = (s)->ax; (d)->bx = (s)->bx; (d)->cx = (s)->cx; (d)->dx = (s)->dx; \
    (d)->si = (s)->si; (d)->di = (s)->di; (d)->bp = (s)->bp; (d)->sp = (s)->sp; (d)->cs = (s)->cs; (d)->ip = (s)->ip; \
    (d)->ds = (s)->ds; (d)->es = (s)->es; (d)->ss = (s)->ss; (d)->flags = (s)->flags; } while (0)

typedef pr_r0802_025d_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)v; ram[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c)
{
    uint16_t v = rd16(ram, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    return v;
}
static unsigned parity(unsigned v)
{
    v &= 0xFFu; v ^= v >> 4u; v ^= v >> 2u; v ^= v >> 1u;
    return (v & 1u) ? 0u : PF;
}
static void cmp_flags(cpu_t *c, unsigned a, unsigned b, unsigned bits)
{
    unsigned mask = (1u << bits) - 1u, top = 1u << (bits - 1u), r = (a - b) & mask;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & top ? SF : 0u) | parity(r) | (a < b ? CF : 0u) |
                          ((a ^ b ^ r) & AF) | (((a ^ b) & (a ^ r) & top) ? OF : 0u));
}
static void push_lea(uint8_t *ram, cpu_t *c, uint16_t displacement)
{
    c->di = (uint16_t)(c->bp - displacement);
    push(ram, c, c->ss); push(ram, c, c->di);
}
static void les_bp(const uint8_t *ram, cpu_t *c, uint16_t displacement)
{
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + displacement));
    c->es = rd16(ram, c->ss, (uint16_t)(c->bp + displacement + 2u));
}
static uint8_t byte_at(const uint8_t *ram, const cpu_t *c, uint16_t off) { return ram[physical(c->ss, off)]; }

static int far_call(uint8_t *ram, size_t size, cpu_t *c, const pr_r0802_02eb_env *env, uint16_t at, pr_r0802_025d_diagnostic *d)
{
    uint16_t home = c->cs, target = rd16(ram, c->cs, (uint16_t)(at + 1u)), seg = rd16(ram, c->cs, (uint16_t)(at + 3u));
    int status = 0, ok = 0;
    push(ram, c, c->cs); push(ram, c, (uint16_t)(at + 5u));
    c->cs = seg; c->ip = target;
    d->site = at; d->callee_status = -1;
    if (target == 0x0B25u) {
        pr_system_0b25_cpu m; COPY_CPU(&m, c); status = pr_system_strstore_0b25(ram, size, &m); COPY_CPU(c, &m);
        ok = status == PR_SYSTEM_0B25_RETURNED;
    } else if (target == 0x13E2u) {
        pr_system_leaf_cpu m; COPY_CPU(&m, c); status = pr_system_assign_13e2(ram, size, &m); COPY_CPU(c, &m);
        ok = status == PR_SYSTEM_LEAF_RETURNED;
    } else if (target == 0x1410u) {
        pr_reset_cpu m; COPY_CPU(&m, c); status = pr_system_reset_1410(ram, size, &m, env->open, env->open_context);
        if (status == PR_RESET_CLOSE) {                                /* the nested Close, then Reset resumed at 1445h */
            pr_system_close_cpu k; int cs_;
            COPY_CPU(&k, &m);
            cs_ = pr_system_close_1491(ram, size, &k, env->close, env->close_context);
            if (cs_ == PR_SYSTEM_CLOSE_SERVICE) { COPY_CPU(c, &k); d->callee_status = cs_; return PR_R0802_025D_SERVICE; }
            if (cs_ != PR_SYSTEM_CLOSE_RETURNED || k.cs != m.cs || k.ip != 0x1445u) { COPY_CPU(c, &k); d->callee_status = cs_;
                return PR_R0802_025D_CALLEE; }
            COPY_CPU(&m, &k);
            status = pr_system_reset_1410(ram, size, &m, env->open, env->open_context);
        }
        COPY_CPU(c, &m);
        if (status == PR_RESET_SERVICE) { d->callee_status = status; return PR_R0802_025D_SERVICE; }
        ok = status == PR_RESET_RETURNED;
    } else {
        return PR_R0802_025D_CALLEE;                                   /* unreachable: the code is immutable */
    }
    d->callee_status = status;
    if (!ok || c->cs != home || c->ip != (uint16_t)(at + 5u))
        return PR_R0802_025D_CALLEE;
    d->site = 0; d->callee_status = -1;
    return CONTINUE;
}

#define FAR(at) do { int s_ = far_call(ram, ram_size, c, env, (at), d); if (s_ != CONTINUE) return (pr_r0802_025d_status)s_; } while (0)

pr_r0802_025d_status pr_r0802_025d(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_02eb_env *env,
                                   unsigned max_retries, pr_r0802_025d_diagnostic *diagnostic)
{
    pr_r0802_025d_diagnostic local, *d = diagnostic ? diagnostic : &local;
    unsigned retries = 0;
    if (ram == NULL || c == NULL || env == NULL || env->open == NULL || env->close == NULL || ram_size != PR_R0802_025D_RAM_SIZE)
        return PR_R0802_025D_ARGUMENT;
    if (c->ip == 0x025Du) {
        if (c->sp < 0x0200u || c->sp > 0xFFEEu)
            return PR_R0802_025D_ARGUMENT;
    } else if (c->ip != 0x02ADu) {
        return PR_R0802_025D_ARGUMENT;
    }
    d->site = 0; d->callee_status = -1;
    if (c->ip == 0x02ADu)
        goto after_prompt;
    push(ram, c, c->bp); c->bp = c->sp;                               /* 025D PUSH BP; MOV BP,SP */
    { unsigned a = c->sp; c->sp = (uint16_t)(a - 0x54u); cmp_flags(c, a, 0x54u, 16u); }       /* 0260 SUB SP,54h */
    les_bp(ram, c, 0x10u); push(ram, c, c->es); push(ram, c, c->di);  /* 0263 name */
    push_lea(ram, c, 0x2Au); c->ax = 0x28u; push(ram, c, c->ax);
    FAR(0x0271);                                                      /* StrStore */
    les_bp(ram, c, 0x0Cu); push(ram, c, c->es); push(ram, c, c->di);  /* 0276 message */
    push_lea(ram, c, 0x53u); c->ax = 0x28u; push(ram, c, c->ax);
    FAR(0x0284);                                                      /* StrStore */
    for (;;) {
        pr_r0802_02eb_cpu m; int s;
        push_lea(ram, c, 0x2Au);                                      /* 0289 name */
        push(ram, c, c->cs); push(ram, c, 0x0292u);                   /* 028E PUSH CS; CALL 02EB */
        c->ip = 0x02EBu; COPY_CPU(&m, c);
        s = pr_r0802_02eb(ram, ram_size, &m, env, d);
        COPY_CPU(c, &m);
        if (s == PR_R0802_02EB_SERVICE) return PR_R0802_025D_SERVICE;
        if (s != PR_R0802_02EB_RETURNED || c->ip != 0x0292u) { d->site = 0x028Fu; d->callee_status = s; return PR_R0802_025D_CALLEE; }
        d->site = 0; d->callee_status = -1;
        ram[physical(c->ss, (uint16_t)(c->bp - 0x54u))] = (uint8_t)c->ax;          /* 0292 MOV [BP-54h],AL */
        cmp_flags(c, byte_at(ram, c, (uint16_t)(c->bp - 0x54u)), 0u, 8u);           /* 0295 CMP BYTE [BP-54h],0 */
        if ((c->flags & ZF) != 0u) {                                  /* 0299 JNE 02AD */
            cmp_flags(c, byte_at(ram, c, (uint16_t)(c->bp + 0x0Au)), 0u, 8u);       /* 029B CMP BYTE [BP+0Ah],0 */
            if ((c->flags & ZF) != 0u) {                              /* 029F JNE 02AD */
                c->ax = (uint16_t)((c->ax & 0xFF00u) | 0x0Eu); push(ram, c, c->ax); /* 02A1 MOV AL,0Eh; PUSH AX */
                push_lea(ram, c, 0x53u);                              /* 02A4 the message */
                push(ram, c, c->cs); push(ram, c, 0x02ADu);           /* 02A9 PUSH CS; CALL 0193 */
                c->ip = 0x0193u; d->site = 0x02AAu;
                return PR_R0802_025D_PROMPT;
            }
        }
after_prompt:
        cmp_flags(c, byte_at(ram, c, (uint16_t)(c->bp - 0x54u)), 0u, 8u);           /* 02AD CMP BYTE [BP-54h],0 */
        if ((c->flags & ZF) == 0u) break;                             /* 02B1 JNE 02B9 */
        cmp_flags(c, byte_at(ram, c, (uint16_t)(c->bp + 0x0Au)), 0u, 8u);           /* 02B3 CMP BYTE [BP+0Ah],0 */
        if ((c->flags & ZF) == 0u) break;                             /* 02B7 JE 0289 */
        if (retries++ >= max_retries) {                               /* the loop-back past its budget: stop at the JE */
            c->ip = 0x02B7u; d->site = 0x02B7u; d->callee_status = -1;
            return PR_R0802_025D_LIMIT;
        }
    }
    cmp_flags(c, byte_at(ram, c, (uint16_t)(c->bp - 0x54u)), 0u, 8u);               /* 02B9 CMP BYTE [BP-54h],0 */
    if ((c->flags & ZF) == 0u) {                                      /* 02BD JE 02DC */
        les_bp(ram, c, 0x06u); push(ram, c, c->es); push(ram, c, c->di);            /* 02BF f */
        push_lea(ram, c, 0x2Au);                                      /* 02C4 name */
        FAR(0x02C9);                                                  /* Assign */
        les_bp(ram, c, 0x06u); push(ram, c, c->es); push(ram, c, c->di);            /* 02CE f */
        c->ax = 1u; push(ram, c, c->ax);                              /* 02D3 MOV AX,1; PUSH AX */
        FAR(0x02D7);                                                  /* Reset */
    }
    c->ax = (uint16_t)((c->ax & 0xFF00u) | byte_at(ram, c, (uint16_t)(c->bp - 0x54u)));   /* 02DC MOV AL,[BP-54h] */
    ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)c->ax;    /* 02DF MOV [BP-1],AL; 02E2 MOV AL,[BP-1] */
    c->sp = c->bp; c->bp = pop(ram, c);                               /* 02E5 MOV SP,BP; POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 02E8 RETF 0Eh */
    c->sp = (uint16_t)(c->sp + 0x0Eu);
    return PR_R0802_025D_RETURNED;
}
