/* START.EXE resident 0899:0059, the game's ReadKey with hotkeys, raw CPU (docs/re/RES_0899_READKEY_0059_CPU.md). */
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"
#include "poolrad/recovered/system_paramstr_16bc_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include <string.h>

#define MASK (PR_R0899_KEY_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u
#define ARITH (CF | PF | AF | ZF | SF | OF)

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
static void push(uint8_t *ram, pr_r0899_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t parity(unsigned v)
{
    v &= 0xFFu; v ^= v >> 4u; v ^= v >> 2u; v ^= v >> 1u;
    return (v & 1u) ? 0u : PF;
}
/* CMP r/m8,imm8 */
static void cmp8(pr_r0899_cpu *c, unsigned a, unsigned b)
{
    unsigned r = (a - b) & 0xFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & 0x80u ? SF : 0u) | parity(r) | (a < b ? CF : 0u) |
                          ((a ^ b ^ r) & AF) | (((a ^ b) & (a ^ r) & 0x80u) ? OF : 0u));
}
static uint8_t key_at(const uint8_t *ram, const pr_r0899_cpu *c) { return ram[physical(c->ss, (uint16_t)(c->bp - 2u))]; }

/* LCALL with the segment word read live from CS:at; pushes CS and the return IP. */
static void far_call(uint8_t *ram, pr_r0899_cpu *c, uint16_t at, uint16_t off, uint16_t next)
{
    uint16_t seg = rd16(ram, c->cs, at);
    push(ram, c, c->cs); push(ram, c, next);
    c->cs = seg; c->ip = off;
}

typedef struct { uint8_t *ram; size_t size; const pr_r0899_key_env *env; pr_r0899_key_diagnostic *d; uint16_t body_cs; } run;

static int back(const run *r, const pr_r0899_cpu *c, uint16_t next) { return c->cs == r->body_cs && c->ip == next; }

/* Crt ReadKey (030F) or KeyPressed (02FD) at the call site `site`, returning to `next`. 0 = returned there. */
static pr_r0899_key_status crt(const run *r, pr_r0899_cpu *c, uint16_t site, uint16_t at, uint16_t off, uint16_t next)
{
    pr_crt_cpu m; int s;
    far_call(r->ram, c, at, off, next);
    COPY(&m, c);
    if (off == 0x030Fu) { ++r->d->readkeys; s = (int)pr_crt_readkey_030f(r->ram, r->size, &m, r->env->keyboard, r->env->keyboard_context); }
    else { ++r->d->keypressed; s = (int)pr_crt_keypressed_02fd(r->ram, r->size, &m, r->env->keyboard, r->env->keyboard_context); }
    COPY(c, &m);
    if (s != PR_CRT_RETURNED) {
        r->d->callee = site; r->d->status = s;
        return s == PR_CRT_BREAK ? PR_R0899_KEY_BREAK : PR_R0899_KEY_CRT;
    }
    return back(r, c, next) ? PR_R0899_KEY_RETURN : PR_R0899_KEY_TRANSFER;
}
static pr_r0899_key_status driver(const run *r, pr_r0899_cpu *c, uint16_t site, uint16_t at, uint16_t off, uint16_t next)
{
    pr_r08ae_driver_cpu m; int s;
    far_call(r->ram, c, at, off, next);
    COPY(&m, c); s = (int)pr_r08ae_driver_call(r->ram, r->size, &m, &r->env->quit.driver, &r->d->driver); COPY(c, &m);
    if (s != PR_R08AE_DRIVER_RETURN) { r->d->callee = site; r->d->status = s; return PR_R0899_KEY_DRIVER; }
    return back(r, c, next) ? PR_R0899_KEY_RETURN : PR_R0899_KEY_TRANSFER;
}
/* SYSTEM leaves (pure memory): 0DC0 set IN, 0C10 string compare, 16BC ParamStr. */
static pr_r0899_key_status leaf(const run *r, pr_r0899_cpu *c, uint16_t site, uint16_t at, uint16_t off, uint16_t next)
{
    int s;
    far_call(r->ram, c, at, off, next);
    if (off == 0x16BCu) {
        pr_paramstr_cpu m;
        COPY(&m, c); s = (int)pr_system_paramstr_16bc(r->ram, r->size, &m); COPY(c, &m);
    } else {
        pr_system_leaf_cpu m;
        COPY(&m, c);
        s = (int)(off == 0x0DC0u ? pr_system_set_in_0dc0(r->ram, r->size, &m) : pr_system_strcmp_0c10(r->ram, r->size, &m));
        COPY(c, &m);
    }
    if (s != 0) { r->d->callee = site; r->d->status = s; return PR_R0899_KEY_LEAF; }
    return back(r, c, next) ? PR_R0899_KEY_RETURN : PR_R0899_KEY_TRANSFER;
}
#define TRY(x) do { pr_r0899_key_status t_ = (x); if (t_ != PR_R0899_KEY_RETURN) return t_; } while (0)

