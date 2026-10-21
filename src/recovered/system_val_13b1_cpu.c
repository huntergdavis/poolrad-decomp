/* START.EXE 0AF8:13B1 + 12CE, Turbo Pascal SYSTEM Val (longint), raw CPU (docs/re/SYSTEM_VAL_13B1_CPU.md). */
#include "poolrad/recovered/system_val_13b1_cpu.h"

#define MASK (PR_VAL_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_val_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, pr_val_cpu *c)
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
static void set(pr_val_cpu *c, unsigned mask, unsigned bits) { c->flags = (uint16_t)((c->flags & ~mask) | bits); }
static unsigned szp(unsigned r, unsigned msb) { return (r == 0u ? ZF : 0u) | (r & msb ? SF : 0u) | parity(r); }
static unsigned flag(pr_val_cpu *c, unsigned f) { return (c->flags & f) != 0u; }

/* ADD/ADC/SUB/CMP of width w (8 or 16): returns the result, sets all six flags. */
static unsigned arith(pr_val_cpu *c, unsigned a, unsigned b, unsigned carry, int sub, unsigned w)
{
    unsigned m = w == 8u ? 0xFFu : 0xFFFFu, msb = w == 8u ? 0x80u : 0x8000u, r, cf, of;
    if (sub) { r = (a - b - carry) & m; cf = a < b + carry; of = ((a ^ b) & (a ^ r) & msb) != 0u; }
    else { r = (a + b + carry) & m; cf = a + b + carry > m; of = (~(a ^ b) & (a ^ r) & msb) != 0u; }
    set(c, ARITH, szp(r, msb) | (cf ? CF : 0u) | ((a ^ b ^ r) & AF) | (of ? OF : 0u));
    return r;
}
static unsigned logic(pr_val_cpu *c, unsigned r, unsigned w) { set(c, ARITH, szp(r, w == 8u ? 0x80u : 0x8000u)); return r; }
/* INC/DEC of width w: CF kept. */
static unsigned incdec(pr_val_cpu *c, unsigned a, int dec, unsigned w)
{
    unsigned m = w == 8u ? 0xFFu : 0xFFFFu, msb = w == 8u ? 0x80u : 0x8000u, r = (dec ? a - 1u : a + 1u) & m;
    set(c, ARITH & ~CF, szp(r, msb) | ((a ^ 1u ^ r) & AF) | ((dec ? a == msb : a == msb - 1u) ? OF : 0u));
    return r;
}
/* SHL r16,1: CF = old bit 15, OF = MSB(result) ^ CF, SZP from the result, AF = 1 (DOSBox-X; overwritten before use). */
static uint16_t shl1(pr_val_cpu *c, uint16_t a)
{
    unsigned r = (unsigned)(a << 1u) & 0xFFFFu, cf = a >> 15u;
    set(c, ARITH, szp(r, 0x8000u) | AF | (cf ? CF : 0u) | (((r >> 15u) ^ cf) ? OF : 0u));
    return (uint16_t)r;
}
/* RCL r16,1: only CF and OF change; OF = MSB(result) ^ new CF. */
static uint16_t rcl1(pr_val_cpu *c, uint16_t a)
{
    unsigned r = ((unsigned)(a << 1u) | flag(c, CF)) & 0xFFFFu, cf = a >> 15u;
    set(c, CF | OF, (cf ? CF : 0u) | (((r >> 15u) ^ cf) ? OF : 0u));
    return (uint16_t)r;
}
static uint8_t byte_at(const uint8_t *ram, const pr_val_cpu *c) { return ram[physical(c->es, c->di)]; }
static void bl(pr_val_cpu *c, unsigned v) { c->bx = (uint16_t)((c->bx & 0xFF00u) | (v & 0xFFu)); }

