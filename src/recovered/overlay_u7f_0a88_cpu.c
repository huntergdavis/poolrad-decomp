/* Original unit 0x7F +0A88: take coins of one kind from the pool into the active character (docs/re/OVL_007F_0A88_CPU.md). */
#include "poolrad/recovered/overlay_u7f_0a88_cpu.h"
#include "poolrad/recovered/overlay_u7f_record_word_add_003a.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_u7f_0a88_regs cpu_t;

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
static uint16_t szp(uint16_t r) { return (uint16_t)(parity((uint8_t)r) | (r ? 0u : 0x0040u) | ((r & 0x8000u) ? 0x0080u : 0u)); }
/* ADD/SUB/SBB/CMP at 16 bits with an incoming borrow for SBB: every arithmetic flag. */
static uint16_t arith16(cpu_t *c, uint16_t a, uint16_t b, int subtract, unsigned carry)
{
    uint32_t wide = subtract ? (uint32_t)a - b - carry : (uint32_t)a + b + carry;
    uint16_t r = (uint16_t)wide, f = szp(r);
    if (subtract ? (uint32_t)a < (uint32_t)b + carry : wide > 0xFFFFu) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if ((subtract ? ((a ^ b) & (a ^ r)) : (~(a ^ b) & (a ^ r))) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
static void logic(cpu_t *c, uint16_t r, unsigned bits)                                  /* OR AL,AL / XOR DX,DX: CF, OF and AF clear; PF, ZF, SF from the result */
{ c->flags = (uint16_t)((c->flags & ~0x08D5u) | parity((uint8_t)r) | (r ? 0u : 0x0040u) | ((r & (bits == 8u ? 0x80u : 0x8000u)) ? 0x0080u : 0u)); }
/* SHL r16,1: CF the bit shifted out, OF = MSB(result) ^ CF, AF set as the pinned core does (get_AF = count & 1Fh). */
static uint16_t shl1(cpu_t *c, uint16_t v)
{
    uint16_t r = (uint16_t)(v << 1u), f = (uint16_t)(szp(r) | 0x0010u);
    if (v & 0x8000u) f |= 0x0001u;
    if ((v ^ r) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
static void cbw(cpu_t *c) { c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; }
static void load_index(const uint8_t *ram, cpu_t *c) { c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ss, (uint16_t)(c->bp + 6))); }   /* MOV AL,[BP+6] */
static uint16_t lea_bp(const cpu_t *c, int off) { return (uint16_t)(c->bp + off); }

#define STEP() do { if (*budget == 0u) return PR_U7F_0A88_LIMIT; --*budget; } while (0)
/* MOV AL,[BP+6]; CBW; MOV DI,AX; SHL DI,1; SHL DI,1 at `at` (four instructions of 3, 1, 2, 2 and 2 bytes): DI = 4I. */
#define INDEX4(at) do { \
    STEP(); load_index(ram, c); c->ip = (uint16_t)((at) + 3u); \
    STEP(); cbw(c); c->ip = (uint16_t)((at) + 4u); \
    STEP(); c->di = c->ax; c->ip = (uint16_t)((at) + 6u); \
    STEP(); c->di = shl1(c, c->di); c->ip = (uint16_t)((at) + 8u); \
    STEP(); c->di = shl1(c, c->di); c->ip = (uint16_t)((at) + 10u); } while (0)
/* MOV AL,[BP+6]; CBW; SHL AX,1; LES DI,[BP+0Ah]; ADD DI,AX at `at` (3, 1, 2, 3 and 2 bytes): ES:DI = the record plus 2I. */
#define RECORD2(at) do { \
    STEP(); load_index(ram, c); c->ip = (uint16_t)((at) + 3u); \
    STEP(); cbw(c); c->ip = (uint16_t)((at) + 4u); \
    STEP(); c->ax = shl1(c, c->ax); c->ip = (uint16_t)((at) + 6u); \
    STEP(); c->di = bp16(ram, c, 0x0A); c->es = bp16(ram, c, 0x0C); c->ip = (uint16_t)((at) + 9u); \
    STEP(); c->di = arith16(c, c->di, c->ax, 0, 0); c->ip = (uint16_t)((at) + 11u); } while (0)

