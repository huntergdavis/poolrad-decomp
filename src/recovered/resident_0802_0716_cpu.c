/* START.EXE resident 0802:0716, the ECL/DAX block loader, raw CPU (docs/re/RES_0802_0716_CPU.md). */
#include "poolrad/recovered/resident_0802_0716_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include "poolrad/recovered/system_iocheck_04a9_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_str_1366_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#define MASK (PR_R0802_0716_RAM_SIZE - 1u)
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

typedef pr_r0802_0716_cpu cpu_t;

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
/* ADD (carry_in 0) or ADC of 16 bits. */
static uint16_t add16(cpu_t *c, unsigned a, unsigned b, unsigned carry_in)
{
    unsigned full = a + b + carry_in, r = full & 0xFFFFu;
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
static void xor_ax(cpu_t *c) { c->ax = 0u; logic_flags(c, 0u, 16u); }
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
static int stop_near(uint8_t *ram, cpu_t *c, uint16_t call_ip, uint16_t target, pr_r0802_0716_status status,
                     pr_r0802_0716_diagnostic *d)
{
    push(ram, c, c->cs);                                              /* PUSH CS */
    push(ram, c, (uint16_t)(call_ip + 3u)); c->ip = target;           /* CALL target */
    d->site = call_ip;
    return (int)status;
}

/* LCALL at `at`: pushes the return frame, takes the live segment word, runs the composed SYSTEM model, and requires the
 * return to at+5 in the caller's segment. Returns CONTINUE, or a status to stop with. */
static int far_call(uint8_t *ram, size_t size, cpu_t *c, const pr_r0802_0716_env *env, uint16_t at,
                    pr_r0802_0716_diagnostic *d)
{
    uint16_t home = c->cs, target = rd16(ram, c->cs, (uint16_t)(at + 1u)), seg = rd16(ram, c->cs, (uint16_t)(at + 3u));
    int status = 0, ok = 0;
    push(ram, c, c->cs); push(ram, c, (uint16_t)(at + 5u));
    c->cs = seg; c->ip = target;
    d->site = at; d->callee_status = -1;
    switch (target) {
    case 0x0B25: { pr_system_0b25_cpu m; COPY_CPU(&m, c); status = pr_system_strstore_0b25(ram, size, &m); COPY_CPU(c, &m);
        ok = status == PR_SYSTEM_0B25_RETURNED; break; }
    case 0x0B0B: case 0x0B98: case 0x1366: { pr_system_leaf_cpu m; COPY_CPU(&m, c);
        status = target == 0x0B0B ? pr_system_strload_0b0b(ram, size, &m) : target == 0x0B98 ? pr_system_strcat_0b98(ram, size, &m)
               : pr_system_str_1366(ram, size, &m);
        COPY_CPU(c, &m); ok = status == PR_SYSTEM_LEAF_RETURNED; break; }
    case 0x04A9: { pr_system_iocheck_cpu m; COPY_CPU(&m, c); status = pr_system_iocheck_04a9(ram, size, &m); COPY_CPU(c, &m);
        if (status == PR_SYSTEM_IOCHECK_RUNERROR) { d->callee_status = status; return PR_R0802_0716_RUNERROR; }
        ok = status == PR_SYSTEM_IOCHECK_RETURNED; break; }
    case 0x023F: { pr_system_getmem_cpu m; COPY_CPU(&m, c); status = pr_system_getmem_023f(ram, size, &m, env->getmem_steps);
        COPY_CPU(c, &m);
        if (status == PR_SYSTEM_GETMEM_RUNERROR) { d->callee_status = status; return PR_R0802_0716_RUNERROR; }
        ok = status == PR_SYSTEM_GETMEM_RETURNED; break; }
    case 0x14FB: { pr_blockread_cpu m; COPY_CPU(&m, c); status = pr_system_blockread_14fb(ram, size, &m, env->read, env->read_context);
        COPY_CPU(c, &m); d->callee_status = status;
        if (status == PR_BLOCKREAD_DIVIDE) return PR_R0802_0716_DIVIDE;
        if (status == PR_BLOCKREAD_SERVICE) return PR_R0802_0716_SERVICE;
        ok = status == PR_BLOCKREAD_RETURNED; break; }
    case 0x1593: { pr_filepos_cpu m; COPY_CPU(&m, c); status = pr_system_filepos_1593(ram, size, &m, env->filepos, env->filepos_context);
        COPY_CPU(c, &m); d->callee_status = status;
        if (status == PR_FILEPOS_DIVIDE) return PR_R0802_0716_DIVIDE;
        if (status == PR_FILEPOS_SERVICE) return PR_R0802_0716_SERVICE;
        ok = status == PR_FILEPOS_RETURNED; break; }
    case 0x1563: { pr_seek_cpu m; COPY_CPU(&m, c); status = pr_system_seek_1563(ram, size, &m, env->seek, env->seek_context);
        COPY_CPU(c, &m); d->callee_status = status;
        if (status == PR_SEEK_SERVICE) return PR_R0802_0716_SERVICE;
        ok = status == PR_SEEK_RETURNED; break; }
    default:
        return PR_R0802_0716_CALLEE;                                  /* unreachable: the code is immutable */
    }
    d->callee_status = status;
    if (!ok || c->cs != home || c->ip != (uint16_t)(at + 5u))
        return PR_R0802_0716_CALLEE;
    d->site = 0; d->callee_status = -1;
    return CONTINUE;
}

#define FAR(at) do { int s_ = far_call(ram, ram_size, c, env, (at), d); if (s_ != CONTINUE) return (pr_r0802_0716_status)s_; } while (0)

/* BlockRead(f = SS:BP-file, buffer pushed by the caller, count, result = SS:BP-33h) + IOCheck. */
static int block_read(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_0716_env *env, pr_r0802_0716_diagnostic *d,
                      uint16_t count, uint16_t lcall)
{
    c->ax = count; push(ram, c, c->ax);                               /* MOV AX,count; PUSH AX */
    push_lea(ram, c, 0x33u);                                          /* LEA DI,[BP-33h]; PUSH SS; PUSH DI */
    { int s = far_call(ram, ram_size, c, env, lcall, d); if (s != CONTINUE) return s; }
    return far_call(ram, ram_size, c, env, (uint16_t)(lcall + 5u), d);  /* LCALL 04A9 */
}
#define READ(count, at) do { int s_ = block_read(ram, ram_size, c, env, d, (count), (at)); \
    if (s_ != CONTINUE) return (pr_r0802_0716_status)s_; } while (0)

