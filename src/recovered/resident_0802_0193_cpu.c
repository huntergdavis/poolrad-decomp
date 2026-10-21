/* START.EXE resident 0802:0193, the status-line prompt, raw CPU (docs/re/RES_0802_0193_CPU.md). */
#include "poolrad/recovered/resident_0802_0193_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#include <string.h>

#define MASK (PR_R0802_0193_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_r0802_0193_cpu cpu_t;

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
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static void push_imm(uint8_t *ram, cpu_t *c, uint8_t v) { set_al(c, v); push(ram, c, c->ax); }      /* MOV AL,imm; PUSH AX (AH as it stands) */
static unsigned parity(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) ? 0u : 0x04u; }
static void sub_flags(cpu_t *c, unsigned a, unsigned b)                            /* SUB word: the six arithmetic flags */
{
    unsigned r = (a - b) & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | (r == 0u ? 0x40u : 0u) | (r & 0x8000u ? 0x80u : 0u) | parity((uint8_t)r) | (a < b ? 0x01u : 0u) |
                          ((a ^ b ^ r) & 0x10u) | (((a ^ b) & (a ^ r) & 0x8000u) ? 0x800u : 0u));
}
static uint16_t near_target(const uint8_t *ram, uint16_t home, uint16_t rel_at, uint16_t next)   /* CALL near: the displacement is read live */
{
    return (uint16_t)(next + rd16(ram, home, rel_at));
}

/* 0175(0, 18h, 0, 28h): four MOV AL,imm; PUSH AX, PUSH CS, CALL near; returns 0 when 0175 returned to `next`. */
static int fill_call(uint8_t *ram, size_t size, cpu_t *c, const pr_r0802_0193_env *env, pr_r0802_0193_diagnostic *d, uint16_t site, uint16_t home)
{
    uint16_t next = (uint16_t)(site + 3u), target;
    pr_r0802_0175_cpu m; int s;
    push_imm(ram, c, 0x00u); push_imm(ram, c, 0x18u); push_imm(ram, c, 0x00u); push_imm(ram, c, 0x28u);
    push(ram, c, c->cs);                                                              /* PUSH CS */
    target = near_target(ram, home, (uint16_t)(site + 1u), next);
    push(ram, c, next); c->ip = target; d->site = site;
    if (target != 0x0175u) return PR_R0802_0193_CALLEE;
    COPY(&m, c); s = (int)pr_r0802_0175(ram, size, &m, &env->display, &d->fill); COPY(c, &m);
    d->callee_status = s;
    if (s != PR_R0802_0175_RETURNED) return PR_R0802_0193_DISPLAY;
    if (c->cs != home || c->ip != next) return PR_R0802_0193_CALLEE;
    d->site = 0; d->callee_status = -1;
    return 0;
}

