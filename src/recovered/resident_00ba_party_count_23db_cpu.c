/* START.EXE resident 00BA +23DB: the party count, full CPU state (docs/re/RES_00BA_23DB_CPU.md). */
#include "poolrad/recovered/resident_00ba_party_count_23db_cpu.h"

#define MASK (PR_RES_2E03_RAM_SIZE - 1u)
#define ARITH 0x08D5u
typedef pr_res_23db_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
static void les(const uint8_t *ram, cpu_t *c, uint16_t seg, uint16_t off)   /* LES as pinned DOSBox-X (prefix_none.h C4h): the offset word at the EA,
                                                                              * the segment word at EA+2-10000h when the offset is above FFFCh, i.e. at
                                                                              * (offset+2) mod 64K; each word itself is read physically contiguous */
{ uint16_t o = rd16(ram, seg, off); c->es = rd16(ram, seg, (uint16_t)(off + 2u)); c->di = o; }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 0x0004u; }
static void set_arith(cpu_t *c, uint16_t f) { c->flags = (uint16_t)((c->flags & ~ARITH) | f); }
static uint16_t sub16(cpu_t *c, uint16_t a, uint16_t b)                     /* SUB r16,imm: the six arithmetic flags (stored rule, PF of the low byte) */
{
    uint16_t r = (uint16_t)(a - b), f = parity((uint8_t)r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x8000u) f |= 0x0800u;
    set_arith(c, f); return r;
}
static void cmp8(cpu_t *c, uint8_t a, uint8_t b)
{
    uint8_t r = (uint8_t)(a - b); uint16_t f = parity(r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x80u) f |= 0x0800u;
    set_arith(c, f);
}
static void logic16(cpu_t *c, uint16_t r)                                   /* OR/XOR word: CF=OF=AF=0 (DOSBox-X), ZF/SF of the word, PF low byte */
{ uint16_t f = parity((uint8_t)r); if (!r) f |= 0x0040u; if (r & 0x8000u) f |= 0x0080u; set_arith(c, f); }
static uint8_t inc8(cpu_t *c, uint8_t v)                                   /* INC r/m8: CF kept; OF/SF/ZF/AF/PF of the result */
{
    uint8_t r = (uint8_t)(v + 1u); uint16_t f = parity(r);
    if ((v & 0x0Fu) == 0x0Fu) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if (r == 0x80u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D4u) | f); return r;
}

#define STEP(ip_) do { if (*budget == 0u) { c->ip = (ip_); return PR_RES_23DB_LIMIT; } --*budget; } while (0)

pr_res_23db_status pr_res_00ba_party_count_23db_cpu(uint8_t *ram, size_t ram_size, pr_res_23db_cpu *c, size_t *budget)
{
    uint16_t ret_ip, ret_cs;
    if (ram == NULL || c == NULL || budget == NULL || ram_size != PR_RES_2E03_RAM_SIZE || c->ip != 0x23DBu || c->sp < 0x0006u || c->sp > 0xFFFCu)
        return PR_RES_23DB_ARGUMENT;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    STEP(0x23DBu); push(ram, c, c->bp);                                                                  /* 23DB PUSH BP */
    STEP(0x23DCu); c->bp = c->sp;                                                                        /* 23DC MOV BP,SP */
    STEP(0x23DEu); c->sp = sub16(c, c->sp, 4u);                                                          /* 23DE SUB SP,4 */
    STEP(0x23E1u); wr8(ram, c->ds, 0x6814u, 0u);                                                         /* 23E1 MOV BYTE [6814],0 */
    STEP(0x23E6u); wr8(ram, c->ds, 0x6815u, 0u);                                                         /* 23E6 MOV BYTE [6815],0 */
    STEP(0x23EBu); c->ax = rd16(ram, c->ds, 0x5D96u);                                                    /* 23EB MOV AX,[5D96] */
    STEP(0x23EEu); c->dx = rd16(ram, c->ds, 0x5D98u);                                                    /* 23EE MOV DX,[5D98] */
    STEP(0x23F2u); wr16(ram, c->ss, (uint16_t)(c->bp - 4u), c->ax);                                      /* 23F2 MOV [BP-4],AX */
    STEP(0x23F5u); wr16(ram, c->ss, (uint16_t)(c->bp - 2u), c->dx);                                      /* 23F5 MOV [BP-2],DX */
    for (;;) {
        STEP(0x23F8u); c->ax = rd16(ram, c->ss, (uint16_t)(c->bp - 4u));                                 /* 23F8 MOV AX,[BP-4] */
        STEP(0x23FBu); c->ax = (uint16_t)(c->ax | rd16(ram, c->ss, (uint16_t)(c->bp - 2u))); logic16(c, c->ax);   /* 23FB OR AX,[BP-2] */
        STEP(0x23FEu); if (c->flags & 0x0040u) break;                                                    /* 23FE JE 242F */
        STEP(0x2400u); les(ram, c, c->ss, (uint16_t)(c->bp - 4u));                                       /* 2400 LES DI,[BP-4] */
        STEP(0x2403u); cmp8(c, rd8(ram, c->es, (uint16_t)(c->di + 0x010Du)), 0u);                        /* 2403 CMP BYTE ES:[DI+10D],0 */
        STEP(0x2409u);                                                                                   /* 2409 JE 241A */
        if (!(c->flags & 0x0040u)) {
            STEP(0x240Bu); les(ram, c, c->ss, (uint16_t)(c->bp - 4u));                                   /* 240B LES DI,[BP-4] */
            STEP(0x240Eu); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->es, (uint16_t)(c->di + 0x010Eu)));   /* 240E MOV AL,ES:[DI+10E] */
            STEP(0x2413u); c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax;                            /* 2413 CBW */
            STEP(0x2414u); c->di = c->ax;                                                                /* 2414 MOV DI,AX */
            STEP(0x2416u); wr8(ram, c->ds, (uint16_t)(c->di + 0x6814u), inc8(c, rd8(ram, c->ds, (uint16_t)(c->di + 0x6814u))));   /* 2416 INC BYTE [DI+6814] */
        }
        STEP(0x241Au); les(ram, c, c->ss, (uint16_t)(c->bp - 4u));                                       /* 241A LES DI,[BP-4] */
        STEP(0x241Du); c->ax = rd16(ram, c->es, (uint16_t)(c->di + 0x0104u));                            /* 241D MOV AX,ES:[DI+104] */
        STEP(0x2422u); c->dx = rd16(ram, c->es, (uint16_t)(c->di + 0x0106u));                            /* 2422 MOV DX,ES:[DI+106] */
        STEP(0x2427u); wr16(ram, c->ss, (uint16_t)(c->bp - 4u), c->ax);                                  /* 2427 MOV [BP-4],AX */
        STEP(0x242Au); wr16(ram, c->ss, (uint16_t)(c->bp - 2u), c->dx);                                  /* 242A MOV [BP-2],DX */
        STEP(0x242Du);                                                                                   /* 242D JMP 23F8 */
    }
    STEP(0x242Fu); c->sp = c->bp;                                                                        /* 242F MOV SP,BP */
    STEP(0x2431u); c->bp = pop(ram, c);                                                                  /* 2431 POP BP */
    STEP(0x2432u); c->ip = pop(ram, c); c->cs = pop(ram, c);                                             /* 2432 RETF */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_RES_23DB_TRANSFER;
    return PR_RES_23DB_RETURNED;
}
