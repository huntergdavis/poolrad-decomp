/* START.EXE resident 0802:05CD, open the archive pair through 025D and position it, raw CPU (docs/re/RES_0802_05CD_CPU.md). */
#include "poolrad/recovered/resident_0802_05cd_cpu.h"
#include "poolrad/recovered/system_iocheck_04a9_cpu.h"
#include "poolrad/recovered/system_ioresult_assign_cpu.h"
#include "poolrad/recovered/system_reset_1410_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#define MASK (PR_R0802_05CD_RAM_SIZE - 1u)
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

typedef pr_r0802_05cd_cpu cpu_t;

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
/* SUB of 16 bits; writes all six arithmetic flags. */
static void sub_flags(cpu_t *c, unsigned a, unsigned b)
{
    unsigned r = (a - b) & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & 0x8000u ? SF : 0u) | parity(r) | (a < b ? CF : 0u) |
                          ((a ^ b ^ r) & AF) | (((a ^ b) & (a ^ r) & 0x8000u) ? OF : 0u));
}
/* ADD of 16 bits. */
static uint16_t add16(cpu_t *c, unsigned a, unsigned b)
{
    unsigned full = a + b, r = full & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & 0x8000u ? SF : 0u) | parity(r) |
                          (full > 0xFFFFu ? CF : 0u) | ((a ^ b ^ r) & AF) | ((~(a ^ b) & (a ^ r) & 0x8000u) ? OF : 0u));
    return (uint16_t)r;
}
/* OR/XOR (logic): CF = OF = AF = 0 (as both oracles set AF), SF/ZF/PF from the result of width bits. */
static void logic_flags(cpu_t *c, unsigned r, unsigned bits)
{
    unsigned top = 1u << (bits - 1u);
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & top ? SF : 0u) | parity(r));
}
static void push_lea(uint8_t *ram, cpu_t *c, uint16_t displacement)   /* LEA DI,[BP-d]; PUSH SS; PUSH DI */
{
    c->di = (uint16_t)(c->bp - displacement);
    push(ram, c, c->ss); push(ram, c, c->di);
}
static void les_bp(const uint8_t *ram, cpu_t *c, uint16_t displacement)  /* LES DI,[BP+d] */
{
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + displacement));
    c->es = rd16(ram, c->ss, (uint16_t)(c->bp + displacement + 2u));
}
static void push_far_arg(uint8_t *ram, cpu_t *c, uint16_t displacement)  /* LES DI,[BP+d]; PUSH ES; PUSH DI */
{
    les_bp(ram, c, displacement); push(ram, c, c->es); push(ram, c, c->di);
}
static uint8_t ds_byte(const uint8_t *ram, const cpu_t *c, uint16_t off) { return ram[physical(c->ds, off)]; }

/* LCALL at `at`: pushes the return frame, takes the live segment word, runs the composed SYSTEM model, and requires the
 * return to at+5 in the caller's segment. Returns CONTINUE, or a status to stop with. */