pr_u7f_0a88_status pr_u7f_0a88_cpu(pr_bus *bus, size_t ram_size, pr_u7f_0a88_regs *c, size_t *budget, const pr_u7f_0a88_limits *limits, pr_u7f_0a88_diagnostic *diagnostic)
{
    pr_u7f_0a88_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint8_t *ram; uint16_t cs, ret_ip, ret_cs;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || limits == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0A88u || c->sp < 0x0200u || c->sp > 0xFFF4u)
        return PR_U7F_0A88_ARGUMENT;
    ram = bus->ram; cs = c->cs; memset(d, 0, sizeof *d); d->callee = -1; d->callee_status = -1;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    STEP(); push(ram, c, c->bp); c->ip = 0x0A89u;                                                    /* 0A88 PUSH BP */
    STEP(); c->bp = c->sp; c->ip = 0x0A8Bu;                                                          /* 0A89 MOV BP,SP */
    STEP(); c->sp = arith16(c, c->sp, 0x000Eu, 1, 0); c->ip = 0x0A8Eu;                               /* 0A8B SUB SP,0Eh */
    STEP(); push(ram, c, bp16(ram, c, 0x0C)); c->ip = 0x0A91u;                                       /* 0A8E PUSH [BP+0Ch] */
    STEP(); push(ram, c, bp16(ram, c, 0x0A)); c->ip = 0x0A94u;                                       /* 0A91 PUSH [BP+0Ah] */
    STEP(); push(ram, c, bp16(ram, c, 8)); c->ip = 0x0A97u;                                          /* 0A94 PUSH [BP+8] */
    STEP(); c->di = lea_bp(c, -2); c->ip = 0x0A9Au;                                                  /* 0A97 LEA DI,[BP-2] */
    STEP(); push(ram, c, c->ss); c->ip = 0x0A9Bu;                                                    /* 0A9A PUSH SS */
    STEP(); push(ram, c, c->di); c->ip = 0x0A9Cu;                                                    /* 0A9B PUSH DI */
    STEP(); push(ram, c, c->cs); c->ip = 0x0A9Du;                                                    /* 0A9C PUSH CS */
    if (*budget == 0u) return PR_U7F_0A88_LIMIT;                                                     /* 0A9D CALL 0054 */
    d->site = 0x0A9Du;
    if (rd8(ram, cs, 0x0A9Du) != 0xE8u || rd16(ram, cs, 0x0A9Eu) != 0xF5B4u) return PR_U7F_0A88_CODE;
    --*budget; push(ram, c, 0x0AA0u); c->ip = 0x0054u; d->callee = 0;
    {
        pr_u7f_0054_status s = pr_u7f_0054_cpu(bus, ram_size, c, budget, &d->d0054);
        if (s == PR_U7F_0054_LIMIT) return PR_U7F_0A88_LIMIT;
        if (s != PR_U7F_0054_RETURNED) { d->callee_status = (int)s; return s == PR_U7F_0054_TRANSFER ? PR_U7F_0A88_TRANSFER : PR_U7F_0A88_CALLEE; }
        if (c->cs != cs || c->ip != 0x0AA0u) return PR_U7F_0A88_TRANSFER;
    }
    STEP(); logic(c, (uint8_t)c->ax, 8u); c->ip = 0x0AA2u;                                           /* 0AA0 OR AL,AL */
    STEP(); c->ip = (c->flags & 0x0040u) ? 0x0ABBu : 0x0AA4u;                                        /* 0AA2 JE 0ABB */
    if (c->ip == 0x0AA4u) {
        STEP(); c->di = lea_bp(c, -0x0D); c->ip = 0x0AA7u;                                           /* 0AA4 LEA DI,[BP-0Dh] */
        STEP(); push(ram, c, c->ss); c->ip = 0x0AA8u;                                                /* 0AA7 PUSH SS */
        STEP(); push(ram, c, c->di); c->ip = 0x0AA9u;                                                /* 0AA8 PUSH DI */
        STEP(); c->di = 0x0A7Du; c->ip = 0x0AACu;                                                    /* 0AA9 MOV DI,0A7D */
        STEP(); push(ram, c, c->cs); c->ip = 0x0AADu;                                                /* 0AAC PUSH CS */
        STEP(); push(ram, c, c->di); c->ip = 0x0AAEu;                                                /* 0AAD PUSH DI */
        if (*budget == 0u) return PR_U7F_0A88_LIMIT;                                                 /* 0AAE LCALL 0AF8:0B0B (the segment word at 0AB1 is relocated) */
        d->site = 0x0AAEu;
        if (rd8(ram, cs, 0x0AAEu) != 0x9Au || rd16(ram, cs, 0x0AAFu) != 0x0B0Bu) return PR_U7F_0A88_CODE;
        --*budget; d->callee = 1;
        {
            pr_system_leaf_cpu x; pr_system_leaf_status s; uint16_t seg = rd16(ram, cs, 0x0AB1u);
            push(ram, c, cs); push(ram, c, 0x0AB3u); c->cs = seg; c->ip = 0x0B0Bu;
            COPY(&x, c); s = pr_system_strload_0b0b(ram, ram_size, &x); COPY(c, &x);
            if (s != PR_SYSTEM_LEAF_RETURNED) { d->callee_status = (int)s; return PR_U7F_0A88_CALLEE; }
            if (c->cs != cs || c->ip != 0x0AB3u) return PR_U7F_0A88_TRANSFER;
        }
        if (*budget == 0u) return PR_U7F_0A88_LIMIT;                                                 /* 0AB3 LCALL 00BA:1693 (the segment word at 0AB6 is relocated) */
        d->site = 0x0AB3u;
        if (rd8(ram, cs, 0x0AB3u) != 0x9Au || rd16(ram, cs, 0x0AB4u) != 0x1693u) return PR_U7F_0A88_CODE;
        --*budget; d->callee = 2;
        {
            pr_r00ba_1693_cpu x; pr_r00ba_1693_status s; uint16_t seg = rd16(ram, cs, 0x0AB6u);
            push(ram, c, cs); push(ram, c, 0x0AB8u); c->cs = seg; c->ip = 0x1693u;
            COPY(&x, c); s = pr_r00ba_display_1693_cpu(bus, ram_size, &x, limits->clear, limits->rect, limits->printer, limits->glyph, limits->delay, &d->d1693); COPY(c, &x);
            if (s != PR_R00BA_1693_OK) { d->callee_status = (int)s; return s == PR_R00BA_1693_TRANSFER ? PR_U7F_0A88_TRANSFER : PR_U7F_0A88_CALLEE; }
            if (c->cs != cs || c->ip != 0x0AB8u) return PR_U7F_0A88_TRANSFER;
        }
        STEP(); c->ip = 0x0B54u;                                                                     /* 0AB8 JMP 0B54 */
    } else {
        int clamp;
        INDEX4(0x0ABBu);                                                                             /* 0ABB..0AC3 DI = 4I */
        STEP(); c->cx = rd16(ram, c->ds, (uint16_t)(c->di + 0x67F4u)); c->ip = 0x0AC9u;              /* 0AC5 MOV CX,[DI+67F4] */
        STEP(); c->bx = rd16(ram, c->ds, (uint16_t)(c->di + 0x67F6u)); c->ip = 0x0ACDu;              /* 0AC9 MOV BX,[DI+67F6] */
        STEP(); c->ax = bp16(ram, c, 8); c->ip = 0x0AD0u;                                            /* 0ACD MOV AX,[BP+8] */
        STEP(); c->dx = 0u; logic(c, 0u, 16u); c->ip = 0x0AD2u;                                      /* 0AD0 XOR DX,DX */
        STEP(); (void)arith16(c, c->dx, c->bx, 1, 0); c->ip = 0x0AD4u;                               /* 0AD2 CMP DX,BX */
        STEP(); clamp = !(c->flags & 0x0040u) && !(c->flags & 0x0080u) == !(c->flags & 0x0800u); c->ip = clamp ? 0x0ADCu : 0x0AD6u;   /* 0AD4 JG 0ADC */
        if (!clamp) {
            STEP(); c->ip = (!(c->flags & 0x0080u) != !(c->flags & 0x0800u)) ? 0x0AEDu : 0x0AD8u;       /* 0AD6 JL 0AED */
            if (c->ip == 0x0AD8u) {
                STEP(); (void)arith16(c, c->ax, c->cx, 1, 0); c->ip = 0x0ADAu;                        /* 0AD8 CMP AX,CX */
                STEP(); clamp = !(c->flags & 0x0041u); c->ip = clamp ? 0x0ADCu : 0x0AEDu;             /* 0ADA JBE 0AED */
            }
        }
        if (clamp) {
            INDEX4(0x0ADCu);                                                                         /* 0ADC..0AE4 DI = 4I */
            STEP(); c->ax = rd16(ram, c->ds, (uint16_t)(c->di + 0x67F4u)); c->ip = 0x0AEAu;          /* 0AE6 MOV AX,[DI+67F4] */
            STEP(); wr16(ram, c->ss, (uint16_t)(c->bp + 8), c->ax); c->ip = 0x0AEDu;                 /* 0AEA MOV [BP+8],AX */
        }
        STEP(); c->ax = bp16(ram, c, 8); c->ip = 0x0AF0u;                                            /* 0AED MOV AX,[BP+8] */
        STEP(); c->dx = 0u; logic(c, 0u, 16u); c->ip = 0x0AF2u;                                      /* 0AF0 XOR DX,DX */
        STEP(); c->cx = c->ax; c->ip = 0x0AF4u;                                                      /* 0AF2 MOV CX,AX */
        STEP(); c->bx = c->dx; c->ip = 0x0AF6u;                                                      /* 0AF4 MOV BX,DX */
        INDEX4(0x0AF6u);                                                                             /* 0AF6..0AFE DI = 4I */
        STEP(); c->ax = rd16(ram, c->ds, (uint16_t)(c->di + 0x67F4u)); c->ip = 0x0B04u;              /* 0B00 MOV AX,[DI+67F4] */
        STEP(); c->dx = rd16(ram, c->ds, (uint16_t)(c->di + 0x67F6u)); c->ip = 0x0B08u;              /* 0B04 MOV DX,[DI+67F6] */
        STEP(); c->ax = arith16(c, c->ax, c->cx, 1, 0); c->ip = 0x0B0Au;                             /* 0B08 SUB AX,CX */
        STEP(); c->dx = arith16(c, c->dx, c->bx, 1, (c->flags & 1u)); c->ip = 0x0B0Cu;               /* 0B0A SBB DX,BX */
        STEP(); c->cx = c->ax; c->ip = 0x0B0Eu;                                                      /* 0B0C MOV CX,AX */
        STEP(); c->bx = c->dx; c->ip = 0x0B10u;                                                      /* 0B0E MOV BX,DX */
        INDEX4(0x0B10u);                                                                             /* 0B10..0B18 DI = 4I */
        STEP(); wr16(ram, c->ds, (uint16_t)(c->di + 0x67F4u), c->cx); c->ip = 0x0B1Eu;               /* 0B1A MOV [DI+67F4],CX */
        STEP(); wr16(ram, c->ds, (uint16_t)(c->di + 0x67F6u), c->bx); c->ip = 0x0B22u;               /* 0B1E MOV [DI+67F6],BX */
        RECORD2(0x0B22u);                                                                            /* 0B22..0B2B ES:DI = record + 2I */
        STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x88u)); c->ip = 0x0B32u;               /* 0B2D MOV AX,ES:[DI+88h] (through the bus) */
        STEP(); c->ax = arith16(c, c->ax, bp16(ram, c, 8), 0, 0); c->ip = 0x0B35u;                   /* 0B32 ADD AX,[BP+8] */
        STEP(); c->dx = c->ax; c->ip = 0x0B37u;                                                      /* 0B35 MOV DX,AX */
        RECORD2(0x0B37u);                                                                            /* 0B37..0B40 ES:DI = record + 2I */
        STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x88u), c->dx); c->ip = 0x0B47u;                /* 0B42 MOV ES:[DI+88h],DX (through the bus) */
        STEP(); push(ram, c, bp16(ram, c, 0x0C)); c->ip = 0x0B4Au;                                   /* 0B47 PUSH [BP+0Ch] */
        STEP(); push(ram, c, bp16(ram, c, 0x0A)); c->ip = 0x0B4Du;                                   /* 0B4A PUSH [BP+0Ah] */
        STEP(); push(ram, c, bp16(ram, c, 8)); c->ip = 0x0B50u;                                      /* 0B4D PUSH [BP+8] */
        STEP(); push(ram, c, c->cs); c->ip = 0x0B51u;                                                /* 0B50 PUSH CS */
        if (*budget == 0u) return PR_U7F_0A88_LIMIT;                                                 /* 0B51 CALL 003A */
        d->site = 0x0B51u;
        if (rd8(ram, cs, 0x0B51u) != 0xE8u || rd16(ram, cs, 0x0B52u) != 0xF4E6u) return PR_U7F_0A88_CODE;
        --*budget; d->callee = 3; push(ram, c, 0x0B54u); c->ip = 0x003Au;
        {
            pr_u7f_003a_cpu x; pr_u7f_003a_status s; COPY(&x, c); s = pr_u7f_record_word_add_003a(ram, ram_size, &x); COPY(c, &x);
            if (s != PR_U7F_003A_OK) { d->callee_status = (int)s; return PR_U7F_0A88_CALLEE; }
            if (c->cs != cs || c->ip != 0x0B54u) return PR_U7F_0A88_TRANSFER;
        }
    }
    STEP(); c->sp = c->bp; c->ip = 0x0B56u;                                                          /* 0B54 MOV SP,BP */
    STEP(); c->bp = pop(ram, c); c->ip = 0x0B57u;                                                    /* 0B56 POP BP */
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 8u);                /* 0B57 RETF 8 */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U7F_0A88_TRANSFER;
    return PR_U7F_0A88_RETURNED;
}
