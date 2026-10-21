/* START.EXE 0AF8:1593 + 15DA, Turbo Pascal SYSTEM FilePos(f), raw CPU (docs/re/SYSTEM_FILEPOS_1593_CPU.md). */
#include "poolrad/recovered/system_filepos_1593_cpu.h"
#include "poolrad/recovered/system_longdiv_0a76_cpu.h"

#define MASK (PR_FILEPOS_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_filepos_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, pr_filepos_cpu *c)
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
static void cmp16(pr_filepos_cpu *c, unsigned a, unsigned b)
{
    unsigned r = (a - b) & 0xFFFFu, of = ((a ^ b) & (a ^ r) & 0x8000u) != 0u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & 0x8000u ? SF : 0u) | parity(r) | (a < b ? CF : 0u) |
                          ((a ^ b ^ r) & AF) | (of ? OF : 0u));
}
/* XOR r16,r16 (same register): result 0; CF = OF = SF = AF = 0, ZF = PF = 1 (AF 0 as both DOSBox-X and Unicorn set it). */
static void zero16(pr_filepos_cpu *c, uint16_t *reg)
{
    *reg = 0u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | ZF | PF);
}
/* INT 21h at `at` (declared LSEEK): continuation at at + 2 with the same CS, SS, SP, BP. */
static pr_filepos_status lseek(pr_filepos_cpu *c, uint16_t at, pr_filepos_dos_service service, void *context)
{
    pr_filepos_cpu before, after;
    c->ip = at;
    if ((c->ax >> 8u) != 0x42u)
        return PR_FILEPOS_ENV;
    before = *c; after = *c;
    if (service(context, &before, &after) != 0 || after.cs != c->cs || after.ip != (uint16_t)(at + 2u) || after.ss != c->ss ||
        after.sp != c->sp || after.bp != c->bp)
        return PR_FILEPOS_SERVICE;
    *c = after;
    return PR_FILEPOS_RETURNED;
}
/* 15DA: the position helper (near; [SP] is the return IP). Returns RETURNED with the RET executed (IP := reread slot). */
static pr_filepos_status position(uint8_t *ram, pr_filepos_cpu *c, pr_filepos_dos_service service, void *context)
{
    pr_filepos_status s;
    c->bx = c->sp;                                                    /* 15DA MOV BX,SP */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bx + 6u));                 /* 15DC LES DI,SS:[BX+6] */
    c->es = rd16(ram, c->ss, (uint16_t)(c->bx + 8u));
    cmp16(c, rd16(ram, c->es, (uint16_t)(c->di + 2u)), 0xD7B3u);      /* 15E0 CMP WORD ES:[DI+2],D7B3h */
    if ((c->flags & ZF) == 0u) {                                      /* 15E6 JNE 1613 */
        wr16(ram, c->ds, 0x4426u, 0x0067u);                           /* 1613 MOV [4426],67h */
        zero16(c, &c->ax); zero16(c, &c->dx);                         /* 1619 XOR AX,AX; 161B XOR DX,DX */
        c->flags |= CF;                                               /* 161D STC */
    } else {
        zero16(c, &c->cx); zero16(c, &c->dx);                         /* 15E8 XOR CX,CX; 15EA XOR DX,DX */
        c->bx = rd16(ram, c->es, c->di);                              /* 15EC MOV BX,ES:[DI] */
        c->ax = 0x4201u;                                              /* 15EF MOV AX,4201h */
        if ((s = lseek(c, 0x15F2u, service, context)) != PR_FILEPOS_RETURNED)   /* 15F2 INT 21h: current */
            return s;
        push(ram, c, c->dx); push(ram, c, c->ax);                     /* 15F4 PUSH DX; 15F5 PUSH AX */
        zero16(c, &c->cx); zero16(c, &c->dx);                         /* 15F6 XOR CX,CX; 15F8 XOR DX,DX */
        c->bx = rd16(ram, c->es, c->di);                              /* 15FA MOV BX,ES:[DI] */
        c->ax = 0x4202u;                                              /* 15FD MOV AX,4202h */
        if ((s = lseek(c, 0x1600u, service, context)) != PR_FILEPOS_RETURNED)   /* 1600 INT 21h: end */
            return s;
        c->bx = pop(ram, c); c->cx = pop(ram, c);                     /* 1602 POP BX; 1603 POP CX */
        push(ram, c, c->dx); push(ram, c, c->ax);                     /* 1604 PUSH DX; 1605 PUSH AX */
        c->dx = c->bx;                                                /* 1606 MOV DX,BX */
        c->bx = rd16(ram, c->es, c->di);                              /* 1608 MOV BX,ES:[DI] */
        c->ax = 0x4200u;                                              /* 160B MOV AX,4200h */
        if ((s = lseek(c, 0x160Eu, service, context)) != PR_FILEPOS_RETURNED)   /* 160E INT 21h: restore */
            return s;
        c->cx = pop(ram, c); c->bx = pop(ram, c);                     /* 1610 POP CX; 1611 POP BX */
    }
    c->ip = pop(ram, c);                                              /* 1612 / 161E RET (the slot reread) */
    return PR_FILEPOS_RETURNED;
}
static void to_div(pr_system_longdiv_cpu *h, const pr_filepos_cpu *c)
{
    h->ax = c->ax; h->bx = c->bx; h->cx = c->cx; h->dx = c->dx; h->si = c->si; h->di = c->di; h->bp = c->bp; h->sp = c->sp;
    h->cs = c->cs; h->ip = c->ip; h->ds = c->ds; h->es = c->es; h->ss = c->ss; h->flags = c->flags;
}
static void from_div(pr_filepos_cpu *c, const pr_system_longdiv_cpu *h)
{
    c->ax = h->ax; c->bx = h->bx; c->cx = h->cx; c->dx = h->dx; c->si = h->si; c->di = h->di; c->bp = h->bp; c->sp = h->sp;
    c->cs = h->cs; c->ip = h->ip; c->ds = h->ds; c->es = h->es; c->ss = h->ss; c->flags = h->flags;
}

