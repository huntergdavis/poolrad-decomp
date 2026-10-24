/* Original unit 0x3BD +057A, public entry 4: the cell lookup (docs/re/OVL_03BD_057A_CPU.md). */
#include "poolrad/recovered/overlay_u3bd_cell_057a_cpu.h"
#include "poolrad/recovered/overlay_u3bd_in_grid_031f.h"

#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define ARITH 0x08D5u   /* OF SF ZF AF PF CF */
typedef pr_u3bd_057a_cpu cpu_t;

_Static_assert(sizeof(pr_u3bd_057a_cpu) == sizeof(pr_u3bd_031f_cpu), "u3BD +031F CPU state layout differs");

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
static void mov_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static void cbw(cpu_t *c) { c->ax = (uint16_t)(int16_t)(int8_t)(uint8_t)c->ax; }

static uint16_t sub16(cpu_t *c, uint16_t a, uint16_t b)
{
    uint16_t r = (uint16_t)(a - b), f = parity((uint8_t)r);
    if (a < b) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if ((a ^ b) & (a ^ r) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
    return r;
}
static uint16_t add16(cpu_t *c, uint16_t a, uint16_t b)
{
    uint32_t wide = (uint32_t)a + b; uint16_t r = (uint16_t)wide, f = parity((uint8_t)r);
    if (wide > 0xFFFFu) f |= 0x0001u;
    if ((a ^ b ^ r) & 0x10u) f |= 0x0010u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if (~(a ^ b) & (a ^ r) & 0x8000u) f |= 0x0800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
    return r;
}
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
static void or8(cpu_t *c, uint8_t r)                                             /* OR AL,AL: CF/OF clear; AF 0 as in DOSBox-X */
{
    uint16_t f = parity(r);
    if (!r) f |= 0x0040u;
    if (r & 0x80u) f |= 0x0080u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
}
static void shl16(cpu_t *c, unsigned count)                                     /* SHL AX,CL as DOSBox-X's 386 core: count masked to 5 bits, */
{                                                                               /* AF = (count != 0), OF = MSB(result) ^ CF (flags.cpp:719-727) */
    uint16_t a = c->ax, r; uint16_t f;
    count &= 0x1Fu;
    if (count == 0u) return;
    r = count >= 16u ? 0u : (uint16_t)(a << count);
    f = parity((uint8_t)r) | 0x0010u;
    if (count <= 16u && ((a >> (16u - count)) & 1u)) f |= 0x0001u;
    if (!r) f |= 0x0040u;
    if (r & 0x8000u) f |= 0x0080u;
    if (((r >> 15u) ^ f) & 1u) f |= 0x0800u;                                    /* f bit 0 is CF */
    c->flags = (uint16_t)((c->flags & ~ARITH) | f);
    c->ax = r;
}
/* CMP BYTE [BP+k],0Fh / JLE, MOV 0 / CMP BYTE [BP+k],0 / JGE, MOV 0Fh: the byte is reread after each step. */
static void wrap(uint8_t *ram, cpu_t *c, uint16_t k)
{
    uint8_t v = rd8(ram, c->ss, (uint16_t)(c->bp + k));
    cmp8(c, v, 0x0Fu);
    if ((int8_t)v > 15) wr8(ram, c->ss, (uint16_t)(c->bp + k), 0u);
    v = rd8(ram, c->ss, (uint16_t)(c->bp + k));
    cmp8(c, v, 0u);
    if ((int8_t)v < 0) wr8(ram, c->ss, (uint16_t)(c->bp + k), 0x0Fu);
}

pr_u3bd_057a_status pr_u3bd_cell_057a_cpu(pr_bus *bus, size_t ram_size, pr_u3bd_057a_cpu *c)
{
    uint8_t *ram; uint16_t ret_ip, ret_cs, caller_cs; pr_u3bd_031f_cpu leaf; cpu_t saved; uint8_t map, below[16]; unsigned i;
    if (bus == NULL || bus->ram == NULL || c == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x057Au || c->sp < 0x0010u || c->sp > 0xFFF8u)
        return PR_U3BD_057A_ARGUMENT;
    ram = bus->ram; saved = *c;
    for (i = 0u; i < 16u; ++i) below[i] = rd8(ram, c->ss, (uint16_t)(c->sp - 16u + i));   /* restored if the leaf rejects its entry */
    ret_ip = rd16(ram, c->ss, c->sp); ret_cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));
    push(ram, c, c->bp);                                                          /* 057A PUSH BP */
    c->bp = c->sp;                                                                /* 057B MOV BP,SP */
    c->sp = sub16(c, c->sp, 2u);                                                  /* 057D SUB SP,2 */
    mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 8u))); push(ram, c, c->ax);      /* 0580 MOV AL,[BP+8]; 0583 PUSH AX */
    mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 6u))); push(ram, c, c->ax);      /* 0584 MOV AL,[BP+6]; 0587 PUSH AX */
    caller_cs = c->cs; push(ram, c, c->cs); push(ram, c, 0x058Cu);                /* 0588 PUSH CS; 0589 CALL 031F */
    c->ip = 0x031Fu; memcpy(&leaf, c, sizeof leaf);
    if (pr_u3bd_in_grid_031f(ram, ram_size, &leaf) != PR_U3BD_031F_OK) {          /* the leaf rejected its entry: undo everything */
        for (i = 0u; i < 16u; ++i) wr8(ram, saved.ss, (uint16_t)(saved.sp - 16u + i), below[i]);
        *c = saved; return PR_U3BD_057A_ARGUMENT;
    }
    memcpy(c, &leaf, sizeof leaf);
    if (c->ip != 0x058Cu || c->cs != caller_cs) return PR_U3BD_057A_TRANSFER;
    or8(c, (uint8_t)c->ax);                                                       /* 058C OR AL,AL */
    if ((uint8_t)c->ax == 0u) {                                                   /* 058E JNE 05A4 */
        map = rd8(ram, c->ds, 0x84DCu); cmp8(c, map, 0u);                         /* 0590 CMP BYTE [84DC],0 */
        if (map == 0u) goto zero;                                                 /* 0595 JE 059E */
        map = rd8(ram, c->ds, 0x84DCu); cmp8(c, map, 0x0Au);                      /* 0597 CMP BYTE [84DC],0A */
        if (map == 0x0Au) goto zero;                                              /* 059C JNE 05A4 */
    }
    wrap(ram, c, 8u);                                                             /* 05A4..05B4: x at [BP+8] */
    wrap(ram, c, 6u);                                                             /* 05B8..05C8: y at [BP+6] */
    mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 8u))); cbw(c); c->dx = c->ax;   /* 05CC MOV AL,[BP+8]; CBW; MOV DX,AX */
    mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp + 6u))); cbw(c);                  /* 05D2 MOV AL,[BP+6]; CBW */
    c->cx = (uint16_t)((c->cx & 0xFF00u) | 4u); shl16(c, 4u);                     /* 05D6 MOV CL,4; SHL AX,CL */
    c->di = rd16(ram, c->ds, 0x6A5Cu); c->es = rd16(ram, c->ds, 0x6A5Eu);        /* 05DA LES DI,[6A5C] */
    c->di = add16(c, c->di, c->ax);                                              /* 05DE ADD DI,AX */
    c->di = add16(c, c->di, c->dx);                                              /* 05E0 ADD DI,DX */
    mov_al(c, pr_bus_read8(bus, physical(c->es, (uint16_t)(c->di + 0x0200u)))); /* 05E2 MOV AL,ES:[DI+200] (through the bus) */
    wr8(ram, c->ss, (uint16_t)(c->bp - 1u), (uint8_t)c->ax);                     /* 05E7 MOV [BP-1],AL */
    goto leave;
zero:
    wr8(ram, c->ss, (uint16_t)(c->bp - 1u), 0u);                                  /* 059E MOV BYTE [BP-1],0; 05A2 JMP 05EA */
leave:
    mov_al(c, rd8(ram, c->ss, (uint16_t)(c->bp - 1u)));                           /* 05EA MOV AL,[BP-1] */
    c->sp = c->bp;                                                                /* 05ED MOV SP,BP */
    c->bp = pop(ram, c);                                                          /* 05EF POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c); c->sp = (uint16_t)(c->sp + 4u);     /* 05F0 RETF 4 */
    if (c->ip != ret_ip || c->cs != ret_cs) return PR_U3BD_057A_TRANSFER;
    return PR_U3BD_057A_RETURNED;
}
