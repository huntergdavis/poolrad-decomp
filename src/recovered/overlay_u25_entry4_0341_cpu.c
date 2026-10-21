/* Unit 0x25 +0341, the 1E00h-byte screen block loader, raw CPU (docs/re/OVL_0025_0341_CPU.md). */
#include "poolrad/recovered/overlay_u25_entry4_0341_cpu.h"
#include "poolrad/recovered/system_freemem_0254_cpu.h"
#include "poolrad/recovered/system_str_1366_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
#define CONTINUE (-1)
typedef pr_u25_0341_cpu cpu_t;
typedef struct { uint8_t *ram; pr_bus *bus; size_t size; cpu_t *c; const pr_u25_0341_env *e; pr_u25_0341_diagnostic *d; } machine;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)v; ram[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static void push(machine *m, uint16_t v) { m->c->sp = (uint16_t)(m->c->sp - 2u); wr16(m->ram, m->c->ss, m->c->sp, v); }
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static uint16_t bp16(machine *m, int off) { return rd16(m->ram, m->c->ss, (uint16_t)(m->c->bp + off)); }
static void lea_di(machine *m, int off) { m->c->di = (uint16_t)(m->c->bp + off); }
static void push_ss_di(machine *m) { push(m, m->c->ss); push(m, m->c->di); }
static void push_cs_di(machine *m, uint16_t imm) { m->c->di = imm; push(m, m->c->cs); push(m, m->c->di); }
static unsigned parity(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) ? 0u : 0x04u; }
#define ARITH 0x08D5u
static void sub16_flags(cpu_t *c, unsigned a, unsigned b)
{
    unsigned r = (a - b) & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? 0x40u : 0u) | (r & 0x8000u ? 0x80u : 0u) | parity((uint8_t)r) | (a < b ? 0x01u : 0u) |
                          ((a ^ b ^ r) & 0x10u) | (((a ^ b) & (a ^ r) & 0x8000u) ? 0x800u : 0u));
}
static void add16_flags(cpu_t *c, unsigned a, unsigned b)
{
    unsigned r = (a + b) & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? 0x40u : 0u) | (r & 0x8000u ? 0x80u : 0u) | parity((uint8_t)r) | (a + b > 0xFFFFu ? 0x01u : 0u) |
                          ((a ^ b ^ r) & 0x10u) | ((~(a ^ b) & (a ^ r) & 0x8000u) ? 0x800u : 0u));
}
static void dec16_flags(cpu_t *c, unsigned a)                                      /* DEC word: CF kept */
{
    unsigned r = (a - 1u) & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~(ARITH & ~0x01u)) | (r == 0u ? 0x40u : 0u) | (r & 0x8000u ? 0x80u : 0u) | parity((uint8_t)r) | ((a ^ 1u ^ r) & 0x10u) |
                          (r == 0x7FFFu ? 0x800u : 0u));
}
static void zero_flags(cpu_t *c) { c->flags = (uint16_t)((c->flags & ~ARITH) | 0x44u); }   /* XOR r,r */

static const uint16_t ENTRY_OF[] = {0x16A8u, 0x002Fu, 0x0B0Bu, 0x0626u, 0x1366u, 0x0B98u, 0x0716u, 0x1685u, 0x0254u};

static int leaf_stop(int s) { return s == PR_SYSTEM_LEAF_RETURNED ? CONTINUE : (int)PR_U25_0341_CALLEE; }