pr_filepos_status pr_system_filepos_1593(uint8_t *ram, size_t ram_size, pr_filepos_cpu *c, pr_filepos_dos_service service, void *context)
{
    pr_filepos_status s;
    uint16_t cs;
    if (ram == NULL || c == NULL || service == NULL || ram_size != PR_FILEPOS_RAM_SIZE || c->ip != 0x1593u || c->sp < 0x000Au ||
        c->sp > 0xFFF8u)
        return PR_FILEPOS_ARGUMENT;
    cs = c->cs;
    push(ram, c, 0x1596u);                                            /* 1593 CALL 15DA */
    if ((s = position(ram, c, service, context)) != PR_FILEPOS_RETURNED)
        return s;
    if (c->ip != 0x1596u)
        return PR_FILEPOS_TRANSFER;
    if ((c->flags & CF) == 0u) {                                      /* 1596 JB 15A7 */
        c->cx = rd16(ram, c->es, (uint16_t)(c->di + 4u));             /* 1598 MOV CX,ES:[DI+4] (recsize) */
        cmp16(c, c->cx, 1u);                                          /* 159C CMP CX,1 */
        if ((c->flags & ZF) == 0u) {                                  /* 159F JE 15A7 */
            pr_system_longdiv_cpu h;
            pr_system_longdiv_status d;
            zero16(c, &c->bx);                                        /* 15A1 XOR BX,BX */
            push(ram, c, c->cs); push(ram, c, 0x15A7u);               /* 15A3 PUSH CS; 15A4 CALL 0A76 */
            c->ip = 0x0A76u;
            to_div(&h, c);
            d = pr_system_longdiv_0a76(ram, ram_size, &h);            /* LongDiv (covered): DX:AX := DX:AX DIV BX:CX */
            from_div(c, &h);
            if (d == PR_SYSTEM_LONGDIV_DIVIDE_ERROR)
                return PR_FILEPOS_DIVIDE;
            if (d != PR_SYSTEM_LONGDIV_RETURNED || c->cs != cs || c->ip != 0x15A7u)   /* ARGUMENT unreachable: RAM checked */
                return PR_FILEPOS_TRANSFER;
        }
    }
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 15A7 RETF 4 */
    c->sp = (uint16_t)(c->sp + 4u);
    return PR_FILEPOS_RETURNED;
}
