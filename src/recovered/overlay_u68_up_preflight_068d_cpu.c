/* Original unit 0x68 +068D: the Up preflight, composing unit 0x3BD +0350 through its stub (docs/re/OVL_0068_068D_CPU.md). */
#include "poolrad/recovered/overlay_u68_up_preflight_068d_cpu.h"
#include "poolrad/recovered/overlay_u3bd_lookup_fullstate.h"

#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define ARITH 0x08D5u
typedef pr_u68_068d_cpu cpu_t;

_Static_assert(sizeof(pr_u68_068d_cpu) == sizeof(pr_u3bd_lookup_cpu), "u3BD lookup CPU state layout differs");

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
static void bus_wr16(pr_bus *bus, uint16_t seg, uint16_t off, uint16_t v)        /* a word store through the bus: low byte first, each byte at its own wrapped offset */
{ pr_bus_write8(bus, physical(seg, off), (uint8_t)v); pr_bus_write8(bus, physical(seg, (uint16_t)(off + 1u)), (uint8_t)(v >> 8u)); }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 0x0004u; }
static void mov_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static void cbw(cpu_t *c) { c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; }
static void set_flags16(cpu_t *c, uint16_t a, uint16_t b, uint16_t r, int sub, unsigned carry)
{
    uint16_t f = parity((uint8_t)r);
    if (carry) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((sub ? (a ^ b) & (a ^ r) : ~(a ^ b) & (a ^ r)) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
static uint16_t sub16(cpu_t *c, uint16_t a, uint16_t b) { uint16_t r = (uint16_t)(a - b); set_flags16(c, a, b, r, 1, a < b); return r; }
static uint16_t add16(cpu_t *c, uint16_t a, uint16_t b) { uint16_t r = (uint16_t)(a + b); set_flags16(c, a, b, r, 0, (uint32_t)a + b > 0xFFFFu); return r; }
static void cmp16(cpu_t *c, uint16_t a, uint16_t b) { (void)sub16(c, a, b); }
static void logic(cpu_t *c, uint16_t r, int word)                                /* XOR/OR: CF/OF/AF clear (DOSBox-X), SF/ZF/PF from the result */
{
    uint16_t f = parity((uint8_t)r);
    if (word ? r == 0u : (uint8_t)r == 0u) f |= 0x0040u;
    if (word ? (r & 0x8000u) : (r & 0x80u)) f |= 0x0080u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
static void flag_window(pr_bus *bus, uint8_t *ram, cpu_t *c)                     /* LES DI,[49D6]; MOV WORD ES:[DI+5AA],1 */
{
    c->di = rd16(ram, c->ds, 0x49D6u); c->es = rd16(ram, c->ds, 0x49D8u);
    bus_wr16(bus, c->es, (uint16_t)(c->di + 0x05AAu), 1u);
}

pr_u68_068d_status pr_u68_up_preflight_068d_cpu(pr_bus *bus, size_t ram_size, pr_u68_068d_cpu *c)
{
    uint8_t *ram; uint16_t ret_ip, ret_cs, caller_cs, stub_seg, stub_off, target_cs; pr_u3bd_lookup_cpu leaf; int16_t v;
    if (bus == NULL || bus->ram == NULL || c == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x068Du || c->sp < 0x0030u || c->sp > 0xFFFCu)
        return PR_U68_068D_ARGUMENT;
    ram = bus->ram; caller_cs = c->cs;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    push(ram, c, c->bp); c->bp = c->sp;                                          /* 068D PUSH BP; 068E MOV BP,SP */
    c->sp = sub16(c, c->sp, 6u);                                                 /* 0690 SUB SP,6 */
    mov_al(c, rd8(ram, c->ds, 0x6AADu)); cbw(c); wr16(ram, c->ss, (uint16_t)(c->bp - 2u), c->ax);   /* 0693..0697: x local */
    mov_al(c, rd8(ram, c->ds, 0x6AAEu)); cbw(c); wr16(ram, c->ss, (uint16_t)(c->bp - 4u), c->ax);   /* 069A..069E: y local */
    mov_al(c, rd8(ram, c->ds, 0x6AAFu)); c->ax &= 0x00FFu; logic(c, 0u, 0);                         /* 06A1 MOV AL,[6AAF]; 06A4 XOR AH,AH */
    wr16(ram, c->ss, (uint16_t)(c->bp - 6u), c->ax);                                                /* 06A6: facing local */
    c->di = rd16(ram, c->ds, 0x49D6u); c->es = rd16(ram, c->ds, 0x49D8u);       /* 06A9 LES DI,[49D6] */
    c->ax = 0u; logic(c, 0u, 1);                                                  /* 06AD XOR AX,AX */
    bus_wr16(bus, c->es, (uint16_t)(c->di + 0x05AAu), c->ax);                     /* 06AF MOV ES:[DI+5AA],AX (through the bus) */
    mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp - 2u))); push(ram, c, c->ax);     /* 06B4..06B7 PUSH x */
    mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp - 4u))); push(ram, c, c->ax);     /* 06B8..06BB PUSH y */
    mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp - 6u))); push(ram, c, c->ax);     /* 06BC..06BF PUSH facing */
    stub_off = rd16(ram, c->cs, 0x06C1u); stub_seg = rd16(ram, c->cs, 0x06C3u);   /* 06C0 LCALL seg:off, both read from the (relocated) code */
    push(ram, c, c->cs); push(ram, c, 0x06C5u);
    c->cs = stub_seg; c->ip = stub_off;
    if (rd8(ram, stub_seg, stub_off) != 0xEAu || rd16(ram, stub_seg, (uint16_t)(stub_off + 1u)) != 0x0350u)
        return PR_U68_068D_OVERLAY;                                               /* not loaded: the original enters the overlay manager here */
    target_cs = rd16(ram, stub_seg, (uint16_t)(stub_off + 3u));
    c->cs = target_cs; c->ip = 0x0350u;                                           /* the stub's JMP FAR: no other effect */
    memcpy(&leaf, c, sizeof leaf);
    if (pr_u3bd_field_0350_fullstate(ram, ram_size, &leaf) == PR_U3BD_LOOKUP_ARGUMENT) return PR_U68_068D_CALLEE;
    memcpy(c, &leaf, sizeof leaf);
    if (c->ip != 0x06C5u || c->cs != caller_cs) return PR_U68_068D_TRANSFER;
    logic(c, (uint8_t)c->ax, 0);                                                  /* 06C5 OR AL,AL */
    if ((uint8_t)c->ax != 0u) {                                                   /* 06C7 JE 0737 */
        c->di = rd16(ram, c->ss, (uint16_t)(c->bp - 6u));                         /* 06C9 MOV DI,[BP-6] */
        mov_al(c, rd8(ram, c->ds, (uint16_t)(c->di + 0x2736u))); cbw(c);          /* 06CC; 06D0 CBW */
        wr16(ram, c->ss, (uint16_t)(c->bp - 2u), add16(c, rd16(ram, c->ss, (uint16_t)(c->bp - 2u)), c->ax));   /* 06D1 ADD [BP-2],AX */
        c->di = rd16(ram, c->ss, (uint16_t)(c->bp - 6u));                         /* 06D4 MOV DI,[BP-6] */
        mov_al(c, rd8(ram, c->ds, (uint16_t)(c->di + 0x273Fu))); cbw(c);          /* 06D7; 06DB CBW */
        wr16(ram, c->ss, (uint16_t)(c->bp - 4u), add16(c, rd16(ram, c->ss, (uint16_t)(c->bp - 4u)), c->ax));   /* 06DC ADD [BP-4],AX */
        v = (int16_t)rd16(ram, c->ss, (uint16_t)(c->bp - 2u)); cmp16(c, (uint16_t)v, 0x000Fu);       /* 06DF CMP [BP-2],0F */
        if (v > 15) { wr8(ram, c->ds, 0x6AADu, 0x0Fu); flag_window(bus, ram, c); }                   /* 06E3 JLE; 06E5..06EE */
        v = (int16_t)rd16(ram, c->ss, (uint16_t)(c->bp - 2u)); cmp16(c, (uint16_t)v, 0u);           /* 06F5 CMP [BP-2],0 */
        if (v < 0) { wr8(ram, c->ds, 0x6AADu, 0x00u); flag_window(bus, ram, c); }                    /* 06F9 JGE; 06FB..0704 */
        v = (int16_t)rd16(ram, c->ss, (uint16_t)(c->bp - 4u)); cmp16(c, (uint16_t)v, 0x000Fu);       /* 070B CMP [BP-4],0F */
        if (v > 15) { wr8(ram, c->ds, 0x6AAEu, 0x0Fu); flag_window(bus, ram, c); }                   /* 070F JLE; 0711..071A */
        v = (int16_t)rd16(ram, c->ss, (uint16_t)(c->bp - 4u)); cmp16(c, (uint16_t)v, 0u);           /* 0721 CMP [BP-4],0 */
        if (v < 0) { wr8(ram, c->ds, 0x6AAEu, 0x00u); flag_window(bus, ram, c); }                    /* 0725 JGE; 0727..0730 */
    }
    c->sp = c->bp; c->bp = pop(ram, c);                                           /* 0737 MOV SP,BP; 0739 POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                                     /* 073A RETF */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U68_068D_TRANSFER;
    return PR_U68_068D_RETURNED;
}
