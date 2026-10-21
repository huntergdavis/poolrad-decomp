/* START.EXE resident 0802:050F, the block decompressor, raw CPU (docs/re/RES_0802_050F_CPU.md). */
#include "poolrad/recovered/resident_0802_050f_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"

#define MASK (PR_R0802_050F_RAM_SIZE - 1u)
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u
#define ARITH (CF | PF | AF | ZF | SF | OF)
#define COPY_CPU(d, s) do { (d)->ax = (s)->ax; (d)->bx = (s)->bx; (d)->cx = (s)->cx; (d)->dx = (s)->dx; \
    (d)->si = (s)->si; (d)->di = (s)->di; (d)->bp = (s)->bp; (d)->sp = (s)->sp; (d)->cs = (s)->cs; (d)->ip = (s)->ip; \
    (d)->ds = (s)->ds; (d)->es = (s)->es; (d)->ss = (s)->ss; (d)->flags = (s)->flags; } while (0)

typedef pr_r0802_050f_cpu cpu_t;

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
/* SUB/CMP of width bits (8 or 16); writes all six arithmetic flags. */
static void sub_flags(cpu_t *c, unsigned a, unsigned b, unsigned bits)
{
    unsigned mask = (1u << bits) - 1u, top = 1u << (bits - 1u), r = (a - b) & mask;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & top ? SF : 0u) | parity(r) | (a < b ? CF : 0u) |
                          ((a ^ b ^ r) & AF) | (((a ^ b) & (a ^ r) & top) ? OF : 0u));
}
static uint16_t add16(cpu_t *c, unsigned a, unsigned b)
{
    unsigned full = a + b, r = full & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & 0x8000u ? SF : 0u) | parity(r) |
                          (full > 0xFFFFu ? CF : 0u) | ((a ^ b ^ r) & AF) | ((~(a ^ b) & (a ^ r) & 0x8000u) ? OF : 0u));
    return (uint16_t)r;
}
/* INC r16: as ADD 1 but CF is kept. */
static uint16_t inc16(cpu_t *c, unsigned a)
{
    uint16_t keep = (uint16_t)(c->flags & CF), r = add16(c, a, 1u);
    c->flags = (uint16_t)((c->flags & ~CF) | keep);
    return r;
}
/* NEG r16: 0 - a; CF := a != 0. */
static uint16_t neg16(cpu_t *c, unsigned a)
{
    unsigned r = (0u - a) & 0xFFFFu;
    sub_flags(c, 0u, a, 16u);
    return (uint16_t)r;
}
static void logic_flags(cpu_t *c, unsigned r, unsigned bits)  /* XOR: CF = OF = AF = 0 */
{
    unsigned top = 1u << (bits - 1u);
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & top ? SF : 0u) | parity(r));
}
static void cbw(cpu_t *c) { c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; }
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static int jl(const cpu_t *c) { return ((c->flags & SF) != 0u) != ((c->flags & OF) != 0u); }
static int jg(const cpu_t *c) { return (c->flags & ZF) == 0u && !jl(c); }
static uint8_t bp_byte(const uint8_t *ram, const cpu_t *c, uint16_t displacement) { return ram[physical(c->ss, (uint16_t)(c->bp - displacement))]; }
static uint16_t bp_word(const uint8_t *ram, const cpu_t *c, uint16_t displacement) { return rd16(ram, c->ss, (uint16_t)(c->bp - displacement)); }
/* LES DI,[BP+d]; LES DI,ES:[DI]; ADD DI,AX: ES:DI := the pointer variable's value with AX added to its offset. */
static void deref_add(const uint8_t *ram, cpu_t *c, uint16_t displacement)
{
    uint16_t vo = rd16(ram, c->ss, (uint16_t)(c->bp + displacement)), vs = rd16(ram, c->ss, (uint16_t)(c->bp + displacement + 2u));
    c->di = rd16(ram, vs, vo); c->es = rd16(ram, vs, (uint16_t)(vo + 2u));
    c->di = add16(c, c->di, c->ax);
}
static int same_cpu(const cpu_t *a, const cpu_t *b)
{
    return a->ax == b->ax && a->bx == b->bx && a->cx == b->cx && a->dx == b->dx && a->si == b->si && a->di == b->di &&
           a->bp == b->bp && a->sp == b->sp && a->cs == b->cs && a->ip == b->ip && a->ds == b->ds && a->es == b->es &&
           a->ss == b->ss && a->flags == b->flags;
}

