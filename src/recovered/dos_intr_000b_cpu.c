/* START.EXE 0A32:000B, Turbo Pascal Dos.Intr, raw CPU (docs/re/DOS_INTR_000B_CPU.md). */
#include "poolrad/recovered/dos_intr_000b_cpu.h"

#define MASK (PR_DOS_INTR_RAM_SIZE - 1u)

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
static void push(uint8_t *ram, pr_dos_intr_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, pr_dos_intr_cpu *c)
{
    uint16_t v = rd16(ram, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    return v;
}
/* LODSW with DF clear: AX := DS:[SI]; SI += 2. */
static uint16_t lodsw(const uint8_t *ram, pr_dos_intr_cpu *c)
{
    uint16_t v = rd16(ram, c->ds, c->si);
    c->si = (uint16_t)(c->si + 2u);
    return v;
}
/* STOSW with DF clear: ES:[DI] := AX; DI += 2. */
static void stosw(uint8_t *ram, pr_dos_intr_cpu *c)
{
    wr16(ram, c->es, c->di, c->ax);
    c->di = (uint16_t)(c->di + 2u);
}
static unsigned parity_even(unsigned byte)
{
    byte ^= byte >> 4u; byte ^= byte >> 2u; byte ^= byte >> 1u;
    return (~byte) & 1u;
}
/* SHL BX,1 on a value below 8000h: CF = 0, OF = MSB(result) ^ MSB(operand) = 0, SF = 0, ZF/PF from the
 * result, AF = 1 (DOSBox-X: AF set after any non-zero shift; architecturally undefined). */
static void shl_bx(pr_dos_intr_cpu *c)
{
    c->bx = (uint16_t)(c->bx << 1u);
    c->flags = (uint16_t)((c->flags & (uint16_t)~0x08D5u) | 0x0010u | (c->bx == 0u ? 0x0040u : 0u) |
                          (parity_even(c->bx & 0xFFu) ? 0x0004u : 0u));
}

pr_dos_intr_status pr_dos_intr_000b(uint8_t *ram, size_t ram_size, pr_dos_intr_cpu *c,
                                    pr_dos_intr_service service, void *context)
{
    pr_dos_intr_cpu at_handler, after;
    uint16_t body_cs, entry_sp;
    if (ram == NULL || c == NULL || service == NULL || ram_size != PR_DOS_INTR_RAM_SIZE || c->ip != 0x000Bu ||
        c->sp < 0x0014u || c->sp > 0xFFF6u)
        return PR_DOS_INTR_ARGUMENT;
    body_cs = c->cs;
    push(ram, c, c->bp);                                              /* 000B PUSH BP */
    push(ram, c, c->ds);                                              /* 000C PUSH DS */
    c->bp = c->sp;                                                    /* 000D MOV BP,SP */
    push(ram, c, c->flags);                                           /* 000F PUSHF */
    c->bx = 0x0045u;                                                  /* 0010 MOV BX,0045h */
    push(ram, c, c->cs);                                              /* 0013 PUSH CS */
    push(ram, c, c->bx);                                              /* 0014 PUSH BX */
    /* 0015 XOR BX,BX: CF OF SF AF clear, ZF PF set. */
    c->bx = 0u;
    c->flags = (uint16_t)((c->flags & (uint16_t)~0x08D5u) | 0x0044u);
    c->ds = 0u;                                                       /* 0017 MOV DS,BX */
    c->bx = (uint16_t)(ram[physical(c->ss, (uint16_t)(c->bp + 0x0Cu))]);  /* 0019 MOV BL,[BP+0Ch] (BH is 0) */
    shl_bx(c);                                                        /* 001C SHL BX,1 */
    shl_bx(c);                                                        /* 001E SHL BX,1 */
    {   /* 0020 LDS BX,[BX] (DS = 0: the interrupt vector) */
        uint16_t off = rd16(ram, c->ds, c->bx), seg = rd16(ram, c->ds, (uint16_t)(c->bx + 2u));
        c->bx = off; c->ds = seg;
    }
    push(ram, c, c->ds);                                              /* 0022 PUSH DS */
    push(ram, c, c->bx);                                              /* 0023 PUSH BX */
    {   /* 0024 LDS SI,[BP+8] (SS-relative: the Regs pointer) */
        uint16_t off = rd16(ram, c->ss, (uint16_t)(c->bp + 8u)), seg = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Au));
        c->si = off; c->ds = seg;
    }
    c->flags &= (uint16_t)~0x0400u;                                   /* 0027 CLD */
    c->ax = lodsw(ram, c); push(ram, c, c->ax);                       /* 0028 LODSW; PUSH AX (Regs.AX) */
    c->ax = lodsw(ram, c); c->bx = c->ax;                             /* 002A LODSW; MOV BX,AX */
    c->ax = lodsw(ram, c); c->cx = c->ax;                             /* 002D LODSW; MOV CX,AX */
    c->ax = lodsw(ram, c); c->dx = c->ax;                             /* 0030 LODSW; MOV DX,AX */
    c->ax = lodsw(ram, c); c->bp = c->ax;                             /* 0033 LODSW; MOV BP,AX */
    c->ax = lodsw(ram, c); push(ram, c, c->ax);                       /* 0036 LODSW; PUSH AX (Regs.SI) */
    c->ax = lodsw(ram, c); c->di = c->ax;                             /* 0038 LODSW; MOV DI,AX */
    c->ax = lodsw(ram, c); push(ram, c, c->ax);                       /* 003B LODSW; PUSH AX (Regs.DS) */
    c->ax = lodsw(ram, c); c->es = c->ax;                             /* 003D LODSW; MOV ES,AX */
    c->ds = pop(ram, c);                                              /* 0040 POP DS */
    c->si = pop(ram, c);                                              /* 0041 POP SI */
    c->ax = pop(ram, c);                                              /* 0042 POP AX */
    c->flags &= (uint16_t)~0x0200u;                                   /* 0043 CLI */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 0044 RETF: transfer to the handler */
    at_handler = *c;
    entry_sp = c->sp;
    if (service(context, ram, &at_handler, &after) != 0 || after.cs != body_cs || after.ip != 0x0045u ||
        after.ss != c->ss || after.sp != (uint16_t)(entry_sp + 6u))
        return PR_DOS_INTR_SERVICE;
    *c = after;
    push(ram, c, c->flags);                                           /* 0045 PUSHF */
    push(ram, c, c->es);                                              /* 0046 PUSH ES */
    push(ram, c, c->di);                                              /* 0047 PUSH DI */
    push(ram, c, c->bp);                                              /* 0048 PUSH BP */
    c->bp = c->sp;                                                    /* 0049 MOV BP,SP */
    {   /* 004B LES DI,[BP+10h] (the Regs pointer again) */
        uint16_t off = rd16(ram, c->ss, (uint16_t)(c->bp + 0x10u)), seg = rd16(ram, c->ss, (uint16_t)(c->bp + 0x12u));
        c->di = off; c->es = seg;
    }
    c->flags &= (uint16_t)~0x0400u;                                   /* 004E CLD */
    stosw(ram, c);                                                    /* 004F STOSW (AX) */
    c->ax = c->bx; stosw(ram, c);                                     /* 0050 MOV AX,BX; STOSW */
    c->ax = c->cx; stosw(ram, c);                                     /* 0053 MOV AX,CX; STOSW */
    c->ax = c->dx; stosw(ram, c);                                     /* 0056 MOV AX,DX; STOSW */
    c->ax = pop(ram, c); stosw(ram, c);                               /* 0059 POP AX (handler BP); STOSW */
    c->ax = c->si; stosw(ram, c);                                     /* 005B MOV AX,SI; STOSW */
    c->ax = pop(ram, c); stosw(ram, c);                               /* 005E POP AX (handler DI); STOSW */
    c->ax = c->ds; stosw(ram, c);                                     /* 0060 MOV AX,DS; STOSW */
    c->ax = pop(ram, c); stosw(ram, c);                               /* 0063 POP AX (handler ES); STOSW */
    c->ax = pop(ram, c); stosw(ram, c);                               /* 0065 POP AX (handler FLAGS); STOSW */
    c->ds = pop(ram, c);                                              /* 0067 POP DS */
    c->bp = pop(ram, c);                                              /* 0068 POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 0069 RETF 6 */
    c->sp = (uint16_t)(c->sp + 6u);
    return PR_DOS_INTR_RETURNED;
}
