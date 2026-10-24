/* START.EXE resident 00BA +1509: remove a record from the character's +0C8h list, full CPU state (docs/re/RES_00BA_1509_CPU.md). */
#include "poolrad/recovered/resident_00ba_sidecar_remove_1509_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_freemem_0254_cpu.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_res_1509_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
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

#define STEP() do { if (*budget == 0u) return PR_RES_1509_LIMIT; --*budget; } while (0)
/* LCALL 0AF8:0B0B (StrLoad); the segment word at site+3 is relocated */
#define STRLOAD(site_, next_) do { \
    if (*budget == 0u) return PR_RES_1509_LIMIT; \
    d->site = (site_); \
    if (rd8(ram, cs, (site_)) != 0x9Au || rd16(ram, cs, (uint16_t)((site_) + 1u)) != 0x0B0Bu) return PR_RES_1509_CODE; \
    --*budget; d->callee = 1; \
    { pr_system_leaf_cpu x; pr_system_leaf_status s; uint16_t seg = rd16(ram, cs, (uint16_t)((site_) + 3u)); \
      push(ram, c, cs); push(ram, c, (next_)); c->cs = seg; c->ip = 0x0B0Bu; \
      COPY(&x, c); s = pr_system_strload_0b0b(ram, ram_size, &x); COPY(c, &x); \
      if (s != PR_SYSTEM_LEAF_RETURNED) { d->callee_status = (int)s; return PR_RES_1509_CALLEE; } \
      if (c->cs != cs || c->ip != (next_)) return PR_RES_1509_TRANSFER; } } while (0)
/* LCALL 0709:0BF5 (the prompt); the segment word at site+3 is relocated */
#define PROMPT(site_, next_) do { \
    if (*budget == 0u) return PR_RES_1509_LIMIT; \
    d->site = (site_); \
    if (rd8(ram, cs, (site_)) != 0x9Au || rd16(ram, cs, (uint16_t)((site_) + 1u)) != 0x0BF5u) return PR_RES_1509_CODE; \
    --*budget; d->callee = 2; \
    { pr_r0709_0bf5_status s; uint16_t seg = rd16(ram, cs, (uint16_t)((site_) + 3u)); \
      push(ram, c, cs); push(ram, c, (next_)); c->cs = seg; c->ip = 0x0BF5u; \
      s = pr_r0709_msgbox_0bf5(bus, ram_size, c, &env->prompt, &d->d0bf5); \
      if (s == PR_R0709_0BF5_KEY) { d->callee_status = (int)s; return PR_RES_1509_KEY; } \
      if (s != PR_R0709_0BF5_RETURNED) { d->callee_status = (int)s; return PR_RES_1509_CALLEE; } \
      if (c->cs != cs || c->ip != (next_)) return PR_RES_1509_TRANSFER; } } while (0)
#define SPECIAL_1524() STRLOAD(0x1524u, 0x1529u)
#define SPECIAL_1529() PROMPT(0x1529u, 0x152Eu)
#define SPECIAL_15B3() STRLOAD(0x15B3u, 0x15B8u)
#define SPECIAL_15B8() PROMPT(0x15B8u, 0x15BDu)
/* 15EC LCALL 0AF8:0254 (FreeMem(record, 3Fh)); the segment word at 15EF is relocated */
#define SPECIAL_15EC() do { \
    if (*budget == 0u) return PR_RES_1509_LIMIT; \
    d->site = 0x15ECu; \
    if (rd8(ram, cs, 0x15ECu) != 0x9Au || rd16(ram, cs, 0x15EDu) != 0x0254u) return PR_RES_1509_CODE; \
    --*budget; d->callee = 3; \
    { pr_system_freemem_cpu x; pr_system_freemem_status s; uint16_t seg = rd16(ram, cs, 0x15EFu); \
      push(ram, c, cs); push(ram, c, 0x15F1u); c->cs = seg; c->ip = 0x0254u; \
      COPY(&x, c); s = pr_system_freemem_0254(ram, ram_size, &x, env->freemem_steps); COPY(c, &x); \
      if (s == PR_SYSTEM_FREEMEM_RUNERROR) { d->callee_status = (int)s; return PR_RES_1509_RUNERROR; } \
      if (s == PR_SYSTEM_FREEMEM_LIMIT) { d->callee_status = (int)s; return PR_RES_1509_LIMIT; } \
      if (s != PR_SYSTEM_FREEMEM_RETURNED) { d->callee_status = (int)s; return s == PR_SYSTEM_FREEMEM_TRANSFER ? PR_RES_1509_TRANSFER : PR_RES_1509_CALLEE; } \
      if (c->cs != cs || c->ip != 0x15F1u) return PR_RES_1509_TRANSFER; } } while (0)
