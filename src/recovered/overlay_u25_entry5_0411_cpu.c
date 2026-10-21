/* Unit 0x25 +0411, the LOAD MONSTER wrapper, raw CPU (docs/re/OVL_0025_0411_CPU.md). */
#include "poolrad/recovered/overlay_u25_entry5_0411_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
#define CONTINUE (-1)
typedef pr_u25_0411_cpu cpu_t;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)v; ram[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
/* Accesses through a pointer the caller supplies (the output variables, the record pointer and the record's fields) go through the bus: an aliased or corrupted
 * pointer into A0000..AFFFF reaches the EGA as in the original. Stack and code accesses are flat RAM. */
static uint16_t brd16(pr_bus *bus, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(pr_bus_read8(bus, at) | ((uint16_t)pr_bus_read8(bus, (at + 1u) & MASK) << 8u));
}
static void bwr16(pr_bus *bus, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    pr_bus_write8(bus, at, (uint8_t)v); pr_bus_write8(bus, (at + 1u) & MASK, (uint8_t)(v >> 8u));
}
static void push(uint8_t *ram, cpu_t *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static void les_bp(uint8_t *ram, cpu_t *c, int off)                                 /* LES DI,SS:[BP+off] */
{
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + off)); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + off + 2));
}
static void les_es_di(pr_bus *bus, cpu_t *c)                                        /* LES DI,ES:[DI] */
{
    size_t at = physical(c->es, c->di);                                             /* the pair is read physically contiguous, as the other models' LES (not wrapped at 64 KiB) */
    uint16_t off = (uint16_t)(pr_bus_read8(bus, at) | ((uint16_t)pr_bus_read8(bus, (at + 1u) & MASK) << 8u));
    uint16_t seg = (uint16_t)(pr_bus_read8(bus, (at + 2u) & MASK) | ((uint16_t)pr_bus_read8(bus, (at + 3u) & MASK) << 8u));
    c->di = off; c->es = seg;
}
static unsigned parity(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) ? 0u : 0x04u; }
static void sub16_flags(cpu_t *c, unsigned a, unsigned b)
{
    unsigned r = (a - b) & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~0x08D5u) | (r == 0u ? 0x40u : 0u) | (r & 0x8000u ? 0x80u : 0u) | parity((uint8_t)r) | (a < b ? 0x01u : 0u) |
                          ((a ^ b ^ r) & 0x10u) | (((a ^ b) & (a ^ r) & 0x8000u) ? 0x800u : 0u));
}

static int map_inner(int r)
{
    switch (r) {
    case PR_U6F_0EC4_RETURNED: return CONTINUE;
    case PR_U6F_0EC4_EXIT: return (int)PR_U25_0411_EXIT;
    case PR_U6F_0EC4_RUNERROR: return (int)PR_U25_0411_RUNERROR;
    case PR_U6F_0EC4_DIVIDE: return (int)PR_U25_0411_DIVIDE;
    case PR_U6F_0EC4_BREAK: return (int)PR_U25_0411_BREAK;
    case PR_U6F_0EC4_STALLED: return (int)PR_U25_0411_STALLED;
    case PR_U6F_0EC4_LIMIT: return (int)PR_U25_0411_LIMIT;
    case PR_U6F_0EC4_SERVICE: return (int)PR_U25_0411_SERVICE;
    case PR_U6F_0EC4_TRANSFER: return (int)PR_U25_0411_TRANSFER;
    case PR_U6F_0EC4_CODE: return (int)PR_U25_0411_CODE;
    default: return (int)PR_U25_0411_CALLEE;                        /* ARGUMENT inside, or anything else */
    }
}