/* The near parser 12CE..1365; returns with RET executed (the pushed 13CB popped), CF the error flag. */
static void parse(uint8_t *ram, pr_val_cpu *c)
{
    c->ax = (uint16_t)logic(c, 0u, 16u);                              /* 12CE XOR AX,AX */
    c->dx = (uint16_t)logic(c, 0u, 16u);                              /* 12D0 XOR DX,DX */
    c->si = (uint16_t)logic(c, 0u, 16u);                              /* 12D2 XOR SI,SI */
    if (c->cx == 0u) goto fail;                                       /* 12D4 JCXZ 132F */
    arith(c, byte_at(ram, c), 0x2Bu, 0u, 1, 8u);                      /* 12D6 CMP ES:[DI],'+' */
    if (!flag(c, ZF)) {                                               /* 12DA JE 12E3 */
        arith(c, byte_at(ram, c), 0x2Du, 0u, 1, 8u);                  /* 12DC CMP ES:[DI],'-' */
        if (!flag(c, ZF)) goto digits;                                /* 12E0 JNE 12E7 */
        c->si = (uint16_t)incdec(c, c->si, 0, 16u);                   /* 12E2 INC SI */
    }
    c->di = (uint16_t)incdec(c, c->di, 0, 16u);                       /* 12E3 INC DI */
    c->cx = (uint16_t)incdec(c, c->cx, 1, 16u);                       /* 12E4 DEC CX */
    if (c->cx == 0u) goto fail;                                       /* 12E5 JCXZ 132F */
digits:
    arith(c, byte_at(ram, c), 0x24u, 0u, 1, 8u);                      /* 12E7 CMP ES:[DI],'$' */
    if (flag(c, ZF)) goto hex;                                        /* 12EB JE 1331 */
    for (;;) {                                                        /* decimal loop at 12ED */
        bl(c, byte_at(ram, c));                                       /* 12ED MOV BL,ES:[DI] */
        bl(c, arith(c, c->bx & 0xFFu, 0x3Au, 0u, 1, 8u));             /* 12F0 SUB BL,3Ah */
        bl(c, arith(c, c->bx & 0xFFu, 0x0Au, 0u, 0, 8u));             /* 12F3 ADD BL,0Ah */
        if (!flag(c, CF)) goto finish;                                /* 12F6 JAE 1320: not a digit */
        logic(c, c->dx & 0xF000u, 16u);                               /* 12F8 TEST DX,F000h */
        if (!flag(c, ZF)) goto fail;                                  /* 12FC JNE 132F */
        push(ram, c, c->bx);                                          /* 12FE PUSH BX */
        c->ax = shl1(c, c->ax); c->dx = rcl1(c, c->dx);               /* 12FF SHL AX,1; 1301 RCL DX,1 */
        push(ram, c, c->dx); push(ram, c, c->ax);                     /* 1303 PUSH DX; 1304 PUSH AX */
        c->ax = shl1(c, c->ax); c->dx = rcl1(c, c->dx);               /* 1305 SHL AX,1; 1307 RCL DX,1 */
        c->ax = shl1(c, c->ax); c->dx = rcl1(c, c->dx);               /* 1309 SHL AX,1; 130B RCL DX,1 */
        c->bx = pop(ram, c);                                          /* 130D POP BX */
        c->ax = (uint16_t)arith(c, c->ax, c->bx, 0u, 0, 16u);         /* 130E ADD AX,BX */
        c->bx = pop(ram, c);                                          /* 1310 POP BX */
        c->dx = (uint16_t)arith(c, c->dx, c->bx, flag(c, CF), 0, 16u);   /* 1311 ADC DX,BX */
        c->bx = pop(ram, c);                                          /* 1313 POP BX */
        c->bx = (uint16_t)((c->bx & 0x00FFu) | (logic(c, 0u, 8u) << 8u));   /* 1314 XOR BH,BH */
        c->ax = (uint16_t)arith(c, c->ax, c->bx, 0u, 0, 16u);         /* 1316 ADD AX,BX */
        c->dx = (uint16_t)arith(c, c->dx, 0u, flag(c, CF), 0, 16u);   /* 1318 ADC DX,0 */
        if (flag(c, SF)) goto fail;                                   /* 131B JS 132F */
        c->di = (uint16_t)incdec(c, c->di, 0, 16u);                   /* 131D INC DI */
        c->cx = (uint16_t)(c->cx - 1u);                               /* 131E LOOP 12ED */
        if (c->cx == 0u) goto finish;
    }
hex:
    c->di = (uint16_t)incdec(c, c->di, 0, 16u);                       /* 1331 INC DI */
    c->cx = (uint16_t)incdec(c, c->cx, 1, 16u);                       /* 1332 DEC CX */
    if (c->cx == 0u) goto fail;                                       /* 1333 JCXZ 132F */
    for (;;) {                                                        /* hex loop at 1335 */
        bl(c, byte_at(ram, c));                                       /* 1335 MOV BL,ES:[DI] */
        arith(c, c->bx & 0xFFu, 0x61u, 0u, 1, 8u);                    /* 1338 CMP BL,'a' */
        if (!flag(c, CF))                                             /* 133B JB 1340 */
            bl(c, arith(c, c->bx & 0xFFu, 0x20u, 0u, 1, 8u));         /* 133D SUB BL,20h */
        bl(c, arith(c, c->bx & 0xFFu, 0x3Au, 0u, 1, 8u));             /* 1340 SUB BL,3Ah */
        bl(c, arith(c, c->bx & 0xFFu, 0x0Au, 0u, 0, 8u));             /* 1343 ADD BL,0Ah */
        if (!flag(c, CF)) {                                           /* 1346 JB 1353 */
            bl(c, arith(c, c->bx & 0xFFu, 0x17u, 0u, 1, 8u));         /* 1348 SUB BL,17h */
            bl(c, arith(c, c->bx & 0xFFu, 0x06u, 0u, 0, 8u));         /* 134B ADD BL,6 */
            if (!flag(c, CF)) goto finish;                            /* 134E JAE 1320: not a hex digit */
            bl(c, arith(c, c->bx & 0xFFu, 0x0Au, 0u, 0, 8u));         /* 1350 ADD BL,0Ah */
        }
        c->bx = (uint16_t)((c->bx & 0x00FFu) | 0x0400u);              /* 1353 MOV BH,4 */
        do {
            c->ax = shl1(c, c->ax); c->dx = rcl1(c, c->dx);           /* 1355 SHL AX,1; 1357 RCL DX,1 */
            if (flag(c, CF)) goto fail;                               /* 1359 JB 132F */
            c->bx = (uint16_t)((c->bx & 0x00FFu) | (incdec(c, c->bx >> 8u, 1, 8u) << 8u));   /* 135B DEC BH */
        } while ((c->bx >> 8u) != 0u);                                /* 135D JNE 1355 */
        c->ax = (uint16_t)((c->ax & 0xFF00u) | logic(c, (c->ax | c->bx) & 0xFFu, 8u));   /* 135F OR AL,BL */
        c->di = (uint16_t)incdec(c, c->di, 0, 16u);                   /* 1361 INC DI */
        c->cx = (uint16_t)(c->cx - 1u);                               /* 1362 LOOP 1335 */
        if (c->cx == 0u) goto finish;                                 /* 1364 JMP 1320 */
    }
finish:
    c->si = (uint16_t)incdec(c, c->si, 1, 16u);                       /* 1320 DEC SI */
    if (flag(c, ZF)) {                                                /* 1321 JNE 132D */
        c->ax = (uint16_t)~c->ax; c->dx = (uint16_t)~c->dx;           /* 1323 NOT AX; 1325 NOT DX */
        c->ax = (uint16_t)arith(c, c->ax, 1u, 0u, 0, 16u);            /* 1327 ADD AX,1 */
        c->dx = (uint16_t)arith(c, c->dx, 0u, flag(c, CF), 0, 16u);   /* 132A ADC DX,0 */
    }
    set(c, CF, 0u);                                                   /* 132D CLC */
    c->ip = pop(ram, c);                                              /* 132E RET */
    return;
fail:
    set(c, CF, CF);                                                   /* 132F STC */
    c->ip = pop(ram, c);                                              /* 1330 RET */
}

