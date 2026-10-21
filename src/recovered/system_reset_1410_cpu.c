/* START.EXE 0AF8:1410, Turbo Pascal SYSTEM Reset(f, recsize), raw CPU (docs/re/SYSTEM_RESET_1410_CPU.md). */
#include "poolrad/recovered/system_reset_1410_cpu.h"

#define MASK (PR_RESET_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_reset_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, pr_reset_cpu *c)
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
/* CMP a,b of width w (8 or 16). */
static void cmp(pr_reset_cpu *c, unsigned a, unsigned b, unsigned w)
{
    unsigned m = w == 8u ? 0xFFu : 0xFFFFu, msb = w == 8u ? 0x80u : 0x8000u, r = (a - b) & m;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & msb ? SF : 0u) | parity(r) | (a < b ? CF : 0u) |
                          ((a ^ b ^ r) & AF) | (((a ^ b) & (a ^ r) & msb) ? OF : 0u));
}

static pr_reset_status finish(uint8_t *ram, pr_reset_cpu *c)
{
    c->bp = pop(ram, c);                                              /* 1471 POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 1472 RETF 6 */
    c->sp = (uint16_t)(c->sp + 6u);
    return PR_RESET_RETURNED;
}

pr_reset_status pr_system_reset_1410(uint8_t *ram, size_t ram_size, pr_reset_cpu *c, pr_reset_dos_service service, void *context)
{
    if (ram == NULL || c == NULL || service == NULL || ram_size != PR_RESET_RAM_SIZE || c->sp > 0xFFF6u ||
        (c->ip == 0x1410u ? c->sp < 0x000Eu : c->ip == 0x1445u ? c->sp < 0x0006u : 1))
        return PR_RESET_ARGUMENT;
    if (c->ip == 0x1410u) {
        c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, 0x442Cu)]);   /* 1410 MOV AL,[442C] (FileMode) */
        c->ax = (uint16_t)((c->ax & 0x00FFu) | 0x3D00u);              /* 1413 MOV AH,3Dh */
        c->dx = 0u;                                                   /* 1415 XOR DX,DX */
        c->flags = (uint16_t)((c->flags & ~ARITH) | ZF | PF);
        push(ram, c, c->bp); c->bp = c->sp;                           /* 1417 JMP 141F; 141F PUSH BP; 1420 MOV BP,SP */
        c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));             /* 1422 LES DI,[BP+8] */
        c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Au));
        cmp(c, rd16(ram, c->es, (uint16_t)(c->di + 2u)), 0xD7B0u, 16u);   /* 1425 CMP ES:[DI+2],D7B0h */
        if ((c->flags & ZF) == 0u) {                                  /* 142B JE 1447 */
            cmp(c, rd16(ram, c->es, (uint16_t)(c->di + 2u)), 0xD7B3u, 16u);   /* 142D CMP ES:[DI+2],D7B3h */
            if ((c->flags & ZF) == 0u) {                              /* 1433 JE 143D */
                wr16(ram, c->ds, 0x4426u, 0x0066u);                   /* 1435 MOV [4426],102; 143B JMP 1471 */
                return finish(ram, c);
            }
            push(ram, c, c->ax); push(ram, c, c->dx);                 /* 143D PUSH AX; 143E PUSH DX */
            push(ram, c, c->es); push(ram, c, c->di);                 /* 143F PUSH ES; 1440 PUSH DI */
            push(ram, c, c->cs); push(ram, c, 0x1445u);               /* 1441 PUSH CS; 1442 CALL 1491 */
            c->ip = 0x1491u;
            return PR_RESET_CLOSE;
        }
    } else {
        c->dx = pop(ram, c);                                          /* 1445 POP DX */
        c->ax = pop(ram, c);                                          /* 1446 POP AX */
    }
    /* 1447 */
    cmp(c, ram[physical(c->es, (uint16_t)(c->di + 0x30u))], 0u, 8u);   /* 1447 CMP BYTE ES:[DI+30h],0 */
    if ((c->flags & ZF) != 0u) {                                      /* 144C JE 1460 */
        uint16_t t = c->dx; c->dx = c->ax; c->ax = t;                 /* 1460 XCHG DX,AX: the handle is DX (0) */
    } else {
        pr_reset_cpu at_int, after;
        push(ram, c, c->ds);                                          /* 144E PUSH DS */
        c->dx = (uint16_t)(c->di + 0x30u);                            /* 144F LEA DX,[DI+30h] */
        push(ram, c, c->es); c->ds = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);   /* 1452 PUSH ES; 1453 POP DS */
        c->cx = 0u;                                                   /* 1454 XOR CX,CX */
        c->flags = (uint16_t)((c->flags & ~ARITH) | ZF | PF);
        c->ip = 0x1456u;                                              /* 1456 INT 21h (declared) */
        if ((c->ax >> 8u) != 0x3Du)
            return PR_RESET_ENV;
        at_int = *c;
        if (service(context, &at_int, &after) != 0 || after.cs != c->cs || after.ip != 0x1458u || after.ss != c->ss ||
            after.sp != c->sp || after.bp != c->bp)
            return PR_RESET_SERVICE;
        *c = after;
        c->ds = pop(ram, c);                                          /* 1458 POP DS */
        if ((c->flags & CF) != 0u) {                                  /* 1459 JAE 1461 */
            wr16(ram, c->ds, 0x4426u, c->ax);                         /* 145B MOV [4426],AX; 145E JMP 1471 */
            return finish(ram, c);
        }
    }
    wr16(ram, c->es, (uint16_t)(c->di + 2u), 0xD7B3u);               /* 1461 MOV ES:[DI+2],D7B3h */
    wr16(ram, c->es, c->di, c->ax);                                   /* 1467 MOV ES:[DI],AX (the handle) */
    c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));                 /* 146A MOV AX,[BP+6] */
    wr16(ram, c->es, (uint16_t)(c->di + 4u), c->ax);                  /* 146D MOV ES:[DI+4],AX (recsize) */
    return finish(ram, c);
}
