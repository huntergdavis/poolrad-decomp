/* START.EXE resident 00BA +16EC: the window message, full CPU state composed with its raw callees (docs/re/RES_00BA_16EC_CPU.md). */
#include "poolrad/recovered/resident_00ba_window_16ec_cpu.h"
#include "poolrad/recovered/resident_03f1_clear_0137_cpu.h"
#include "poolrad/recovered/resident_00ba_display_1816_cpu.h"
#include "poolrad/recovered/resident_00ba_clear_17e3_cpu.h"
#include "poolrad/recovered/resident_0709_delay_wrapper_0dbe_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
typedef pr_res_16ec_cpu cpu_t;
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)   /* the same 14-word layout */

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
static uint16_t szp(uint16_t r, unsigned bits)
{
    uint16_t sign = (uint16_t)(1u << (bits - 1u)), mask = (uint16_t)(bits == 8u ? 0xFFu : 0xFFFFu), f = parity((uint8_t)r);
    if (!(r & mask)) f |= 0x0040u;
    if (r & sign) f |= 0x0080u;
    return f;
}
/* SUB SP,102h and CMP byte,imm: the six arithmetic flags, stored rule (PF of the low byte) */
static uint16_t arith(cpu_t *c, uint16_t a, uint16_t b, int subtract, unsigned carry, unsigned bits)
{
    uint16_t mask = (uint16_t)(bits == 8u ? 0xFFu : 0xFFFFu), sign = (uint16_t)(1u << (bits - 1u));
    uint32_t wide = subtract ? (uint32_t)(a & mask) - (b & mask) - carry : (uint32_t)(a & mask) + (b & mask) + carry;
    uint16_t r = (uint16_t)(wide & mask), f = szp(r, bits);
    if (subtract ? (uint32_t)(a & mask) < (uint32_t)(b & mask) + carry : wide > mask) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if ((subtract ? ((a ^ b) & (a ^ r)) : (~(a ^ b) & (a ^ r))) & sign) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
/* XOR AH,AH: CF, OF and AF clear; SZP of the byte result */
static void logic(cpu_t *c, uint16_t r, unsigned bits) { c->flags = (uint16_t)((c->flags & ~0x08D5u) | szp(r, bits)); }
/* INC AX: CF kept; AF (v&Fh)==Fh, OF r==8000h, SZP (PF low byte) */
static uint16_t inc16(cpu_t *c, uint16_t v)
{
    uint16_t r = (uint16_t)(v + 1u), f = szp(r, 16u);
    if ((v & 0x0Fu) == 0x0Fu) f |= 0x0010u;
    if (r == 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D4u) | f);
    return r;
}

#define STEP() do { if (*budget == 0u) return PR_RES_16EC_LIMIT; --*budget; } while (0)
/* a far call site: 9Ah, the offset word, the relocated segment word read from RAM */
#define FARSITE(site_, off_) do { \
    if (*budget == 0u) return PR_RES_16EC_LIMIT; \
    d->site = (site_); \
    if (rd8(ram, cs, (site_)) != 0x9Au || rd16(ram, cs, (uint16_t)((site_) + 1u)) != (off_)) return PR_RES_16EC_CODE; \
    --*budget; push(ram, c, cs); push(ram, c, (uint16_t)((site_) + 5u)); c->cs = rd16(ram, cs, (uint16_t)((site_) + 3u)); c->ip = (off_); } while (0)
/* PUSH CS (already executed); CALL near at site_ (E8h, displacement to target_) */
#define NEARSITE(site_, target_) do { \
    if (*budget == 0u) return PR_RES_16EC_LIMIT; \
    d->site = (site_); \
    if (rd8(ram, cs, (site_)) != 0xE8u || (uint16_t)((site_) + 3u + rd16(ram, cs, (uint16_t)((site_) + 1u))) != (target_)) return PR_RES_16EC_CODE; \
    --*budget; push(ram, c, (uint16_t)((site_) + 3u)); c->ip = (target_); } while (0)
#define BACK(next_) do { if (c->cs != cs || c->ip != (next_)) return PR_RES_16EC_TRANSFER; } while (0)
#define CALLEE_STOP(id_, s_) do { d->callee = (id_); d->callee_status = (int)(s_); } while (0)

