/* START.EXE resident 0802:02EB, FileExists, raw CPU (docs/re/RES_0802_02EB_CPU.md). */
#include "poolrad/recovered/resident_0802_02eb_cpu.h"
#include "poolrad/recovered/system_ioresult_assign_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#define MASK (PR_R0802_02EB_RAM_SIZE - 1u)
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

typedef pr_r0802_02eb_cpu cpu_t;

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
static void cmp_flags(cpu_t *c, unsigned a, unsigned b, unsigned bits)          /* CMP of width bits */
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

static int far_call(uint8_t *ram, size_t size, cpu_t *c, const pr_r0802_02eb_env *env, uint16_t at, pr_r0802_02eb_diagnostic *d)
{
    uint16_t home = c->cs, target = rd16(ram, c->cs, (uint16_t)(at + 1u)), seg = rd16(ram, c->cs, (uint16_t)(at + 3u));
    int status = 0, ok = 0;
    push(ram, c, c->cs); push(ram, c, (uint16_t)(at + 5u));
    c->cs = seg; c->ip = target;
    d->site = at; d->callee_status = -1;
    switch (target) {
    case 0x0B25: { pr_system_0b25_cpu m; COPY_CPU(&m, c); status = pr_system_strstore_0b25(ram, size, &m); COPY_CPU(c, &m);
        ok = status == PR_SYSTEM_0B25_RETURNED; break; }
    case 0x13E2: case 0x04A2: { pr_system_leaf_cpu m; COPY_CPU(&m, c);
        status = target == 0x13E2 ? pr_system_assign_13e2(ram, size, &m) : pr_system_ioresult_04a2(ram, size, &m);
        COPY_CPU(c, &m); ok = status == PR_SYSTEM_LEAF_RETURNED; break; }
    case 0x1491: { pr_system_close_cpu m; COPY_CPU(&m, c); status = pr_system_close_1491(ram, size, &m, env->close, env->close_context);
        COPY_CPU(c, &m);
        if (status == PR_SYSTEM_CLOSE_SERVICE) { d->callee_status = status; return PR_R0802_02EB_SERVICE; }
        ok = status == PR_SYSTEM_CLOSE_RETURNED; break; }
    case 0x1410: { pr_reset_cpu m; COPY_CPU(&m, c); status = pr_system_reset_1410(ram, size, &m, env->open, env->open_context);
        if (status == PR_RESET_CLOSE) {                               /* the nested Close, then Reset resumed at 1445h */
            pr_system_close_cpu k; COPY_CPU(&k, &m);
            int cs_ = pr_system_close_1491(ram, size, &k, env->close, env->close_context);
            if (cs_ == PR_SYSTEM_CLOSE_SERVICE) { COPY_CPU(c, &k); d->callee_status = cs_; return PR_R0802_02EB_SERVICE; }
            if (cs_ != PR_SYSTEM_CLOSE_RETURNED || k.cs != m.cs || k.ip != 0x1445u) { COPY_CPU(c, &k); d->callee_status = cs_;
                return PR_R0802_02EB_CALLEE; }
            COPY_CPU(&m, &k);
            status = pr_system_reset_1410(ram, size, &m, env->open, env->open_context);
        }
        COPY_CPU(c, &m);
        if (status == PR_RESET_SERVICE) { d->callee_status = status; return PR_R0802_02EB_SERVICE; }
        ok = status == PR_RESET_RETURNED; break; }
    default:
        return PR_R0802_02EB_CALLEE;                                  /* unreachable: the code is immutable */
    }
    d->callee_status = status;
    if (!ok || c->cs != home || c->ip != (uint16_t)(at + 5u))
        return PR_R0802_02EB_CALLEE;
    d->site = 0; d->callee_status = -1;
    return CONTINUE;
}

#define FAR(at) do { int s_ = far_call(ram, ram_size, c, env, (at), d); if (s_ != CONTINUE) return (pr_r0802_02eb_status)s_; } while (0)

pr_r0802_02eb_status pr_r0802_02eb(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_02eb_env *env,
                                   pr_r0802_02eb_diagnostic *diagnostic)
{
    pr_r0802_02eb_diagnostic local, *d = diagnostic ? diagnostic : &local;
    if (ram == NULL || c == NULL || env == NULL || env->open == NULL || env->close == NULL || ram_size != PR_R0802_02EB_RAM_SIZE ||
        c->ip != 0x02EBu || c->sp < 0x0100u || c->sp > 0xFFF8u)
        return PR_R0802_02EB_ARGUMENT;
    d->site = 0; d->callee_status = -1;
    push(ram, c, c->bp); c->bp = c->sp;                               /* 02EB PUSH BP; MOV BP,SP */
    { unsigned a = c->sp; c->sp = (uint16_t)(a - 0xAAu); cmp_flags(c, a, 0xAAu, 16u); }       /* 02EE SUB SP,0AAh */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 6u)); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));   /* 02F2 LES DI,[BP+6] */
    push(ram, c, c->es); push(ram, c, c->di);
    push_lea(ram, c, 0x2Au); c->ax = 0x28u; push(ram, c, c->ax);      /* 02F7 [BP-2Ah], 40 */
    FAR(0x0300);                                                      /* StrStore */
    push_lea(ram, c, 0xAAu); push_lea(ram, c, 0x2Au);                 /* 0305 f, name */
    FAR(0x0310);                                                      /* Assign */
    push_lea(ram, c, 0xAAu); c->ax = 0x80u; push(ram, c, c->ax);      /* 0315 f, 128 */
    FAR(0x031F);                                                      /* Reset */
    push_lea(ram, c, 0xAAu);                                          /* 0324 f */
    FAR(0x032A);                                                      /* Close */
    FAR(0x032F);                                                      /* IOResult */
    wr16(ram, c->ds, 0x49CEu, c->ax);                                 /* 0334 MOV [49CE],AX */
    cmp_flags(c, rd16(ram, c->ds, 0x49CEu), 0u, 16u);                 /* 0337 CMP WORD [49CE],0 */
    {   int exists = 0;
        if ((c->flags & ZF) != 0u) {                                  /* 033C JNE 0344 */
            uint8_t len = ram[physical(c->ss, (uint16_t)(c->bp - 0x2Au))];
            cmp_flags(c, len, 0u, 8u);                                /* 033E CMP BYTE [BP-2Ah],0 */
            exists = len != 0u;                                       /* 0342 JNE 0348 */
        }
        c->ax = (uint16_t)((c->ax & 0xFF00u) | (exists ? 1u : 0u));  /* 0344 MOV AL,0 / 0348 MOV AL,1 */
    }
    ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)c->ax;    /* 034A MOV [BP-1],AL; 034D MOV AL,[BP-1] */
    c->sp = c->bp; c->bp = pop(ram, c);                               /* 0350 MOV SP,BP; POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 0353 RETF 4 */
    c->sp = (uint16_t)(c->sp + 4u);
    return PR_R0802_02EB_RETURNED;
}
