/* Original unit 0x7F +044C: the capped transfer between two records with a message (docs/re/OVL_007F_044C_CPU.md). */
#include "poolrad/recovered/overlay_u7f_044c_cpu.h"
#include "poolrad/recovered/overlay_u7f_record_subtract_0020.h"
#include "poolrad/recovered/overlay_u7f_record_word_add_003a.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
typedef pr_u7f_044c_regs cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
/* Accesses through the caller-supplied pointers (PP, QQ and what they point at) go through the bus. */
static uint16_t brd16(pr_bus *bus, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(pr_bus_read8(bus, p) | ((uint16_t)pr_bus_read8(bus, (p + 1u) & MASK) << 8u)); }
static void bwr16(pr_bus *bus, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); pr_bus_write8(bus, p, (uint8_t)v); pr_bus_write8(bus, (p + 1u) & MASK, (uint8_t)(v >> 8u)); }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t bp16(const uint8_t *ram, const cpu_t *c, int off) { return rd16(ram, c->ss, (uint16_t)(c->bp + off)); }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 4u; }
static uint16_t arith16(cpu_t *c, uint16_t a, uint16_t b, int subtract)
{
    uint32_t wide = subtract ? (uint32_t)a - b : (uint32_t)a + b;
    uint16_t r = (uint16_t)wide, f = parity((uint8_t)r);
    if (subtract ? a < b : wide > 0xFFFFu) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((subtract ? ((a ^ b) & (a ^ r)) : (~(a ^ b) & (a ^ r))) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
static uint16_t shl1(cpu_t *c, uint16_t a)                                              /* SHL AX,1: CF = old bit 15, OF = CF xor the new bit 15, SF/ZF/PF of the result, AF 1 (pinned; raw Unicorn leaves it clear: a declared splits it) */
{
    uint16_t r = (uint16_t)(a << 1u), f = (uint16_t)(parity((uint8_t)r) | 0x0010u);   /* the PINNED rule: AF is set after a non-zero shift (get_AF = count & 1Fh) */
    if (a & 0x8000u) f |= 0x0001u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if (((a >> 15u) ^ (r >> 15u)) & 1u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static void cbw(cpu_t *c) { c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; }
static void les_bp(const uint8_t *ram, cpu_t *c, int off) { c->di = bp16(ram, c, off); c->es = bp16(ram, c, off + 2); }                /* LES DI,[BP+off] */
static void les_es_di(pr_bus *bus, cpu_t *c)                                            /* LES DI,ES:[DI] through the bus; the pair is read physically contiguous */
{
    size_t at = physical(c->es, c->di);
    uint16_t off = (uint16_t)(pr_bus_read8(bus, at) | ((uint16_t)pr_bus_read8(bus, (at + 1u) & MASK) << 8u));
    uint16_t seg = (uint16_t)(pr_bus_read8(bus, (at + 2u) & MASK) | ((uint16_t)pr_bus_read8(bus, (at + 3u) & MASK) << 8u));
    c->di = off; c->es = seg;
}
#define STEP() do { if (*budget == 0u) return PR_U7F_044C_LIMIT; --*budget; } while (0)
#define AT(next_ip) (c->ip = (next_ip))

pr_u7f_044c_status pr_u7f_044c_cpu(pr_bus *bus, size_t ram_size, pr_u7f_044c_regs *c, size_t *budget, const pr_u7f_044c_limits *limits, pr_u7f_044c_diagnostic *diagnostic)
{
    pr_u7f_044c_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint8_t *ram; uint16_t cs, ret_ip, ret_cs, dx; int above;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || limits == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x044Cu || c->sp < 0x0200u || c->sp > 0xFFEEu)
        return PR_U7F_044C_ARGUMENT;
    ram = bus->ram; cs = c->cs; memset(d, 0, sizeof *d); d->callee = -1; d->callee_status = -1;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    STEP(); push(ram, c, c->bp); AT(0x044Du);                                                         /* 044C PUSH BP */
    STEP(); c->bp = c->sp; AT(0x044Fu);                                                               /* 044D MOV BP,SP */
    STEP(); c->sp = arith16(c, c->sp, 0x000Cu, 1); AT(0x0452u);                                       /* 044F SUB SP,0Ch */
    STEP(); les_bp(ram, c, 0x0A); AT(0x0455u);                                                        /* 0452 LES DI,[BP+0A] */
    STEP(); push(ram, c, brd16(bus, c->es, (uint16_t)(c->di + 2u))); AT(0x0459u);                     /* 0455 PUSH ES:[DI+2] (through the bus) */
    STEP(); push(ram, c, brd16(bus, c->es, c->di)); AT(0x045Cu);                                      /* 0459 PUSH ES:[DI] */
    STEP(); push(ram, c, c->cs); AT(0x045Du);                                                         /* 045C PUSH CS */
    if (*budget == 0u) return PR_U7F_044C_LIMIT;                                                      /* 045D CALL 0000 */
    d->site = 0x045Du;
    if (rd8(ram, cs, 0x045Du) != 0xE8u || rd16(ram, cs, 0x045Eu) != 0xFBA0u) return PR_U7F_044C_CODE;
    --*budget; push(ram, c, 0x0460u); AT(0x0000u); d->callee = 0;
    {
        pr_u7f_0000_status s = pr_u7f_entry0_0000_cpu(ram, ram_size, c, budget, &d->d0000);
        if (s == PR_U7F_0000_LIMIT) return PR_U7F_044C_LIMIT;
        if (s != PR_U7F_0000_RETURNED) { d->callee_status = (int)s; return PR_U7F_044C_CALLEE; }
        if (c->cs != cs || c->ip != 0x0460u) return PR_U7F_044C_TRANSFER;
    }
    STEP(); c->dx = c->ax; dx = c->dx; AT(0x0462u);                                                   /* 0460 MOV DX,AX */
    STEP(); les_bp(ram, c, 0x0A); AT(0x0465u);                                                        /* 0462 LES DI,[BP+0A] */
    STEP(); les_es_di(bus, c); AT(0x0468u);                                                           /* 0465 LES DI,ES:[DI] (through the bus) */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0102u)); AT(0x046Du);                      /* 0468 MOV AX,ES:[DI+0102] */
    STEP(); c->ax = arith16(c, c->ax, bp16(ram, c, 8), 0); AT(0x0470u);                               /* 046D ADD AX,[BP+8] */
    STEP(); (void)arith16(c, c->ax, dx, 1); AT(0x0472u);                                              /* 0470 CMP AX,DX */
    STEP(); above = (c->flags & 0x0041u) == 0u; AT(above ? 0x04EEu : 0x0474u);                        /* 0472 JA 04EE */
    if (!above) {
        STEP(); set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 6u))); AT(0x0477u);                      /* 0474 MOV AL,[BP+6] */
        STEP(); cbw(c); AT(0x0478u);                                                                  /* 0477 CBW */
        STEP(); c->ax = shl1(c, c->ax); AT(0x047Au);                                                  /* 0478 SHL AX,1 */
        STEP(); les_bp(ram, c, 0x0E); AT(0x047Du);                                                    /* 047A LES DI,[BP+0E] */
        STEP(); les_es_di(bus, c); AT(0x0480u);                                                       /* 047D LES DI,ES:[DI] */
        STEP(); c->di = arith16(c, c->di, c->ax, 0); AT(0x0482u);                                     /* 0480 ADD DI,AX */
        STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x88u)); AT(0x0487u);                    /* 0482 MOV AX,ES:[DI+88] */
        STEP(); c->ax = arith16(c, c->ax, bp16(ram, c, 8), 1); AT(0x048Au);                           /* 0487 SUB AX,[BP+8] */
        STEP(); c->dx = c->ax; AT(0x048Cu);                                                           /* 048A MOV DX,AX */
        STEP(); set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 6u))); AT(0x048Fu);                      /* 048C MOV AL,[BP+6] */
        STEP(); cbw(c); AT(0x0490u);                                                                  /* 048F CBW */
        STEP(); c->ax = shl1(c, c->ax); AT(0x0492u);                                                  /* 0490 SHL AX,1 */
        STEP(); les_bp(ram, c, 0x0E); AT(0x0495u);                                                    /* 0492 LES DI,[BP+0E] */
        STEP(); les_es_di(bus, c); AT(0x0498u);                                                       /* 0495 LES DI,ES:[DI] */
        STEP(); c->di = arith16(c, c->di, c->ax, 0); AT(0x049Au);                                     /* 0498 ADD DI,AX */
        STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x88u), c->dx); AT(0x049Fu);                     /* 049A MOV ES:[DI+88],DX (through the bus) */
        STEP(); les_bp(ram, c, 0x0E); AT(0x04A2u);                                                    /* 049F LES DI,[BP+0E] */
        STEP(); push(ram, c, brd16(bus, c->es, (uint16_t)(c->di + 2u))); AT(0x04A6u);                 /* 04A2 PUSH ES:[DI+2] */
        STEP(); push(ram, c, brd16(bus, c->es, c->di)); AT(0x04A9u);                                  /* 04A6 PUSH ES:[DI] */
        STEP(); push(ram, c, bp16(ram, c, 8)); AT(0x04ACu);                                           /* 04A9 PUSH [BP+8] */
        STEP(); push(ram, c, c->cs); AT(0x04ADu);                                                     /* 04AC PUSH CS */
        if (*budget == 0u) return PR_U7F_044C_LIMIT;                                                  /* 04AD CALL 0020 */
        d->site = 0x04ADu;
        if (rd8(ram, cs, 0x04ADu) != 0xE8u || rd16(ram, cs, 0x04AEu) != 0xFB70u) return PR_U7F_044C_CODE;
        --*budget; push(ram, c, 0x04B0u); AT(0x0020u); d->callee = 1;
        {
            pr_u7f_0020_cpu x; pr_u7f_0020_status s; COPY(&x, c); s = pr_u7f_record_subtract_0020(ram, ram_size, &x); COPY(c, &x);
            if (s != PR_U7F_0020_OK) { d->callee_status = (int)s; return PR_U7F_044C_CALLEE; }
            if (c->cs != cs || c->ip != 0x04B0u) return PR_U7F_044C_TRANSFER;
        }
        STEP(); set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 6u))); AT(0x04B3u);                      /* 04B0 MOV AL,[BP+6] */
        STEP(); cbw(c); AT(0x04B4u);                                                                  /* 04B3 CBW */
        STEP(); c->ax = shl1(c, c->ax); AT(0x04B6u);                                                  /* 04B4 SHL AX,1 */
        STEP(); les_bp(ram, c, 0x0A); AT(0x04B9u);                                                    /* 04B6 LES DI,[BP+0A] */
        STEP(); les_es_di(bus, c); AT(0x04BCu);                                                       /* 04B9 LES DI,ES:[DI] */
        STEP(); c->di = arith16(c, c->di, c->ax, 0); AT(0x04BEu);                                     /* 04BC ADD DI,AX */
        STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x88u)); AT(0x04C3u);                    /* 04BE MOV AX,ES:[DI+88] */
        STEP(); c->ax = arith16(c, c->ax, bp16(ram, c, 8), 0); AT(0x04C6u);                           /* 04C3 ADD AX,[BP+8] */
        STEP(); c->dx = c->ax; AT(0x04C8u);                                                           /* 04C6 MOV DX,AX */
        STEP(); set_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 6u))); AT(0x04CBu);                      /* 04C8 MOV AL,[BP+6] */
        STEP(); cbw(c); AT(0x04CCu);                                                                  /* 04CB CBW */
        STEP(); c->ax = shl1(c, c->ax); AT(0x04CEu);                                                  /* 04CC SHL AX,1 */
        STEP(); les_bp(ram, c, 0x0A); AT(0x04D1u);                                                    /* 04CE LES DI,[BP+0A] */
        STEP(); les_es_di(bus, c); AT(0x04D4u);                                                       /* 04D1 LES DI,ES:[DI] */
        STEP(); c->di = arith16(c, c->di, c->ax, 0); AT(0x04D6u);                                     /* 04D4 ADD DI,AX */
        STEP(); bwr16(bus, c->es, (uint16_t)(c->di + 0x88u), c->dx); AT(0x04DBu);                     /* 04D6 MOV ES:[DI+88],DX (through the bus) */
        STEP(); les_bp(ram, c, 0x0E); AT(0x04DEu);                                                    /* 04DB LES DI,[BP+0E] */
        STEP(); push(ram, c, brd16(bus, c->es, (uint16_t)(c->di + 2u))); AT(0x04E2u);                 /* 04DE PUSH ES:[DI+2] */
        STEP(); push(ram, c, brd16(bus, c->es, c->di)); AT(0x04E5u);                                  /* 04E2 PUSH ES:[DI] */
        STEP(); push(ram, c, bp16(ram, c, 8)); AT(0x04E8u);                                           /* 04E5 PUSH [BP+8] */
        STEP(); push(ram, c, c->cs); AT(0x04E9u);                                                     /* 04E8 PUSH CS */
        if (*budget == 0u) return PR_U7F_044C_LIMIT;                                                  /* 04E9 CALL 003A */
        d->site = 0x04E9u;
        if (rd8(ram, cs, 0x04E9u) != 0xE8u || rd16(ram, cs, 0x04EAu) != 0xFB4Eu) return PR_U7F_044C_CODE;
        --*budget; push(ram, c, 0x04ECu); AT(0x003Au); d->callee = 2;
        {
            pr_u7f_003a_cpu x; pr_u7f_003a_status s; COPY(&x, c); s = pr_u7f_record_word_add_003a(ram, ram_size, &x); COPY(c, &x);
            if (s != PR_U7F_003A_OK) { d->callee_status = (int)s; return PR_U7F_044C_CALLEE; }
            if (c->cs != cs || c->ip != 0x04ECu) return PR_U7F_044C_TRANSFER;
        }
        STEP(); AT(0x0502u);                                                                          /* 04EC JMP 0502 */
    } else {
        STEP(); c->di = (uint16_t)(c->bp - 0x0Bu); AT(0x04F1u);                                       /* 04EE LEA DI,[BP-0Bh] */
        STEP(); push(ram, c, c->ss); AT(0x04F2u);                                                     /* 04F1 PUSH SS */
        STEP(); push(ram, c, c->di); AT(0x04F3u);                                                     /* 04F2 PUSH DI */
        STEP(); c->di = 0x0441u; AT(0x04F6u);                                                         /* 04F3 MOV DI,0441 */
        STEP(); push(ram, c, c->cs); AT(0x04F7u);                                                     /* 04F6 PUSH CS */
        STEP(); push(ram, c, c->di); AT(0x04F8u);                                                     /* 04F7 PUSH DI */
        if (*budget == 0u) return PR_U7F_044C_LIMIT;                                                  /* 04F8 LCALL 0AF8:0B0B (segment word at 04FB relocated) */
        d->site = 0x04F8u;
        if (rd8(ram, cs, 0x04F8u) != 0x9Au || rd16(ram, cs, 0x04F9u) != 0x0B0Bu) return PR_U7F_044C_CODE;
        --*budget; d->callee = 3;
        {
            pr_system_leaf_cpu x; pr_system_leaf_status s; uint16_t seg = rd16(ram, cs, 0x04FBu);
            push(ram, c, cs); push(ram, c, 0x04FDu); c->cs = seg; AT(0x0B0Bu);
            COPY(&x, c); s = pr_system_strload_0b0b(ram, ram_size, &x); COPY(c, &x);
            if (s != PR_SYSTEM_LEAF_RETURNED) { d->callee_status = (int)s; return PR_U7F_044C_CALLEE; }
            if (c->cs != cs || c->ip != 0x04FDu) return PR_U7F_044C_TRANSFER;
        }
        if (*budget == 0u) return PR_U7F_044C_LIMIT;                                                  /* 04FD LCALL 00BA:1693 (segment word at 0500 relocated) */
        d->site = 0x04FDu;
        if (rd8(ram, cs, 0x04FDu) != 0x9Au || rd16(ram, cs, 0x04FEu) != 0x1693u) return PR_U7F_044C_CODE;
        --*budget; d->callee = 4;
        {
            pr_r00ba_1693_cpu x; pr_r00ba_1693_status s; uint16_t seg = rd16(ram, cs, 0x0500u);
            push(ram, c, cs); push(ram, c, 0x0502u); c->cs = seg; AT(0x1693u);
            COPY(&x, c); s = pr_r00ba_display_1693_cpu(bus, ram_size, &x, limits->clear, limits->rect, limits->printer, limits->glyph, limits->delay, &d->d1693); COPY(c, &x);
            if (s != PR_R00BA_1693_OK) { d->callee_status = (int)s; return s == PR_R00BA_1693_TRANSFER ? PR_U7F_044C_TRANSFER : PR_U7F_044C_CALLEE; }
            if (c->cs != cs || c->ip != 0x0502u) return PR_U7F_044C_TRANSFER;
        }
    }
    STEP(); c->sp = c->bp; AT(0x0504u);                                                               /* 0502 MOV SP,BP */
    STEP(); c->bp = pop(ram, c); AT(0x0505u);                                                         /* 0504 POP BP */
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 0x0Cu);              /* 0505 RETF 000C */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U7F_044C_TRANSFER;
    return PR_U7F_044C_RETURNED;
}