pr_r0802_0716_status pr_r0802_0716(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_0716_env *env,
                                       pr_r0802_0716_diagnostic *diagnostic)
{
    pr_r0802_0716_diagnostic local, *d = diagnostic ? diagnostic : &local;
    if (ram == NULL || c == NULL || env == NULL || ram_size != PR_R0802_0716_RAM_SIZE)
        return PR_R0802_0716_ARGUMENT;
    switch (c->ip) {
    case 0x0716: if (c->sp < 0x0300u || c->sp > 0xFFEAu) return PR_R0802_0716_ARGUMENT; break;
    case 0x0778: case 0x084D: case 0x08B0: case 0x08BF: case 0x0920: case 0x0938: case 0x0944: break;
    default: return PR_R0802_0716_ARGUMENT;
    }
    d->site = 0; d->callee_status = -1;
    switch (c->ip) {
    case 0x0778: goto after_open;
    case 0x084D: goto after_prompt;
    case 0x08B0: goto epilogue;
    case 0x08BF: goto after_alloc;
    case 0x0920: goto after_close;
    case 0x0938: goto after_decode;
    case 0x0944: goto epilogue;
    default: break;
    }
    push(ram, c, c->bp); c->bp = c->sp;                               /* 0716 PUSH BP; MOV BP,SP */
    { unsigned a = c->sp; c->sp = (uint16_t)(a - 0x23Eu); sub_flags(c, a, 0x23Eu, 16u); }   /* 0719 SUB SP,23Eh */
    les_bp(ram, c, 0x14u); push(ram, c, c->es); push(ram, c, c->di);  /* 071D name */
    push_lea(ram, c, 0x15u); c->ax = 0x14u; push(ram, c, c->ax);      /* 0722 [BP-15h], 20 */
    FAR(0x072B);                                                      /* StrStore */
    les_bp(ram, c, 0x10u); push(ram, c, c->es); push(ram, c, c->di);  /* 0730 ext */
    push_lea(ram, c, 0x2Au); c->ax = 0x14u; push(ram, c, c->ax);
    FAR(0x073E);
    push_lea(ram, c, 0x23Du);                                         /* 0743 the path buffer */
    c->di = 0x547Cu; push(ram, c, c->ds); push(ram, c, c->di);        /* 0749 DS:547C */
    FAR(0x074E);                                                      /* string load */
    push_lea(ram, c, 0x15u);                                          /* 0753 + name */
    FAR(0x0758);                                                      /* concat */
    push_lea(ram, c, 0x2Au); push_lea(ram, c, 0x135u); push_lea(ram, c, 0x0B3u); push_lea(ram, c, 0x133u);
    return (pr_r0802_0716_status)stop_near(ram, c, 0x0775u, 0x05CDu, PR_R0802_0716_OPEN, d);   /* 0774 PUSH CS; CALL 05CD */

after_open:
    logic_flags(c, c->ax & 0xFFu, 8u);                                /* 0778 OR AL,AL */
    if ((c->ax & 0xFFu) == 0u) {                                      /* 077A JNE 0793 */
        les_bp(ram, c, 0x0Au); xor_ax(c); wr16(ram, c->es, c->di, c->ax);             /* 077C U^ := 0 */
        les_bp(ram, c, 0x06u); xor_ax(c); wr16(ram, c->es, c->di, c->ax);             /* 0784 P^ := nil */
        wr16(ram, c->es, (uint16_t)(c->di + 2u), c->ax);
        goto epilogue;                                                /* 0790 JMP 0944 */
    }
    for (;;) {
        push_lea(ram, c, 0x133u); push_lea(ram, c, 0x31u); READ(1u, 0x07A7);          /* 0793 id */
        push_lea(ram, c, 0x133u); push_lea(ram, c, 0x30u); READ(4u, 0x07C5);          /* 07B1 offset */
        push_lea(ram, c, 0x133u); les_bp(ram, c, 0x0Au); push(ram, c, c->es); push(ram, c, c->di);
        READ(2u, 0x07E3);                                                             /* 07CF U^ */
        push_lea(ram, c, 0x133u); push_lea(ram, c, 0x2Cu); READ(2u, 0x0801);          /* 07ED packed */
        {   uint16_t packed = rd16(ram, c->ss, (uint16_t)(c->bp - 0x2Cu));
            sub_flags(c, packed, 0x8CA0u, 16u);                       /* 080B CMP [BP-2Ch],8CA0h */
            if ((c->flags & (CF | ZF)) == 0u) {                       /* 0810 JBE 0852 */
                c->ax = packed; c->dx = 0u; logic_flags(c, 0u, 16u);  /* 0812 MOV AX; XOR DX,DX */
                push(ram, c, c->dx); push(ram, c, c->ax);
                xor_ax(c); push(ram, c, c->ax);                       /* 0819 width 0 */
                push_lea(ram, c, 0x13Du); c->ax = 6u; push(ram, c, c->ax);
                FAR(0x0826);                                          /* Str */
                c->ax = (uint16_t)((c->ax & 0xFF00u) | 0x0Eu); push(ram, c, c->ax);    /* 082B MOV AL,0Eh; PUSH AX */
                push_lea(ram, c, 0x23Du);
                c->di = 0x070Cu; push(ram, c, c->cs); push(ram, c, c->di);            /* 0834 CS:070C */
                FAR(0x0839);                                          /* string load */
                push_lea(ram, c, 0x13Du);
                FAR(0x0844);                                          /* concat */
                return (pr_r0802_0716_status)stop_near(ram, c, 0x084Au, 0x0193u, PR_R0802_0716_PROMPT, d);
            }
        }
        {   uint8_t id = ram[physical(c->ss, (uint16_t)(c->bp - 0x31u))];
            c->ax = (uint16_t)((c->ax & 0xFF00u) | id);               /* 0852 MOV AL,[BP-31h] */
            sub_flags(c, id, ram[physical(c->ss, (uint16_t)(c->bp + 0x0Eu))], 8u);   /* 0855 CMP AL,[BP+0Eh] */
            if ((c->flags & ZF) != 0u)                                /* 0858 JE 0884 */
                break;
        }
        push_lea(ram, c, 0x133u);
        FAR(0x0860);                                                  /* FilePos */
        FAR(0x0865);                                                  /* IOCheck */
        c->cx = c->ax; c->bx = c->dx;                                 /* 086A MOV CX,AX; MOV BX,DX */
        c->ax = rd16(ram, c->ss, (uint16_t)(c->bp - 0x135u));         /* 086E header length */
        c->dx = 0u; logic_flags(c, 0u, 16u);                          /* 0872 XOR DX,DX */
        sub_flags(c, c->dx, c->bx, 16u);                              /* 0874 CMP DX,BX */
        {   int less = ((c->flags & SF) != 0u) != ((c->flags & OF) != 0u);
            if (less) break;                                          /* 0876 JL 0884 */
            if ((c->flags & ZF) == 0u) continue;                      /* 0878 JLE 087D; 087A JMP 0793 */
        }
        sub_flags(c, c->ax, c->cx, 16u);                              /* 087D CMP AX,CX */
        if ((c->flags & (CF | ZF)) != 0u) break;                      /* 087F JBE 0884 */
    }                                                                 /* 0881 JMP 0793 */
    {   uint8_t id = ram[physical(c->ss, (uint16_t)(c->bp - 0x31u))];
        c->ax = (uint16_t)((c->ax & 0xFF00u) | id);                   /* 0884 MOV AL,[BP-31h] */
        sub_flags(c, id, ram[physical(c->ss, (uint16_t)(c->bp + 0x0Eu))], 8u);       /* 0887 CMP AL,[BP+0Eh] */
    }
    if ((c->flags & ZF) == 0u) {                                      /* 088A JE 08B3 */
        les_bp(ram, c, 0x0Au); xor_ax(c); wr16(ram, c->es, c->di, c->ax);             /* 088C U^ := 0 */
        les_bp(ram, c, 0x06u); xor_ax(c); wr16(ram, c->es, c->di, c->ax);             /* 0894 P^ := nil */
        wr16(ram, c->es, (uint16_t)(c->di + 2u), c->ax);
        push_lea(ram, c, 0x0B3u); push_lea(ram, c, 0x133u);
        return (pr_r0802_0716_status)stop_near(ram, c, 0x08ADu, 0x06E7u, PR_R0802_0716_CLOSE, d);   /* resume 08B0: JMP 0944 */
    }
    c->di = 0x557Eu; push(ram, c, c->ds); push(ram, c, c->di);        /* 08B3 DS:557E */
    push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp - 0x2Cu)));         /* 08B8 PUSH packed */
    return (pr_r0802_0716_status)stop_near(ram, c, 0x08BCu, 0x0020u, PR_R0802_0716_ALLOC, d);

