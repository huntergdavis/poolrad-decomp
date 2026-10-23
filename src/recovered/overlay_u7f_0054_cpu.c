/* Original unit 0x7F +0054: the bounded subtraction around +0000 (docs/re/OVL_007F_0054_CPU.md). */
#include "poolrad/recovered/overlay_u7f_0054_cpu.h"

#define MASK (PR_BUS_RAM_SIZE - 1u)
typedef pr_res00ba_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
/* Accesses through a pointer the caller supplies go through the bus; stack and code accesses are flat RAM. */
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
    uint16_t r = (uint16_t)(wide & 0xFFFFu), f = parity((uint8_t)r);
    if (subtract ? a < b : wide > 0xFFFFu) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((subtract ? ((a ^ b) & (a ^ r)) : (~(a ^ b) & (a ^ r))) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
static void logic_flags(cpu_t *c, uint16_t r)                                       /* XOR AX,AX: CF, OF, AF clear; PF, ZF, SF from the result */
{ c->flags = (uint16_t)((c->flags & ~0x08D5u) | parity((uint8_t)r) | (r ? 0u : 0x0040u) | ((r & 0x8000u) ? 0x0080u : 0u)); }

/* One `CALL 0000` (after PUSH CS): site is the call's IP, ret the IP after it. Returns -1 to continue or the status to return. */
static int call_0000(pr_bus *bus, cpu_t *c, size_t *budget, pr_u7f_0054_diagnostic *d, uint16_t site, uint16_t ret, uint8_t disp_lo, uint8_t disp_hi, uint16_t cs)
{
    uint8_t *ram = bus->ram; pr_u7f_0000_diagnostic sub; pr_u7f_0000_status s;
    if (*budget == 0u) return PR_U7F_0054_LIMIT;
    d->site = site;
    if (rd8(ram, cs, site) != 0xE8u || rd8(ram, cs, (uint16_t)(site + 1u)) != disp_lo || rd8(ram, cs, (uint16_t)(site + 2u)) != disp_hi) return PR_U7F_0054_CODE;
    --*budget;
    push(ram, c, ret); c->ip = 0x0000u;
    s = pr_u7f_entry0_0000_cpu(ram, PR_BUS_RAM_SIZE, c, budget, &sub);
    if (s == PR_U7F_0000_LIMIT) return PR_U7F_0054_LIMIT;
    if (s != PR_U7F_0000_RETURNED) { d->callee_status = (int)s; return PR_U7F_0054_CALLEE; }
    if (c->cs != cs || c->ip != ret) return PR_U7F_0054_TRANSFER;
    return -1;
}

pr_u7f_0054_status pr_u7f_0054_cpu(pr_bus *bus, size_t ram_size, pr_res00ba_cpu *c, size_t *budget, pr_u7f_0054_diagnostic *diagnostic)
{
    pr_u7f_0054_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint8_t *ram; uint16_t cs, ret_ip, ret_cs, dx; int r;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0054u || c->sp < 0x0050u || c->sp > 0xFFF2u)
        return PR_U7F_0054_ARGUMENT;
    ram = bus->ram; d->callee_status = -1; d->site = 0; cs = c->cs;
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));       /* the entry frame's return address, for the TRANSFER check at RETF */
#define STEP() do { if (*budget == 0u) return PR_U7F_0054_LIMIT; --*budget; } while (0)
    STEP(); push(ram, c, c->bp); c->ip = 0x0055u;                                             /* 0054 PUSH BP */
    STEP(); c->bp = c->sp; c->ip = 0x0057u;                                                   /* 0055 MOV BP,SP */
    STEP(); c->sp = arith16(c, c->sp, 2u, 1); c->ip = 0x005Au;                                /* 0057 SUB SP,2 */
    STEP(); push(ram, c, bp16(ram, c, 0xE)); c->ip = 0x005Du;                                 /* 005A PUSH [BP+0E]: the record pointer's segment */
    STEP(); push(ram, c, bp16(ram, c, 0xC)); c->ip = 0x0060u;                                 /* 005D PUSH [BP+0C]: its offset */
    STEP(); push(ram, c, c->cs); c->ip = 0x0061u;                                             /* 0060 PUSH CS */
    c->ip = 0x0061u; if ((r = call_0000(bus, c, budget, d, 0x0061u, 0x0064u, 0x9Cu, 0xFFu, cs)) >= 0) return (pr_u7f_0054_status)r;      /* 0061 CALL 0000 */
    STEP(); c->dx = c->ax; dx = c->dx; c->ip = 0x0066u;                                       /* 0064 MOV DX,AX */
    STEP(); c->di = bp16(ram, c, 0xC); c->es = bp16(ram, c, 0xE); c->ip = 0x0069u;            /* 0066 LES DI,[BP+0C] */
    STEP(); c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x0102u)); c->ip = 0x006Eu;          /* 0069 MOV AX,ES:[DI+0102] (through the bus) */
    STEP(); c->ax = arith16(c, c->ax, bp16(ram, c, 0xA), 0); c->ip = 0x0071u;                 /* 006E ADD AX,[BP+0A] */
    STEP(); (void)arith16(c, c->ax, dx, 1); c->ip = 0x0073u;                                  /* 0071 CMP AX,DX */
    STEP();                                                                                   /* 0073 JBE 0093 */
    if (c->flags & 0x0041u) {                                                                 /* CF or ZF: the body was not above */
        c->ip = 0x0093u;
        STEP(); ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = 0u; c->ip = 0x0097u;           /* 0093 MOV BYTE [BP-1],0 */
        STEP(); c->di = bp16(ram, c, 6); c->es = bp16(ram, c, 8); c->ip = 0x009Au;            /* 0097 LES DI,[BP+6] */
        STEP(); c->ax = 0u; logic_flags(c, 0u); c->ip = 0x009Cu;                              /* 009A XOR AX,AX */
        STEP(); bwr16(bus, c->es, c->di, c->ax); c->ip = 0x009Fu;                             /* 009C MOV ES:[DI],AX (through the bus) */
    } else {
        c->ip = 0x0075u;
        STEP(); ram[physical(c->ss, (uint16_t)(c->bp - 1u))] = 1u; c->ip = 0x0079u;           /* 0075 MOV BYTE [BP-1],1 */
        STEP(); push(ram, c, bp16(ram, c, 0xE)); c->ip = 0x007Cu;                             /* 0079 PUSH [BP+0E] */
        STEP(); push(ram, c, bp16(ram, c, 0xC)); c->ip = 0x007Fu;                             /* 007C PUSH [BP+0C] */
        STEP(); push(ram, c, c->cs); c->ip = 0x0080u;                                         /* 007F PUSH CS */
        if ((r = call_0000(bus, c, budget, d, 0x0080u, 0x0083u, 0x7Du, 0xFFu, cs)) >= 0) return (pr_u7f_0054_status)r;                    /* 0080 CALL 0000 */
        STEP(); c->di = bp16(ram, c, 0xC); c->es = bp16(ram, c, 0xE); c->ip = 0x0086u;        /* 0083 LES DI,[BP+0C] */
        STEP(); c->ax = arith16(c, c->ax, brd16(bus, c->es, (uint16_t)(c->di + 0x0102u)), 1); c->ip = 0x008Bu;    /* 0086 SUB AX,ES:[DI+0102] (through the bus) */
        STEP(); c->di = bp16(ram, c, 6); c->es = bp16(ram, c, 8); c->ip = 0x008Eu;            /* 008B LES DI,[BP+6] */
        STEP(); bwr16(bus, c->es, c->di, c->ax); c->ip = 0x0091u;                             /* 008E MOV ES:[DI],AX (through the bus) */
        STEP(); c->ip = 0x009Fu;                                                              /* 0091 JMP 009F */
    }
    STEP(); c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ss, (uint16_t)(c->bp - 1u))); c->ip = 0x00A2u;       /* 009F MOV AL,[BP-1] */
    STEP(); c->sp = c->bp; c->ip = 0x00A4u;                                                   /* 00A2 MOV SP,BP */
    STEP(); c->bp = pop(ram, c); c->ip = 0x00A5u;                                             /* 00A4 POP BP */
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 10u);        /* 00A5 RETF 000A */
#undef STEP
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U7F_0054_TRANSFER;
    return PR_U7F_0054_RETURNED;
}
