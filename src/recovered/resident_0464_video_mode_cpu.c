/* START.EXE resident 0464:00CF (video-mode set over Dos.Intr) and 0464:00FF (restore), raw CPU
 * (docs/re/RES_0464_VIDEO_MODE_00CF_CPU.md). */
#include "poolrad/recovered/resident_0464_video_mode_cpu.h"
#include <string.h>

#define MASK (PR_R0464_VIDEO_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_r0464_video_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, pr_r0464_video_cpu *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }

static void set_al(pr_r0464_video_cpu *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }

/* The body after the caller's frame is in place (CS:IP = xxxx:00CF). */
static pr_r0464_video_status set_mode(uint8_t *ram, size_t ram_size, pr_r0464_video_cpu *c, const pr_r0464_video_env *env,
                                      int *intr_status)
{
    uint16_t body_cs = c->cs, a, r;
    push(ram, c, c->bp);                                              /* 00CF PUSH BP */
    c->bp = c->sp;                                                    /* 00D0 MOV BP,SP */
    a = c->sp; r = (uint16_t)(a - 2u);                                /* 00D2 SUB SP,2 */
    c->flags &= (uint16_t)~0x08D5u;
    if (a < 2u) c->flags |= 0x0001u;
    if ((a ^ 2u ^ r) & 0x0010u) c->flags |= 0x0010u;
    if ((a ^ 2u) & (a ^ r) & 0x8000u) c->flags |= 0x0800u;
    {
        uint8_t p = (uint8_t)r; p ^= (uint8_t)(p >> 4); p ^= (uint8_t)(p >> 2); p ^= (uint8_t)(p >> 1);
        if (!(p & 1u)) c->flags |= 0x0004u;
    }
    if (r == 0u) c->flags |= 0x0040u;
    if (r & 0x8000u) c->flags |= 0x0080u;
    c->sp = r;
    set_al(c, ram[physical(c->ds, 0x4A1Du)]);                         /* 00D5 MOV AL,[4A1D] */
    ram[physical(c->ds, 0x4A1Cu)] = (uint8_t)c->ax;                   /* 00D8 MOV [4A1C],AL */
    ram[physical(c->ds, 0x4A03u)] = 0u;                               /* 00DB MOV BYTE [4A03],0 */
    set_al(c, ram[physical(c->ss, (uint16_t)(c->bp + 6u))]);          /* 00E0 MOV AL,[BP+6] */
    ram[physical(c->ds, 0x4A02u)] = (uint8_t)c->ax;                   /* 00E3 MOV [4A02],AL */
    set_al(c, 0x10u);                                                 /* 00E6 MOV AL,10h */
    push(ram, c, c->ax);                                              /* 00E8 PUSH AX */
    c->di = 0x4A02u;                                                  /* 00E9 MOV DI,4A02h */
    push(ram, c, c->ds);                                              /* 00EC PUSH DS */
    push(ram, c, c->di);                                              /* 00ED PUSH DI */
    {                                                                 /* 00EE LCALL 0A32:000B, segment read live at CS:00F1 */
        uint16_t seg = rd16(ram, c->cs, 0x00F1u);
        pr_dos_intr_cpu m;
        push(ram, c, c->cs); push(ram, c, 0x00F3u);
        c->cs = seg; c->ip = 0x000Bu;
        COPY(&m, c);
        *intr_status = (int)pr_dos_intr_000b(ram, ram_size, &m, env->video, env->video_context);
        COPY(c, &m);
        if (*intr_status != (int)PR_DOS_INTR_RETURNED)
            return PR_R0464_VIDEO_INTR;
    }
    if (c->cs != body_cs || c->ip != 0x00F3u)
        return PR_R0464_VIDEO_TRANSFER;
    set_al(c, ram[physical(c->ss, (uint16_t)(c->bp + 6u))]);          /* 00F3 MOV AL,[BP+6] */
    ram[physical(c->ds, 0x4A1Du)] = (uint8_t)c->ax;                   /* 00F6 MOV [4A1D],AL */
    c->sp = c->bp;                                                    /* 00F9 MOV SP,BP */
    c->bp = pop(ram, c);                                              /* 00FB POP BP */
    c->ip = pop(ram, c);                                              /* 00FC RETF 2 */
    c->cs = pop(ram, c);
    c->sp = (uint16_t)(c->sp + 2u);
    return PR_R0464_VIDEO_OK;
}

pr_r0464_video_status pr_r0464_set_mode_00cf_cpu(uint8_t *ram, size_t ram_size, pr_r0464_video_cpu *cpu,
                                                 const pr_r0464_video_env *env, int *intr_status)
{
    if (intr_status == NULL || ram == NULL || cpu == NULL || env == NULL || env->video == NULL ||
        ram_size != PR_R0464_VIDEO_RAM_SIZE || cpu->ip != 0x00CFu || cpu->sp < 0x0022u || cpu->sp > 0xFFFAu)
        return PR_R0464_VIDEO_ARGUMENT;
    *intr_status = -1;
    return set_mode(ram, ram_size, cpu, env, intr_status);
}

pr_r0464_video_status pr_r0464_restore_mode_00ff_cpu(uint8_t *ram, size_t ram_size, pr_r0464_video_cpu *c,
                                                     const pr_r0464_video_env *env, int *intr_status)
{
    uint16_t body_cs;
    pr_r0464_video_status s;
    if (intr_status == NULL || ram == NULL || c == NULL || env == NULL || env->video == NULL ||
        ram_size != PR_R0464_VIDEO_RAM_SIZE || c->ip != 0x00FFu || c->sp < 0x002Au || c->sp > 0xFFFCu)
        return PR_R0464_VIDEO_ARGUMENT;
    *intr_status = -1;
    body_cs = c->cs;
    push(ram, c, c->bp);                                              /* 00FF PUSH BP */
    c->bp = c->sp;                                                    /* 0100 MOV BP,SP */
    set_al(c, ram[physical(c->ds, 0x4A1Cu)]);                         /* 0102 MOV AL,[4A1C] */
    push(ram, c, c->ax);                                              /* 0105 PUSH AX */
    push(ram, c, c->cs);                                              /* 0106 PUSH CS */
    push(ram, c, 0x010Au);                                            /* 0107 CALL 00CF */
    c->ip = 0x00CFu;
    s = set_mode(ram, ram_size, c, env, intr_status);
    if (s != PR_R0464_VIDEO_OK)
        return s;
    if (c->cs != body_cs || c->ip != 0x010Au)
        return PR_R0464_VIDEO_TRANSFER;
    c->bp = pop(ram, c);                                              /* 010A POP BP */
    c->ip = pop(ram, c);                                              /* 010B RETF */
    c->cs = pop(ram, c);
    return PR_R0464_VIDEO_OK;
}