after_alloc:
    les_bp(ram, c, 0x0Au); push(ram, c, rd16(ram, c->es, c->di));    /* 08BF PUSH U^ */
    FAR(0x08C5);                                                      /* GetMem */
    les_bp(ram, c, 0x06u);                                            /* 08CA P^ := DX:AX */
    wr16(ram, c->es, c->di, c->ax); wr16(ram, c->es, (uint16_t)(c->di + 2u), c->dx);
    push_lea(ram, c, 0x0B3u);                                         /* 08D4 */
    c->ax = rd16(ram, c->ss, (uint16_t)(c->bp - 0x135u)); c->dx = 0u; logic_flags(c, 0u, 16u);
    c->ax = add16(c, c->ax, rd16(ram, c->ss, (uint16_t)(c->bp - 0x30u)), 0u);        /* 08E0 ADD AX,[BP-30h] */
    c->dx = add16(c, c->dx, rd16(ram, c->ss, (uint16_t)(c->bp - 0x2Eu)), c->flags & CF);   /* 08E3 ADC DX,[BP-2Eh] */
    push(ram, c, c->dx); push(ram, c, c->ax);
    FAR(0x08E8);                                                      /* Seek */
    FAR(0x08ED);                                                      /* IOCheck */
    push_lea(ram, c, 0x0B3u);                                         /* 08F2 */
    c->di = rd16(ram, c->ds, 0x557Eu); c->es = rd16(ram, c->ds, 0x5580u);              /* 08F8 LES DI,[557E] */
    push(ram, c, c->es); push(ram, c, c->di);
    push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp - 0x2Cu)));         /* 08FE PUSH packed */
    push_lea(ram, c, 0x33u);
    FAR(0x0906);                                                      /* BlockRead */
    FAR(0x090B);                                                      /* IOCheck */
    push_lea(ram, c, 0x0B3u); push_lea(ram, c, 0x133u);
    return (pr_r0802_0716_status)stop_near(ram, c, 0x091Du, 0x06E7u, PR_R0802_0716_CLOSE, d);