pr_val_status pr_system_val_13b1(uint8_t *ram, size_t ram_size, pr_val_cpu *c)
{
    uint16_t s_off;
    if (ram == NULL || c == NULL || ram_size != PR_VAL_RAM_SIZE || c->ip != 0x13B1u || c->sp < 0x000Au || c->sp > 0xFFF4u)
        return PR_VAL_ARGUMENT;
    push(ram, c, c->bp);                                              /* 13B1 PUSH BP */
    c->bp = c->sp;                                                    /* 13B2 MOV BP,SP */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Au));              /* 13B4 LES DI,[BP+0Ah] */
    c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Cu));
    c->cx = (uint16_t)((c->cx & 0xFF00u) | byte_at(ram, c));         /* 13B7 MOV CL,ES:[DI] */
    c->cx = (uint16_t)(c->cx & 0x00FFu); logic(c, 0u, 8u);            /* 13BA XOR CH,CH */
    c->di = (uint16_t)incdec(c, c->di, 0, 16u);                       /* 13BC INC DI */
    while (c->cx != 0u) {                                             /* 13BD JCXZ 13C8 */
        arith(c, byte_at(ram, c), 0x20u, 0u, 1, 8u);                  /* 13BF CMP ES:[DI],20h */
        if (!flag(c, ZF)) break;                                      /* 13C3 JNE 13C8 */
        c->di = (uint16_t)incdec(c, c->di, 0, 16u);                   /* 13C5 INC DI */
        c->cx = (uint16_t)(c->cx - 1u);                               /* 13C6 LOOP 13BF */
    }
    push(ram, c, 0x13CBu);                                            /* 13C8 CALL 12CE */
    parse(ram, c);
    if (flag(c, CF) || c->cx != 0u) {                                 /* 13CB JB 13CF; 13CD JCXZ 13D8 */
        s_off = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Au));
        c->cx = c->di;                                                /* 13CF MOV CX,DI */
        c->cx = (uint16_t)arith(c, c->cx, s_off, 0u, 1, 16u);         /* 13D1 SUB CX,[BP+0Ah] */
        c->ax = (uint16_t)logic(c, 0u, 16u);                          /* 13D4 XOR AX,AX */
        c->dx = (uint16_t)logic(c, 0u, 16u);                          /* 13D6 XOR DX,DX */
    }
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));                 /* 13D8 LES DI,[BP+6] */
    c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));
    wr16(ram, c->es, c->di, c->cx);                                   /* 13DB MOV ES:[DI],CX */
    c->bp = pop(ram, c);                                              /* 13DE POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 13DF RETF 8 */
    c->sp = (uint16_t)(c->sp + 8u);
    return PR_VAL_RETURNED;
}
