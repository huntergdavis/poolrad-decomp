/* START.EXE 0AF8:14FB, Turbo Pascal SYSTEM BlockRead(f, buf, count, result), raw CPU (docs/re/SYSTEM_BLOCKREAD_14FB_CPU.md). */
#include "poolrad/recovered/system_blockread_14fb_cpu.h"
#include "poolrad/recovered/system_filecheck_14b6_cpu.h"

#define MASK (PR_BLOCKREAD_RAM_SIZE - 1u)
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
static void push(uint8_t *ram, pr_blockread_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static uint16_t pop(const uint8_t *ram, pr_blockread_cpu *c)
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
static void cmp16(pr_blockread_cpu *c, unsigned a, unsigned b)
{
    unsigned r = (a - b) & 0xFFFFu, of = ((a ^ b) & (a ^ r) & 0x8000u) != 0u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & 0x8000u ? SF : 0u) | parity(r) | (a < b ? CF : 0u) |
                          ((a ^ b ^ r) & AF) | (of ? OF : 0u));
}
/* OR/XOR r16,r16: CF = OF = 0, AF = 0 (as both DOSBox-X and Unicorn set it), SF/ZF/PF from the result. */
static void logic16(pr_blockread_cpu *c, unsigned r)
{
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? ZF : 0u) | (r & 0x8000u ? SF : 0u) | parity(r));
}
/* MUL r/m16: DX:AX := AX * operand; CF = OF = (DX != 0). SF/ZF/AF/PF are undefined; set as DOSBox-X's 386 core does
 * (ZF from AX, PF from all 32 bits, SF and AF kept). They reach only the service's input flags. */
static void mul16(pr_blockread_cpu *c, uint16_t operand)
{
    uint32_t r = (uint32_t)c->ax * operand, x = r;
    c->ax = (uint16_t)r; c->dx = (uint16_t)(r >> 16u);
    x ^= x >> 16u; x ^= x >> 8u; /* pinned MULW: PF is the even parity of all four bytes of DX:AX */
    c->flags = (uint16_t)((c->flags & ~(CF | OF | ZF | PF)) | (c->dx != 0u ? CF | OF : 0u) | (c->ax == 0u ? ZF : 0u) | parity(x));
}
static void to_check(pr_system_filecheck_cpu *h, const pr_blockread_cpu *c)
{
    h->ax = c->ax; h->bx = c->bx; h->cx = c->cx; h->dx = c->dx; h->si = c->si; h->di = c->di; h->bp = c->bp; h->sp = c->sp;
    h->cs = c->cs; h->ip = c->ip; h->ds = c->ds; h->es = c->es; h->ss = c->ss; h->flags = c->flags;
}
static void from_check(pr_blockread_cpu *c, const pr_system_filecheck_cpu *h)
{
    c->ax = h->ax; c->bx = h->bx; c->cx = h->cx; c->dx = h->dx; c->si = h->si; c->di = h->di; c->bp = h->bp; c->sp = h->sp;
    c->cs = h->cs; c->ip = h->ip; c->ds = h->ds; c->es = h->es; c->ss = h->ss; c->flags = h->flags;
}
/* 1551: LES DI,[BP+6]; MOV DX,ES; OR DX,DI; JE 155F; XOR AX,AX; MOV ES:[DI],AX (falls into 155F). */
static void result_zero(uint8_t *ram, pr_blockread_cpu *c)
{
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 6u)); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));
    c->dx = (uint16_t)(c->es | c->di); logic16(c, c->dx);
    if (c->dx != 0u) {
        c->ax = 0u; logic16(c, 0u);
        wr16(ram, c->es, c->di, 0u);
    }
}

