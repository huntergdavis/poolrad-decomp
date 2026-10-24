/* Original unit 0x38 +0CC3: remove a map object (U9E dispatcher slot 40) (docs/re/OVL_0038_0CC3_CPU.md). */
#include "poolrad/recovered/overlay_u38_0cc3_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_freemem_0254_cpu.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_u38_0cc3_regs cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
static uint8_t brd8(pr_bus *bus, uint16_t seg, uint16_t off) { return pr_bus_read8(bus, physical(seg, off)); }
static void bwr8(pr_bus *bus, uint16_t seg, uint16_t off, uint8_t v) { pr_bus_write8(bus, physical(seg, off), v); }
static uint16_t brd16(pr_bus *bus, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(pr_bus_read8(bus, p) | ((uint16_t)pr_bus_read8(bus, (p + 1u) & MASK) << 8u)); }
static void bwr16(pr_bus *bus, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); pr_bus_write8(bus, p, (uint8_t)v); pr_bus_write8(bus, (p + 1u) & MASK, (uint8_t)(v >> 8u)); }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 4u; }
static uint16_t szp(uint16_t r, unsigned bits)
{ uint16_t m = bits == 8u ? 0x00FFu : 0xFFFFu; r &= m; return (uint16_t)(parity((uint8_t)r) | (r ? 0u : 0x0040u) | ((r & (bits == 8u ? 0x80u : 0x8000u)) ? 0x0080u : 0u)); }
/* ADD/SUB/CMP at 8 or 16 bits: every arithmetic flag (PF from the low byte, as the stored FillFlags path). */
static uint16_t arith(cpu_t *c, uint16_t a, uint16_t b, int subtract, unsigned carry, unsigned bits)
{
    uint32_t mask = bits == 8u ? 0xFFu : 0xFFFFu, sign = bits == 8u ? 0x80u : 0x8000u;
    uint32_t wide = subtract ? (uint32_t)(a & mask) - (b & mask) - carry : (uint32_t)(a & mask) + (b & mask) + carry;
    uint16_t r = (uint16_t)(wide & mask), f = szp(r, bits);
    if (subtract ? (uint32_t)(a & mask) < (uint32_t)(b & mask) + carry : wide > mask) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if ((subtract ? ((a ^ b) & (a ^ r)) : (~(a ^ b) & (a ^ r))) & sign) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
static void logic(cpu_t *c, uint16_t r, unsigned bits)                     /* XOR AH,AH / OR AX,mem: CF, OF and AF clear */
{ c->flags = (uint16_t)((c->flags & ~0x08D5u) | szp(r, bits)); }
static uint8_t inc8(cpu_t *c, uint8_t v)                                   /* INC r/m8: CF kept */
{
    uint8_t r = (uint8_t)(v + 1u);
    uint16_t f = (uint16_t)(szp(r, 8u) | ((v & 0x0Fu) == 0x0Fu ? 0x0010u : 0u) | (r == 0x80u ? 0x0800u : 0u));
    c->flags = (uint16_t)((c->flags & ~0x08D4u) | f);
    return r;
}
/* SHR r16,CL as the pinned core's stored path: count masked to 5 bits, zero leaves FLAGS; CF the last bit out, OF = (v >= 8000h) for
 * a count of 1 and clear otherwise, AF set for any nonzero count (get_AF = count & 1Fh), SZP from the result (PF low byte). */
static uint16_t shr16(cpu_t *c, uint16_t v, unsigned count)
{
    uint16_t r, f;
    count &= 0x1Fu;
    if (count == 0u) return v;
    r = (uint16_t)((uint32_t)v >> count);
    f = (uint16_t)(szp(r, 16u) | 0x0010u);
    if (count <= 16u && (((uint32_t)v >> (count - 1u)) & 1u)) f |= 0x0001u;
    if (count == 1u && v >= 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
/* MUL r16 as the pinned MULW (instructions.h:605-619): DX:AX = AX * src; SF and AF kept; ZF from AX alone; PF the even parity of
 * DX:AX (PARITY16(AX) ^ PARITY16(DX) ^ FLAG_PF); CF = OF = (DX != 0). */
static void mulw(cpu_t *c, uint16_t src)
{
    uint32_t p = (uint32_t)c->ax * src;
    uint16_t lo = (uint16_t)p, hi = (uint16_t)(p >> 16u), x = (uint16_t)(lo ^ hi), f = (uint16_t)(c->flags & 0x0090u);
    x ^= (uint16_t)(x >> 8u);
    c->ax = lo; c->dx = hi;
    if (!lo) f |= 0x0040u;
    f |= parity((uint8_t)x);
    if (hi) f |= 0x0801u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
}

#define STEP() do { if (*budget == 0u) return PR_U38_0CC3_LIMIT; --*budget; } while (0)
/* 0D44 LCALL 0AF8:0B0B (StrLoad of the message at CS:0CA8; the segment word at 0D47 is relocated) */
#define SPECIAL_0D44() do { \
    if (*budget == 0u) return PR_U38_0CC3_LIMIT; \
    d->site = 0x0D44u; \
    if (rd8(ram, cs, 0x0D44u) != 0x9Au || rd16(ram, cs, 0x0D45u) != 0x0B0Bu) return PR_U38_0CC3_CODE; \
    --*budget; d->callee = 1; \
    { pr_system_leaf_cpu x; pr_system_leaf_status s; uint16_t seg = rd16(ram, cs, 0x0D47u); \
      push(ram, c, cs); push(ram, c, 0x0D49u); c->cs = seg; c->ip = 0x0B0Bu; \
      COPY(&x, c); s = pr_system_strload_0b0b(ram, ram_size, &x); COPY(c, &x); \
      if (s != PR_SYSTEM_LEAF_RETURNED) { d->callee_status = (int)s; return PR_U38_0CC3_CALLEE; } \
      if (c->cs != cs || c->ip != 0x0D49u) return PR_U38_0CC3_TRANSFER; } } while (0)
/* 0D49 LCALL 00BA:1693 (the segment word at 0D4C is relocated) */
#define SPECIAL_0D49() do { \
    if (*budget == 0u) return PR_U38_0CC3_LIMIT; \
    d->site = 0x0D49u; \
    if (rd8(ram, cs, 0x0D49u) != 0x9Au || rd16(ram, cs, 0x0D4Au) != 0x1693u) return PR_U38_0CC3_CODE; \
    --*budget; d->callee = 2; \
    { pr_r00ba_1693_cpu x; pr_r00ba_1693_status s; uint16_t seg = rd16(ram, cs, 0x0D4Cu); \
      push(ram, c, cs); push(ram, c, 0x0D4Eu); c->cs = seg; c->ip = 0x1693u; \
      COPY(&x, c); s = pr_r00ba_display_1693_cpu(bus, ram_size, &x, limits->clear, limits->rect, limits->printer, limits->glyph, limits->delay, &d->d1693); COPY(c, &x); \
      if (s != PR_R00BA_1693_OK) { d->callee_status = (int)s; return s == PR_R00BA_1693_TRANSFER ? PR_U38_0CC3_TRANSFER : PR_U38_0CC3_CALLEE; } \
      if (c->cs != cs || c->ip != 0x0D4Eu) return PR_U38_0CC3_TRANSFER; } } while (0)
/* 0EF7 LCALL 0AF8:0254 (FreeMem(node, 13h); the segment word at 0EFA is relocated) */
#define SPECIAL_0EF7() do { \
    if (*budget == 0u) return PR_U38_0CC3_LIMIT; \
    d->site = 0x0EF7u; \
    if (rd8(ram, cs, 0x0EF7u) != 0x9Au || rd16(ram, cs, 0x0EF8u) != 0x0254u) return PR_U38_0CC3_CODE; \
    --*budget; d->callee = 3; \
    { pr_system_freemem_cpu x; pr_system_freemem_status s; uint16_t seg = rd16(ram, cs, 0x0EFAu); \
      push(ram, c, cs); push(ram, c, 0x0EFCu); c->cs = seg; c->ip = 0x0254u; \
      COPY(&x, c); s = pr_system_freemem_0254(ram, ram_size, &x, limits->freemem_steps); COPY(c, &x); \
      if (s == PR_SYSTEM_FREEMEM_RUNERROR) { d->callee_status = (int)s; return PR_U38_0CC3_RUNERROR; } \
      if (s == PR_SYSTEM_FREEMEM_LIMIT) { d->callee_status = (int)s; return PR_U38_0CC3_LIMIT; } \
      if (s != PR_SYSTEM_FREEMEM_RETURNED) { d->callee_status = (int)s; return s == PR_SYSTEM_FREEMEM_TRANSFER ? PR_U38_0CC3_TRANSFER : PR_U38_0CC3_CALLEE; } \
      if (c->cs != cs || c->ip != 0x0EFCu) return PR_U38_0CC3_TRANSFER; } } while (0)
