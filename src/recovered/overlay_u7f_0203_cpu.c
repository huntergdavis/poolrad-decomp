/* Original unit 0x7F +0203: the capped add with a message (docs/re/OVL_007F_0203_CPU.md). */
#include "poolrad/recovered/overlay_u7f_0203_cpu.h"
#include "poolrad/recovered/overlay_u7f_record_word_add_003a.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_u7f_0203_regs cpu_t;

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
static uint16_t bp16(const uint8_t *ram, const cpu_t *c, int off) { return rd16(ram, c->ss, (uint16_t)(c->bp + off)); }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 4u; }
/* ADD/ADC/SUB/SBB/CMP at 16 bits with an incoming carry for ADC and SBB: every arithmetic flag. */
static uint16_t arith16(cpu_t *c, uint16_t a, uint16_t b, int subtract, unsigned carry)
{
    uint32_t wide = subtract ? (uint32_t)a - b - carry : (uint32_t)a + b + carry;
    uint16_t r = (uint16_t)wide, f = parity((uint8_t)r);
    if (subtract ? (uint32_t)a < (uint32_t)b + carry : wide > 0xFFFFu) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((subtract ? ((a ^ b) & (a ^ r)) : (~(a ^ b) & (a ^ r))) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
static void logic(cpu_t *c, uint16_t r, unsigned bits)                                  /* OR AL,AL / XOR DX,DX: CF, OF and AF clear; PF, ZF, SF from the result */
{ c->flags = (uint16_t)((c->flags & ~0x08D5u) | parity((uint8_t)r) | (r ? 0u : 0x0040u) | ((r & (bits == 8u ? 0x80u : 0x8000u)) ? 0x0080u : 0u)); }
/* ADD WORD ES:[DI+90h],AX through the bus (the accumulator word of the record at G); both branches use this one helper. */
static void acc_add(pr_bus *bus, cpu_t *c)
{ uint16_t at = (uint16_t)(c->di + 0x90u); bwr16(bus, c->es, at, arith16(c, brd16(bus, c->es, at), c->ax, 0, 0)); }
static uint16_t lea_bp(const cpu_t *c, int off) { return (uint16_t)(c->bp + off); }

/* One call: the callee's return frame is pushed (far: CS then IP; near after PUSH CS: IP). Returns -1 to continue, or the status. */
#define STEP() do { if (*budget == 0u) return PR_U7F_0203_LIMIT; --*budget; } while (0)

pr_u7f_0203_status pr_u7f_0203_cpu(pr_bus *bus, size_t ram_size, pr_u7f_0203_regs *c, size_t *budget, const pr_u7f_0203_limits *limits, pr_u7f_0203_diagnostic *diagnostic)
{
    pr_u7f_0203_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint8_t *ram; uint16_t cs, ret_ip, ret_cs; int above;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || limits == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0203u || c->sp < 0x0200u || c->sp > 0xFFF8u)
        return PR_U7F_0203_ARGUMENT;
    ram = bus->ram; cs = c->cs; memset(d, 0, sizeof *d); d->callee = -1; d->callee_status = -1;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    STEP(); push(ram, c, c->bp); c->ip = 0x0204u;                                                    /* 0203 PUSH BP */
    STEP(); c->bp = c->sp; c->ip = 0x0206u;                                                          /* 0204 MOV BP,SP */
    STEP(); c->sp = arith16(c, c->sp, 0x002Au, 1, 0); c->ip = 0x0209u;                               /* 0206 SUB SP,2Ah */
    STEP(); push(ram, c, rd16(ram, c->ds, 0x5D94u)); c->ip = 0x020Du;                                /* 0209 PUSH [5D94] */
    STEP(); push(ram, c, rd16(ram, c->ds, 0x5D92u)); c->ip = 0x0211u;                                /* 020D PUSH [5D92] */
    STEP(); push(ram, c, bp16(ram, c, 6)); c->ip = 0x0214u;                                          /* 0211 PUSH [BP+6] */
    STEP(); c->di = lea_bp(c, -2); c->ip = 0x0217u;                                                  /* 0214 LEA DI,[BP-2] */
    STEP(); push(ram, c, c->ss); c->ip = 0x0218u;                                                    /* 0217 PUSH SS */
    STEP(); push(ram, c, c->di); c->ip = 0x0219u;                                                    /* 0218 PUSH DI */
    STEP(); push(ram, c, c->cs); c->ip = 0x021Au;                                                    /* 0219 PUSH CS */
    if (*budget == 0u) return PR_U7F_0203_LIMIT;                                                     /* 021A CALL 0054 */
    d->site = 0x021Au;
    if (rd8(ram, cs, 0x021Au) != 0xE8u || rd16(ram, cs, 0x021Bu) != 0xFE37u) return PR_U7F_0203_CODE;
    --*budget; push(ram, c, 0x021Du); c->ip = 0x0054u; d->callee = 0;
    {
        pr_u7f_0054_status s = pr_u7f_0054_cpu(bus, ram_size, c, budget, &d->d0054);
        if (s == PR_U7F_0054_LIMIT) return PR_U7F_0203_LIMIT;
        if (s != PR_U7F_0054_RETURNED) { d->callee_status = (int)s; return s == PR_U7F_0054_TRANSFER ? PR_U7F_0203_TRANSFER : PR_U7F_0203_CALLEE; }
        if (c->cs != cs || c->ip != 0x021Du) return PR_U7F_0203_TRANSFER;
    }
    STEP(); logic(c, (uint8_t)c->ax, 8u); c->ip = 0x021Fu;                                           /* 021D OR AL,AL */
    STEP(); above = (c->flags & 0x0040u) == 0u; c->ip = above ? 0x0221u : 0x0273u;                   /* 021F JE 0273 */
    if (above) {
        STEP(); c->di = lea_bp(c, -0x2A); c->ip = 0x0224u;                                           /* 0221 LEA DI,[BP-2Ah] */
        STEP(); push(ram, c, c->ss); c->ip = 0x0225u;                                                /* 0224 PUSH SS */
        STEP(); push(ram, c, c->di); c->ip = 0x0226u;                                                /* 0225 PUSH DI */
        STEP(); c->di = 0x01DBu; c->ip = 0x0229u;                                                    /* 0226 MOV DI,01DB */
        STEP(); push(ram, c, c->cs); c->ip = 0x022Au;                                                /* 0229 PUSH CS */
        STEP(); push(ram, c, c->di); c->ip = 0x022Bu;                                                /* 022A PUSH DI */
        if (*budget == 0u) return PR_U7F_0203_LIMIT;                                                 /* 022B LCALL 0AF8:0B0B (the segment word at 022E is relocated) */
        d->site = 0x022Bu;
        if (rd8(ram, cs, 0x022Bu) != 0x9Au || rd16(ram, cs, 0x022Cu) != 0x0B0Bu) return PR_U7F_0203_CODE;
        --*budget; d->callee = 1;
        {
            pr_system_leaf_cpu x; pr_system_leaf_status s; uint16_t seg = rd16(ram, cs, 0x022Eu);
            push(ram, c, cs); push(ram, c, 0x0230u); c->cs = seg; c->ip = 0x0B0Bu;
            COPY(&x, c); s = pr_system_strload_0b0b(ram, ram_size, &x); COPY(c, &x);
            if (s != PR_SYSTEM_LEAF_RETURNED) { d->callee_status = (int)s; return PR_U7F_0203_CALLEE; }
            if (c->cs != cs || c->ip != 0x0230u) return PR_U7F_0203_TRANSFER;
        }
        if (*budget == 0u) return PR_U7F_0203_LIMIT;                                                 /* 0230 LCALL 00BA:1693 (the segment word at 0233 is relocated) */
        d->site = 0x0230u;
        if (rd8(ram, cs, 0x0230u) != 0x9Au || rd16(ram, cs, 0x0231u) != 0x1693u) return PR_U7F_0203_CODE;
        --*budget; d->callee = 2;
        {
            pr_r00ba_1693_cpu x; pr_r00ba_1693_status s; uint16_t seg = rd16(ram, cs, 0x0233u);
            push(ram, c, cs); push(ram, c, 0x0235u); c->cs = seg; c->ip = 0x1693u;
            COPY(&x, c); s = pr_r00ba_display_1693_cpu(bus, ram_size, &x, limits->clear, limits->rect, limits->printer, limits->glyph, limits->delay, &d->d1693); COPY(c, &x);
            if (s != PR_R00BA_1693_OK) { d->callee_status = (int)s; return s == PR_R00BA_1693_TRANSFER ? PR_U7F_0203_TRANSFER : PR_U7F_0203_CALLEE; }
            if (c->cs != cs || c->ip != 0x0235u) return PR_U7F_0203_TRANSFER;
        }
        STEP(); c->ax = bp16(ram, c, -2); c->ip = 0x0238u;                                           /* 0235 MOV AX,[BP-2] */
        STEP(); c->di = rd16(ram, c->ds, 0x5D92u); c->es = rd16(ram, c->ds, 0x5D94u); c->ip = 0x023Cu;   /* 0238 LES DI,[5D92] */
        STEP(); acc_add(bus, c); c->ip = 0x0241u;   /* 023C ADD ES:[DI+90],AX (through the bus) */
        STEP(); push(ram, c, rd16(ram, c->ds, 0x5D94u)); c->ip = 0x0245u;                            /* 0241 PUSH [5D94] */
        STEP(); push(ram, c, rd16(ram, c->ds, 0x5D92u)); c->ip = 0x0249u;                            /* 0245 PUSH [5D92] */
        STEP(); push(ram, c, bp16(ram, c, -2)); c->ip = 0x024Cu;                                     /* 0249 PUSH [BP-2] */
        STEP(); push(ram, c, c->cs); c->ip = 0x024Du;                                                /* 024C PUSH CS */
        if (*budget == 0u) return PR_U7F_0203_LIMIT;                                                 /* 024D CALL 003A */
        d->site = 0x024Du;
        if (rd8(ram, cs, 0x024Du) != 0xE8u || rd16(ram, cs, 0x024Eu) != 0xFDEAu) return PR_U7F_0203_CODE;
        --*budget; d->callee = 3; push(ram, c, 0x0250u); c->ip = 0x003Au;
        {
            pr_u7f_003a_cpu x; pr_u7f_003a_status s; COPY(&x, c); s = pr_u7f_record_word_add_003a(ram, ram_size, &x); COPY(c, &x);
            if (s != PR_U7F_003A_OK) { d->callee_status = (int)s; return PR_U7F_0203_CALLEE; }
            if (c->cs != cs || c->ip != 0x0250u) return PR_U7F_0203_TRANSFER;
        }
        STEP(); c->ax = bp16(ram, c, -2); c->ip = 0x0253u;                                           /* 0250 MOV AX,[BP-2] */
        STEP(); c->dx = 0u; logic(c, 0u, 16u); c->ip = 0x0255u;                                      /* 0253 XOR DX,DX */
        STEP(); c->cx = c->ax; c->ip = 0x0257u;                                                      /* 0255 MOV CX,AX */
        STEP(); c->bx = c->dx; c->ip = 0x0259u;                                                      /* 0257 MOV BX,DX */
        STEP(); c->ax = bp16(ram, c, 6); c->ip = 0x025Cu;                                            /* 0259 MOV AX,[BP+6] */
        STEP(); c->dx = 0u; logic(c, 0u, 16u); c->ip = 0x025Eu;                                      /* 025C XOR DX,DX */
        STEP(); c->ax = arith16(c, c->ax, rd16(ram, c->ds, 0x6804u), 0, 0); c->ip = 0x0262u;         /* 025E ADD AX,[6804] */
        STEP(); c->dx = arith16(c, c->dx, rd16(ram, c->ds, 0x6806u), 0, (c->flags & 1u)); c->ip = 0x0266u;   /* 0262 ADC DX,[6806] */
        STEP(); c->ax = arith16(c, c->ax, c->cx, 1, 0); c->ip = 0x0268u;                             /* 0266 SUB AX,CX */
        STEP(); c->dx = arith16(c, c->dx, c->bx, 1, (c->flags & 1u)); c->ip = 0x026Au;               /* 0268 SBB DX,BX */
        STEP(); wr16(ram, c->ds, 0x6804u, c->ax); c->ip = 0x026Du;                                   /* 026A MOV [6804],AX */
        STEP(); wr16(ram, c->ds, 0x6806u, c->dx); c->ip = 0x0271u;                                   /* 026D MOV [6806],DX */
        STEP(); c->ip = 0x028Eu;                                                                     /* 0271 JMP 028E */
    } else {
        STEP(); c->ax = bp16(ram, c, 6); c->ip = 0x0276u;                                            /* 0273 MOV AX,[BP+6] */
        STEP(); c->di = rd16(ram, c->ds, 0x5D92u); c->es = rd16(ram, c->ds, 0x5D94u); c->ip = 0x027Au;   /* 0276 LES DI,[5D92] */
        STEP(); acc_add(bus, c); c->ip = 0x027Fu;   /* 027A ADD ES:[DI+90],AX (through the bus) */
        STEP(); push(ram, c, rd16(ram, c->ds, 0x5D94u)); c->ip = 0x0283u;                            /* 027F PUSH [5D94] */
        STEP(); push(ram, c, rd16(ram, c->ds, 0x5D92u)); c->ip = 0x0287u;                            /* 0283 PUSH [5D92] */
        STEP(); push(ram, c, bp16(ram, c, 6)); c->ip = 0x028Au;                                      /* 0287 PUSH [BP+6] */
        STEP(); push(ram, c, c->cs); c->ip = 0x028Bu;                                                /* 028A PUSH CS */
        if (*budget == 0u) return PR_U7F_0203_LIMIT;                                                 /* 028B CALL 003A */
        d->site = 0x028Bu;
        if (rd8(ram, cs, 0x028Bu) != 0xE8u || rd16(ram, cs, 0x028Cu) != 0xFDACu) return PR_U7F_0203_CODE;
        --*budget; d->callee = 4; push(ram, c, 0x028Eu); c->ip = 0x003Au;
        {
            pr_u7f_003a_cpu x; pr_u7f_003a_status s; COPY(&x, c); s = pr_u7f_record_word_add_003a(ram, ram_size, &x); COPY(c, &x);
            if (s != PR_U7F_003A_OK) { d->callee_status = (int)s; return PR_U7F_0203_CALLEE; }
            if (c->cs != cs || c->ip != 0x028Eu) return PR_U7F_0203_TRANSFER;
        }
    }
    STEP(); c->sp = c->bp; c->ip = 0x0290u;                                                          /* 028E MOV SP,BP */
    STEP(); c->bp = pop(ram, c); c->ip = 0x0291u;                                                    /* 0290 POP BP */
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 2u);                /* 0291 RETF 2 */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U7F_0203_TRANSFER;
    return PR_U7F_0203_RETURNED;
}