pr_u25_0411_status pr_u25_entry5_0411_cpu(pr_bus *bus, size_t ram_size, cpu_t *c, const pr_u6f_0ec4_env *env, pr_u25_0411_diagnostic *diagnostic)
{
    pr_u25_0411_diagnostic local, *d = diagnostic ? diagnostic : &local;
    uint8_t *ram;
    uint16_t home, back, seg, stub;
    int r;
    if (bus == NULL || bus->ram == NULL || c == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0411u || c->sp < 0x0A04u || c->sp > 0xFFEEu)
        return PR_U25_0411_ARGUMENT;
    ram = bus->ram;
    memset(d, 0, sizeof *d); d->callee = -1; d->callee_status = -1;
    home = c->cs;
    push(ram, c, c->bp); c->bp = c->sp;                                               /* 0411 PUSH BP; MOV BP,SP */
    { unsigned x = c->sp; c->sp = (uint16_t)(x - 4u); sub16_flags(c, x, 4u); }       /* 0414 SUB SP,4 */
    les_bp(ram, c, 0x0E);                                                              /* 0417 LES DI,[BP+0Eh] */
    c->ax = 0; c->flags = (uint16_t)((c->flags & ~0x08D5u) | 0x44u);                  /* 041A XOR AX,AX */
    bwr16(bus, c->es, c->di, c->ax); bwr16(bus, c->es, (uint16_t)(c->di + 2u), c->ax);  /* 041C, 041F: AE^ := nil */
    c->ax = 0x011Du; push(ram, c, c->ax);                                              /* 0423 MOV AX,11Dh; PUSH AX */
    /* 0427 LCALL 0AF8:023F: opcode and offset checked before the return frame is pushed, the segment word live */
    c->ip = 0x0427u; d->site = 0x0427u; d->callee = PR_U25_0411_GETMEM;
    if (rd8(ram, home, 0x0427u) != 0x9Au || rd16(ram, home, 0x0428u) != 0x023Fu) return PR_U25_0411_CODE;
    seg = rd16(ram, home, 0x042Au); back = 0x042Cu;
    push(ram, c, home); push(ram, c, back); c->cs = seg; c->ip = 0x023Fu;
    {
        pr_system_getmem_cpu x;
        COPY(&x, c); r = (int)pr_system_getmem_023f(ram, ram_size, &x, env->heap_steps); COPY(c, &x);
        d->callee_status = r;
        if (r == PR_SYSTEM_GETMEM_RUNERROR) return PR_U25_0411_RUNERROR;
        if (r == PR_SYSTEM_GETMEM_LIMIT) return PR_U25_0411_LIMIT;
        if (r == PR_SYSTEM_GETMEM_TRANSFER) return PR_U25_0411_TRANSFER;
        if (r != PR_SYSTEM_GETMEM_RETURNED) return PR_U25_0411_CALLEE;
        if (c->cs != home || c->ip != back) return PR_U25_0411_TRANSFER;
    }
    ++d->calls; d->site = 0; d->callee = -1; d->callee_status = -1;
    les_bp(ram, c, 0x0E);                                                              /* 042C LES DI,[BP+0Eh]; MOV ES:[DI],AX; MOV ES:[DI+2],DX */
    bwr16(bus, c->es, c->di, c->ax); bwr16(bus, c->es, (uint16_t)(c->di + 2u), c->dx);
    c->ax = (uint16_t)((c->ax & 0xFF00u) | rd8(ram, c->ss, (uint16_t)(c->bp + 0x12u))); push(ram, c, c->ax);   /* 0436 MOV AL,[BP+12h]; PUSH AX */
    les_bp(ram, c, 0x0E);                                                              /* 043A LES DI,[BP+0Eh]; PUSH ES:[DI+2]; PUSH ES:[DI] */
    push(ram, c, brd16(bus, c->es, (uint16_t)(c->di + 2u))); push(ram, c, brd16(bus, c->es, c->di));
    /* 0444 LCALL 007C:003E: the stub segment word at CS:0447 is live; the stub is a far JMP to 0EC4 in the loaded unit 0x6F */
    c->ip = 0x0444u; d->site = 0x0444u; d->callee = PR_U25_0411_ENTRY6;
    if (rd8(ram, home, 0x0444u) != 0x9Au || rd16(ram, home, 0x0445u) != 0x003Eu) return PR_U25_0411_CODE;
    stub = rd16(ram, home, 0x0447u); back = 0x0449u;
    if (rd8(ram, stub, 0x3Eu) != 0xEAu || rd16(ram, stub, 0x3Fu) != 0x0EC4u) return PR_U25_0411_CODE;
    seg = rd16(ram, stub, 0x41u);
    push(ram, c, home); push(ram, c, back); c->cs = seg; c->ip = 0x0EC4u;
    {
        pr_u6f_0ec4_cpu x;
        COPY(&x, c); r = (int)pr_u6f_entry6_0ec4_cpu(bus, ram_size, &x, env, &d->inner); COPY(c, &x);
        d->callee_status = r;
        r = map_inner(r);
        if (r != CONTINUE) return (pr_u25_0411_status)r;
        if (c->cs != home || c->ip != back) return PR_U25_0411_TRANSFER;
    }
    ++d->calls; d->site = 0; d->callee = -1; d->callee_status = -1;
    les_bp(ram, c, 0x0E); les_es_di(bus, c);                                           /* 0449 LES DI,[BP+0Eh]; LES DI,ES:[DI] */
    c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0xC8u)); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0xCAu));   /* 044F, 0454 */
    les_bp(ram, c, 0x0A); bwr16(bus, c->es, c->di, c->ax); bwr16(bus, c->es, (uint16_t)(c->di + 2u), c->dx);     /* 0459 .. 045F: AA^ := R+C8/CA */
    les_bp(ram, c, 0x0E); les_es_di(bus, c);                                           /* 0463, 0466 */
    c->ax = brd16(bus, c->es, (uint16_t)(c->di + 0x7Fu)); c->dx = brd16(bus, c->es, (uint16_t)(c->di + 0x81u));   /* 0469, 046D */
    les_bp(ram, c, 0x06); bwr16(bus, c->es, c->di, c->ax); bwr16(bus, c->es, (uint16_t)(c->di + 2u), c->dx);     /* 0472 .. 0478: A6^ := R+7F/81 */
    c->sp = c->bp;                                                                      /* 047C MOV SP,BP */
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);                    /* 047E POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));  /* 047F RETF 0Eh */
    c->sp = (uint16_t)(c->sp + 4u + 0x0Eu);
    return PR_U25_0411_RETURNED;
}