after_close:
    c->di = 0x557Eu; push(ram, c, c->ds); push(ram, c, c->di);        /* 0920 DS:557E */
    les_bp(ram, c, 0x06u); push(ram, c, c->es); push(ram, c, c->di);  /* 0925 P */
    push_lea(ram, c, 0x2Cu);                                          /* 092A &packed */
    les_bp(ram, c, 0x0Au); push(ram, c, c->es); push(ram, c, c->di);  /* 092F U */
    return (pr_r0802_0716_status)stop_near(ram, c, 0x0935u, 0x050Fu, PR_R0802_0716_DECODE, d);

after_decode:
    c->di = 0x557Eu; push(ram, c, c->ds); push(ram, c, c->di);        /* 0938 DS:557E */
    push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp - 0x2Cu)));         /* 093D PUSH packed */
    return (pr_r0802_0716_status)stop_near(ram, c, 0x0941u, 0x0000u, PR_R0802_0716_FREE, d);

after_prompt:
    push(ram, c, c->cs); push(ram, c, 0x0852u);                       /* 084D LCALL 0899:0000 (QUIT) */
    d->site = 0x084Du;
    {   uint16_t target = rd16(ram, c->cs, 0x084Eu), seg = rd16(ram, c->cs, 0x0850u);
        c->cs = seg; c->ip = target; }
    return PR_R0802_0716_QUIT;

epilogue:
    c->sp = c->bp; c->bp = pop(ram, c);                               /* 0944 MOV SP,BP; POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 0947 RETF 12h */
    c->sp = (uint16_t)(c->sp + 0x12u);
    return PR_R0802_0716_RETURNED;
}