/* LCALL at `at` into Move (1685) or FillChar (16A8): live segment word; the return must be at+5 in this segment. */
static int far_call(uint8_t *ram, size_t size, cpu_t *c, uint16_t at, pr_r0802_050f_diagnostic *d)
{
    uint16_t home = c->cs, target = rd16(ram, c->cs, (uint16_t)(at + 1u)), seg = rd16(ram, c->cs, (uint16_t)(at + 3u));
    pr_system_leaf_cpu m; int status;
    push(ram, c, c->cs); push(ram, c, (uint16_t)(at + 5u));
    c->cs = seg; c->ip = target;
    COPY_CPU(&m, c);
    if (target == 0x1685u) status = pr_system_move_1685(ram, size, &m);
    else if (target == 0x16A8u) status = pr_system_fillchar_16a8(ram, size, &m);
    else { d->site = at; return 0; }                                  /* unreachable: the code is immutable */
    COPY_CPU(c, &m);
    if (status != PR_SYSTEM_LEAF_RETURNED || c->cs != home || c->ip != (uint16_t)(at + 5u)) {
        d->site = at; d->callee_status = status; return 0;
    }
    return 1;
}

pr_r0802_050f_status pr_r0802_050f(uint8_t *ram, size_t ram_size, cpu_t *c, uint32_t max_arrivals,
                                   pr_r0802_050f_diagnostic *diagnostic)
{
    pr_r0802_050f_diagnostic local, *d = diagnostic ? diagnostic : &local;
    cpu_t prev;
    int quiet = 0;                                                    /* the last iteration: no copy path, [BP-5] unchanged */
    if (ram == NULL || c == NULL || ram_size != PR_R0802_050F_RAM_SIZE || c->ip != 0x050Fu || c->sp < 0x0016u || c->sp > 0xFFECu)
        return PR_R0802_050F_ARGUMENT;
    d->site = 0; d->callee_status = -1; d->arrivals = 0;
    push(ram, c, c->bp); c->bp = c->sp;                               /* 050F PUSH BP; MOV BP,SP */
    { unsigned a = c->sp; c->sp = (uint16_t)(a - 6u); sub_flags(c, a, 6u, 16u); }            /* 0512 SUB SP,6 */
    c->ax = 0u; logic_flags(c, 0u, 16u); wr16(ram, c->ss, (uint16_t)(c->bp - 2u), 0u);       /* 0515 in := 0 */
    c->ax = 0u; logic_flags(c, 0u, 16u); wr16(ram, c->ss, (uint16_t)(c->bp - 4u), 0u);       /* 051A out := 0 */
    COPY_CPU(&prev, c);
    for (;;) {
        uint8_t old5, ctl;
        c->ip = 0x051Fu;
        ++d->arrivals;
        if (d->arrivals > 1u && quiet && same_cpu(c, &prev))
            return PR_R0802_050F_STALLED;
        if (d->arrivals > max_arrivals)
            return PR_R0802_050F_BUDGET;
        COPY_CPU(&prev, c);
        quiet = 0;
        c->ax = bp_word(ram, c, 2u);                                  /* 051F MOV AX,[BP-2] */
        deref_add(ram, c, 0x12u);                                     /* 0522 packed^ + in */
        set_al(c, ram[physical(c->es, c->di)]);                       /* 052A MOV AL,ES:[DI] */
        old5 = bp_byte(ram, c, 5u);
        ram[physical(c->ss, (uint16_t)(c->bp - 5u))] = (uint8_t)c->ax;                       /* 052D MOV [BP-5],AL */
        ctl = bp_byte(ram, c, 5u); set_al(c, ctl);                    /* 0530 MOV AL,[BP-5] */
        sub_flags(c, ctl, 0u, 8u);                                    /* 0533 CMP AL,0 */
        if (!jl(c) && (sub_flags(c, ctl, 0x7Eu, 8u), !jg(c))) {       /* 0535 JL 0574; 0537 CMP AL,7Eh; 0539 JG 0574 */
            c->ax = inc16(c, bp_word(ram, c, 2u));                    /* 053B MOV AX,[BP-2]; INC AX */
            deref_add(ram, c, 0x12u); push(ram, c, c->es); push(ram, c, c->di);               /* 053F source */
            c->ax = bp_word(ram, c, 4u);                              /* 0549 MOV AX,[BP-4] */
            deref_add(ram, c, 0x0Eu); push(ram, c, c->es); push(ram, c, c->di);               /* 054C destination */
            set_al(c, bp_byte(ram, c, 5u)); cbw(c); c->ax = inc16(c, c->ax); push(ram, c, c->ax);   /* 0556 count */
            if (!far_call(ram, ram_size, c, 0x055Cu, d)) return PR_R0802_050F_CALLEE;        /* Move */
            set_al(c, bp_byte(ram, c, 5u)); cbw(c); c->ax = inc16(c, c->ax); c->ax = inc16(c, c->ax);
            wr16(ram, c->ss, (uint16_t)(c->bp - 2u), add16(c, bp_word(ram, c, 2u), c->ax));   /* 0567 ADD [BP-2],AX */
            set_al(c, bp_byte(ram, c, 5u)); cbw(c); c->ax = inc16(c, c->ax);
            wr16(ram, c->ss, (uint16_t)(c->bp - 4u), add16(c, bp_word(ram, c, 4u), c->ax));   /* 056F ADD [BP-4],AX; JMP 05B7 */
        } else {
            sub_flags(c, (uint8_t)c->ax, 0x81u, 8u);                  /* 0574 CMP AL,81h */
            if (!jl(c) && (sub_flags(c, (uint8_t)c->ax, 0xFFu, 8u), !jg(c))) {               /* 0576 JL; 0578 CMP AL,0FFh; 057A JG */
                set_al(c, bp_byte(ram, c, 5u)); cbw(c); c->ax = neg16(c, c->ax);             /* 057C MOV AL,[BP-5]; CBW; NEG AX */
                ram[physical(c->ss, (uint16_t)(c->bp - 5u))] = (uint8_t)c->ax;               /* 0582 MOV [BP-5],AL */
                c->ax = bp_word(ram, c, 4u);                          /* 0585 MOV AX,[BP-4] */
                deref_add(ram, c, 0x0Eu); push(ram, c, c->es); push(ram, c, c->di);           /* 0588 destination */
                set_al(c, bp_byte(ram, c, 5u)); cbw(c); push(ram, c, c->ax);                 /* 0592 count */
                c->ax = inc16(c, bp_word(ram, c, 2u));                /* 0597 MOV AX,[BP-2]; INC AX */
                deref_add(ram, c, 0x12u);
                set_al(c, ram[physical(c->es, c->di)]); push(ram, c, c->ax);                 /* 05A3 the value */
                if (!far_call(ram, ram_size, c, 0x05A7u, d)) return PR_R0802_050F_CALLEE;    /* FillChar */
                wr16(ram, c->ss, (uint16_t)(c->bp - 2u), add16(c, bp_word(ram, c, 2u), 2u)); /* 05AC ADD WORD [BP-2],2 */
                set_al(c, bp_byte(ram, c, 5u)); cbw(c);
                wr16(ram, c->ss, (uint16_t)(c->bp - 4u), add16(c, bp_word(ram, c, 4u), c->ax));   /* 05B4 ADD [BP-4],AX */
            } else {
                quiet = old5 == ctl;                                  /* 7Fh / 80h: no copy; only [BP-5] was written */
            }
        }
        c->ax = bp_word(ram, c, 2u);                                  /* 05B7 MOV AX,[BP-2] */
        c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Au)); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Cu));   /* LES DI,[BP+0Ah] */
        sub_flags(c, c->ax, rd16(ram, c->es, c->di), 16u);            /* 05BD CMP AX,ES:[DI] */
        if ((c->flags & CF) == 0u)                                    /* 05C0 JAE 05C5 */
            break;
    }
    c->sp = c->bp; c->bp = pop(ram, c);                               /* 05C5 MOV SP,BP; POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 05C8 RETF 10h */
    c->sp = (uint16_t)(c->sp + 0x10u);
    return PR_R0802_050F_RETURNED;
}
