/* GAME.OVR unit 0x25 +19DB (entry 19): the party step, full CPU state (docs/re/OVL_0025_19DB_CPU.md). */
#include "poolrad/recovered/overlay_u25_move_19db_cpu.h"
#include "poolrad/recovered/overlay_u3bd_cell_057a_cpu.h"
#include "poolrad/recovered/overlay_u3bd_lookup_fullstate.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define ARITH 0x08D5u
typedef pr_u25_19db_cpu cpu_t;

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
/* CMP r/m8,imm8: the six arithmetic flags */
static void cmp8(cpu_t *c, uint8_t a, uint8_t b)
{
    uint8_t r = (uint8_t)(a - b); uint16_t f = parity(r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x80u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
/* INC/DEC r/m8: CF kept; AF, OF and SZP of the result */
static uint8_t incdec8(cpu_t *c, uint8_t v, int dec)
{
    uint8_t r = (uint8_t)(dec ? v - 1u : v + 1u); uint16_t f = parity(r);
    if (dec ? (v & 0x0Fu) == 0u : (v & 0x0Fu) == 0x0Fu) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if (dec ? v == 0x80u : r == 0x80u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D4u) | f); return r;
}
static int jle(const cpu_t *c) { return (c->flags & 0x0040u) || (!(c->flags & 0x0080u) != !(c->flags & 0x0800u)); }
static int jge(const cpu_t *c) { return !(c->flags & 0x0080u) == !(c->flags & 0x0800u); }
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)   /* the same 14-word layout */

#define STEP(ip_) do { if (*budget == 0u) { c->ip = (ip_); return PR_U25_19DB_LIMIT; } --*budget; } while (0)
/* LCALL stub:slot at site_ (9Ah, offset word site_+1, relocated segment word site_+3); the slot must be the loaded JMP FAR (EAh) to unit 3BDh's entry */
#define FARCALL(site_, next_, slot_, entry_) \
    if (*budget == 0u) { c->ip = (site_); return PR_U25_19DB_LIMIT; } \
    stub = rd16(ram, cs, (uint16_t)((site_) + 3u)); \
    if (rd16(ram, cs, (uint16_t)((site_) + 1u)) != (slot_) || rd8(ram, stub, (slot_)) != 0xEAu || rd16(ram, stub, (uint16_t)((slot_) + 1u)) != (entry_)) \
        { c->ip = (site_); return PR_U25_19DB_STUB; } \
    --*budget; push(ram, c, cs); push(ram, c, (next_)); c->cs = rd16(ram, stub, (uint16_t)((slot_) + 3u)); c->ip = (entry_)

pr_u25_19db_status pr_u25_move_19db_cpu(pr_bus *bus, size_t ram_size, pr_u25_19db_cpu *c, size_t *budget)
{
    uint8_t *ram; uint16_t cs, stub, ret_ip, ret_cs; uint8_t al;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x19DBu || c->sp < 0x0030u || c->sp > 0xFFFCu)
        return PR_U25_19DB_ARGUMENT;
    ram = bus->ram; cs = c->cs; ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    STEP(0x19DBu); push(ram, c, c->bp);                                                                  /* 19DB PUSH BP */
    STEP(0x19DCu); c->bp = c->sp;                                                                        /* 19DC MOV BP,SP */
    STEP(0x19DEu); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ds, 0x6AAFu));                     /* 19DE MOV AL,[6AAF] */
    STEP(0x19E1u); cmp8(c, (uint8_t)c->ax, 0u);                                                          /* 19E1 CMP AL,0 */
    STEP(0x19E3u);                                                                                       /* 19E3 JNE 19F9 */
    if (c->flags & 0x0040u) {
        STEP(0x19E5u); cmp8(c, rd8(ram, c->ds, 0x6AAEu), 0u);                                            /* 19E5 CMP BYTE [6AAE],0 */
        STEP(0x19EAu);                                                                                   /* 19EA JLE 19F2 */
        if (!jle(c)) { STEP(0x19ECu); wr8(ram, c->ds, 0x6AAEu, incdec8(c, rd8(ram, c->ds, 0x6AAEu), 1));  /* 19EC DEC BYTE [6AAE] */
                       STEP(0x19F0u); }                                                                  /* 19F0 JMP 19F7 */
        else { STEP(0x19F2u); wr8(ram, c->ds, 0x6AAEu, 0x0Fu); }                                         /* 19F2 MOV BYTE [6AAE],0F */
        STEP(0x19F7u);                                                                                   /* 19F7 JMP 1A3F */
        goto L_1A3F;
    }
    STEP(0x19F9u); cmp8(c, (uint8_t)c->ax, 2u);                                                          /* 19F9 CMP AL,2 */
    STEP(0x19FBu);                                                                                       /* 19FB JNE 1A11 */
    if (c->flags & 0x0040u) {
        STEP(0x19FDu); cmp8(c, rd8(ram, c->ds, 0x6AADu), 0x0Fu);                                         /* 19FD CMP BYTE [6AAD],0F */
        STEP(0x1A02u);                                                                                   /* 1A02 JGE 1A0A */
        if (!jge(c)) { STEP(0x1A04u); wr8(ram, c->ds, 0x6AADu, incdec8(c, rd8(ram, c->ds, 0x6AADu), 0));  /* 1A04 INC BYTE [6AAD] */
                       STEP(0x1A08u); }                                                                  /* 1A08 JMP 1A0F */
        else { STEP(0x1A0Au); wr8(ram, c->ds, 0x6AADu, 0x00u); }                                         /* 1A0A MOV BYTE [6AAD],0 */
        STEP(0x1A0Fu);                                                                                   /* 1A0F JMP 1A3F */
        goto L_1A3F;
    }
    STEP(0x1A11u); cmp8(c, (uint8_t)c->ax, 4u);                                                          /* 1A11 CMP AL,4 */
    STEP(0x1A13u);                                                                                       /* 1A13 JNE 1A29 */
    if (c->flags & 0x0040u) {
        STEP(0x1A15u); cmp8(c, rd8(ram, c->ds, 0x6AAEu), 0x0Fu);                                         /* 1A15 CMP BYTE [6AAE],0F */
        STEP(0x1A1Au);                                                                                   /* 1A1A JGE 1A22 */
        if (!jge(c)) { STEP(0x1A1Cu); wr8(ram, c->ds, 0x6AAEu, incdec8(c, rd8(ram, c->ds, 0x6AAEu), 0));  /* 1A1C INC BYTE [6AAE] */
                       STEP(0x1A20u); }                                                                  /* 1A20 JMP 1A27 */
        else { STEP(0x1A22u); wr8(ram, c->ds, 0x6AAEu, 0x00u); }                                         /* 1A22 MOV BYTE [6AAE],0 */
        STEP(0x1A27u);                                                                                   /* 1A27 JMP 1A3F */
        goto L_1A3F;
    }
    STEP(0x1A29u); cmp8(c, (uint8_t)c->ax, 6u);                                                          /* 1A29 CMP AL,6 */
    STEP(0x1A2Bu);                                                                                       /* 1A2B JNE 1A3F */
    if (c->flags & 0x0040u) {
        STEP(0x1A2Du); cmp8(c, rd8(ram, c->ds, 0x6AADu), 0u);                                            /* 1A2D CMP BYTE [6AAD],0 */
        STEP(0x1A32u);                                                                                   /* 1A32 JLE 1A3A */
        if (!jle(c)) { STEP(0x1A34u); wr8(ram, c->ds, 0x6AADu, incdec8(c, rd8(ram, c->ds, 0x6AADu), 1));  /* 1A34 DEC BYTE [6AAD] */
                       STEP(0x1A38u); }                                                                  /* 1A38 JMP 1A3F */
        else { STEP(0x1A3Au); wr8(ram, c->ds, 0x6AADu, 0x0Fu); }                                         /* 1A3A MOV BYTE [6AAD],0F */
    }
