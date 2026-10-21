/* START.EXE 0AF8:1563 + 14B6, Turbo Pascal SYSTEM Seek(f, n), raw CPU (docs/re/SYSTEM_SEEK_1563_CPU.md). */
#include "poolrad/recovered/system_seek_1563_cpu.h"

#define MASK (PR_SEEK_RAM_SIZE - 1u)
#define CF 0x0001u
#define PF 0x0004u
#define AF 0x0010u
#define ZF 0x0040u
#define SF 0x0080u
#define OF 0x0800u
#define ARITH (CF | PF | AF | ZF | SF | OF)

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
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
static void push(uint8_t *ram, pr_seek_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, pr_seek_cpu *c)
{
    uint16_t v = rd16(ram, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    return v;
}
static unsigned parity(unsigned v)
{
    v &= 0xFFu; v ^= v >> 4u; v ^= v >> 2u; v ^= v >> 1u;
    return (v & 1u) ? 0u : PF;
}
static void arith(pr_seek_cpu *c, unsigned a, unsigned b, unsigned r, int sub)
{
    unsigned cf = sub ? a < b : a + b > 0xFFFFu, of = sub ? ((a ^ b) & (a ^ r) & 0x8000u) != 0u : (~(a ^ b) & (a ^ r) & 0x8000u) != 0u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & 0x8000u ? SF : 0u) | parity(r) | (cf ? CF : 0u) |
                          ((a ^ b ^ r) & AF) | (of ? OF : 0u));
}
/* MUL r/m16: DX:AX := AX * operand; CF = OF = (DX != 0). SF/ZF/AF/PF are undefined and overwritten before use (the ADD);
 * they are set as DOSBox-X's 386 core does (ZF from AX, PF from all 32 bits, SF and AF kept). */
static void mul16(pr_seek_cpu *c, uint16_t operand)
{
    uint32_t r = (uint32_t)c->ax * operand, x = r;
    c->ax = (uint16_t)r; c->dx = (uint16_t)(r >> 16u);
    x ^= x >> 16u;
    c->flags = (uint16_t)((c->flags & ~(CF | OF | ZF | PF)) | (c->dx != 0u ? CF | OF : 0u) | (c->ax == 0u ? ZF : 0u) | parity(x));
}

pr_seek_status pr_system_seek_1563(uint8_t *ram, size_t ram_size, pr_seek_cpu *c, pr_seek_dos_service service, void *context)
{
    pr_seek_cpu at_int, after;
    uint16_t recsize;
    if (ram == NULL || c == NULL || service == NULL || ram_size != PR_SEEK_RAM_SIZE || c->ip != 0x1563u || c->sp < 0x0006u ||
        c->sp > 0xFFF4u)
        return PR_SEEK_ARGUMENT;
    push(ram, c, c->bp); c->bp = c->sp;                               /* 1563 PUSH BP; 1564 MOV BP,SP */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Au));              /* 1566 LES DI,[BP+0Ah] */
    c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Cu));
    push(ram, c, 0x156Cu);                                            /* 1569 CALL 14B6 */
    {   /* 14B6 CMP WORD ES:[DI+2],D7B3h; 14BC JE 14C4; 14BE MOV [4426],103; 14C4 RET */
        unsigned a = rd16(ram, c->es, (uint16_t)(c->di + 2u)), r = (a - 0xD7B3u) & 0xFFFFu;
        arith(c, a, 0xD7B3u, r, 1);
        if (r != 0u)
            wr16(ram, c->ds, 0x4426u, 0x0067u);
        c->sp = (uint16_t)(c->sp + 2u);
    }
    if ((c->flags & ZF) != 0u) {                                      /* 156C JNE 158F */
        recsize = rd16(ram, c->es, (uint16_t)(c->di + 4u));
        c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));             /* 156E MOV AX,[BP+8] (n high) */
        mul16(c, recsize);                                            /* 1571 MUL WORD ES:[DI+4] */
        c->cx = c->ax;                                                /* 1575 MOV CX,AX (DX dropped) */
        c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));             /* 1577 MOV AX,[BP+6] (n low) */
        mul16(c, rd16(ram, c->es, (uint16_t)(c->di + 4u)));           /* 157A MUL WORD ES:[DI+4] */
        {   unsigned a = c->cx, b = c->dx, r = (a + b) & 0xFFFFu;     /* 157E ADD CX,DX */
            arith(c, a, b, r, 0); c->cx = (uint16_t)r; }
        c->dx = c->ax;                                                /* 1580 MOV DX,AX */
        c->bx = rd16(ram, c->es, c->di);                              /* 1582 MOV BX,ES:[DI] (handle) */
        c->ax = 0x4200u;                                              /* 1585 MOV AX,4200h */
        c->ip = 0x1588u;                                              /* 1588 INT 21h (declared) */
        if ((c->ax >> 8u) != 0x42u)
            return PR_SEEK_ENV;
        at_int = *c;
        if (service(context, &at_int, &after) != 0 || after.cs != c->cs || after.ip != 0x158Au || after.ss != c->ss ||
            after.sp != c->sp || after.bp != c->bp)
            return PR_SEEK_SERVICE;
        *c = after;
        if ((c->flags & CF) != 0u)                                    /* 158A JAE 158F */
            wr16(ram, c->ds, 0x4426u, c->ax);                         /* 158C MOV [4426],AX */
    }
    c->bp = pop(ram, c);                                              /* 158F POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 1590 RETF 8 */
    c->sp = (uint16_t)(c->sp + 8u);
    return PR_SEEK_RETURNED;
}
