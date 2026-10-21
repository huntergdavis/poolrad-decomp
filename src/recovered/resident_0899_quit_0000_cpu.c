/* START.EXE resident 0899:0000, the game's QUIT routine, raw CPU (docs/re/RES_0899_QUIT_0000_CPU.md). */
#include "poolrad/recovered/resident_0899_quit_0000_cpu.h"
#include <string.h>

#define MASK (PR_R0899_QUIT_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)

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
/* LCALL seg:off with the segment word read live from CS:at (a loader relocation); returns the target segment. */
static uint16_t far_call(uint8_t *ram, pr_r0899_cpu *c, uint16_t at, uint16_t off, uint16_t next)
{
    uint16_t seg = rd16(ram, c->cs, at);
    push(ram, c, c->cs); push(ram, c, next);
    c->cs = seg; c->ip = off;
    return seg;
}

pr_r0899_quit_status pr_r0899_quit_0000_cpu(uint8_t *ram, size_t ram_size, pr_r0899_cpu *c,
                                            const pr_r0899_quit_env *env, pr_r0899_quit_diagnostic *d)
{
    uint16_t body_cs;
    if (ram == NULL || c == NULL || env == NULL || d == NULL || env->video == NULL || ram_size != PR_R0899_QUIT_RAM_SIZE ||
        c->ip != 0x0000u || c->sp < 0x000Cu || c->sp > 0xFFFEu)
        return PR_R0899_QUIT_ARGUMENT;
    d->intr_status = -1;
    memset(&d->driver, 0, sizeof d->driver); d->driver.time_status = -1;
    body_cs = c->cs;
    push(ram, c, c->bp);                                              /* 0000 PUSH BP */
    c->bp = c->sp;                                                    /* 0001 MOV BP,SP */
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, 0x5373u)]);  /* 0003 MOV AL,[5373] */
    ram[physical(c->ds, 0x5372u)] = (uint8_t)c->ax;                   /* 0006 MOV [5372],AL */
    push(ram, c, rd16(ram, c->ds, 0x2612u));                          /* 0009 PUSH WORD [2612] */
    far_call(ram, c, 0x0010u, 0x00DCu, 0x0012u);                      /* 000D LCALL 08AE:00DC */
    {
        pr_r08ae_driver_cpu m; int s;
        COPY(&m, c); s = (int)pr_r08ae_driver_call(ram, ram_size, &m, &env->driver, &d->driver); COPY(c, &m);
        if (s != PR_R08AE_DRIVER_RETURN)
            return PR_R0899_QUIT_DRIVER;
    }
    if (c->cs != body_cs || c->ip != 0x0012u)
        return PR_R0899_QUIT_TRANSFER;
    ram[physical(c->ds, 0x4A03u)] = 0u;                               /* 0012 MOV BYTE [4A03],0 */
    c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, 0x4A1Cu)]);  /* 0017 MOV AL,[4A1C] */
    ram[physical(c->ds, 0x4A02u)] = (uint8_t)c->ax;                   /* 001A MOV [4A02],AL */
    c->ax = (uint16_t)((c->ax & 0xFF00u) | 0x10u);                    /* 001D MOV AL,10h */
    push(ram, c, c->ax);                                              /* 001F PUSH AX */
    c->di = 0x4A02u;                                                  /* 0020 MOV DI,4A02h */
    push(ram, c, c->ds);                                              /* 0023 PUSH DS */
    push(ram, c, c->di);                                              /* 0024 PUSH DI */
    far_call(ram, c, 0x0028u, 0x000Bu, 0x002Au);                      /* 0025 LCALL 0A32:000B */
    {
        pr_dos_intr_cpu m;
        COPY(&m, c); d->intr_status = (int)pr_dos_intr_000b(ram, ram_size, &m, env->video, env->video_context); COPY(c, &m);
        if (d->intr_status != PR_DOS_INTR_RETURNED)
            return PR_R0899_QUIT_INTR;
    }
    if (c->cs != body_cs || c->ip != 0x002Au)
        return PR_R0899_QUIT_TRANSFER;
    /* 002A XOR AX,AX: CF OF SF AF clear, ZF PF set. */
    c->ax = 0u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~0x08D5u) | 0x0044u);
    far_call(ram, c, 0x002Fu, 0x00E9u, 0x0031u);                      /* 002C LCALL 0AF8:00E9 (Halt): EXIT */
    return PR_R0899_QUIT_EXIT;                                        /* 0031 POP BP; 0032 RETF: dead */
}
