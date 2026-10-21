/* START.EXE resident 0802:0040, the character writer, raw CPU (docs/re/RES_0802_0040_CPU.md). */
#include "poolrad/recovered/resident_0802_0040_cpu.h"
#include <string.h>

#define MASK (PR_R0802_0040_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_r0802_0040_cpu cpu_t;

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
static uint8_t arg(const uint8_t *ram, const cpu_t *c, uint16_t disp) { return rd8(ram, c->ss, (uint16_t)(c->bp + disp)); }   /* MOV AL,[BP+disp] */
static void cmp8(cpu_t *c, uint8_t a, uint8_t b)
{
    uint8_t r = (uint8_t)(a - b);
    unsigned v = r; v ^= v >> 4; v ^= v >> 2; v ^= v >> 1;
    c->flags &= (uint16_t)~0x08D5u;
    if (a < b) c->flags |= 0x01;
    if ((a ^ b ^ r) & 0x10) c->flags |= 0x10;
    if ((a ^ b) & (a ^ r) & 0x80) c->flags |= 0x800;
    if ((v & 1u) == 0) c->flags |= 0x04;
    if (r == 0) c->flags |= 0x40;
    if (r & 0x80) c->flags |= 0x80;
}
static void xor_ah(cpu_t *c)   /* XOR AH,AH: AH := 0; CF OF AF SF clear, ZF PF set */
{
    c->ax &= 0x00FFu;
    c->flags = (uint16_t)((c->flags & (uint16_t)~0x08D5u) | 0x0044u);
}

/* PUSH AX (AL := 10h first); MOV DI,4A02h; PUSH DS; PUSH DI; LCALL 0A32:000B (segment word at CS:at); Intr runs; it must return to `next`. */
static int call_intr(uint8_t *ram, size_t size, cpu_t *c, const pr_r0802_0040_env *env, pr_r0802_0040_diagnostic *d, uint16_t at, uint16_t next)
{
    uint16_t body_cs = c->cs, seg;
    pr_dos_intr_cpu m;
    set_al(c, 0x10u);
    push(ram, c, c->ax);
    c->di = 0x4A02u;
    push(ram, c, c->ds); push(ram, c, c->di);
    seg = rd16(ram, c->cs, at);
    push(ram, c, c->cs); push(ram, c, next);
    c->cs = seg; c->ip = 0x000Bu;
    COPY(&m, c); d->intr_status = (int)pr_dos_intr_000b(ram, size, &m, env->video, env->video_context); COPY(c, &m);
    if (d->intr_status != PR_DOS_INTR_RETURNED) return PR_R0802_0040_INTR;
    ++d->calls;
    if (c->cs != body_cs || c->ip != next) return PR_R0802_0040_TRANSFER;
    return PR_R0802_0040_RETURNED;
}
/* Regs.AH := 2; Regs.BH := [page]; Regs.DH := [BP+0Ch]; Regs.DL := [BP+0Eh]  (0043..0057 and 00B1..00C5) */
static void set_cursor_block(uint8_t *ram, cpu_t *c, uint16_t page_at)
{
    wr8(ram, c->ds, 0x4A03u, 2u);
    set_al(c, rd8(ram, c->ds, page_at)); wr8(ram, c->ds, 0x4A05u, (uint8_t)c->ax);
    set_al(c, arg(ram, c, 0x0Cu));       wr8(ram, c->ds, 0x4A09u, (uint8_t)c->ax);
    set_al(c, arg(ram, c, 0x0Eu));       wr8(ram, c->ds, 0x4A08u, (uint8_t)c->ax);
}

