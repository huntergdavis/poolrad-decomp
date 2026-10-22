/* Original unit 0x33 +0B97: two grid-cell stores and a counter increment (docs/re/OVL_0033_0B97_CPU.md). */
#include "poolrad/recovered/overlay_u33_0b97_cpu.h"

#define MASK (PR_BUS_RAM_SIZE - 1u)
typedef pr_u33_0b97_regs cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{ size_t p = physical(seg, off); return (uint16_t)(ram[p] | ((uint16_t)ram[(p + 1u) & MASK] << 8u)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{ size_t p = physical(seg, off); ram[p] = (uint8_t)v; ram[(p + 1u) & MASK] = (uint8_t)(v >> 8u); }
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, cpu_t *c) { uint16_t v = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); return v; }
static uint16_t parity(uint8_t v) { v ^= (uint8_t)(v >> 4u); v ^= (uint8_t)(v >> 2u); v ^= (uint8_t)(v >> 1u); return (v & 1u) ? 0u : 4u; }
static uint16_t add16(cpu_t *c, uint16_t a, uint16_t b)
{
    uint32_t wide = (uint32_t)a + b; uint16_t r = (uint16_t)wide, f = parity((uint8_t)r);
    if (wide > 0xFFFFu) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((~(a ^ b) & (a ^ r)) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | f);
    return r;
}
static uint16_t inc16(cpu_t *c, uint16_t a)                                          /* INC r16: every arithmetic flag but CF */
{
    uint16_t r = (uint16_t)(a + 1u), f = parity((uint8_t)r);
    if ((a ^ 1u ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if (a == 0x7FFFu) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D4u) | f);
    return r;
}
static uint8_t inc8(cpu_t *c, uint8_t a)
{
    uint8_t r = (uint8_t)(a + 1u); uint16_t f = parity(r);
    if ((a ^ 1u ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    if (a == 0x7Fu) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~0x08D4u) | f);
    return r;
}
static void mul_dx(cpu_t *c)                                                         /* MUL DX: DX:AX := AX * DX; CF and OF set when DX is nonzero; the rest undefined */
{
    uint32_t p = (uint32_t)c->ax * c->dx;
    c->ax = (uint16_t)p; c->dx = (uint16_t)(p >> 16u);
    c->flags = (uint16_t)((c->flags & ~0x0801u) | (c->dx ? 0x0801u : 0u));
}
static void cbw(cpu_t *c) { c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; }
static void mov_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static uint8_t arg8(const uint8_t *ram, const cpu_t *c, uint16_t off) { return rd8(ram, c->ss, (uint16_t)(c->bp + off)); }

pr_u33_0b97_status pr_u33_0b97_cpu(pr_bus *bus, size_t ram_size, pr_u33_0b97_regs *c, size_t *budget)
{
    uint8_t *ram; uint16_t ret_ip, ret_cs;
    if (bus == NULL || bus->ram == NULL || c == NULL || budget == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0B97u || c->sp < 0x0010u || c->sp > 0xFFF6u)
        return PR_U33_0B97_ARGUMENT;
    ram = bus->ram; ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
#define STEP() do { if (*budget == 0u) return PR_U33_0B97_LIMIT; --*budget; } while (0)
#define AT(next_ip) (c->ip = (next_ip))
    STEP(); push(ram, c, c->bp); AT(0x0B98u);                                                /* 0B97 PUSH BP */
    STEP(); c->bp = c->sp; AT(0x0B9Au);                                                      /* 0B98 MOV BP,SP */
    STEP(); mov_al(c, arg8(ram, c, 8)); AT(0x0B9Du);                                         /* 0B9A MOV AL,[BP+8]: X */
    STEP(); cbw(c); AT(0x0B9Eu);                                                             /* 0B9D CBW */
    STEP(); c->cx = c->ax; AT(0x0BA0u);                                                      /* 0B9E MOV CX,AX */
    STEP(); mov_al(c, arg8(ram, c, 0xA)); AT(0x0BA3u);                                       /* 0BA0 MOV AL,[BP+0A]: Y */
    STEP(); cbw(c); AT(0x0BA4u);                                                             /* 0BA3 CBW */
    STEP(); c->dx = 0x0032u; AT(0x0BA7u);                                                    /* 0BA4 MOV DX,0032 */
    STEP(); mul_dx(c); AT(0x0BA9u);                                                          /* 0BA7 MUL DX */
    STEP(); c->di = rd16(ram, c->ds, 0x6716u); c->es = rd16(ram, c->ds, 0x6718u); AT(0x0BADu);   /* 0BA9 LES DI,[6716] */
    STEP(); c->di = add16(c, c->di, c->ax); AT(0x0BAFu);                                     /* 0BAD ADD DI,AX */
    STEP(); c->di = add16(c, c->di, c->cx); AT(0x0BB1u);                                     /* 0BAF ADD DI,CX */
    STEP(); pr_bus_write8(bus, physical(c->es, (uint16_t)(c->di + 7u)), 0x34u); AT(0x0BB6u); /* 0BB1 MOV BYTE ES:[DI+7],34h (through the bus) */
    STEP(); mov_al(c, arg8(ram, c, 8)); AT(0x0BB9u);                                         /* 0BB6 MOV AL,[BP+8] */
    STEP(); cbw(c); AT(0x0BBAu);                                                             /* 0BB9 CBW */
    STEP(); c->ax = inc16(c, c->ax); AT(0x0BBBu);                                            /* 0BBA INC AX */
    STEP(); c->cx = c->ax; AT(0x0BBDu);                                                      /* 0BBB MOV CX,AX */
    STEP(); mov_al(c, arg8(ram, c, 0xA)); AT(0x0BC0u);                                       /* 0BBD MOV AL,[BP+0A] */
    STEP(); cbw(c); AT(0x0BC1u);                                                             /* 0BC0 CBW */
    STEP(); c->dx = 0x0032u; AT(0x0BC4u);                                                    /* 0BC1 MOV DX,0032 */
    STEP(); mul_dx(c); AT(0x0BC6u);                                                          /* 0BC4 MUL DX */
    STEP(); c->di = rd16(ram, c->ds, 0x6716u); c->es = rd16(ram, c->ds, 0x6718u); AT(0x0BCAu);   /* 0BC6 LES DI,[6716] */
    STEP(); c->di = add16(c, c->di, c->ax); AT(0x0BCCu);                                     /* 0BCA ADD DI,AX */
    STEP(); c->di = add16(c, c->di, c->cx); AT(0x0BCEu);                                     /* 0BCC ADD DI,CX */
    STEP(); pr_bus_write8(bus, physical(c->es, (uint16_t)(c->di + 7u)), 0x35u); AT(0x0BD3u); /* 0BCE MOV BYTE ES:[DI+7],35h (through the bus) */
    STEP(); c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 6u)); AT(0x0BD6u);                   /* 0BD3 MOV DI,[BP+6] */
    STEP();                                                                                   /* 0BD6 INC BYTE SS:[DI-2] (through the bus) */
    { size_t at = physical(c->ss, (uint16_t)(c->di - 2u)); uint8_t v = pr_bus_read8(bus, at); pr_bus_write8(bus, at, inc8(c, v)); }
    AT(0x0BDAu);
    STEP(); c->bp = pop(ram, c); AT(0x0BDBu);                                                 /* 0BDA POP BP */
    STEP(); c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 6u);         /* 0BDB RETF 6 */
#undef AT
#undef STEP
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U33_0B97_TRANSFER;
    return PR_U33_0B97_RETURNED;
}