L_1A3F:
    STEP(0x1A3Fu); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ds, 0x6AADu));                     /* 1A3F MOV AL,[6AAD] */
    STEP(0x1A42u); push(ram, c, c->ax);                                                                  /* 1A42 PUSH AX */
    STEP(0x1A43u); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ds, 0x6AAEu));                     /* 1A43 MOV AL,[6AAE] */
    STEP(0x1A46u); push(ram, c, c->ax);                                                                  /* 1A46 PUSH AX */
    FARCALL(0x1A47u, 0x1A4Cu, 0x0034u, 0x057Au);                                                         /* 1A47 LCALL stub:0034 (3BD +057A) */
    {
        pr_u3bd_057a_cpu x; pr_u3bd_057a_status s;
        COPY(&x, c); s = pr_u3bd_cell_057a_cpu(bus, ram_size, &x); COPY(c, &x);
        if (s == PR_U3BD_057A_ARGUMENT) return PR_U25_19DB_CALLEE;
        if (s != PR_U3BD_057A_RETURNED || c->cs != cs || c->ip != 0x1A4Cu) return PR_U25_19DB_TRANSFER;
    }
    STEP(0x1A4Cu); wr8(ram, c->ds, 0x6AB1u, (uint8_t)c->ax);                                             /* 1A4C MOV [6AB1],AL */
    STEP(0x1A4Fu); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ds, 0x6AADu));                     /* 1A4F MOV AL,[6AAD] */
    STEP(0x1A52u); push(ram, c, c->ax);                                                                  /* 1A52 PUSH AX */
    STEP(0x1A53u); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ds, 0x6AAEu));                     /* 1A53 MOV AL,[6AAE] */
    STEP(0x1A56u); push(ram, c, c->ax);                                                                  /* 1A56 PUSH AX */
    STEP(0x1A57u); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ds, 0x6AAFu));                     /* 1A57 MOV AL,[6AAF] */
    STEP(0x1A5Au); push(ram, c, c->ax);                                                                  /* 1A5A PUSH AX */
    FARCALL(0x1A5Bu, 0x1A60u, 0x002Au, 0x047Bu);                                                         /* 1A5B LCALL stub:002A (3BD +047B) */
    {
        pr_u3bd_lookup_cpu x; pr_u3bd_lookup_status s;
        COPY(&x, c); s = pr_u3bd_nibble_047b_fullstate(ram, ram_size, &x); COPY(c, &x);
        if (s == PR_U3BD_LOOKUP_ARGUMENT) return PR_U25_19DB_CALLEE;
        if (s != PR_U3BD_LOOKUP_OK || c->cs != cs || c->ip != 0x1A60u) return PR_U25_19DB_TRANSFER;
    }
    al = (uint8_t)c->ax;
    STEP(0x1A60u); wr8(ram, c->ds, 0x6AB0u, al);                                                         /* 1A60 MOV [6AB0],AL */
    STEP(0x1A63u); c->bp = pop(ram, c);                                                                  /* 1A63 POP BP */
    STEP(0x1A64u); c->ip = pop(ram, c); c->cs = pop(ram, c);                                             /* 1A64 RETF */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U25_19DB_TRANSFER;
    return PR_U25_19DB_RETURNED;
}
