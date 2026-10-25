/* GAME.OVR unit 0x25 +18EB: the ECL word compare, full CPU state (docs/re/OVL_0025_18EB_CPU.md). */
#include "poolrad/recovered/overlay_u25_compare_words_18eb_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

#define MASK (PR_U25_18EB_RAM_SIZE - 1u)
#define ARITH 0x08D5u
typedef pr_u25_18eb_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 0x0004u; }
static void cmp16(cpu_t *c, uint16_t a, uint16_t b)
{
    uint16_t r = (uint16_t)(a - b), f = parity((uint8_t)r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
#define COPY_CPU(d, s) do { (d)->ax = (s)->ax; (d)->bx = (s)->bx; (d)->cx = (s)->cx; (d)->dx = (s)->dx; (d)->si = (s)->si; (d)->di = (s)->di; \
    (d)->bp = (s)->bp; (d)->sp = (s)->sp; (d)->cs = (s)->cs; (d)->ip = (s)->ip; (d)->ds = (s)->ds; (d)->es = (s)->es; (d)->ss = (s)->ss; \
    (d)->flags = (s)->flags; } while (0)

/* The six compares: the flag byte, then the condition that SKIPS the store (the original Jcc). */
static int skip(uint16_t f, int k)
{
    int cf = (f & 0x0001u) != 0, zf = (f & 0x0040u) != 0;
    switch (k) {
    case 0: return !zf;            /* 1905 JNE */
    case 1: return zf;             /* 1912 JE */
    case 2: return !cf;            /* 191F JAE */
    case 3: return cf || zf;       /* 192C JBE */
    case 4: return !cf && !zf;     /* 1939 JA */
    default: return cf;            /* 1946 JB */
    }
}

pr_u25_18eb_status pr_u25_compare_words_18eb_cpu(uint8_t *ram, size_t ram_size, pr_u25_18eb_cpu *c)
{
    uint16_t ret_ip, ret_cs, caller_cs; pr_system_leaf_cpu state; int k;
    if (ram == NULL || c == NULL || ram_size != PR_U25_18EB_RAM_SIZE || c->ip != 0x18EBu || c->sp < 0x000Eu || c->sp > 0xFFF8u)
        return PR_U25_18EB_ARGUMENT;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u)); caller_cs = c->cs;
    push(ram, c, c->bp); c->bp = c->sp;                                                     /* 18EB PUSH BP; 18EC MOV BP,SP */
    c->di = 0x6F78u; push(ram, c, c->ds); push(ram, c, c->di);                              /* 18EE MOV DI,6F78; 18F1 PUSH DS; 18F2 PUSH DI */
    c->ax = 0x0006u; push(ram, c, c->ax);                                                   /* 18F3 MOV AX,6; 18F6 PUSH AX */
    c->ax &= 0xFF00u; push(ram, c, c->ax);                                                  /* 18F7 MOV AL,0; 18F9 PUSH AX */
    c->ip = 0x18FAu;
    if (rd8(ram, caller_cs, 0x18FAu) != 0x9Au || rd16(ram, caller_cs, 0x18FBu) != 0x16A8u) return PR_U25_18EB_CODE;
    push(ram, c, caller_cs); push(ram, c, 0x18FFu);                                         /* 18FA LCALL 0AF8:16A8 (relocated segment word) */
    c->cs = rd16(ram, caller_cs, 0x18FDu); c->ip = 0x16A8u;
    COPY_CPU(&state, c);
    (void)pr_system_fillchar_16a8(ram, ram_size, &state);                                   /* checked above: cannot be ARGUMENT */
    COPY_CPU(c, &state);
    if (c->ip != 0x18FFu || c->cs != caller_cs) return PR_U25_18EB_TRANSFER;
    for (k = 0; k < 6; ++k) {                                                               /* 18FF..194C */
        c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));                                   /* MOV AX,[BP+8] */
        cmp16(c, c->ax, rd16(ram, c->ss, (uint16_t)(c->bp + 6u)));                          /* CMP AX,[BP+6] */
        if (!skip(c->flags, k)) wr8(ram, c->ds, (uint16_t)(0x6F78u + (unsigned)k), 1u);     /* Jcc; MOV BYTE [6F78+k],1 */
    }
    c->bp = pop(ram, c);                                                                    /* 194D POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u);               /* 194E RETF 4 */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U25_18EB_TRANSFER;
    return PR_U25_18EB_RETURNED;
}