pr_r0802_0040_status pr_r0802_0040(uint8_t *ram, size_t ram_size, cpu_t *c, const pr_r0802_0040_env *env, pr_r0802_0040_diagnostic *d)
{
    pr_r0802_0040_diagnostic local;
    int s;
    if (ram == NULL || c == NULL || env == NULL || env->video == NULL || ram_size != PR_R0802_0040_RAM_SIZE || c->ip != 0x0040u ||
        c->sp < 0x0022u || c->sp > 0xFFF0u)
        return PR_R0802_0040_ARGUMENT;
    if (d == NULL) d = &local;
    d->intr_status = -1; d->calls = 0;
    push(ram, c, c->bp);                                                       /* 0040 PUSH BP */
    c->bp = c->sp;                                                             /* 0041 MOV BP,SP */
    set_cursor_block(ram, c, 0x4A1Bu);                                         /* 0043..0057 */
    if ((s = call_intr(ram, ram_size, c, env, d, 0x0065u, 0x0067u)) != 0) return (pr_r0802_0040_status)s;   /* 005A..0062 */
    wr8(ram, c->ds, 0x4A03u, 9u);                                              /* 0067 MOV BYTE [4A03],9 */
    set_al(c, arg(ram, c, 0x06u)); wr8(ram, c->ds, 0x4A02u, (uint8_t)c->ax);   /* 006C MOV AL,[BP+6]; 006F MOV [4A02],AL */
    set_al(c, rd8(ram, c->ds, 0x4A1Bu)); wr8(ram, c->ds, 0x4A05u, (uint8_t)c->ax);   /* 0072, 0075 */
    cmp8(c, rd8(ram, c->ds, 0x4A1Eu), 0u);                                     /* 0078 CMP BYTE [4A1E],0 */
    if (!(c->flags & 0x40u)) {                                                 /* 007D JNE 008F */
        set_al(c, arg(ram, c, 0x0Au)); wr8(ram, c->ds, 0x4A04u, (uint8_t)c->ax);   /* 008F, 0092 */
    } else {
        set_al(c, arg(ram, c, 0x0Au)); xor_ah(c);                              /* 007F MOV AL,[BP+0Ah]; 0082 XOR AH,AH */
        c->di = c->ax;                                                         /* 0084 MOV DI,AX */
        set_al(c, rd8(ram, c->ds, (uint16_t)(c->di + 0x25D5u)));               /* 0086 MOV AL,[DI+25D5h]: the attribute map */
        wr8(ram, c->ds, 0x4A04u, (uint8_t)c->ax);                              /* 008A MOV [4A04],AL */
    }
    set_al(c, arg(ram, c, 0x08u)); xor_ah(c);                                  /* 0095 MOV AL,[BP+8]; 0098 XOR AH,AH */
    wr16(ram, c->ds, 0x4A06u, c->ax);                                          /* 009A MOV [4A06],AX: Regs.CX := count */
    if ((s = call_intr(ram, ram_size, c, env, d, 0x00A8u, 0x00AAu)) != 0) return (pr_r0802_0040_status)s;   /* 009D..00A5 */
    cmp8(c, rd8(ram, c->ds, 0x4A1Eu), 1u);                                     /* 00AA CMP BYTE [4A1E],1 */
    if (c->flags & 0x40u) {                                                    /* 00AF JNE 0101 not taken: the second page */
        set_cursor_block(ram, c, 0x4A1Au);                                     /* 00B1..00C5 */
        if ((s = call_intr(ram, ram_size, c, env, d, 0x00D3u, 0x00D5u)) != 0) return (pr_r0802_0040_status)s;   /* 00C8..00D0 */
        wr8(ram, c->ds, 0x4A03u, 9u);                                          /* 00D5 */
        set_al(c, arg(ram, c, 0x06u)); wr8(ram, c->ds, 0x4A02u, (uint8_t)c->ax);   /* 00DA, 00DD */
        set_al(c, rd8(ram, c->ds, 0x4A1Au)); wr8(ram, c->ds, 0x4A05u, (uint8_t)c->ax);   /* 00E0, 00E3 */
        set_al(c, arg(ram, c, 0x0Au)); wr8(ram, c->ds, 0x4A04u, (uint8_t)c->ax);   /* 00E6, 00E9 */
        set_al(c, arg(ram, c, 0x08u)); xor_ah(c);                              /* 00EC, 00EF */
        wr16(ram, c->ds, 0x4A06u, c->ax);                                      /* 00F1 */
        if ((s = call_intr(ram, ram_size, c, env, d, 0x00FFu, 0x0101u)) != 0) return (pr_r0802_0040_status)s;   /* 00F4..00FC */
    }
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);          /* 0101 POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));   /* 0102 RETF 0Ah */
    c->sp = (uint16_t)(c->sp + 4u + 0x0Au);
    return PR_R0802_0040_RETURNED;
}