static int far_call(machine *m, uint16_t site, pr_u25_0341_callee kind)
{
    cpu_t *c = m->c;
    pr_u25_0341_diagnostic *d = m->d;
    uint8_t *ram = m->ram;
    uint16_t home = c->cs, back = (uint16_t)(site + 5u), seg, ip = ENTRY_OF[kind];
    int s = CONTINUE;
    d->site = site; d->callee = (int)kind; d->callee_status = -1; c->ip = site;
    if (rd8(ram, home, site) != 0x9Au || rd16(ram, home, (uint16_t)(site + 1u)) != ip) return (int)PR_U25_0341_CODE;
    seg = rd16(ram, home, (uint16_t)(site + 3u));
    if (kind == PR_U25_0341_ROWCLEAR) {                  /* the stub entry: a far JMP to +06FB in the loaded unit 0x3B0 */
        if (rd8(ram, seg, 0x2Fu) != 0xEAu || rd16(ram, seg, 0x30u) != 0x06FBu) return (int)PR_U25_0341_CODE;
        ip = 0x06FBu; seg = rd16(ram, seg, 0x32u);
    }
    push(m, home); push(m, back);
    c->cs = seg; c->ip = ip;
    switch (kind) {
    case PR_U25_0341_FILL: case PR_U25_0341_STRLOAD: case PR_U25_0341_STR: case PR_U25_0341_STRCAT: case PR_U25_0341_MOVE: {
        pr_system_leaf_cpu x; int r;
        COPY(&x, c);
        r = kind == PR_U25_0341_FILL ? (int)pr_system_fillchar_16a8(ram, m->size, &x)
          : kind == PR_U25_0341_STRLOAD ? (int)pr_system_strload_0b0b(ram, m->size, &x)
          : kind == PR_U25_0341_STR ? (int)pr_system_str_1366(ram, m->size, &x)
          : kind == PR_U25_0341_STRCAT ? (int)pr_system_strcat_0b98(ram, m->size, &x) : (int)pr_system_move_1685(ram, m->size, &x);
        COPY(c, &x); d->callee_status = r; s = leaf_stop(r);
        break;
    }
    case PR_U25_0341_FREEMEM: {
        pr_system_freemem_cpu x; int r;
        COPY(&x, c); r = (int)pr_system_freemem_0254(ram, m->size, &x, m->e->heap_steps); COPY(c, &x);
        d->callee_status = r;
        s = r == PR_SYSTEM_FREEMEM_RETURNED ? CONTINUE : r == PR_SYSTEM_FREEMEM_RUNERROR ? (int)PR_U25_0341_RUNERROR
          : r == PR_SYSTEM_FREEMEM_TRANSFER ? (int)PR_U25_0341_TRANSFER : r == PR_SYSTEM_FREEMEM_LIMIT ? (int)PR_U25_0341_LIMIT : (int)PR_U25_0341_CALLEE;
        break;
    }
    case PR_U25_0341_ROWCLEAR: {
        pr_u3b0_06fb_cpu x; int r;
        COPY(&x, c); r = (int)pr_u3b0_row_clear_06fb_cpu(m->bus, m->size, &x, m->e->space_loopbacks, &d->rowclear); COPY(c, &x);
        d->callee_status = r;
        s = r == PR_U3B0_06FB_OK ? CONTINUE : r == PR_U3B0_06FB_CODE ? (int)PR_U25_0341_CODE : r == PR_U3B0_06FB_TRANSFER ? (int)PR_U25_0341_TRANSFER : (int)PR_U25_0341_CALLEE;
        break;
    }
    case PR_U25_0341_PRINT: {
        pr_r0709_printer_cpu x; int r;
        COPY(&x, c); r = (int)pr_r0709_printer_0626_cpu(m->bus, m->size, &x, m->e->printer_loopbacks, m->e->glyph_loopbacks, &d->printer); COPY(c, &x);
        d->callee_status = r;
        s = r == PR_R0709_PRINTER_CPU_OK ? CONTINUE : r == PR_R0709_PRINTER_CPU_LIMIT ? (int)PR_U25_0341_LIMIT : r == PR_R0709_PRINTER_CPU_CODE ? (int)PR_U25_0341_CODE
          : r == PR_R0709_PRINTER_CPU_TRANSFER ? (int)PR_U25_0341_TRANSFER : (int)PR_U25_0341_CALLEE;
        break;
    }
    case PR_U25_0341_LOAD: {
        pr_r0802_0716c_cpu x; int r;
        COPY(&x, c); r = (int)pr_r0802_0716_composed(ram, m->size, &x, &m->e->load, &d->load); COPY(c, &x);
        d->callee_status = r;
        s = r == PR_R0802_0716C_RETURNED ? CONTINUE : r == PR_R0802_0716C_EXIT ? (int)PR_U25_0341_EXIT : r == PR_R0802_0716C_RUNERROR ? (int)PR_U25_0341_RUNERROR
          : r == PR_R0802_0716C_DIVIDE ? (int)PR_U25_0341_DIVIDE : r == PR_R0802_0716C_BREAK ? (int)PR_U25_0341_BREAK
          : r == PR_R0802_0716C_STALLED ? (int)PR_U25_0341_STALLED : r == PR_R0802_0716C_LIMIT ? (int)PR_U25_0341_LIMIT
          : r == PR_R0802_0716C_SERVICE ? (int)PR_U25_0341_SERVICE : (int)PR_U25_0341_CALLEE;
        break;
    }
    default: return (int)PR_U25_0341_CALLEE;
    }
    if (s != CONTINUE) return s;
    if (c->cs != home || c->ip != back) return (int)PR_U25_0341_TRANSFER;
    ++d->calls; d->site = 0; d->callee = -1; d->callee_status = -1;
    return CONTINUE;
}