/* 0F9F RETF 0Ah */
#define SPECIAL_0F9F() do { \
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 10u); \
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U38_0CC3_TRANSFER; \
    return PR_U38_0CC3_RETURNED; } while (0)

pr_u38_0cc3_status pr_u38_0cc3_cpu(pr_bus *bus, size_t ram_size, pr_u38_0cc3_regs *c, size_t *budget, const pr_u38_0cc3_limits *limits,
                                   pr_u38_0cc3_diagnostic *diagnostic)
{
    pr_u38_0cc3_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint8_t *ram; uint16_t cs, ret_ip, ret_cs;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || limits == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0CC3u || c->sp < 0x0200u || c->sp > 0xFFF2u)
        return PR_U38_0CC3_ARGUMENT;
    ram = bus->ram; cs = c->cs; memset(d, 0, sizeof *d); d->callee = -1; d->callee_status = -1;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    STEP(); push(ram, c, c->bp); c->ip = 0x0CC4u;    /* 0CC3 PUSH bp */
    STEP(); c->bp = c->sp; c->ip = 0x0CC6u;    /* 0CC4 MOV bp, sp */
    STEP(); c->sp = arith(c, c->sp, 0x0028u, 1, 0, 16u); c->ip = 0x0CC9u;    /* 0CC6 SUB sp, 0x28 */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (8))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (10))); c->ip = 0x0CCCu;    /* 0CC9 LES di, ptr [bp + 8] */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(brd8(bus, c->es, (uint16_t)(c->di + 0x0003u))) << 0u)); c->ip = 0x0CD0u;    /* 0CCC MOV al, byte ptr es:[di + 3] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0CD2u;    /* 0CD0 XOR ah, ah */
    STEP(); c->cx = (uint16_t)((c->cx & 0xFF00u) | ((uint16_t)(uint8_t)(0x04u) << 0u)); c->ip = 0x0CD4u;    /* 0CD2 MOV cl, 4 */
    STEP(); c->ax = shr16(c, c->ax, (unsigned)(uint8_t)c->cx); c->ip = 0x0CD6u;    /* 0CD4 SHR ax, cl */
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-10)), (uint8_t)(c->ax >> 0u)); c->ip = 0x0CD9u;    /* 0CD6 MOV byte ptr [bp - 0xa], al */
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-9)), 0x00u); c->ip = 0x0CDDu;    /* 0CD9 MOV byte ptr [bp - 9], 0 */
    STEP(); c->ax = rd16(ram, c->ds, 0x6DBBu); c->ip = 0x0CE0u;    /* 0CDD MOV ax, word ptr [0x6dbb] */
    STEP(); c->dx = rd16(ram, c->ds, 0x6DBDu); c->ip = 0x0CE4u;    /* 0CE0 MOV dx, word ptr [0x6dbd] */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-8)), c->ax); c->ip = 0x0CE7u;    /* 0CE4 MOV word ptr [bp - 8], ax */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-6)), c->dx); c->ip = 0x0CEAu;    /* 0CE7 MOV word ptr [bp - 6], dx */
    L_0CEA: STEP(); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + (-8))); c->ip = 0x0CEDu;    /* 0CEA MOV ax, word ptr [bp - 8] */
    STEP(); c->ax = (uint16_t)(c->ax | rd16(ram, c->ss, (uint16_t)(c->bp + (-6)))); logic(c, c->ax, 16u); c->ip = 0x0CF0u;    /* 0CED OR ax, word ptr [bp - 6] */
    STEP(); if ((c->flags & 0x0040u)) { c->ip = 0x0D31u; goto L_0D31; } c->ip = 0x0CF2u;    /* 0CF0 JE 0xd31 */
    STEP(); (void)arith(c, rd8(ram, c->ss, (uint16_t)(c->bp + (-9))), 0x00u, 1, 0, 8u); c->ip = 0x0CF6u;    /* 0CF2 CMP byte ptr [bp - 9], 0 */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0D31u; goto L_0D31; } c->ip = 0x0CF8u;    /* 0CF6 JNE 0xd31 */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-8))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-6))); c->ip = 0x0CFBu;    /* 0CF8 LES di, ptr [bp - 8] */
    STEP(); c->ax = brd16(bus, c->es, c->di); c->ip = 0x0CFEu;    /* 0CFB MOV ax, word ptr es:[di] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x0002u)); c->ip = 0x0D02u;    /* 0CFE MOV dx, word ptr es:[di + 2] */
    STEP(); (void)arith(c, c->dx, rd16(ram, c->ss, (uint16_t)(c->bp + (14))), 1, 0, 16u); c->ip = 0x0D05u;    /* 0D02 CMP dx, word ptr [bp + 0xe] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0D1Eu; goto L_0D1E; } c->ip = 0x0D07u;    /* 0D05 JNE 0xd1e */
    STEP(); (void)arith(c, c->ax, rd16(ram, c->ss, (uint16_t)(c->bp + (12))), 1, 0, 16u); c->ip = 0x0D0Au;    /* 0D07 CMP ax, word ptr [bp + 0xc] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0D1Eu; goto L_0D1E; } c->ip = 0x0D0Cu;    /* 0D0A JNE 0xd1e */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-8))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-6))); c->ip = 0x0D0Fu;    /* 0D0C LES di, ptr [bp - 8] */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(brd8(bus, c->es, (uint16_t)(c->di + 0x0012u))) << 0u)); c->ip = 0x0D13u;    /* 0D0F MOV al, byte ptr es:[di + 0x12] */
    STEP(); (void)arith(c, (uint8_t)(c->ax >> 0u), rd8(ram, c->ss, (uint16_t)(c->bp + (-10))), 1, 0, 8u); c->ip = 0x0D16u;    /* 0D13 CMP al, byte ptr [bp - 0xa] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0D1Eu; goto L_0D1E; } c->ip = 0x0D18u;    /* 0D16 JNE 0xd1e */
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-9)), 0x01u); c->ip = 0x0D1Cu;    /* 0D18 MOV byte ptr [bp - 9], 1 */
    STEP(); c->ip = 0x0D2Fu; goto L_0D2F;    /* 0D1C JMP 0xd2f */
    L_0D1E: STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-8))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-6))); c->ip = 0x0D21u;    /* 0D1E LES di, ptr [bp - 8] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0004u)); c->ip = 0x0D25u;    /* 0D21 MOV ax, word ptr es:[di + 4] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x0006u)); c->ip = 0x0D29u;    /* 0D25 MOV dx, word ptr es:[di + 6] */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-8)), c->ax); c->ip = 0x0D2Cu;    /* 0D29 MOV word ptr [bp - 8], ax */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-6)), c->dx); c->ip = 0x0D2Fu;    /* 0D2C MOV word ptr [bp - 6], dx */
    L_0D2F: STEP(); c->ip = 0x0CEAu; goto L_0CEA;    /* 0D2F JMP 0xcea */
    L_0D31: STEP(); (void)arith(c, rd8(ram, c->ss, (uint16_t)(c->bp + (-9))), 0x00u, 1, 0, 8u); c->ip = 0x0D35u;    /* 0D31 CMP byte ptr [bp - 9], 0 */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0D3Au; goto L_0D3A; } c->ip = 0x0D37u;    /* 0D35 JNE 0xd3a */
    STEP(); c->ip = 0x0F9Cu; goto L_0F9C;    /* 0D37 JMP 0xf9c */
    L_0D3A: STEP(); c->di = (uint16_t)(c->bp - 0x28u); c->ip = 0x0D3Du;    /* 0D3A LEA di, [bp - 0x28] */
    STEP(); push(ram, c, c->ss); c->ip = 0x0D3Eu;    /* 0D3D PUSH ss */
    STEP(); push(ram, c, c->di); c->ip = 0x0D3Fu;    /* 0D3E PUSH di */
    STEP(); c->di = 0x0CA8u; c->ip = 0x0D42u;    /* 0D3F MOV di, 0xca8 */
    STEP(); push(ram, c, c->cs); c->ip = 0x0D43u;    /* 0D42 PUSH cs */
    STEP(); push(ram, c, c->di); c->ip = 0x0D44u;    /* 0D43 PUSH di */
    /* 0D44 LCALL 0xaf8, 0xb0b */
    SPECIAL_0D44();
    /* 0D49 LCALL 0xba, 0x1693 */
    SPECIAL_0D49();
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-11)), 0x01u); c->ip = 0x0D52u;    /* 0D4E MOV byte ptr [bp - 0xb], 1 */
    STEP(); c->ip = 0x0D57u; goto L_0D57;    /* 0D52 JMP 0xd57 */
    L_0D54: STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-11)), inc8(c, rd8(ram, c->ss, (uint16_t)(c->bp + (-11))))); c->ip = 0x0D57u;    /* 0D54 INC byte ptr [bp - 0xb] */
    L_0D57: STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-11)))) << 0u)); c->ip = 0x0D5Au;    /* 0D57 MOV al, byte ptr [bp - 0xb] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0D5Cu;    /* 0D5A XOR ah, ah */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-8))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-6))); c->ip = 0x0D5Fu;    /* 0D5C LES di, ptr [bp - 8] */
    STEP(); c->di = arith(c, c->di, c->ax, 0, 0, 16u); c->ip = 0x0D61u;    /* 0D5F ADD di, ax */
    STEP(); (void)arith(c, brd8(bus, c->es, (uint16_t)(c->di + 0x000Bu)), 0x00u, 1, 0, 8u); c->ip = 0x0D66u;    /* 0D61 CMP byte ptr es:[di + 0xb], 0 */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0D6Bu; goto L_0D6B; } c->ip = 0x0D68u;    /* 0D66 JNE 0xd6b */
    STEP(); c->ip = 0x0E75u; goto L_0E75;    /* 0D68 JMP 0xe75 */
    L_0D6B: STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-11)))) << 0u)); c->ip = 0x0D6Eu;    /* 0D6B MOV al, byte ptr [bp - 0xb] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0D70u;    /* 0D6E XOR ah, ah */
    STEP(); c->di = c->ax; c->ip = 0x0D72u;    /* 0D70 MOV di, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, (uint16_t)(c->di + 0x290Du))) << 0u)); c->ip = 0x0D76u;    /* 0D72 MOV al, byte ptr [di + 0x290d] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0D78u;    /* 0D76 XOR ah, ah */
    STEP(); c->di = c->ax; c->ip = 0x0D7Au;    /* 0D78 MOV di, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, (uint16_t)(c->di + 0x2736u))) << 0u)); c->ip = 0x0D7Eu;    /* 0D7A MOV al, byte ptr [di + 0x2736] */
    STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0D7Fu;    /* 0D7E CBW  */
    STEP(); c->dx = c->ax; c->ip = 0x0D81u;    /* 0D7F MOV dx, ax */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-8))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-6))); c->ip = 0x0D84u;    /* 0D81 LES di, ptr [bp - 8] */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(brd8(bus, c->es, (uint16_t)(c->di + 0x0010u))) << 0u)); c->ip = 0x0D88u;    /* 0D84 MOV al, byte ptr es:[di + 0x10] */
    STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0D89u;    /* 0D88 CBW  */
    STEP(); c->ax = arith(c, c->ax, c->dx, 0, 0, 16u); c->ip = 0x0D8Bu;    /* 0D89 ADD ax, dx */
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-12)), (uint8_t)(c->ax >> 0u)); c->ip = 0x0D8Eu;    /* 0D8B MOV byte ptr [bp - 0xc], al */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-11)))) << 0u)); c->ip = 0x0D91u;    /* 0D8E MOV al, byte ptr [bp - 0xb] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0D93u;    /* 0D91 XOR ah, ah */
    STEP(); c->di = c->ax; c->ip = 0x0D95u;    /* 0D93 MOV di, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, (uint16_t)(c->di + 0x290Du))) << 0u)); c->ip = 0x0D99u;    /* 0D95 MOV al, byte ptr [di + 0x290d] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0D9Bu;    /* 0D99 XOR ah, ah */
    STEP(); c->di = c->ax; c->ip = 0x0D9Du;    /* 0D9B MOV di, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, (uint16_t)(c->di + 0x273Fu))) << 0u)); c->ip = 0x0DA1u;    /* 0D9D MOV al, byte ptr [di + 0x273f] */
    STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0DA2u;    /* 0DA1 CBW  */
    STEP(); c->dx = c->ax; c->ip = 0x0DA4u;    /* 0DA2 MOV dx, ax */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-8))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-6))); c->ip = 0x0DA7u;    /* 0DA4 LES di, ptr [bp - 8] */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(brd8(bus, c->es, (uint16_t)(c->di + 0x0011u))) << 0u)); c->ip = 0x0DABu;    /* 0DA7 MOV al, byte ptr es:[di + 0x11] */
    STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0DACu;    /* 0DAB CBW  */
    STEP(); c->ax = arith(c, c->ax, c->dx, 0, 0, 16u); c->ip = 0x0DAEu;    /* 0DAC ADD ax, dx */
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-13)), (uint8_t)(c->ax >> 0u)); c->ip = 0x0DB1u;    /* 0DAE MOV byte ptr [bp - 0xd], al */
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-9)), 0x00u); c->ip = 0x0DB5u;    /* 0DB1 MOV byte ptr [bp - 9], 0 */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, 0x6715u)) << 0u)); c->ip = 0x0DB8u;    /* 0DB5 MOV al, byte ptr [0x6715] */
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-14)), (uint8_t)(c->ax >> 0u)); c->ip = 0x0DBBu;    /* 0DB8 MOV byte ptr [bp - 0xe], al */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x01u) << 0u)); c->ip = 0x0DBDu;    /* 0DBB MOV al, 1 */
    STEP(); (void)arith(c, (uint8_t)(c->ax >> 0u), rd8(ram, c->ss, (uint16_t)(c->bp + (-14))), 1, 0, 8u); c->ip = 0x0DC0u;    /* 0DBD CMP al, byte ptr [bp - 0xe] */
    STEP(); if (!(c->flags & 0x0001u) && !(c->flags & 0x0040u)) { c->ip = 0x0E24u; goto L_0E24; } c->ip = 0x0DC2u;    /* 0DC0 JA 0xe24 */
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-10)), (uint8_t)(c->ax >> 0u)); c->ip = 0x0DC5u;    /* 0DC2 MOV byte ptr [bp - 0xa], al */
    STEP(); c->ip = 0x0DCAu; goto L_0DCA;    /* 0DC5 JMP 0xdca */
    L_0DC7: STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-10)), inc8(c, rd8(ram, c->ss, (uint16_t)(c->bp + (-10))))); c->ip = 0x0DCAu;    /* 0DC7 INC byte ptr [bp - 0xa] */
    L_0DCA: STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-10)))) << 0u)); c->ip = 0x0DCDu;    /* 0DCA MOV al, byte ptr [bp - 0xa] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0DCFu;    /* 0DCD XOR ah, ah */
    STEP(); c->dx = 0x0007u; c->ip = 0x0DD2u;    /* 0DCF MOV dx, 7 */
    STEP(); mulw(c, c->dx); c->ip = 0x0DD4u;    /* 0DD2 MUL dx */
    STEP(); c->di = c->ax; c->ip = 0x0DD6u;    /* 0DD4 MOV di, ax */
    STEP(); c->ax = rd16(ram, c->ds, (uint16_t)(c->di + 0x66D6u)); c->ip = 0x0DDAu;    /* 0DD6 MOV ax, word ptr [di + 0x66d6] */
    STEP(); c->ax = (uint16_t)(c->ax | rd16(ram, c->ds, (uint16_t)(c->di + 0x66D8u))); logic(c, c->ax, 16u); c->ip = 0x0DDEu;    /* 0DDA OR ax, word ptr [di + 0x66d8] */
    STEP(); if ((c->flags & 0x0040u)) { c->ip = 0x0E1Cu; goto L_0E1C; } c->ip = 0x0DE0u;    /* 0DDE JE 0xe1c */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-12)))) << 0u)); c->ip = 0x0DE3u;    /* 0DE0 MOV al, byte ptr [bp - 0xc] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0DE5u;    /* 0DE3 XOR ah, ah */
    STEP(); c->cx = c->ax; c->ip = 0x0DE7u;    /* 0DE5 MOV cx, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-10)))) << 0u)); c->ip = 0x0DEAu;    /* 0DE7 MOV al, byte ptr [bp - 0xa] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0DECu;    /* 0DEA XOR ah, ah */
    STEP(); c->dx = 0x0007u; c->ip = 0x0DEFu;    /* 0DEC MOV dx, 7 */
    STEP(); mulw(c, c->dx); c->ip = 0x0DF1u;    /* 0DEF MUL dx */
    STEP(); c->di = c->ax; c->ip = 0x0DF3u;    /* 0DF1 MOV di, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, (uint16_t)(c->di + 0x66DAu))) << 0u)); c->ip = 0x0DF7u;    /* 0DF3 MOV al, byte ptr [di + 0x66da] */
    STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0DF8u;    /* 0DF7 CBW  */
    STEP(); (void)arith(c, c->ax, c->cx, 1, 0, 16u); c->ip = 0x0DFAu;    /* 0DF8 CMP ax, cx */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0E1Cu; goto L_0E1C; } c->ip = 0x0DFCu;    /* 0DFA JNE 0xe1c */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-13)))) << 0u)); c->ip = 0x0DFFu;    /* 0DFC MOV al, byte ptr [bp - 0xd] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0E01u;    /* 0DFF XOR ah, ah */
    STEP(); c->cx = c->ax; c->ip = 0x0E03u;    /* 0E01 MOV cx, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-10)))) << 0u)); c->ip = 0x0E06u;    /* 0E03 MOV al, byte ptr [bp - 0xa] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0E08u;    /* 0E06 XOR ah, ah */
    STEP(); c->dx = 0x0007u; c->ip = 0x0E0Bu;    /* 0E08 MOV dx, 7 */
    STEP(); mulw(c, c->dx); c->ip = 0x0E0Du;    /* 0E0B MUL dx */
    STEP(); c->di = c->ax; c->ip = 0x0E0Fu;    /* 0E0D MOV di, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, (uint16_t)(c->di + 0x66DBu))) << 0u)); c->ip = 0x0E13u;    /* 0E0F MOV al, byte ptr [di + 0x66db] */
    STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0E14u;    /* 0E13 CBW  */
    STEP(); (void)arith(c, c->ax, c->cx, 1, 0, 16u); c->ip = 0x0E16u;    /* 0E14 CMP ax, cx */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0E1Cu; goto L_0E1C; } c->ip = 0x0E18u;    /* 0E16 JNE 0xe1c */
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-9)), 0x01u); c->ip = 0x0E1Cu;    /* 0E18 MOV byte ptr [bp - 9], 1 */
    L_0E1C: STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-10)))) << 0u)); c->ip = 0x0E1Fu;    /* 0E1C MOV al, byte ptr [bp - 0xa] */
    STEP(); (void)arith(c, (uint8_t)(c->ax >> 0u), rd8(ram, c->ss, (uint16_t)(c->bp + (-14))), 1, 0, 8u); c->ip = 0x0E22u;    /* 0E1F CMP al, byte ptr [bp - 0xe] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0DC7u; goto L_0DC7; } c->ip = 0x0E24u;    /* 0E22 JNE 0xdc7 */
    L_0E24: STEP(); (void)arith(c, rd8(ram, c->ss, (uint16_t)(c->bp + (-9))), 0x00u, 1, 0, 8u); c->ip = 0x0E28u;    /* 0E24 CMP byte ptr [bp - 9], 0 */
    STEP(); if ((c->flags & 0x0040u)) { c->ip = 0x0E4Au; goto L_0E4A; } c->ip = 0x0E2Au;    /* 0E28 JE 0xe4a */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-12)))) << 0u)); c->ip = 0x0E2Du;    /* 0E2A MOV al, byte ptr [bp - 0xc] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0E2Fu;    /* 0E2D XOR ah, ah */
    STEP(); c->cx = c->ax; c->ip = 0x0E31u;    /* 0E2F MOV cx, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-13)))) << 0u)); c->ip = 0x0E34u;    /* 0E31 MOV al, byte ptr [bp - 0xd] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0E36u;    /* 0E34 XOR ah, ah */
    STEP(); c->dx = 0x0032u; c->ip = 0x0E39u;    /* 0E36 MOV dx, 0x32 */
    STEP(); mulw(c, c->dx); c->ip = 0x0E3Bu;    /* 0E39 MUL dx */
    STEP(); c->di = rd16(ram, c->ds, 0x6716u); c->es = rd16(ram, c->ds, 0x6718u); c->ip = 0x0E3Fu;    /* 0E3B LES di, ptr [0x6716] */
    STEP(); c->di = arith(c, c->di, c->ax, 0, 0, 16u); c->ip = 0x0E41u;    /* 0E3F ADD di, ax */
    STEP(); c->di = arith(c, c->di, c->cx, 0, 0, 16u); c->ip = 0x0E43u;    /* 0E41 ADD di, cx */
    STEP(); bwr8(bus, c->es, (uint16_t)(c->di + 0x0007u), 0x1Fu); c->ip = 0x0E48u;    /* 0E43 MOV byte ptr es:[di + 7], 0x1f */
    STEP(); c->ip = 0x0E75u; goto L_0E75;    /* 0E48 JMP 0xe75 */
    L_0E4A: STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-11)))) << 0u)); c->ip = 0x0E4Du;    /* 0E4A MOV al, byte ptr [bp - 0xb] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0E4Fu;    /* 0E4D XOR ah, ah */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-8))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-6))); c->ip = 0x0E52u;    /* 0E4F LES di, ptr [bp - 8] */
    STEP(); c->di = arith(c, c->di, c->ax, 0, 0, 16u); c->ip = 0x0E54u;    /* 0E52 ADD di, ax */
    STEP(); c->bx = (uint16_t)((c->bx & 0xFF00u) | ((uint16_t)(uint8_t)(brd8(bus, c->es, (uint16_t)(c->di + 0x0007u))) << 0u)); c->ip = 0x0E58u;    /* 0E54 MOV bl, byte ptr es:[di + 7] */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-12)))) << 0u)); c->ip = 0x0E5Bu;    /* 0E58 MOV al, byte ptr [bp - 0xc] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0E5Du;    /* 0E5B XOR ah, ah */
    STEP(); c->cx = c->ax; c->ip = 0x0E5Fu;    /* 0E5D MOV cx, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-13)))) << 0u)); c->ip = 0x0E62u;    /* 0E5F MOV al, byte ptr [bp - 0xd] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0E64u;    /* 0E62 XOR ah, ah */
    STEP(); c->dx = 0x0032u; c->ip = 0x0E67u;    /* 0E64 MOV dx, 0x32 */
    STEP(); mulw(c, c->dx); c->ip = 0x0E69u;    /* 0E67 MUL dx */
    STEP(); c->di = rd16(ram, c->ds, 0x6716u); c->es = rd16(ram, c->ds, 0x6718u); c->ip = 0x0E6Du;    /* 0E69 LES di, ptr [0x6716] */
    STEP(); c->di = arith(c, c->di, c->ax, 0, 0, 16u); c->ip = 0x0E6Fu;    /* 0E6D ADD di, ax */
    STEP(); c->di = arith(c, c->di, c->cx, 0, 0, 16u); c->ip = 0x0E71u;    /* 0E6F ADD di, cx */
    STEP(); bwr8(bus, c->es, (uint16_t)(c->di + 0x0007u), (uint8_t)(c->bx >> 0u)); c->ip = 0x0E75u;    /* 0E71 MOV byte ptr es:[di + 7], bl */
    L_0E75: STEP(); (void)arith(c, rd8(ram, c->ss, (uint16_t)(c->bp + (-11))), 0x04u, 1, 0, 8u); c->ip = 0x0E79u;    /* 0E75 CMP byte ptr [bp - 0xb], 4 */
    STEP(); if ((c->flags & 0x0040u)) { c->ip = 0x0E7Eu; goto L_0E7E; } c->ip = 0x0E7Bu;    /* 0E79 JE 0xe7e */
    STEP(); c->ip = 0x0D54u; goto L_0D54;    /* 0E7B JMP 0xd54 */
    L_0E7E: STEP(); c->ax = rd16(ram, c->ds, 0x6DBBu); c->ip = 0x0E81u;    /* 0E7E MOV ax, word ptr [0x6dbb] */
    STEP(); c->dx = rd16(ram, c->ds, 0x6DBDu); c->ip = 0x0E85u;    /* 0E81 MOV dx, word ptr [0x6dbd] */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-4)), c->ax); c->ip = 0x0E88u;    /* 0E85 MOV word ptr [bp - 4], ax */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-2)), c->dx); c->ip = 0x0E8Bu;    /* 0E88 MOV word ptr [bp - 2], dx */
    STEP(); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->ip = 0x0E8Eu;    /* 0E8B MOV ax, word ptr [bp - 4] */
    STEP(); c->dx = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x0E91u;    /* 0E8E MOV dx, word ptr [bp - 2] */
    STEP(); (void)arith(c, c->dx, rd16(ram, c->ss, (uint16_t)(c->bp + (-6))), 1, 0, 16u); c->ip = 0x0E94u;    /* 0E91 CMP dx, word ptr [bp - 6] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0EAFu; goto L_0EAF; } c->ip = 0x0E96u;    /* 0E94 JNE 0xeaf */
    STEP(); (void)arith(c, c->ax, rd16(ram, c->ss, (uint16_t)(c->bp + (-8))), 1, 0, 16u); c->ip = 0x0E99u;    /* 0E96 CMP ax, word ptr [bp - 8] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0EAFu; goto L_0EAF; } c->ip = 0x0E9Bu;    /* 0E99 JNE 0xeaf */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x0E9Eu;    /* 0E9B LES di, ptr [bp - 4] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0004u)); c->ip = 0x0EA2u;    /* 0E9E MOV ax, word ptr es:[di + 4] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x0006u)); c->ip = 0x0EA6u;    /* 0EA2 MOV dx, word ptr es:[di + 6] */
    STEP(); wr16(ram, c->ds, 0x6DBBu, c->ax); c->ip = 0x0EA9u;    /* 0EA6 MOV word ptr [0x6dbb], ax */
    STEP(); wr16(ram, c->ds, 0x6DBDu, c->dx); c->ip = 0x0EADu;    /* 0EA9 MOV word ptr [0x6dbd], dx */
    STEP(); c->ip = 0x0ED7u; goto L_0ED7;    /* 0EAD JMP 0xed7 */
    L_0EAF: STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x0EB2u;    /* 0EAF LES di, ptr [bp - 4] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0004u)); c->ip = 0x0EB6u;    /* 0EB2 MOV ax, word ptr es:[di + 4] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x0006u)); c->ip = 0x0EBAu;    /* 0EB6 MOV dx, word ptr es:[di + 6] */
    STEP(); (void)arith(c, c->dx, rd16(ram, c->ss, (uint16_t)(c->bp + (-6))), 1, 0, 16u); c->ip = 0x0EBDu;    /* 0EBA CMP dx, word ptr [bp - 6] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0EC4u; goto L_0EC4; } c->ip = 0x0EBFu;    /* 0EBD JNE 0xec4 */
    STEP(); (void)arith(c, c->ax, rd16(ram, c->ss, (uint16_t)(c->bp + (-8))), 1, 0, 16u); c->ip = 0x0EC2u;    /* 0EBF CMP ax, word ptr [bp - 8] */
    STEP(); if ((c->flags & 0x0040u)) { c->ip = 0x0ED7u; goto L_0ED7; } c->ip = 0x0EC4u;    /* 0EC2 JE 0xed7 */
    L_0EC4: STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x0EC7u;    /* 0EC4 LES di, ptr [bp - 4] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0004u)); c->ip = 0x0ECBu;    /* 0EC7 MOV ax, word ptr es:[di + 4] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x0006u)); c->ip = 0x0ECFu;    /* 0ECB MOV dx, word ptr es:[di + 6] */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-4)), c->ax); c->ip = 0x0ED2u;    /* 0ECF MOV word ptr [bp - 4], ax */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-2)), c->dx); c->ip = 0x0ED5u;    /* 0ED2 MOV word ptr [bp - 2], dx */
    STEP(); c->ip = 0x0EAFu; goto L_0EAF;    /* 0ED5 JMP 0xeaf */
    L_0ED7: STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-8))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-6))); c->ip = 0x0EDAu;    /* 0ED7 LES di, ptr [bp - 8] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0004u)); c->ip = 0x0EDEu;    /* 0EDA MOV ax, word ptr es:[di + 4] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x0006u)); c->ip = 0x0EE2u;    /* 0EDE MOV dx, word ptr es:[di + 6] */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x0EE5u;    /* 0EE2 LES di, ptr [bp - 4] */
    STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x0004u), c->ax); c->ip = 0x0EE9u;    /* 0EE5 MOV word ptr es:[di + 4], ax */
    STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x0006u), c->dx); c->ip = 0x0EEDu;    /* 0EE9 MOV word ptr es:[di + 6], dx */
    STEP(); push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + (-6)))); c->ip = 0x0EF0u;    /* 0EED PUSH word ptr [bp - 6] */
    STEP(); push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + (-8)))); c->ip = 0x0EF3u;    /* 0EF0 PUSH word ptr [bp - 8] */
    STEP(); c->ax = 0x0013u; c->ip = 0x0EF6u;    /* 0EF3 MOV ax, 0x13 */
    STEP(); push(ram, c, c->ax); c->ip = 0x0EF7u;    /* 0EF6 PUSH ax */
    /* 0EF7 LCALL 0xaf8, 0x254 */
    SPECIAL_0EF7();
    STEP(); c->ax = rd16(ram, c->ds, 0x6DBBu); c->ip = 0x0EFFu;    /* 0EFC MOV ax, word ptr [0x6dbb] */
    STEP(); c->dx = rd16(ram, c->ds, 0x6DBDu); c->ip = 0x0F03u;    /* 0EFF MOV dx, word ptr [0x6dbd] */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-4)), c->ax); c->ip = 0x0F06u;    /* 0F03 MOV word ptr [bp - 4], ax */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-2)), c->dx); c->ip = 0x0F09u;    /* 0F06 MOV word ptr [bp - 2], dx */
    L_0F09: STEP(); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->ip = 0x0F0Cu;    /* 0F09 MOV ax, word ptr [bp - 4] */
    STEP(); c->ax = (uint16_t)(c->ax | rd16(ram, c->ss, (uint16_t)(c->bp + (-2)))); logic(c, c->ax, 16u); c->ip = 0x0F0Fu;    /* 0F0C OR ax, word ptr [bp - 2] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0F14u; goto L_0F14; } c->ip = 0x0F11u;    /* 0F0F JNE 0xf14 */
    STEP(); c->ip = 0x0F9Cu; goto L_0F9C;    /* 0F11 JMP 0xf9c */
    L_0F14: STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-11)), 0x01u); c->ip = 0x0F18u;    /* 0F14 MOV byte ptr [bp - 0xb], 1 */
    STEP(); c->ip = 0x0F1Du; goto L_0F1D;    /* 0F18 JMP 0xf1d */
    L_0F1A: STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-11)), inc8(c, rd8(ram, c->ss, (uint16_t)(c->bp + (-11))))); c->ip = 0x0F1Du;    /* 0F1A INC byte ptr [bp - 0xb] */
    L_0F1D: STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-11)))) << 0u)); c->ip = 0x0F20u;    /* 0F1D MOV al, byte ptr [bp - 0xb] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0F22u;    /* 0F20 XOR ah, ah */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x0F25u;    /* 0F22 LES di, ptr [bp - 4] */
    STEP(); c->di = arith(c, c->di, c->ax, 0, 0, 16u); c->ip = 0x0F27u;    /* 0F25 ADD di, ax */
    STEP(); (void)arith(c, brd8(bus, c->es, (uint16_t)(c->di + 0x000Bu)), 0x00u, 1, 0, 8u); c->ip = 0x0F2Cu;    /* 0F27 CMP byte ptr es:[di + 0xb], 0 */
    STEP(); if ((c->flags & 0x0040u)) { c->ip = 0x0F82u; goto L_0F82; } c->ip = 0x0F2Eu;    /* 0F2C JE 0xf82 */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-11)))) << 0u)); c->ip = 0x0F31u;    /* 0F2E MOV al, byte ptr [bp - 0xb] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0F33u;    /* 0F31 XOR ah, ah */
    STEP(); c->di = c->ax; c->ip = 0x0F35u;    /* 0F33 MOV di, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, (uint16_t)(c->di + 0x290Du))) << 0u)); c->ip = 0x0F39u;    /* 0F35 MOV al, byte ptr [di + 0x290d] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0F3Bu;    /* 0F39 XOR ah, ah */
    STEP(); c->di = c->ax; c->ip = 0x0F3Du;    /* 0F3B MOV di, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, (uint16_t)(c->di + 0x2736u))) << 0u)); c->ip = 0x0F41u;    /* 0F3D MOV al, byte ptr [di + 0x2736] */
    STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0F42u;    /* 0F41 CBW  */
    STEP(); c->dx = c->ax; c->ip = 0x0F44u;    /* 0F42 MOV dx, ax */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x0F47u;    /* 0F44 LES di, ptr [bp - 4] */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(brd8(bus, c->es, (uint16_t)(c->di + 0x0010u))) << 0u)); c->ip = 0x0F4Bu;    /* 0F47 MOV al, byte ptr es:[di + 0x10] */
    STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0F4Cu;    /* 0F4B CBW  */
    STEP(); c->ax = arith(c, c->ax, c->dx, 0, 0, 16u); c->ip = 0x0F4Eu;    /* 0F4C ADD ax, dx */
    STEP(); c->cx = c->ax; c->ip = 0x0F50u;    /* 0F4E MOV cx, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-11)))) << 0u)); c->ip = 0x0F53u;    /* 0F50 MOV al, byte ptr [bp - 0xb] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0F55u;    /* 0F53 XOR ah, ah */
    STEP(); c->di = c->ax; c->ip = 0x0F57u;    /* 0F55 MOV di, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, (uint16_t)(c->di + 0x290Du))) << 0u)); c->ip = 0x0F5Bu;    /* 0F57 MOV al, byte ptr [di + 0x290d] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x0F5Du;    /* 0F5B XOR ah, ah */
    STEP(); c->di = c->ax; c->ip = 0x0F5Fu;    /* 0F5D MOV di, ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ds, (uint16_t)(c->di + 0x273Fu))) << 0u)); c->ip = 0x0F63u;    /* 0F5F MOV al, byte ptr [di + 0x273f] */
    STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0F64u;    /* 0F63 CBW  */
    STEP(); c->dx = c->ax; c->ip = 0x0F66u;    /* 0F64 MOV dx, ax */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x0F69u;    /* 0F66 LES di, ptr [bp - 4] */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(brd8(bus, c->es, (uint16_t)(c->di + 0x0011u))) << 0u)); c->ip = 0x0F6Du;    /* 0F69 MOV al, byte ptr es:[di + 0x11] */
    STEP(); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; c->ip = 0x0F6Eu;    /* 0F6D CBW  */
    STEP(); c->ax = arith(c, c->ax, c->dx, 0, 0, 16u); c->ip = 0x0F70u;    /* 0F6E ADD ax, dx */
    STEP(); c->dx = 0x0032u; c->ip = 0x0F73u;    /* 0F70 MOV dx, 0x32 */
    STEP(); mulw(c, c->dx); c->ip = 0x0F75u;    /* 0F73 MUL dx */
    STEP(); c->di = rd16(ram, c->ds, 0x6716u); c->es = rd16(ram, c->ds, 0x6718u); c->ip = 0x0F79u;    /* 0F75 LES di, ptr [0x6716] */
    STEP(); c->di = arith(c, c->di, c->ax, 0, 0, 16u); c->ip = 0x0F7Bu;    /* 0F79 ADD di, ax */
    STEP(); c->di = arith(c, c->di, c->cx, 0, 0, 16u); c->ip = 0x0F7Du;    /* 0F7B ADD di, cx */
    STEP(); bwr8(bus, c->es, (uint16_t)(c->di + 0x0007u), 0x1Eu); c->ip = 0x0F82u;    /* 0F7D MOV byte ptr es:[di + 7], 0x1e */
    L_0F82: STEP(); (void)arith(c, rd8(ram, c->ss, (uint16_t)(c->bp + (-11))), 0x04u, 1, 0, 8u); c->ip = 0x0F86u;    /* 0F82 CMP byte ptr [bp - 0xb], 4 */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x0F1Au; goto L_0F1A; } c->ip = 0x0F88u;    /* 0F86 JNE 0xf1a */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x0F8Bu;    /* 0F88 LES di, ptr [bp - 4] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0004u)); c->ip = 0x0F8Fu;    /* 0F8B MOV ax, word ptr es:[di + 4] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x0006u)); c->ip = 0x0F93u;    /* 0F8F MOV dx, word ptr es:[di + 6] */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-4)), c->ax); c->ip = 0x0F96u;    /* 0F93 MOV word ptr [bp - 4], ax */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-2)), c->dx); c->ip = 0x0F99u;    /* 0F96 MOV word ptr [bp - 2], dx */
    STEP(); c->ip = 0x0F09u; goto L_0F09;    /* 0F99 JMP 0xf09 */
    L_0F9C: STEP(); c->sp = c->bp; c->ip = 0x0F9Eu;    /* 0F9C MOV sp, bp */
    STEP(); c->bp = pop(ram, c); c->ip = 0x0F9Fu;    /* 0F9E POP bp */
    /* 0F9F RETF 0xa */
    SPECIAL_0F9F();
}