/* 15F4 RETF 8 */
#define SPECIAL_15F4() do { \
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 8u); \
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_RES_1509_TRANSFER; \
    return PR_RES_1509_RETURNED; } while (0)

pr_res_1509_status pr_res_00ba_sidecar_remove_1509_cpu(pr_bus *bus, size_t ram_size, pr_res_1509_cpu *c, size_t *budget, const pr_res_1509_env *env,
                                                     pr_res_1509_diagnostic *diagnostic)
{
    pr_res_1509_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint8_t *ram; uint16_t cs, ret_ip, ret_cs;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x1509u || c->sp < 0x0200u || c->sp > 0xFFF2u)
        return PR_RES_1509_ARGUMENT;
    ram = bus->ram; cs = c->cs; memset(d, 0, sizeof *d); d->callee = -1; d->callee_status = -1;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    STEP(); push(ram, c, c->bp); c->ip = 0x150Au;    /* 1509 PUSH bp */
    STEP(); c->bp = c->sp; c->ip = 0x150Cu;    /* 150A MOV bp, sp */
    STEP(); c->sp = arith(c, c->sp, 0x002Cu, 1, 0, 16u); c->ip = 0x150Fu;    /* 150C SUB sp, 0x2c */
    STEP(); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + (6))); c->ip = 0x1512u;    /* 150F MOV ax, word ptr [bp + 6] */
    STEP(); c->ax = (uint16_t)(c->ax | rd16(ram, c->ss, (uint16_t)(c->bp + (8)))); logic(c, c->ax, 16u); c->ip = 0x1515u;    /* 1512 OR ax, word ptr [bp + 8] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x1531u; goto L_1531; } c->ip = 0x1517u;    /* 1515 JNE 0x1531 */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x0Eu) << 0u)); c->ip = 0x1519u;    /* 1517 MOV al, 0xe */
    STEP(); push(ram, c, c->ax); c->ip = 0x151Au;    /* 1519 PUSH ax */
    STEP(); c->di = (uint16_t)(c->bp - 0x18u); c->ip = 0x151Du;    /* 151A LEA di, [bp - 0x18] */
    STEP(); push(ram, c, c->ss); c->ip = 0x151Eu;    /* 151D PUSH ss */
    STEP(); push(ram, c, c->di); c->ip = 0x151Fu;    /* 151E PUSH di */
    STEP(); c->di = 0x14CEu; c->ip = 0x1522u;    /* 151F MOV di, 0x14ce */
    STEP(); push(ram, c, c->cs); c->ip = 0x1523u;    /* 1522 PUSH cs */
    STEP(); push(ram, c, c->di); c->ip = 0x1524u;    /* 1523 PUSH di */
    /* 1524 LCALL 0xaf8, 0xb0b */
    SPECIAL_1524();
    /* 1529 LCALL 0x709, 0xbf5 */
    SPECIAL_1529();
    STEP(); c->ip = 0x15F1u; goto L_15F1;    /* 152E JMP 0x15f1 */
    L_1531: STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (10))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (12))); c->ip = 0x1534u;    /* 1531 LES di, ptr [bp + 0xa] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x00C8u)); c->ip = 0x1539u;    /* 1534 MOV ax, word ptr es:[di + 0xc8] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x00CAu)); c->ip = 0x153Eu;    /* 1539 MOV dx, word ptr es:[di + 0xca] */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-4)), c->ax); c->ip = 0x1541u;    /* 153E MOV word ptr [bp - 4], ax */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-2)), c->dx); c->ip = 0x1544u;    /* 1541 MOV word ptr [bp - 2], dx */
    STEP(); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->ip = 0x1547u;    /* 1544 MOV ax, word ptr [bp - 4] */
    STEP(); c->dx = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x154Au;    /* 1547 MOV dx, word ptr [bp - 2] */
    STEP(); (void)arith(c, c->dx, rd16(ram, c->ss, (uint16_t)(c->bp + (8))), 1, 0, 16u); c->ip = 0x154Du;    /* 154A CMP dx, word ptr [bp + 8] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x156Eu; goto L_156E; } c->ip = 0x154Fu;    /* 154D JNE 0x156e */
    STEP(); (void)arith(c, c->ax, rd16(ram, c->ss, (uint16_t)(c->bp + (6))), 1, 0, 16u); c->ip = 0x1552u;    /* 154F CMP ax, word ptr [bp + 6] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x156Eu; goto L_156E; } c->ip = 0x1554u;    /* 1552 JNE 0x156e */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (6))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (8))); c->ip = 0x1557u;    /* 1554 LES di, ptr [bp + 6] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x002Au)); c->ip = 0x155Bu;    /* 1557 MOV ax, word ptr es:[di + 0x2a] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x002Cu)); c->ip = 0x155Fu;    /* 155B MOV dx, word ptr es:[di + 0x2c] */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (10))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (12))); c->ip = 0x1562u;    /* 155F LES di, ptr [bp + 0xa] */
    STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x00C8u), c->ax); c->ip = 0x1567u;    /* 1562 MOV word ptr es:[di + 0xc8], ax */
    STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x00CAu), c->dx); c->ip = 0x156Cu;    /* 1567 MOV word ptr es:[di + 0xca], dx */
    STEP(); c->ip = 0x159Eu; goto L_159E;    /* 156C JMP 0x159e */
    L_156E: STEP(); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->ip = 0x1571u;    /* 156E MOV ax, word ptr [bp - 4] */
    STEP(); c->ax = (uint16_t)(c->ax | rd16(ram, c->ss, (uint16_t)(c->bp + (-2)))); logic(c, c->ax, 16u); c->ip = 0x1574u;    /* 1571 OR ax, word ptr [bp - 2] */
    STEP(); if ((c->flags & 0x0040u)) { c->ip = 0x159Eu; goto L_159E; } c->ip = 0x1576u;    /* 1574 JE 0x159e */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x1579u;    /* 1576 LES di, ptr [bp - 4] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x002Au)); c->ip = 0x157Du;    /* 1579 MOV ax, word ptr es:[di + 0x2a] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x002Cu)); c->ip = 0x1581u;    /* 157D MOV dx, word ptr es:[di + 0x2c] */
    STEP(); (void)arith(c, c->dx, rd16(ram, c->ss, (uint16_t)(c->bp + (8))), 1, 0, 16u); c->ip = 0x1584u;    /* 1581 CMP dx, word ptr [bp + 8] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x158Bu; goto L_158B; } c->ip = 0x1586u;    /* 1584 JNE 0x158b */
    STEP(); (void)arith(c, c->ax, rd16(ram, c->ss, (uint16_t)(c->bp + (6))), 1, 0, 16u); c->ip = 0x1589u;    /* 1586 CMP ax, word ptr [bp + 6] */
    STEP(); if ((c->flags & 0x0040u)) { c->ip = 0x159Eu; goto L_159E; } c->ip = 0x158Bu;    /* 1589 JE 0x159e */
    L_158B: STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x158Eu;    /* 158B LES di, ptr [bp - 4] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x002Au)); c->ip = 0x1592u;    /* 158E MOV ax, word ptr es:[di + 0x2a] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x002Cu)); c->ip = 0x1596u;    /* 1592 MOV dx, word ptr es:[di + 0x2c] */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-4)), c->ax); c->ip = 0x1599u;    /* 1596 MOV word ptr [bp - 4], ax */
    STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + (-2)), c->dx); c->ip = 0x159Cu;    /* 1599 MOV word ptr [bp - 2], dx */
    STEP(); c->ip = 0x156Eu; goto L_156E;    /* 159C JMP 0x156e */
    L_159E: STEP(); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->ip = 0x15A1u;    /* 159E MOV ax, word ptr [bp - 4] */
    STEP(); c->ax = (uint16_t)(c->ax | rd16(ram, c->ss, (uint16_t)(c->bp + (-2)))); logic(c, c->ax, 16u); c->ip = 0x15A4u;    /* 15A1 OR ax, word ptr [bp - 2] */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x15BFu; goto L_15BF; } c->ip = 0x15A6u;    /* 15A4 JNE 0x15bf */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x0Eu) << 0u)); c->ip = 0x15A8u;    /* 15A6 MOV al, 0xe */
    STEP(); push(ram, c, c->ax); c->ip = 0x15A9u;    /* 15A8 PUSH ax */
    STEP(); c->di = (uint16_t)(c->bp - 0x2Bu); c->ip = 0x15ACu;    /* 15A9 LEA di, [bp - 0x2b] */
    STEP(); push(ram, c, c->ss); c->ip = 0x15ADu;    /* 15AC PUSH ss */
    STEP(); push(ram, c, c->di); c->ip = 0x15AEu;    /* 15AD PUSH di */
    STEP(); c->di = 0x14E2u; c->ip = 0x15B1u;    /* 15AE MOV di, 0x14e2 */
    STEP(); push(ram, c, c->cs); c->ip = 0x15B2u;    /* 15B1 PUSH cs */
    STEP(); push(ram, c, c->di); c->ip = 0x15B3u;    /* 15B2 PUSH di */
    /* 15B3 LCALL 0xaf8, 0xb0b */
    SPECIAL_15B3();
    /* 15B8 LCALL 0x709, 0xbf5 */
    SPECIAL_15B8();
    STEP(); c->ip = 0x15F1u; goto L_15F1;    /* 15BD JMP 0x15f1 */
    L_15BF: STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (6))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (8))); c->ip = 0x15C2u;    /* 15BF LES di, ptr [bp + 6] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x002Au)); c->ip = 0x15C6u;    /* 15C2 MOV ax, word ptr es:[di + 0x2a] */
    STEP(); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x002Cu)); c->ip = 0x15CAu;    /* 15C6 MOV dx, word ptr es:[di + 0x2c] */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (-4))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (-2))); c->ip = 0x15CDu;    /* 15CA LES di, ptr [bp - 4] */
    STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x002Au), c->ax); c->ip = 0x15D1u;    /* 15CD MOV word ptr es:[di + 0x2a], ax */
    STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x002Cu), c->dx); c->ip = 0x15D5u;    /* 15D1 MOV word ptr es:[di + 0x2c], dx */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (6))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (8))); c->ip = 0x15D8u;    /* 15D5 LES di, ptr [bp + 6] */
    STEP(); c->ax = 0u; logic(c, 0u, 16u); c->ip = 0x15DAu;    /* 15D8 XOR ax, ax */
    STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x002Au), c->ax); c->ip = 0x15DEu;    /* 15DA MOV word ptr es:[di + 0x2a], ax */
    STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x002Cu), c->ax); c->ip = 0x15E2u;    /* 15DE MOV word ptr es:[di + 0x2c], ax */
    STEP(); push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + (8)))); c->ip = 0x15E5u;    /* 15E2 PUSH word ptr [bp + 8] */
    STEP(); push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + (6)))); c->ip = 0x15E8u;    /* 15E5 PUSH word ptr [bp + 6] */
    STEP(); c->ax = 0x003Fu; c->ip = 0x15EBu;    /* 15E8 MOV ax, 0x3f */
    STEP(); push(ram, c, c->ax); c->ip = 0x15ECu;    /* 15EB PUSH ax */
    /* 15EC LCALL 0xaf8, 0x254 */
    SPECIAL_15EC();
    L_15F1: STEP(); c->sp = c->bp; c->ip = 0x15F3u;    /* 15F1 MOV sp, bp */
    STEP(); c->bp = pop(ram, c); c->ip = 0x15F4u;    /* 15F3 POP bp */
    /* 15F4 RETF 8 */
    SPECIAL_15F4();
}