static int far_call(uint8_t *ram, size_t size, cpu_t *c, const pr_r0802_05cd_env *env, uint16_t at, pr_r0802_05cd_diagnostic *d)
{
    uint16_t home = c->cs, target = rd16(ram, c->cs, (uint16_t)(at + 1u)), seg = rd16(ram, c->cs, (uint16_t)(at + 3u));
    int status = 0, ok = 0;
    push(ram, c, c->cs); push(ram, c, (uint16_t)(at + 5u));
    c->cs = seg; c->ip = target;
    d->site = at; d->callee_status = -1;
    switch (target) {
    case 0x0B25: { pr_system_0b25_cpu m; COPY_CPU(&m, c); status = pr_system_strstore_0b25(ram, size, &m); COPY_CPU(c, &m);
        ok = status == PR_SYSTEM_0B25_RETURNED; break; }
    case 0x0B0B: case 0x0B98: case 0x0C3B: case 0x13E2: { pr_system_leaf_cpu m; COPY_CPU(&m, c);
        status = target == 0x0B0B ? pr_system_strload_0b0b(ram, size, &m) : target == 0x0B98 ? pr_system_strcat_0b98(ram, size, &m)
               : target == 0x0C3B ? pr_system_charstr_0c3b(ram, size, &m) : pr_system_assign_13e2(ram, size, &m);
        COPY_CPU(c, &m); ok = status == PR_SYSTEM_LEAF_RETURNED; break; }
    case 0x04A9: { pr_system_iocheck_cpu m; COPY_CPU(&m, c); status = pr_system_iocheck_04a9(ram, size, &m); COPY_CPU(c, &m);
        if (status == PR_SYSTEM_IOCHECK_RUNERROR) { d->callee_status = status; return PR_R0802_05CD_RUNERROR; }
        ok = status == PR_SYSTEM_IOCHECK_RETURNED; break; }
    case 0x1410: { pr_reset_cpu m; COPY_CPU(&m, c); status = pr_system_reset_1410(ram, size, &m, env->file.open, env->file.open_context);
        if (status == PR_RESET_CLOSE) {                                /* the nested Close, then Reset resumed at 1445h */
            pr_system_close_cpu k; int cs_;
            COPY_CPU(&k, &m);
            cs_ = pr_system_close_1491(ram, size, &k, env->file.close, env->file.close_context);
            if (cs_ == PR_SYSTEM_CLOSE_SERVICE) { COPY_CPU(c, &k); d->callee_status = cs_; return PR_R0802_05CD_SERVICE; }
            if (cs_ != PR_SYSTEM_CLOSE_RETURNED || k.cs != m.cs || k.ip != 0x1445u) { COPY_CPU(c, &k); d->callee_status = cs_;
                return PR_R0802_05CD_CALLEE; }
            COPY_CPU(&m, &k);
            status = pr_system_reset_1410(ram, size, &m, env->file.open, env->file.open_context);
        }
        COPY_CPU(c, &m);
        if (status == PR_RESET_SERVICE) { d->callee_status = status; return PR_R0802_05CD_SERVICE; }
        ok = status == PR_RESET_RETURNED; break; }
    case 0x14FB: { pr_blockread_cpu m; COPY_CPU(&m, c); status = pr_system_blockread_14fb(ram, size, &m, env->read, env->read_context);
        COPY_CPU(c, &m); d->callee_status = status;
        if (status == PR_BLOCKREAD_DIVIDE) return PR_R0802_05CD_DIVIDE;
        if (status == PR_BLOCKREAD_SERVICE) return PR_R0802_05CD_SERVICE;
        ok = status == PR_BLOCKREAD_RETURNED; break; }
    case 0x1563: { pr_seek_cpu m; COPY_CPU(&m, c); status = pr_system_seek_1563(ram, size, &m, env->seek, env->seek_context);
        COPY_CPU(c, &m); d->callee_status = status;
        if (status == PR_SEEK_SERVICE) return PR_R0802_05CD_SERVICE;
        ok = status == PR_SEEK_RETURNED; break; }
    default:
        return PR_R0802_05CD_CALLEE;                                  /* unreachable: the code is immutable */
    }
    d->callee_status = status;
    if (!ok || c->cs != home || c->ip != (uint16_t)(at + 5u))
        return PR_R0802_05CD_CALLEE;
    d->site = 0; d->callee_status = -1;
    return CONTINUE;
}

#define FAR(at) do { int s_ = far_call(ram, ram_size, c, env, (at), d); if (s_ != CONTINUE) return (pr_r0802_05cd_status)s_; } while (0)

/* 025D from its entry (or its 02ADh resume), composed; it must come back to 065Ch in this body's segment. */
static int open_025d(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_05cd_env *env, pr_r0802_05cd_diagnostic *d)
{
    uint16_t home = c->cs;
    pr_r0802_025d_cpu m; int s;
    COPY_CPU(&m, c);
    s = pr_r0802_025d(ram, ram_size, &m, &env->file, env->retries, d);
    COPY_CPU(c, &m);
    if (s == PR_R0802_025D_PROMPT) return PR_R0802_05CD_PROMPT;
    if (s == PR_R0802_025D_LIMIT) return PR_R0802_05CD_LIMIT;
    if (s == PR_R0802_025D_SERVICE) return PR_R0802_05CD_SERVICE;
    if (s != PR_R0802_025D_RETURNED || c->cs != home || c->ip != 0x065Cu) { d->site = 0x0659u; d->callee_status = s; return PR_R0802_05CD_CALLEE; }
    d->site = 0; d->callee_status = -1;
    return CONTINUE;
}