pr_r0899_key_status pr_r0899_readkey_0059_cpu(uint8_t *ram, size_t ram_size, pr_r0899_cpu *c,
                                              const pr_r0899_key_env *env, pr_r0899_key_diagnostic *d)
{
    run r;
    unsigned drained = 0;
    if (ram == NULL || c == NULL || env == NULL || d == NULL || env->keyboard == NULL || ram_size != PR_R0899_KEY_RAM_SIZE ||
        c->ip != 0x0059u || c->sp < 0x0112u || c->sp > 0xFFFEu)
        return PR_R0899_KEY_ARGUMENT;
    memset(d, 0, sizeof *d); d->driver.time_status = -1; d->quit.intr_status = -1; d->quit.driver.time_status = -1;
    r.ram = ram; r.size = ram_size; r.env = env; r.d = d; r.body_cs = c->cs;
    push(ram, c, c->bp);                                              /* 0059 PUSH BP */
    c->bp = c->sp;                                                    /* 005A MOV BP,SP */
    {   /* 005C SUB SP,0102h */
        unsigned a = c->sp, b = 0x0102u, res = (a - b) & 0xFFFFu;
        c->flags = (uint16_t)((c->flags & ~ARITH) | (res == 0u ? ZF : 0u) | (res & 0x8000u ? SF : 0u) | parity(res) |
                              (a < b ? CF : 0u) | ((a ^ b ^ res) & AF) | (((a ^ b) & (a ^ res) & 0x8000u) ? OF : 0u));
        c->sp = (uint16_t)res;
    }
    TRY(crt(&r, c, 0x0060u, 0x0063u, 0x030Fu, 0x0065u));             /* 0060 LCALL 0A40:030F */
    ram[physical(c->ss, (uint16_t)(c->bp - 2u))] = (uint8_t)c->ax;    /* 0065 MOV [BP-2],AL */
    cmp8(c, key_at(ram, c), 0x13u);                                   /* 0068 CMP [BP-2],13h */
    if ((c->flags & ZF) != 0u) {                                      /* 006C JNE 00A6 */
        cmp8(c, ram[physical(c->ds, 0x5372u)], 0x02u);                /* 006E CMP [5372],2 */
        if ((c->flags & ZF) == 0u) {                                  /* 0073 JE 008B */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, 0x5372u)]);   /* 0075 MOV AL,[5372] */
            ram[physical(c->ds, 0x5373u)] = (uint8_t)c->ax;          /* 0078 MOV [5373],AL */
            push(ram, c, rd16(ram, c->ds, 0x2612u));                  /* 007B PUSH WORD [2612] */
            TRY(driver(&r, c, 0x007Fu, 0x0082u, 0x00DCu, 0x0084u));  /* 007F LCALL 08AE:00DC */
            ram[physical(c->ds, 0x5372u)] = 0x02u;                    /* 0084 MOV [5372],2; 0089 JMP 00A6 */
        } else {
            c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, 0x5373u)]);   /* 008B MOV AL,[5373] */
            push(ram, c, c->ax);                                      /* 008E PUSH AX */
            c->di = 0x0033u;                                          /* 008F MOV DI,0033h */
            push(ram, c, c->cs);                                      /* 0092 PUSH CS */
            push(ram, c, c->di);                                      /* 0093 PUSH DI */
            TRY(leaf(&r, c, 0x0094u, 0x0097u, 0x0DC0u, 0x0099u));    /* 0094 LCALL 0AF8:0DC0 */
            if ((c->flags & ZF) == 0u) {                              /* 0099 JE 00A6 */
                c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, 0x5373u)]);   /* 009B MOV AL,[5373] */
                ram[physical(c->ds, 0x5372u)] = (uint8_t)c->ax;      /* 009E MOV [5372],AL */
                TRY(driver(&r, c, 0x00A1u, 0x00A4u, 0x017Bu, 0x00A6u));   /* 00A1 LCALL 08AE:017B */
            }
        }
    }
    c->di = (uint16_t)(c->bp - 0x0102u);                              /* 00A6 LEA DI,[BP-102h] */
    push(ram, c, c->ss);                                              /* 00AA PUSH SS */
    push(ram, c, c->di);                                              /* 00AB PUSH DI */
    c->ax = 0x0001u;                                                  /* 00AC MOV AX,1 */
    push(ram, c, c->ax);                                              /* 00AF PUSH AX */
    TRY(leaf(&r, c, 0x00B0u, 0x00B3u, 0x16BCu, 0x00B5u));            /* 00B0 LCALL 0AF8:16BC (ParamStr(1)) */
    c->di = 0x0053u;                                                  /* 00B5 MOV DI,0053h */
    push(ram, c, c->cs);                                              /* 00B8 PUSH CS */
    push(ram, c, c->di);                                              /* 00B9 PUSH DI */
    TRY(leaf(&r, c, 0x00BAu, 0x00BDu, 0x0C10u, 0x00BFu));            /* 00BA LCALL 0AF8:0C10 (= 'STING') */
    if ((c->flags & ZF) != 0u) {                                      /* 00BF JNE 00DE */
        cmp8(c, key_at(ram, c), 0x04u);                               /* 00C1 CMP [BP-2],4 */
        if ((c->flags & ZF) != 0u) {                                  /* 00C5 JNE 00D4 */
            cmp8(c, ram[physical(c->ds, 0x49F7u)], 0x00u);            /* 00C7 CMP [49F7],0 */
            c->ax &= 0xFF00u;                                         /* 00CC MOV AL,0 */
            if ((c->flags & ZF) != 0u) {                              /* 00CE JNE 00D1 */
                unsigned a = c->ax, res = (a + 1u) & 0xFFFFu;         /* 00D0 INC AX (CF kept) */
                c->flags = (uint16_t)((c->flags & ~(ARITH & ~CF)) | (res == 0u ? ZF : 0u) | (res & 0x8000u ? SF : 0u) |
                                      parity(res) | ((a ^ 1u ^ res) & AF) | (a == 0x7FFFu ? OF : 0u));
                c->ax = (uint16_t)res;
            }
            ram[physical(c->ds, 0x49F7u)] = (uint8_t)c->ax;           /* 00D1 MOV [49F7],AL */
        }
        cmp8(c, key_at(ram, c), 0x03u);                               /* 00D4 CMP [BP-2],3 */
        if ((c->flags & ZF) != 0u) {                                  /* 00D8 JNE 00DE */
            int s;
            push(ram, c, c->cs);                                      /* 00DA PUSH CS */
            push(ram, c, 0x00DEu); c->ip = 0x0000u;                   /* 00DB CALL 0000 */
            s = (int)pr_r0899_quit_0000_cpu(ram, ram_size, c, &env->quit, &d->quit);
            d->callee = 0x00DBu; d->status = s;
            return s == PR_R0899_QUIT_EXIT ? PR_R0899_KEY_EXIT : PR_R0899_KEY_QUIT;   /* QUIT never returns */
        }
    }
    cmp8(c, key_at(ram, c), 0x00u);                                   /* 00DE CMP [BP-2],0 */
    if ((c->flags & ZF) == 0u) {                                      /* 00E2 JE 00F7 */
        for (;;) {
            if (drained == env->drain_limit) { c->ip = 0x00E4u; d->callee = 0x00E4u; return PR_R0899_KEY_LIMIT; }
            TRY(crt(&r, c, 0x00E4u, 0x00E7u, 0x02FDu, 0x00E9u));    /* 00E4 LCALL 0A40:02FD */
            c->flags = (uint16_t)((c->flags & ~ARITH) | ((c->ax & 0xFFu) == 0u ? ZF : 0u) | (c->ax & 0x80u ? SF : 0u) |
                                  parity(c->ax));                     /* 00E9 OR AL,AL */
            if ((c->flags & ZF) != 0u)                                /* 00EB JE 00F7 */
                break;
            TRY(crt(&r, c, 0x00EDu, 0x00F0u, 0x030Fu, 0x00F2u));    /* 00ED LCALL 0A40:030F */
            ram[physical(c->ss, (uint16_t)(c->bp - 2u))] = (uint8_t)c->ax;   /* 00F2 MOV [BP-2],AL; 00F5 JMP 00E4 */
            ++drained;
        }
    }
    c->ax = (uint16_t)((c->ax & 0xFF00u) | key_at(ram, c));          /* 00F7 MOV AL,[BP-2] */
    ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = (uint8_t)c->ax;    /* 00FA MOV [BP-1],AL */
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ss, (uint16_t)(c->bp - 1u))]);   /* 00FD MOV AL,[BP-1] */
    c->sp = c->bp;                                                    /* 0100 MOV SP,BP */
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); /* 0102 POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));   /* 0103 RETF */
    c->sp = (uint16_t)(c->sp + 4u);
    return PR_R0899_KEY_RETURN;
}