static pr_res_16ec_status clear0137(pr_bus *bus, size_t n, cpu_t *c, const pr_res_16ec_env *e, pr_res_16ec_diagnostic *d)
{
    pr_r03f1_clear_cpu x; pr_r03f1_clear_cpu_diagnostic dd; pr_r03f1_clear_cpu_status s;
    COPY(&x, c); s = pr_r03f1_clear_0137_cpu(bus, n, &x, e->clear_loopbacks, e->clear_rect_loopbacks, &dd); COPY(c, &x);
    if (s == PR_R03F1_CLEAR_CPU_OK) return PR_RES_16EC_RETURNED;
    CALLEE_STOP(2, s);
    return s == PR_R03F1_CLEAR_CPU_LIMIT ? PR_RES_16EC_LIMIT : s == PR_R03F1_CLEAR_CPU_TRANSFER ? PR_RES_16EC_TRANSFER : PR_RES_16EC_CALLEE;
}
static pr_res_16ec_status display1816(pr_bus *bus, size_t n, cpu_t *c, const pr_res_16ec_env *e, pr_res_16ec_diagnostic *d)
{
    pr_r00ba_display_cpu x; pr_r00ba_display_cpu_diagnostic dd; pr_r00ba_display_cpu_status s;
    COPY(&x, c); s = pr_r00ba_display_1816_cpu(bus, n, &x, e->display_printer_loopbacks, e->display_glyph_loopbacks, &dd); COPY(c, &x);
    if (s == PR_R00BA_DISPLAY_CPU_OK) return PR_RES_16EC_RETURNED;
    CALLEE_STOP(3, s);
    return s == PR_R00BA_DISPLAY_CPU_TRANSFER ? PR_RES_16EC_TRANSFER : PR_RES_16EC_CALLEE;
}
/* 0709:0768, with its named STOP_BF5 followed by 0709:0BF5 and the resume at 0999h, at most env->prompt_rounds times */
static pr_res_16ec_status printer0768(pr_bus *bus, size_t n, cpu_t *c, const pr_res_16ec_env *e, pr_res_16ec_diagnostic *d)
{
    d->prompt_rounds = 0;
    for (;;) {
        pr_r0709_printer_0768_regs x; pr_r0709_printer_status s;
        COPY(&x, c); s = pr_r0709_printer_0768_cpu(bus, n, &x, &e->printer, &d->printer); COPY(c, &x);
        if (s == PR_R0709_PRINTER_RETURNED) return PR_RES_16EC_RETURNED;
        if (s != PR_R0709_PRINTER_STOP_BF5) {
            CALLEE_STOP(4, s);
            return s == PR_R0709_PRINTER_LIMIT ? PR_RES_16EC_LIMIT : PR_RES_16EC_CALLEE;
        }
        if (d->prompt_rounds >= e->prompt_rounds) { CALLEE_STOP(5, -1); return PR_RES_16EC_LIMIT; }
        ++d->prompt_rounds;
        {
            pr_r0709_0bf5_cpu y; pr_r0709_0bf5_status t; uint16_t pcs = c->cs;
            COPY(&y, c); t = pr_r0709_msgbox_0bf5(bus, n, &y, &e->prompt, &d->prompt); COPY(c, &y);
            if (t != PR_R0709_0BF5_RETURNED) { CALLEE_STOP(5, t); return t == PR_R0709_0BF5_KEY ? PR_RES_16EC_KEY : PR_RES_16EC_CALLEE; }
            if (c->cs != pcs || c->ip != 0x0999u) { CALLEE_STOP(5, t); return PR_RES_16EC_TRANSFER; }
        }
    }
}
static pr_res_16ec_status box0000(pr_bus *bus, size_t n, cpu_t *c, const pr_res_16ec_env *e, pr_res_16ec_diagnostic *d)
{
    pr_r03f1_box_regs x; pr_r03f1_box_status s;
    COPY(&x, c); s = pr_r03f1_box_0000_cpu(bus, n, &x, &e->box, &d->box); COPY(c, &x);
    if (s == PR_R03F1_BOX_OK) return PR_RES_16EC_RETURNED;
    CALLEE_STOP(6, s);
    switch (s) {
    case PR_R03F1_BOX_TRANSFER: return PR_RES_16EC_TRANSFER;
    case PR_R03F1_BOX_LIMIT: return PR_RES_16EC_LIMIT;
    case PR_R03F1_BOX_OVERLAY: return PR_RES_16EC_OVERLAY;
    case PR_R03F1_BOX_EXIT: return PR_RES_16EC_EXIT;
    default: return PR_RES_16EC_CALLEE;
    }
}
static pr_res_16ec_status delay0dbe(pr_bus *bus, size_t n, cpu_t *c, const pr_res_16ec_env *e, pr_res_16ec_diagnostic *d)
{
    pr_r0709_delay_cpu x; pr_r0709_delay_cpu_status s; int inner = -1;
    COPY(&x, c); s = pr_r0709_delay_wrapper_0dbe_cpu(bus->ram, n, &x, e->delay_loopbacks, &inner); COPY(c, &x);
    if (s == PR_R0709_DELAY_CPU_OK) return PR_RES_16EC_RETURNED;
    CALLEE_STOP(7, s); d->printer.callee_status = inner;
    return s == PR_R0709_DELAY_CPU_TRANSFER ? PR_RES_16EC_TRANSFER : PR_RES_16EC_CALLEE;
}
static pr_res_16ec_status clear17e3(pr_bus *bus, size_t n, cpu_t *c, const pr_res_16ec_env *e, pr_res_16ec_diagnostic *d)
{
    pr_r00ba_clear_cpu x; pr_r00ba_clear_cpu_diagnostic dd; pr_r00ba_clear_cpu_status s;
    COPY(&x, c); s = pr_r00ba_clear_17e3_cpu(bus, n, &x, e->clear17e3_loopbacks, e->clear17e3_rect_loopbacks, &dd); COPY(c, &x);
    if (s == PR_R00BA_CLEAR_CPU_OK) return PR_RES_16EC_RETURNED;
    CALLEE_STOP(8, s);
    return s == PR_R00BA_CLEAR_CPU_TRANSFER ? PR_RES_16EC_TRANSFER : PR_RES_16EC_CALLEE;
}