pr_r0802_05cd_status pr_r0802_05cd(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_05cd_env *env,
                                   pr_r0802_05cd_diagnostic *diagnostic)
{
    pr_r0802_05cd_diagnostic local, *d = diagnostic ? diagnostic : &local;
    int s;
    if (ram == NULL || c == NULL || env == NULL || env->file.open == NULL || env->file.close == NULL || env->read == NULL ||
        env->seek == NULL || ram_size != PR_R0802_05CD_RAM_SIZE)
        return PR_R0802_05CD_ARGUMENT;
    if (c->ip == 0x05CDu) {
        if (c->sp < 0x066Au || c->sp > 0xFFE8u)
            return PR_R0802_05CD_ARGUMENT;
    } else if (c->ip != 0x02ADu) {
        return PR_R0802_05CD_ARGUMENT;
    }
    d->site = 0; d->callee_status = -1;
    if (c->ip == 0x02ADu)
        goto in_025d;
    push(ram, c, c->bp); c->bp = c->sp;                               /* 05CD PUSH BP; MOV BP,SP */
    { unsigned a = c->sp; c->sp = (uint16_t)(a - 0x456u); sub_flags(c, a, 0x456u); }         /* 05D0 SUB SP,456h */
    push_far_arg(ram, c, 0x16u); push_lea(ram, c, 0x2Au); c->ax = 0x28u; push(ram, c, c->ax);
    FAR(0x05E2);                                                      /* StrStore base */
    push_far_arg(ram, c, 0x12u); push_lea(ram, c, 0x53u); c->ax = 0x28u; push(ram, c, c->ax);
    FAR(0x05F5);                                                      /* StrStore extension */
    push_lea(ram, c, 0x155u); push_lea(ram, c, 0x2Au);
    FAR(0x0605);                                                      /* load base into [BP-155h] */
    push_lea(ram, c, 0x53u);
    FAR(0x060F);                                                      /* + extension */
    push_lea(ram, c, 0x355u);                                         /* 0614 */
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ds_byte(ram, c, 0x5376u)); /* 061A MOV AL,[5376] */
    c->ax &= 0x00FFu; logic_flags(c, 0u, 8u);                         /* 061D XOR AH,AH */
    c->dx = 0x1Eu;                                                    /* 061F MOV DX,1Eh */
    {   uint32_t p = (uint32_t)c->ax * c->dx;                         /* 0622 MUL DX: its flags are dead (ADD below) */
        c->ax = (uint16_t)p; c->dx = (uint16_t)(p >> 16u); }
    c->di = c->ax;                                                    /* 0624 MOV DI,AX */
    c->di = add16(c, c->di, 0x2614u);                                 /* 0626 ADD DI,2614h */
    push(ram, c, c->ds); push(ram, c, c->di);
    FAR(0x062C);                                                      /* load table[DS:5376] into [BP-355h] */
    push_lea(ram, c, 0x255u);
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ds_byte(ram, c, 0x547Du)); push(ram, c, c->ax);   /* 0637 MOV AL,[547D]; PUSH AX */
    FAR(0x063B);                                                      /* char to string */
    FAR(0x0640);                                                      /* + that char */
    c->di = 0x05CBu; push(ram, c, c->cs); push(ram, c, c->di);        /* 0645 MOV DI,05CBh; PUSH CS; PUSH DI */
    FAR(0x064A);                                                      /* + the literal at CS:05CB */
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ds_byte(ram, c, 0x557Cu)); push(ram, c, c->ax);   /* 064F MOV AL,[557C]; PUSH AX */
    push_far_arg(ram, c, 0x0Au);                                      /* 0653 f */
    push(ram, c, c->cs); push(ram, c, 0x065Cu); c->ip = 0x025Du;      /* 0658 PUSH CS; CALL 025D */
in_025d:
    s = open_025d(ram, ram_size, c, env, d);
    if (s != CONTINUE) return (pr_r0802_05cd_status)s;
    logic_flags(c, c->ax & 0xFFu, 8u);                                /* 065C OR AL,AL */
    if ((c->ax & 0xFFu) != 0u) {                                      /* 065E JE 06DA */
        push_far_arg(ram, c, 0x06u);                                  /* 0660 g */
        push_lea(ram, c, 0x455u); push_lea(ram, c, 0x2Au);
        FAR(0x0670);                                                  /* load base into [BP-455h] */
        push_lea(ram, c, 0x53u);
        FAR(0x067A);                                                  /* + extension */
        FAR(0x067F);                                                  /* Assign(g, name) */
        push_far_arg(ram, c, 0x06u); c->ax = 1u; push(ram, c, c->ax);
        FAR(0x068D);                                                  /* Reset(g, 1) */
        FAR(0x0692);                                                  /* IOCheck */
        push_far_arg(ram, c, 0x06u); push_far_arg(ram, c, 0x0Eu); c->ax = 2u; push(ram, c, c->ax); push_lea(ram, c, 0x55u);
        FAR(0x06AA);                                                  /* BlockRead(g, header word, 2, [BP-55h]) */
        FAR(0x06AF);                                                  /* IOCheck */
        les_bp(ram, c, 0x0Eu);                                        /* 06B4 LES DI,[BP+0Eh]; 06B7 ADD WORD ES:[DI],2 */
        wr16(ram, c->es, c->di, add16(c, rd16(ram, c->es, c->di), 2u));
        push_far_arg(ram, c, 0x0Au);                                  /* 06BB f */
        les_bp(ram, c, 0x0Eu); c->ax = rd16(ram, c->es, c->di);       /* 06C0 MOV AX,ES:[DI] */
        c->dx = 0u; logic_flags(c, 0u, 16u);                          /* 06C6 XOR DX,DX */
        push(ram, c, c->dx); push(ram, c, c->ax);
        FAR(0x06CA);                                                  /* Seek(f, header word) */
        FAR(0x06CF);                                                  /* IOCheck */
        ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = 1u;            /* 06D4 MOV BYTE [BP-1],1; JMP 06DE */
    } else {
        ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = 0u;            /* 06DA MOV BYTE [BP-1],0 */
    }
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ss, (uint16_t)(c->bp - 1u))]);   /* 06DE MOV AL,[BP-1] */
    c->sp = c->bp; c->bp = pop(ram, c);                               /* 06E1 MOV SP,BP; POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 06E4 RETF 14h */
    c->sp = (uint16_t)(c->sp + 0x14u);
    return PR_R0802_05CD_RETURNED;
}