#define CALL(site, kind) do { int r_ = far_call(&m, (site), (kind)); if (r_ != CONTINUE) return (pr_u25_0341_status)r_; } while (0)

pr_u25_0341_status pr_u25_entry4_0341_cpu(pr_bus *bus, size_t ram_size, pr_u25_0341_cpu *c, const pr_u25_0341_env *env, pr_u25_0341_diagnostic *diagnostic)
{
    pr_u25_0341_diagnostic local, *d = diagnostic ? diagnostic : &local;
    machine m;
    uint8_t *ram;
    if (bus == NULL || bus->ram == NULL || c == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0341u || c->sp < 0x09ECu || c->sp > 0xFFFAu)
        return PR_U25_0341_ARGUMENT;
    ram = bus->ram;
    memset(d, 0, sizeof *d); d->callee = -1; d->callee_status = -1;
    memset(&d->rowclear, 0, sizeof d->rowclear); d->rowclear.space_status = -1;
    d->printer.callee = d->printer.status = d->printer.glyph_callee = d->printer.glyph_status = -1;
    m.ram = ram; m.bus = bus; m.size = ram_size; m.c = c; m.e = env; m.d = d;
    push(&m, c->bp); c->bp = c->sp;                                                    /* 0341 PUSH BP; MOV BP,SP */
    { unsigned x = c->sp; c->sp = (uint16_t)(x - 0x112u); sub16_flags(c, x, 0x112u); } /* 0344 SUB SP,112h */
    c->di = rd16(ram, c->ds, 0x49DEu); c->es = rd16(ram, c->ds, 0x49E0u);              /* 0348 LES DI,[49DEh] */
    push(&m, c->es); push(&m, c->di);                                                  /* 034C, 034D: X := the buffer */
    c->ax = 0x1E00u; push(&m, c->ax);                                                  /* 034E MOV AX,1E00h; PUSH AX */
    set_al(c, 0u); push(&m, c->ax); /* 0352 MOV AL,0; PUSH AX */
    CALL(0x0355u, PR_U25_0341_FILL);
    wr8(ram, c->ds, 0x557Cu, 0u);                                                      /* 035A MOV byte [557Ch],0 */
    for (;;) {                                                                         /* 035F: the retry loop */
        CALL(0x035Fu, PR_U25_0341_ROWCLEAR);
        set_al(c, 0u); push(&m, c->ax); set_al(c, 0x18u); push(&m, c->ax); set_al(c, 0x0Fu); push(&m, c->ax);   /* 0364..036C: x, y, colour */
        lea_di(&m, -0x22); push_ss_di(&m); push_cs_di(&m, 0x0322u);
        CALL(0x0377u, PR_U25_0341_STRLOAD);
        CALL(0x037Cu, PR_U25_0341_PRINT);
        set_al(c, rd8(ram, c->ds, 0x5376u)); c->ax &= 0x00FFu; zero_flags(c);          /* 0381 MOV AL,[5376h]; XOR AH,AH */
        c->dx = 0; zero_flags(c); push(&m, c->dx); push(&m, c->ax);                    /* 0386 XOR DX,DX; PUSH DX; PUSH AX */
        c->ax = 0; zero_flags(c); push(&m, c->ax);                                     /* 038A XOR AX,AX; PUSH AX: width 0 */
        lea_di(&m, -6); push_ss_di(&m); c->ax = 1; push(&m, c->ax);                    /* 038D S := [BP-6], M := 1 */
        CALL(0x0396u, PR_U25_0341_STR);
        lea_di(&m, -0x10C); push_ss_di(&m); push_cs_di(&m, 0x0338u);
        CALL(0x03A6u, PR_U25_0341_STRLOAD);
        lea_di(&m, -6); push_ss_di(&m);
        CALL(0x03B0u, PR_U25_0341_STRCAT);
        lea_di(&m, -0x111); push_ss_di(&m); push_cs_di(&m, 0x033Cu);
        CALL(0x03C0u, PR_U25_0341_STRLOAD);
        set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 6u))); push(&m, c->ax);          /* 03C5 MOV AL,[BP+6]; PUSH AX: the id */
        lea_di(&m, -8); push_ss_di(&m); lea_di(&m, -0xC); push_ss_di(&m);
        CALL(0x03D3u, PR_U25_0341_LOAD);
        sub16_flags(c, bp16(&m, -8), 2u);                                              /* 03D8 CMP word [BP-8],2 */
        c->ip = 0x03DCu;
        if (!(c->flags & 1u)) break;                                                   /* 03DC JB 035F not taken */
        if (d->retries_taken >= env->retries) { d->site = 0x03DCu; return PR_U25_0341_LIMIT; }   /* the budget: the JB is not executed */
        ++d->retries_taken;
    }
    c->di = bp16(&m, -0xC); c->es = bp16(&m, -0xA);                                    /* 03DE LES DI,[BP-0Ch] */
    add16_flags(c, c->di, 2u); c->di = (uint16_t)(c->di + 2u);                         /* 03E1 ADD DI,2 */
    push(&m, c->es); push(&m, c->di);                                                  /* 03E5, 03E6: the source := block + 2 */
    c->di = rd16(ram, c->ds, 0x49DEu); c->es = rd16(ram, c->ds, 0x49E0u);              /* 03E7 LES DI,[49DEh] */
    push(&m, c->es); push(&m, c->di);                                                  /* 03EB, 03EC: the destination := the buffer */
    c->ax = bp16(&m, -8);                                                              /* 03ED MOV AX,[BP-8] */
    dec16_flags(c, c->ax); c->ax = (uint16_t)(c->ax - 1u);                             /* 03F0 DEC AX */
    dec16_flags(c, c->ax); c->ax = (uint16_t)(c->ax - 1u); /* 03F1 DEC AX */
    push(&m, c->ax);                                                                   /* 03F2 PUSH AX: size - 2 */
    CALL(0x03F3u, PR_U25_0341_MOVE);
    push(&m, bp16(&m, -0xA)); push(&m, bp16(&m, -0xC)); push(&m, bp16(&m, -8));       /* 03F8..03FE: FreeMem(block, size) */
    CALL(0x0401u, PR_U25_0341_FREEMEM);
    CALL(0x0406u, PR_U25_0341_ROWCLEAR);
    c->sp = c->bp;                                                                     /* 040B MOV SP,BP */
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);                   /* 040D POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u)); /* 040E RETF 2 */
    c->sp = (uint16_t)(c->sp + 4u + 2u);
    return PR_U25_0341_RETURNED;
}
