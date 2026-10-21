/* START.EXE resident 0802:0175, the block filler (side ii, non-credit), raw CPU (docs/re/RES_0802_0175_CPU.md). */
#include "poolrad/recovered/resident_0802_0175_cpu.h"
#include <string.h>

#define MASK (PR_R0802_0175_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_r0802_0175_cpu cpu_t;

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
static void push_arg(uint8_t *ram, cpu_t *c, uint16_t disp)                       /* MOV AL,[BP+disp]; PUSH AX */
{
    set_al(c, ram[physical(c->ss, (uint16_t)(c->bp + disp))]);
    push(ram, c, c->ax);
}

pr_r0802_0175_status pr_r0802_0175(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_0040_env *env, pr_r0802_0175_diagnostic *diagnostic)
{
    pr_r0802_0175_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint16_t home, target;
    if (ram == NULL || c == NULL || env == NULL || env->video == NULL || ram_size != PR_R0802_0175_RAM_SIZE || c->ip != 0x0175u ||
        c->sp < 0x0040u || c->sp > 0xFFF4u)
        return PR_R0802_0175_ARGUMENT;
    memset(d, 0, sizeof *d); d->callee_status = -1; d->display.intr_status = -1;
    home = c->cs;
    push(ram, c, c->bp); c->bp = c->sp;                                               /* 0175 PUSH BP; 0176 MOV BP,SP */
    push_arg(ram, c, 0x0Cu);                                                          /* 0178 column */
    push_arg(ram, c, 0x0Au);                                                          /* 017C row */
    push_arg(ram, c, 0x08u);                                                          /* 0180 attribute */
    push_arg(ram, c, 0x06u);                                                          /* 0184 count */
    set_al(c, 0xDBu); push(ram, c, c->ax);                                            /* 0188 MOV AL,DBh; 018A PUSH AX */
    push(ram, c, c->cs);                                                              /* 018B PUSH CS */
    target = (uint16_t)(0x018Fu + rd16(ram, home, 0x018Du));                          /* 018C CALL near 0040 */
    push(ram, c, 0x018Fu); c->ip = target;
    if (target != 0x0040u) return PR_R0802_0175_CALLEE;
    {
        pr_r0802_0040_cpu m; int s;
        COPY(&m, c); s = (int)pr_r0802_0040(ram, ram_size, &m, env, &d->display); COPY(c, &m);
        d->callee_status = s;
        if (s != PR_R0802_0040_RETURNED) return PR_R0802_0175_DISPLAY;
        if (c->cs != home || c->ip != 0x018Fu) return PR_R0802_0175_CALLEE;
    }
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);                  /* 018F POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));   /* 0190 RETF 8 */
    c->sp = (uint16_t)(c->sp + 4u + 8u);
    return PR_R0802_0175_RETURNED;
}