#define RUN(call_, next_) do { pr_res_16ec_status s_ = call_(bus, ram_size, c, env, d); if (s_ != PR_RES_16EC_RETURNED) return s_; BACK(next_); } while (0)
#define SPECIAL_1702() do { FARSITE(0x1702u, 0x0B25u); \
    { pr_system_0b25_cpu x; pr_system_0b25_status s; COPY(&x, c); s = pr_system_strstore_0b25(ram, ram_size, &x); COPY(c, &x); \
      if (s != PR_SYSTEM_0B25_RETURNED) { CALLEE_STOP(1, s); return PR_RES_16EC_CALLEE; } } BACK(0x1707u); } while (0)
#define SPECIAL_171B() do { FARSITE(0x171Bu, 0x0137u); RUN(clear0137, 0x1720u); } while (0)
#define SPECIAL_1731() do { NEARSITE(0x1731u, 0x1816u); RUN(display1816, 0x1734u); } while (0)
#define SPECIAL_1750() do { FARSITE(0x1750u, 0x0768u); RUN(printer0768, 0x1755u); } while (0)
#define SPECIAL_177B() do { FARSITE(0x177Bu, 0x0000u); RUN(box0000, 0x1780u); } while (0)
#define SPECIAL_178E() do { FARSITE(0x178Eu, 0x0137u); RUN(clear0137, 0x1793u); } while (0)
#define SPECIAL_17A8() do { NEARSITE(0x17A8u, 0x1816u); RUN(display1816, 0x17ABu); } while (0)
#define SPECIAL_17C9() do { FARSITE(0x17C9u, 0x0768u); RUN(printer0768, 0x17CEu); } while (0)
#define SPECIAL_17D4() do { FARSITE(0x17D4u, 0x0DBEu); RUN(delay0dbe, 0x17D9u); } while (0)
#define SPECIAL_17DA() do { NEARSITE(0x17DAu, 0x17E3u); RUN(clear17e3, 0x17DDu); } while (0)
/* 17E0 RETF 0Ch */
#define SPECIAL_17E0() do { \
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 0x0Cu); \
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_RES_16EC_TRANSFER; \
    return PR_RES_16EC_RETURNED; } while (0)