pr_r0802_0193_status pr_r0802_0193(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_0193_env *env, pr_r0802_0193_diagnostic *diagnostic)
{
    pr_r0802_0193_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint16_t home, seg, target;
    int s;
    if (ram == NULL || c == NULL || env == NULL || env->display.video == NULL || ram_size != PR_R0802_0193_RAM_SIZE || c->ip != 0x0193u ||
        c->sp < 0x0190u || c->sp > 0xFFF0u)
        return PR_R0802_0193_ARGUMENT;
    memset(d, 0, sizeof *d); d->callee_status = -1; d->key_status = -1; d->string.callee_status = -1; d->string.display.intr_status = -1;
    d->fill.callee_status = -1; d->fill.display.intr_status = -1;
    home = c->cs;
    push(ram, c, c->bp); c->bp = c->sp;                                               /* 0193 PUSH BP; MOV BP,SP */
    { unsigned a = c->sp; c->sp = (uint16_t)(a - 0x2Au); sub_flags(c, a, 0x2Au); }    /* 0196 SUB SP,2Ah */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 6u)); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));   /* 0199 LES DI,[BP+6] */
    push(ram, c, c->es); push(ram, c, c->di);                                         /* 019C PUSH ES; 019D PUSH DI */
    c->di = (uint16_t)(c->bp - 0x29u);                                                /* 019E LEA DI,[BP-29h] */
    push(ram, c, c->ss); push(ram, c, c->di);                                         /* 01A1 PUSH SS; 01A2 PUSH DI */
    c->ax = 0x0028u; push(ram, c, c->ax);                                             /* 01A3 MOV AX,0028h; 01A6 PUSH AX */
    seg = rd16(ram, home, 0x01AAu);                                                   /* 01A7 LCALL 0AF8:0B25 (the segment word is a relocation) */
    push(ram, c, c->cs); push(ram, c, 0x01ACu);
    c->cs = seg; c->ip = rd16(ram, home, 0x01A8u);
    {
        pr_system_0b25_cpu m;
        COPY(&m, c); s = (int)pr_system_strstore_0b25(ram, ram_size, &m); COPY(c, &m);
        d->site = 0x01A7u; d->callee_status = s;
        if (s != PR_SYSTEM_0B25_RETURNED || c->cs != home || c->ip != 0x01ACu) return PR_R0802_0193_CALLEE;
        d->site = 0; d->callee_status = -1;
    }
    if ((s = fill_call(ram, ram_size, c, env, d, 0x01B9u, home)) != 0) return (pr_r0802_0193_status)s;     /* 01AC..01B9: blank row 24 */
    push_imm(ram, c, 0x00u); push_imm(ram, c, 0x18u);                                  /* 01BC column 0; 01BF row 18h */
    set_al(c, ram[physical(c->ss, (uint16_t)(c->bp + 0x0Au))]); push(ram, c, c->ax);   /* 01C2 MOV AL,[BP+0Ah]; 01C5 PUSH AX: the attribute */
    c->di = (uint16_t)(c->bp - 0x29u);                                                /* 01C6 LEA DI,[BP-29h] */
    push(ram, c, c->ss); push(ram, c, c->di);                                         /* 01C9 PUSH SS; 01CA PUSH DI */
    push(ram, c, c->cs);                                                              /* 01CB PUSH CS */
    target = near_target(ram, home, 0x01CDu, 0x01CFu);                                /* 01CC CALL near 0105 */
    push(ram, c, 0x01CFu); c->ip = target; d->site = 0x01CCu;
    if (target != 0x0105u) return PR_R0802_0193_CALLEE;
    {
        pr_r0802_0105_cpu m;
        COPY(&m, c); s = (int)pr_r0802_0105(ram, ram_size, &m, &env->display, &d->string); COPY(c, &m);
        d->callee_status = s;
        if (s != PR_R0802_0105_RETURNED) return PR_R0802_0193_DISPLAY;
        if (c->cs != home || c->ip != 0x01CFu) return PR_R0802_0193_CALLEE;
        d->site = 0; d->callee_status = -1;
    }
    seg = rd16(ram, home, 0x01D2u);                                                   /* 01CF LCALL 0899:0059 (the segment word is a relocation) */
    push(ram, c, c->cs); push(ram, c, 0x01D4u);
    c->cs = seg; c->ip = rd16(ram, home, 0x01D0u); d->site = 0x01CFu;
    {
        pr_r0899_cpu m;
        COPY(&m, c); d->key_status = (int)pr_r0899_readkey_0059_cpu(ram, ram_size, &m, &env->key, &d->key); COPY(c, &m);
        if (d->key_status != PR_R0899_KEY_RETURN) return PR_R0802_0193_KEY;
        if (c->cs != home || c->ip != 0x01D4u) return PR_R0802_0193_CALLEE;
        d->site = 0;
    }
    ram[physical(c->ss, (uint16_t)(c->bp - 0x2Au))] = (uint8_t)c->ax;                  /* 01D4 MOV [BP-2Ah],AL */
    if ((s = fill_call(ram, ram_size, c, env, d, 0x01E4u, home)) != 0) return (pr_r0802_0193_status)s;     /* 01D7..01E4: blank row 24 again */
    c->sp = c->bp;                                                                    /* 01E7 MOV SP,BP */
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);                  /* 01E9 POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));   /* 01EA RETF 6 */
    c->sp = (uint16_t)(c->sp + 4u + 6u);
    return PR_R0802_0193_RETURNED;
}
