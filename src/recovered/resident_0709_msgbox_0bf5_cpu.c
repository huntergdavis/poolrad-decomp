/* START.EXE resident 0709:0BF5, the message-line prompt, raw CPU (docs/re/RES_0709_MSGBOX_0BF5_CPU.md). */
#include "poolrad/recovered/resident_0709_msgbox_0bf5_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_r0709_0bf5_cpu cpu_t;

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

pr_r0709_0bf5_status pr_r0709_msgbox_0bf5(pr_bus *bus, size_t ram_size, cpu_t *c, const pr_r0709_0bf5_env *env, pr_r0709_0bf5_diagnostic *diagnostic)
{
    pr_r0709_0bf5_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint8_t *ram;
    uint16_t home, seg, target;
    int s;
    if (bus == NULL || bus->ram == NULL || c == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0BF5u ||
        c->sp < 0x016Au || c->sp > 0xFFF6u)
        return PR_R0709_0BF5_ARGUMENT;
    ram = bus->ram;
    memset(d, 0, sizeof *d); d->callee_status = -1; d->key_status = -1;
    d->space.callee = d->space.status = d->space.glyph_callee = d->space.glyph_status = -1;
    d->printer.callee = d->printer.status = d->printer.glyph_callee = d->printer.glyph_status = -1;
    home = c->cs;
    push(ram, c, c->bp); c->bp = c->sp;                                               /* 0BF5 PUSH BP; MOV BP,SP */
    { unsigned a = c->sp; c->sp = (uint16_t)(a - 0x2Au); sub_flags(c, a, 0x2Au); }    /* 0BF8 SUB SP,2Ah */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 6u)); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));   /* 0BFB LES DI,[BP+6] */
    push(ram, c, c->es); push(ram, c, c->di);                                         /* 0BFE PUSH ES; 0BFF PUSH DI */
    c->di = (uint16_t)(c->bp - 0x29u);                                                /* 0C00 LEA DI,[BP-29h] */
    push(ram, c, c->ss); push(ram, c, c->di);                                         /* 0C03 PUSH SS; 0C04 PUSH DI */
    c->ax = 0x0028u; push(ram, c, c->ax);                                             /* 0C05 MOV AX,0028h; 0C08 PUSH AX */
    seg = rd16(ram, home, 0x0C0Cu);                                                   /* 0C09 LCALL 0AF8:0B25 (the segment word is a relocation) */
    push(ram, c, c->cs); push(ram, c, 0x0C0Eu);
    c->cs = seg; c->ip = rd16(ram, home, 0x0C0Au);
    {
        pr_system_0b25_cpu m;
        COPY(&m, c); s = (int)pr_system_strstore_0b25(ram, ram_size, &m); COPY(c, &m);
        d->site = 0x0C09u; d->callee_status = s;
        if (s != PR_SYSTEM_0B25_RETURNED || c->cs != home || c->ip != 0x0C0Eu) return PR_R0709_0BF5_CALLEE;
        d->site = 0; d->callee_status = -1;
    }
    push_imm(ram, c, 0x00u); push_imm(ram, c, 0x18u); push_imm(ram, c, 0x00u); push_imm(ram, c, 0x28u);   /* 0C0E..0C19 */
    push(ram, c, c->cs);                                                              /* 0C1A PUSH CS */
    target = near_target(ram, home, 0x0C1Cu, 0x0C1Eu);                                /* 0C1B CALL near 05EB */
    push(ram, c, 0x0C1Eu); c->ip = target; d->site = 0x0C1Bu;
    if (target != 0x05EBu) return PR_R0709_0BF5_CALLEE;
    {
        pr_r0709_space_cpu m;
        COPY(&m, c); s = (int)pr_r0709_space_05eb_cpu(bus, ram_size, &m, env->space_loopbacks, &d->space); COPY(c, &m);
        d->callee_status = s;
        if (s != PR_R0709_SPACE_CPU_OK) return PR_R0709_0BF5_SPACE;
        if (c->cs != home || c->ip != 0x0C1Eu) return PR_R0709_0BF5_CALLEE;
        d->site = 0; d->callee_status = -1;
    }
    push_imm(ram, c, 0x00u); push_imm(ram, c, 0x18u);                                  /* 0C1E column 0; 0C21 row 18h */
    set_al(c, ram[physical(c->ss, (uint16_t)(c->bp + 0x0Au))]); push(ram, c, c->ax);   /* 0C24 MOV AL,[BP+0Ah]; 0C27 PUSH AX: the colour */
    c->di = (uint16_t)(c->bp - 0x29u);                                                /* 0C28 LEA DI,[BP-29h] */
    push(ram, c, c->ss); push(ram, c, c->di);                                         /* 0C2B PUSH SS; 0C2C PUSH DI */
    push(ram, c, c->cs);                                                              /* 0C2D PUSH CS */
    target = near_target(ram, home, 0x0C2Fu, 0x0C31u);                                /* 0C2E CALL near 0626 */
    push(ram, c, 0x0C31u); c->ip = target; d->site = 0x0C2Eu;
    if (target != 0x0626u) return PR_R0709_0BF5_CALLEE;
    {
        pr_r0709_printer_cpu m;
        COPY(&m, c);
        s = (int)pr_r0709_printer_0626_cpu(bus, ram_size, &m, env->printer_loopbacks, env->glyph_loopbacks, &d->printer); COPY(c, &m);
        d->callee_status = s;
        if (s != PR_R0709_PRINTER_CPU_OK) return PR_R0709_0BF5_PRINTER;
        if (c->cs != home || c->ip != 0x0C31u) return PR_R0709_0BF5_CALLEE;
        d->site = 0; d->callee_status = -1;
    }
    seg = rd16(ram, home, 0x0C34u);                                                   /* 0C31 LCALL 0899:0059 (the segment word is a relocation) */
    push(ram, c, c->cs); push(ram, c, 0x0C36u);
    c->cs = seg; c->ip = rd16(ram, home, 0x0C32u); d->site = 0x0C31u;
    {
        pr_r0899_cpu m;
        COPY(&m, c); d->key_status = (int)pr_r0899_readkey_0059_cpu(ram, ram_size, &m, &env->key, &d->key); COPY(c, &m);
        if (d->key_status != PR_R0899_KEY_RETURN) return PR_R0709_0BF5_KEY;
        if (c->cs != home || c->ip != 0x0C36u) return PR_R0709_0BF5_CALLEE;
        d->site = 0;
    }
    ram[physical(c->ss, (uint16_t)(c->bp - 0x2Au))] = (uint8_t)c->ax;                  /* 0C36 MOV [BP-2Ah],AL */
    c->sp = c->bp;                                                                    /* 0C39 MOV SP,BP */
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);                  /* 0C3B POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));   /* 0C3C RETF 6 */
    c->sp = (uint16_t)(c->sp + 4u + 6u);
    return PR_R0709_0BF5_RETURNED;
}