pr_res_16ec_status pr_res_00ba_window_16ec_cpu(pr_bus *bus, size_t ram_size, pr_res_16ec_cpu *c, size_t *budget, const pr_res_16ec_env *env,
                                               pr_res_16ec_diagnostic *diagnostic)
{
    pr_res_16ec_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint8_t *ram; uint16_t cs, ret_ip, ret_cs;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x16ECu
        || c->sp < 0x0800u || c->sp > 0xFFF0u)
        return PR_RES_16EC_ARGUMENT;
    ram = bus->ram; cs = c->cs; memset(d, 0, sizeof *d); d->callee = -1; d->callee_status = -1;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    STEP(); push(ram, c, c->bp); c->ip = 0x16EDu;    /* 16EC PUSH bp */
    STEP(); c->bp = c->sp; c->ip = 0x16EFu;    /* 16ED MOV bp, sp */
    STEP(); c->sp = arith(c, c->sp, 0x0102u, 1, 0, 16u); c->ip = 0x16F3u;    /* 16EF SUB sp, 0x102 */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + (10))); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + (12))); c->ip = 0x16F6u;    /* 16F3 LES di, ptr [bp + 0xa] */
    STEP(); push(ram, c, c->es); c->ip = 0x16F7u;    /* 16F6 PUSH es */
    STEP(); push(ram, c, c->di); c->ip = 0x16F8u;    /* 16F7 PUSH di */
    STEP(); c->di = (uint16_t)(c->bp - 0x100u); c->ip = 0x16FCu;    /* 16F8 LEA di, [bp - 0x100] */
    STEP(); push(ram, c, c->ss); c->ip = 0x16FDu;    /* 16FC PUSH ss */
    STEP(); push(ram, c, c->di); c->ip = 0x16FEu;    /* 16FD PUSH di */
    STEP(); c->ax = 0x00FFu; c->ip = 0x1701u;    /* 16FE MOV ax, 0xff */
    STEP(); push(ram, c, c->ax); c->ip = 0x1702u;    /* 1701 PUSH ax */
    /* 1702 LCALL 0xaf8, 0xb25 */
    SPECIAL_1702();
    STEP(); (void)arith(c, rd8(ram, c->ds, 0x49F3u), 0x05u, 1, 0, 8u); c->ip = 0x170Cu;    /* 1707 CMP byte ptr [0x49f3], 5 */
    STEP(); if (!(c->flags & 0x0040u)) { c->ip = 0x1757u; goto L_1757; } c->ip = 0x170Eu;    /* 170C JNE 0x1757 */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x17u) << 0u)); c->ip = 0x1710u;    /* 170E MOV al, 0x17 */
    STEP(); push(ram, c, c->ax); c->ip = 0x1711u;    /* 1710 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (8)))) << 0u)); c->ip = 0x1714u;    /* 1711 MOV al, byte ptr [bp + 8] */
    STEP(); push(ram, c, c->ax); c->ip = 0x1715u;    /* 1714 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x26u) << 0u)); c->ip = 0x1717u;    /* 1715 MOV al, 0x26 */
    STEP(); push(ram, c, c->ax); c->ip = 0x1718u;    /* 1717 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x15u) << 0u)); c->ip = 0x171Au;    /* 1718 MOV al, 0x15 */
    STEP(); push(ram, c, c->ax); c->ip = 0x171Bu;    /* 171A PUSH ax */
    /* 171B LCALL 0x3f1, 0x137 */
    SPECIAL_171B();
    STEP(); push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + (16)))); c->ip = 0x1723u;    /* 1720 PUSH word ptr [bp + 0x10] */
    STEP(); push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + (14)))); c->ip = 0x1726u;    /* 1723 PUSH word ptr [bp + 0xe] */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x17u) << 0u)); c->ip = 0x1728u;    /* 1726 MOV al, 0x17 */
    STEP(); push(ram, c, c->ax); c->ip = 0x1729u;    /* 1728 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (8)))) << 0u)); c->ip = 0x172Cu;    /* 1729 MOV al, byte ptr [bp + 8] */
    STEP(); push(ram, c, c->ax); c->ip = 0x172Du;    /* 172C PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x00u) << 0u)); c->ip = 0x172Fu;    /* 172D MOV al, 0 */
    STEP(); push(ram, c, c->ax); c->ip = 0x1730u;    /* 172F PUSH ax */
    STEP(); push(ram, c, c->cs); c->ip = 0x1731u;    /* 1730 PUSH cs */
    /* 1731 CALL 0x1816 */
    SPECIAL_1731();
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x17u) << 0u)); c->ip = 0x1736u;    /* 1734 MOV al, 0x17 */
    STEP(); push(ram, c, c->ax); c->ip = 0x1737u;    /* 1736 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (8)))) << 0u)); c->ip = 0x173Au;    /* 1737 MOV al, byte ptr [bp + 8] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x173Cu;    /* 173A XOR ah, ah */
    STEP(); c->ax = inc16(c, c->ax); c->ip = 0x173Du;    /* 173C INC ax */
    STEP(); push(ram, c, c->ax); c->ip = 0x173Eu;    /* 173D PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x26u) << 0u)); c->ip = 0x1740u;    /* 173E MOV al, 0x26 */
    STEP(); push(ram, c, c->ax); c->ip = 0x1741u;    /* 1740 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x15u) << 0u)); c->ip = 0x1743u;    /* 1741 MOV al, 0x15 */
    STEP(); push(ram, c, c->ax); c->ip = 0x1744u;    /* 1743 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x0Au) << 0u)); c->ip = 0x1746u;    /* 1744 MOV al, 0xa */
    STEP(); push(ram, c, c->ax); c->ip = 0x1747u;    /* 1746 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x01u) << 0u)); c->ip = 0x1749u;    /* 1747 MOV al, 1 */
    STEP(); push(ram, c, c->ax); c->ip = 0x174Au;    /* 1749 PUSH ax */
    STEP(); c->di = (uint16_t)(c->bp - 0x100u); c->ip = 0x174Eu;    /* 174A LEA di, [bp - 0x100] */
    STEP(); push(ram, c, c->ss); c->ip = 0x174Fu;    /* 174E PUSH ss */
    STEP(); push(ram, c, c->di); c->ip = 0x1750u;    /* 174F PUSH di */
    /* 1750 LCALL 0x709, 0x768 */
    SPECIAL_1750();
    STEP(); c->ip = 0x17CEu; goto L_17CE;    /* 1755 JMP 0x17ce */
    L_1757: STEP(); (void)arith(c, rd8(ram, c->ds, 0x6DDAu), 0x00u, 1, 0, 8u); c->ip = 0x175Cu;    /* 1757 CMP byte ptr [0x6dda], 0 */
    STEP(); if ((c->flags & 0x0040u)) { c->ip = 0x1765u; goto L_1765; } c->ip = 0x175Eu;    /* 175C JE 0x1765 */
    STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-257)), 0x12u); c->ip = 0x1763u;    /* 175E MOV byte ptr [bp - 0x101], 0x12 */
    STEP(); c->ip = 0x1780u; goto L_1780;    /* 1763 JMP 0x1780 */
    L_1765: STEP(); wr8(ram, c->ss, (uint16_t)(c->bp + (-257)), 0x11u); c->ip = 0x176Au;    /* 1765 MOV byte ptr [bp - 0x101], 0x11 */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x01u) << 0u)); c->ip = 0x176Cu;    /* 176A MOV al, 1 */
    STEP(); push(ram, c, c->ax); c->ip = 0x176Du;    /* 176C PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-257)))) << 0u)); c->ip = 0x1771u;    /* 176D MOV al, byte ptr [bp - 0x101] */
    STEP(); push(ram, c, c->ax); c->ip = 0x1772u;    /* 1771 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x26u) << 0u)); c->ip = 0x1774u;    /* 1772 MOV al, 0x26 */
    STEP(); push(ram, c, c->ax); c->ip = 0x1775u;    /* 1774 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x16u) << 0u)); c->ip = 0x1777u;    /* 1775 MOV al, 0x16 */
    STEP(); push(ram, c, c->ax); c->ip = 0x1778u;    /* 1777 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x00u) << 0u)); c->ip = 0x177Au;    /* 1778 MOV al, 0 */
    STEP(); push(ram, c, c->ax); c->ip = 0x177Bu;    /* 177A PUSH ax */
    /* 177B LCALL 0x3f1, 0 */
    SPECIAL_177B();
    L_1780: STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x01u) << 0u)); c->ip = 0x1782u;    /* 1780 MOV al, 1 */
    STEP(); push(ram, c, c->ax); c->ip = 0x1783u;    /* 1782 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-257)))) << 0u)); c->ip = 0x1787u;    /* 1783 MOV al, byte ptr [bp - 0x101] */
    STEP(); push(ram, c, c->ax); c->ip = 0x1788u;    /* 1787 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x26u) << 0u)); c->ip = 0x178Au;    /* 1788 MOV al, 0x26 */
    STEP(); push(ram, c, c->ax); c->ip = 0x178Bu;    /* 178A PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x16u) << 0u)); c->ip = 0x178Du;    /* 178B MOV al, 0x16 */
    STEP(); push(ram, c, c->ax); c->ip = 0x178Eu;    /* 178D PUSH ax */
    /* 178E LCALL 0x3f1, 0x137 */
    SPECIAL_178E();
    STEP(); push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + (16)))); c->ip = 0x1796u;    /* 1793 PUSH word ptr [bp + 0x10] */
    STEP(); push(ram, c, rd16(ram, c->ss, (uint16_t)(c->bp + (14)))); c->ip = 0x1799u;    /* 1796 PUSH word ptr [bp + 0xe] */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x01u) << 0u)); c->ip = 0x179Bu;    /* 1799 MOV al, 1 */
    STEP(); push(ram, c, c->ax); c->ip = 0x179Cu;    /* 179B PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-257)))) << 0u)); c->ip = 0x17A0u;    /* 179C MOV al, byte ptr [bp - 0x101] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x17A2u;    /* 17A0 XOR ah, ah */
    STEP(); c->ax = inc16(c, c->ax); c->ip = 0x17A3u;    /* 17A2 INC ax */
    STEP(); push(ram, c, c->ax); c->ip = 0x17A4u;    /* 17A3 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x00u) << 0u)); c->ip = 0x17A6u;    /* 17A4 MOV al, 0 */
    STEP(); push(ram, c, c->ax); c->ip = 0x17A7u;    /* 17A6 PUSH ax */
    STEP(); push(ram, c, c->cs); c->ip = 0x17A8u;    /* 17A7 PUSH cs */
    /* 17A8 CALL 0x1816 */
    SPECIAL_17A8();
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x01u) << 0u)); c->ip = 0x17ADu;    /* 17AB MOV al, 1 */
    STEP(); push(ram, c, c->ax); c->ip = 0x17AEu;    /* 17AD PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(rd8(ram, c->ss, (uint16_t)(c->bp + (-257)))) << 0u)); c->ip = 0x17B2u;    /* 17AE MOV al, byte ptr [bp - 0x101] */
    STEP(); c->ax = (uint16_t)((c->ax & 0x00FFu) | ((uint16_t)(uint8_t)(0) << 8u)); logic(c, 0u, 8u); c->ip = 0x17B4u;    /* 17B2 XOR ah, ah */
    STEP(); c->ax = inc16(c, c->ax); c->ip = 0x17B5u;    /* 17B4 INC ax */
    STEP(); c->ax = inc16(c, c->ax); c->ip = 0x17B6u;    /* 17B5 INC ax */
    STEP(); push(ram, c, c->ax); c->ip = 0x17B7u;    /* 17B6 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x26u) << 0u)); c->ip = 0x17B9u;    /* 17B7 MOV al, 0x26 */
    STEP(); push(ram, c, c->ax); c->ip = 0x17BAu;    /* 17B9 PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x16u) << 0u)); c->ip = 0x17BCu;    /* 17BA MOV al, 0x16 */
    STEP(); push(ram, c, c->ax); c->ip = 0x17BDu;    /* 17BC PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x0Au) << 0u)); c->ip = 0x17BFu;    /* 17BD MOV al, 0xa */
    STEP(); push(ram, c, c->ax); c->ip = 0x17C0u;    /* 17BF PUSH ax */
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | ((uint16_t)(uint8_t)(0x01u) << 0u)); c->ip = 0x17C2u;    /* 17C0 MOV al, 1 */
    STEP(); push(ram, c, c->ax); c->ip = 0x17C3u;    /* 17C2 PUSH ax */
    STEP(); c->di = (uint16_t)(c->bp - 0x100u); c->ip = 0x17C7u;    /* 17C3 LEA di, [bp - 0x100] */
    STEP(); push(ram, c, c->ss); c->ip = 0x17C8u;    /* 17C7 PUSH ss */
    STEP(); push(ram, c, c->di); c->ip = 0x17C9u;    /* 17C8 PUSH di */
    /* 17C9 LCALL 0x709, 0x768 */
    SPECIAL_17C9();
    L_17CE: STEP(); (void)arith(c, rd8(ram, c->ss, (uint16_t)(c->bp + (6))), 0x00u, 1, 0, 8u); c->ip = 0x17D2u;    /* 17CE CMP byte ptr [bp + 6], 0 */
    STEP(); if ((c->flags & 0x0040u)) { c->ip = 0x17DDu; goto L_17DD; } c->ip = 0x17D4u;    /* 17D2 JE 0x17dd */
    /* 17D4 LCALL 0x709, 0xdbe */
    SPECIAL_17D4();
    STEP(); push(ram, c, c->cs); c->ip = 0x17DAu;    /* 17D9 PUSH cs */
    /* 17DA CALL 0x17e3 */
    SPECIAL_17DA();
    L_17DD: STEP(); c->sp = c->bp; c->ip = 0x17DFu;    /* 17DD MOV sp, bp */
    STEP(); c->bp = pop(ram, c); c->ip = 0x17E0u;    /* 17DF POP bp */
    /* 17E0 RETF 0xc */
    SPECIAL_17E0();

}