pr_blockread_status pr_system_blockread_14fb(uint8_t *ram, size_t ram_size, pr_blockread_cpu *c, pr_blockread_dos_service service,
                                             void *context)
{
    uint8_t staging[0xFFFFu];                                         /* the permitted write, handed to the service */
    pr_blockread_cpu at_int, after;
    if (ram == NULL || c == NULL || service == NULL || ram_size != PR_BLOCKREAD_RAM_SIZE || c->ip != 0x14FBu || c->sp < 0x0006u ||
        c->sp > 0xFFEEu)
        return PR_BLOCKREAD_ARGUMENT;
    c->bx = (uint16_t)((c->bx & 0xFF00u) | 0x3Fu);                   /* 14FB MOV BL,3Fh */
    c->cx = 0x0064u;                                                  /* 14FD MOV CX,64h; 1500 JMP 1507 */
    push(ram, c, c->bp); c->bp = c->sp;                               /* 1507 PUSH BP; 1508 MOV BP,SP */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 0x10u));              /* 150A LES DI,[BP+10h] */
    c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 0x12u));
    push(ram, c, 0x1510u); c->ip = 0x14B6u;                           /* 150D CALL 14B6 */
    {   /* par-d's shared check: InOutRes := 103 unless the magic is D7B3h; the CMP's flags; RET rereads its slot. */
        pr_system_filecheck_cpu h;
        to_check(&h, c);
        if (pr_system_filecheck_14b6(ram, ram_size, &h) != PR_SYSTEM_FILECHECK_RETURNED)
            return PR_BLOCKREAD_TRANSFER;                             /* unreachable: SP <= FFEAh here */
        from_check(c, &h);
        if (c->ip != 0x1510u)
            return PR_BLOCKREAD_TRANSFER;
    }
    if ((c->flags & ZF) == 0u) {                                      /* 1510 JNE 1551 */
        result_zero(ram, c);
        goto done;
    }
    c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Au));              /* 1512 MOV AX,[BP+0Ah] (count) */
    logic16(c, c->ax);                                                /* 1515 OR AX,AX */
    if (c->ax != 0u) {                                                /* 1517 JE 1535 */
        size_t base, k;
        push(ram, c, c->ds); push(ram, c, c->cx);                     /* 1519 PUSH DS; 151A PUSH CX */
        mul16(c, rd16(ram, c->es, (uint16_t)(c->di + 4u)));           /* 151B MUL WORD ES:[DI+4] */
        c->cx = c->ax;                                                /* 151F MOV CX,AX (DX dropped) */
        c->dx = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Cu));          /* 1521 LDS DX,[BP+0Ch] */
        c->ds = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Eu));
        c->ax = (uint16_t)((c->ax & 0x00FFu) | (uint16_t)((c->bx & 0xFFu) << 8u));   /* 1524 MOV AH,BL */
        c->bx = rd16(ram, c->es, c->di);                              /* 1526 MOV BX,ES:[DI] (handle) */
        c->ip = 0x1529u;                                              /* 1529 INT 21h (declared) */
        if ((c->ax >> 8u) != 0x3Fu)
            return PR_BLOCKREAD_ENV;
        at_int = *c; after = *c;
        base = physical(c->ds, c->dx);
        for (k = 0; k < c->cx; ++k)
            staging[k] = ram[(base + k) & MASK];
        if (service(context, &at_int, &after, staging, c->cx) != 0 || after.cs != c->cs || after.ip != 0x152Bu ||
            after.ss != c->ss || after.sp != c->sp || after.bp != c->bp || ((after.flags & CF) == 0u && after.ax > c->cx))
            return PR_BLOCKREAD_SERVICE;
        for (k = 0; k < c->cx; ++k)
            ram[(base + k) & MASK] = staging[k];
        *c = after;
        c->cx = pop(ram, c); c->ds = pop(ram, c);                     /* 152B POP CX; 152C POP DS */
        if ((c->flags & CF) != 0u) {                                  /* 152D JB 154E */
            wr16(ram, c->ds, 0x4426u, c->ax);                         /* 154E MOV [4426],AX */
            result_zero(ram, c);
            goto done;
        }
        c->dx = 0u; logic16(c, 0u);                                   /* 152F XOR DX,DX */
        {   uint16_t recsize = rd16(ram, c->es, (uint16_t)(c->di + 4u));
            if (recsize == 0u) {                                      /* 1531 DIV WORD ES:[DI+4]: INT 0 */
                c->ip = 0x1531u;
                return PR_BLOCKREAD_DIVIDE;
            }
            c->dx = (uint16_t)(c->ax % recsize); c->ax = (uint16_t)(c->ax / recsize);
            /* DIV WORD ES:[DI+4]. Pinned DIVW/IDIVW (instructions.h:655-673, 743-761) overwrite all six flags: AF=SF=OF=0,
             * ZF=(rem==0 && quo odd), CF=((rem&3) in 1..2), PF=even parity of the four bytes of rem:quo. */
            {
                uint16_t rem16 = (uint16_t)(c->dx), quo16 = (uint16_t)(c->ax), fold = (uint16_t)(rem16 ^ quo16);
                fold ^= (uint16_t)(fold >> 8); fold ^= (uint16_t)(fold >> 4); fold ^= (uint16_t)(fold >> 2); fold ^= (uint16_t)(fold >> 1);
                c->flags = (uint16_t)((c->flags & ~0x08D5u) | ((rem16 == 0u && (quo16 & 1u)) ? 0x0040u : 0u) |
                                        (((rem16 & 3u) >= 1u && (rem16 & 3u) <= 2u) ? 0x0001u : 0u) | ((fold & 1u) ? 0u : 0x0004u));
            }
        }
    }
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));                 /* 1535 LES DI,[BP+6] (result) */
    c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 8u));
    c->dx = (uint16_t)(c->es | c->di); logic16(c, c->dx);             /* 1538 MOV DX,ES; 153A OR DX,DI */
    if (c->dx != 0u) {                                                /* 153C JE 1543 */
        wr16(ram, c->es, c->di, c->ax);                               /* 153E MOV ES:[DI],AX; 1541 JMP 155F */
    } else {
        cmp16(c, c->ax, rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Au))); /* 1543 CMP AX,[BP+0Ah] */
        if ((c->flags & ZF) == 0u)                                    /* 1546 JE 155F */
            wr16(ram, c->ds, 0x4426u, c->cx);                         /* 1548 MOV [4426],CX (100); 154C JMP 155F */
    }
done:
    c->bp = pop(ram, c);                                              /* 155F POP BP */
    c->ip = pop(ram, c); c->cs = pop(ram, c);                         /* 1560 RETF 0Eh */
    c->sp = (uint16_t)(c->sp + 0x0Eu);
    return PR_BLOCKREAD_RETURNED;
}
