/* START.EXE resident 0802:0105, the status-line string writer, raw CPU (docs/re/RES_0802_0105_CPU.md). */
#include "poolrad/recovered/resident_0802_0105_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#include <string.h>

#define MASK (PR_R0802_0105_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_r0802_0105_cpu cpu_t;

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
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static unsigned parity(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) ? 0u : 0x04u; }
static void cmp_flags(cpu_t *c, unsigned a, unsigned b, unsigned bits)           /* CMP/SUB of width bits: the six arithmetic flags */
{
    unsigned mask = (1u << bits) - 1u, top = 1u << (bits - 1u), r = (a - b) & mask;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | (r == 0u ? 0x40u : 0u) | (r & top ? 0x80u : 0u) | parity((uint8_t)r) | (a < b ? 0x01u : 0u) |
                          ((a ^ b ^ r) & 0x10u) | (((a ^ b) & (a ^ r) & top) ? 0x800u : 0u));
}
static uint8_t inc8(cpu_t *c, uint8_t v)                                          /* INC byte: CF is kept */
{
    uint8_t r = (uint8_t)(v + 1u);
    c->flags = (uint16_t)((c->flags & ~0x08D4u) | (r == 0 ? 0x40u : 0u) | (r & 0x80u ? 0x80u : 0u) | parity(r) | (((v & 0x0Fu) == 0x0Fu) ? 0x10u : 0u) |
                          (v == 0x7Fu ? 0x800u : 0u));
    return r;
}

pr_r0802_0105_status pr_r0802_0105(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_0040_env *env, pr_r0802_0105_diagnostic *diagnostic)
{
    pr_r0802_0105_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint16_t home, seg;
    uint8_t n;
    if (ram == NULL || c == NULL || env == NULL || env->video == NULL || ram_size != PR_R0802_0105_RAM_SIZE || c->ip != 0x0105u ||
        c->sp < 0x0140u || c->sp > 0xFFF0u)
        return PR_R0802_0105_ARGUMENT;
    memset(d, 0, sizeof *d); d->callee_status = -1; d->display.intr_status = -1;
    home = c->cs;
    push(ram, c, c->bp); c->bp = c->sp;                                               /* 0105 PUSH BP; MOV BP,SP */
    { unsigned a = c->sp; c->sp = (uint16_t)(a - 0x104u); cmp_flags(c, a, 0x104u, 16u); }   /* 0108 SUB SP,104h */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 6u)); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));   /* 010C LES DI,[BP+6] */
    push(ram, c, c->es); push(ram, c, c->di);                                         /* 010F PUSH ES; 0110 PUSH DI */
    c->di = (uint16_t)(c->bp - 0x100u);                                               /* 0111 LEA DI,[BP-100h] */
    push(ram, c, c->ss); push(ram, c, c->di);                                         /* 0115 PUSH SS; 0116 PUSH DI */
    c->ax = 0x00FFu; push(ram, c, c->ax);                                             /* 0117 MOV AX,00FFh; 011A PUSH AX */
    seg = rd16(ram, c->cs, 0x011Eu);                                                  /* 011B LCALL 0AF8:0B25: the segment word is a relocation */
    push(ram, c, c->cs); push(ram, c, 0x0120u);
    c->cs = seg; c->ip = rd16(ram, home, 0x011Cu);
    {
        pr_system_0b25_cpu m; int s;
        COPY(&m, c); s = (int)pr_system_strstore_0b25(ram, ram_size, &m); COPY(c, &m);
        d->callee_status = s;
        if (s != PR_SYSTEM_0B25_RETURNED || c->cs != home || c->ip != 0x0120u) return PR_R0802_0105_CALLEE;
    }
    set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp - 0x100u))); wr8(ram, c->ss, (uint16_t)(c->bp - 0x101u), (uint8_t)c->ax);   /* 0120, 0124 */
    set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp - 0x101u))); wr8(ram, c->ss, (uint16_t)(c->bp - 0x103u), (uint8_t)c->ax);   /* 0128, 012C */
    set_al(c, 1u);                                                                    /* 0130 MOV AL,1 */
    n = rd8(ram, c->ss, (uint16_t)(c->bp - 0x103u));
    cmp_flags(c, 1u, n, 8u);                                                          /* 0132 CMP AL,[BP-103h] */
    if (!(c->flags & 0x41u)) goto done;                                               /* 0136 JA 016F (CF = 0 and ZF = 0) */
    wr8(ram, c->ss, (uint16_t)(c->bp - 0x102u), (uint8_t)c->ax);                      /* 0138 MOV [BP-102h],AL: i := 1 */
    for (;;) {                                                                        /* 013C JMP 0142 ... */
        uint16_t target;
        set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Eu))); push(ram, c, c->ax);   /* 0142 column */
        set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Cu))); push(ram, c, c->ax);   /* 0146 row */
        set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Au))); push(ram, c, c->ax);   /* 014A attribute */
        set_al(c, 1u); push(ram, c, c->ax);                                           /* 014E count */
        set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp - 0x102u))); c->ax &= 0x00FFu;     /* 0151 MOV AL,[BP-102h]; 0155 XOR AH,AH */
        c->flags = (uint16_t)((c->flags & ~0x08D5u) | 0x0044u);
        c->di = c->ax;                                                                /* 0157 MOV DI,AX */
        set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + c->di - 0x100u))); push(ram, c, c->ax);   /* 0159 MOV AL,[BP+DI-100h]; 015D PUSH AX */
        push(ram, c, c->cs);                                                          /* 015E PUSH CS */
        target = (uint16_t)(0x0162u + rd16(ram, home, 0x0160u));                      /* 015F CALL near 0040 */
        push(ram, c, 0x0162u); c->ip = target;
        if (target != 0x0040u) return PR_R0802_0105_CALLEE;
        {
            pr_r0802_0040_cpu m; int s;
            COPY(&m, c); s = (int)pr_r0802_0040(ram, ram_size, &m, env, &d->display); COPY(c, &m);
            d->callee_status = s;
            if (s != PR_R0802_0040_RETURNED) return PR_R0802_0105_DISPLAY;
            if (c->cs != home || c->ip != 0x0162u) return PR_R0802_0105_CALLEE;
            ++d->chars;
        }
        wr8(ram, c->ss, (uint16_t)(c->bp + 0x0Eu), inc8(c, rd8(ram, c->ss, (uint16_t)(c->bp + 0x0Eu))));   /* 0162 INC BYTE [BP+0Eh] */
        set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp - 0x102u)));                       /* 0165 MOV AL,[BP-102h] */
        cmp_flags(c, (uint8_t)c->ax, rd8(ram, c->ss, (uint16_t)(c->bp - 0x103u)), 8u);   /* 0169 CMP AL,[BP-103h] */
        if (c->flags & 0x40u) break;                                                  /* 016D JNE 013E not taken */
        wr8(ram, c->ss, (uint16_t)(c->bp - 0x102u), inc8(c, rd8(ram, c->ss, (uint16_t)(c->bp - 0x102u))));   /* 013E INC BYTE [BP-102h] */
    }
done:
    c->sp = c->bp;                                                                    /* 016F MOV SP,BP */
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);                  /* 0171 POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));   /* 0172 RETF 0Ah */
    c->sp = (uint16_t)(c->sp + 4u + 0x0Au);
    return PR_R0802_0105_RETURNED;
}
