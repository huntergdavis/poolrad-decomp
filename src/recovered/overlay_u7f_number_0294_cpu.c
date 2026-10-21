/* GAME.OVR unit 7Fh +0294, the game's numeric entry line editor, raw CPU (docs/re/U7F_NUMBER_0294_CPU.md). */
#include "poolrad/recovered/overlay_u7f_number_0294_cpu.h"
#include "poolrad/recovered/system_str_1366_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"
#include "poolrad/recovered/system_val_13b1_cpu.h"
#include <string.h>

#define MASK (PR_U7F_NUMBER_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
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
static uint8_t *b(uint8_t *ram, const pr_u7f_cpu *c, int disp) { return &ram[physical(c->ss, (uint16_t)(c->bp + disp))]; }
static void push(uint8_t *ram, pr_u7f_cpu *c, uint16_t v) { c->sp = (uint16_t)(c->sp - 2u); wr16(ram, c->ss, c->sp, v); }
static unsigned parity(unsigned v)
{
    v &= 0xFFu; v ^= v >> 4u; v ^= v >> 2u; v ^= v >> 1u;
    return (v & 1u) ? 0u : PF;
}
static void set(pr_u7f_cpu *c, unsigned mask, unsigned bits) { c->flags = (uint16_t)((c->flags & ~mask) | bits); }
static unsigned szp(unsigned r, unsigned msb) { return (r == 0u ? ZF : 0u) | (r & msb ? SF : 0u) | parity(r); }
static int flag(const pr_u7f_cpu *c, unsigned f) { return (c->flags & f) != 0u; }
/* SUB/CMP/ADD of width w. */
static unsigned arith(pr_u7f_cpu *c, unsigned a, unsigned bb, int sub, unsigned w)
{
    unsigned m = w == 8u ? 0xFFu : 0xFFFFu, msb = w == 8u ? 0x80u : 0x8000u, r, cf, of;
    if (sub) { r = (a - bb) & m; cf = a < bb; of = ((a ^ bb) & (a ^ r) & msb) != 0u; }
    else { r = (a + bb) & m; cf = a + bb > m; of = (~(a ^ bb) & (a ^ r) & msb) != 0u; }
    set(c, ARITH, szp(r, msb) | (cf ? CF : 0u) | ((a ^ bb ^ r) & AF) | (of ? OF : 0u));
    return r;
}
static unsigned zero(pr_u7f_cpu *c, unsigned w) { set(c, ARITH, szp(0u, w == 8u ? 0x80u : 0x8000u)); return 0u; }   /* XOR r,r */
static unsigned incdec8(pr_u7f_cpu *c, unsigned a, int dec)
{
    unsigned r = (dec ? a - 1u : a + 1u) & 0xFFu;
    set(c, ARITH & ~CF, szp(r, 0x80u) | ((a ^ 1u ^ r) & AF) | ((dec ? a == 0x80u : a == 0x7Fu) ? OF : 0u));
    return r;
}
static void al(pr_u7f_cpu *c, unsigned v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | (v & 0xFFu)); }
static void lea_push(uint8_t *ram, pr_u7f_cpu *c, int disp) { c->di = (uint16_t)(c->bp + disp); push(ram, c, c->ss); push(ram, c, c->di); }

typedef struct { pr_bus *bus; size_t size; const pr_u7f_number_env *env; pr_u7f_number_diagnostic *d; uint16_t body_cs; } run;
enum { STORE, SPACE, PRINT, STR, KEY, LOAD, CHAR, CAT, VAL, COPYS };

/* LCALL at `site` (its segment word, a unit fixup, read live at site+3), composing the callee; 0 = returned to `next`. */
static pr_u7f_number_status call(const run *r, pr_u7f_cpu *c, uint16_t site, int kind, uint16_t off)
{
    uint8_t *ram = r->bus->ram; uint16_t next = (uint16_t)(site + 5u), seg = rd16(ram, c->cs, (uint16_t)(site + 3u)); int s, ok;
    push(ram, c, c->cs); push(ram, c, next); c->cs = seg; c->ip = off;
    switch (kind) {
    case STORE: { pr_system_0b25_cpu m; COPY(&m, c); s = (int)pr_system_strstore_0b25(ram, r->size, &m); COPY(c, &m); ok = s == 0; break; }
    case STR: case LOAD: case CHAR: case CAT: case COPYS: {
        pr_system_leaf_cpu m; COPY(&m, c);
        s = (int)(kind == STR ? pr_system_str_1366(ram, r->size, &m) : kind == LOAD ? pr_system_strload_0b0b(ram, r->size, &m) :
                  kind == CHAR ? pr_system_charstr_0c3b(ram, r->size, &m) : kind == CAT ? pr_system_strcat_0b98(ram, r->size, &m) :
                  pr_system_strcopy_0b57(ram, r->size, &m));
        COPY(c, &m); ok = s == 0; break;
    }
    case VAL: { pr_val_cpu m; COPY(&m, c); s = (int)pr_system_val_13b1(ram, r->size, &m); COPY(c, &m); ok = s == 0; break; }
    case SPACE: {
        pr_r0709_space_cpu m; COPY(&m, c);
        s = (int)pr_r0709_space_05eb_cpu(r->bus, r->size, &m, r->env->space_loopbacks, &r->d->space); COPY(c, &m); ok = s == 0; break;
    }
    case PRINT: {
        pr_r0709_printer_cpu m; COPY(&m, c);
        s = (int)pr_r0709_printer_0626_cpu(r->bus, r->size, &m, r->env->printer_loopbacks, r->env->glyph_loopbacks, &r->d->printer);
        COPY(c, &m); ok = s == 0; break;
    }
    default: {
        pr_r0899_cpu m; COPY(&m, c);
        ++r->d->keys;
        s = (int)pr_r0899_readkey_0059_cpu(ram, r->size, &m, &r->env->key, &r->d->key); COPY(c, &m);
        if (s == PR_R0899_KEY_EXIT || s == PR_R0899_KEY_BREAK) {
            r->d->site = site; r->d->status = s;
            return s == PR_R0899_KEY_EXIT ? PR_U7F_NUMBER_EXIT : PR_U7F_NUMBER_BREAK;
        }
        ok = s == PR_R0899_KEY_RETURN; break;
    }
    }
    if (!ok) { r->d->site = site; r->d->status = s; return PR_U7F_NUMBER_CALLEE; }
    return c->cs == r->body_cs && c->ip == next ? PR_U7F_NUMBER_RETURN : PR_U7F_NUMBER_TRANSFER;
}
#define CALL(site, kind, off) do { pr_u7f_number_status t_ = call(&r, c, (site), (kind), (off)); if (t_ != PR_U7F_NUMBER_RETURN) return t_; } while (0)
/* MOV AL,imm8; PUSH AX (AH as it is). */
static void push_al(uint8_t *ram, pr_u7f_cpu *c, unsigned v) { al(c, v); push(ram, c, c->ax); }

pr_u7f_number_status pr_u7f_number_0294_cpu(pr_bus *bus, size_t ram_size, pr_u7f_cpu *c, const pr_u7f_number_env *env,
                                            pr_u7f_number_diagnostic *d)
{
    run r; uint8_t *ram; unsigned loops = 0;
    if (bus == NULL || bus->ram == NULL || bus->ega == NULL || c == NULL || env == NULL || d == NULL ||
        ram_size != PR_U7F_NUMBER_RAM_SIZE || c->ip != 0x0294u || c->sp < 0x025Au || c->sp > 0xFFF4u)
        return PR_U7F_NUMBER_ARGUMENT;
    ram = bus->ram;
    memset(d, 0, sizeof *d);
    r.bus = bus; r.size = ram_size; r.env = env; r.d = d; r.body_cs = c->cs;
    push(ram, c, c->bp); c->bp = c->sp;                               /* 0294 PUSH BP; 0295 MOV BP,SP */
    c->sp = (uint16_t)arith(c, c->sp, 0x0244u, 1, 16u);               /* 0297 SUB SP,0244h */
    c->di = rd16(ram, c->ss, (uint16_t)(c->bp + 8u)); c->es = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Au));   /* 029B LES DI,[BP+8] */
    push(ram, c, c->es); push(ram, c, c->di);                         /* 029E PUSH ES; 029F PUSH DI */
    lea_push(ram, c, -0x2B);                                          /* 02A0 LEA DI,[BP-2Bh]; PUSH SS; PUSH DI */
    c->ax = 0x0028u; push(ram, c, c->ax);                             /* 02A5 MOV AX,28h; PUSH AX */
    CALL(0x02A9, STORE, 0x0B25);
    push_al(ram, c, 0x00); push_al(ram, c, 0x18); push_al(ram, c, 0x00); push_al(ram, c, 0x28);   /* 02AE..02B9 */
    CALL(0x02BA, SPACE, 0x05EB);
    push_al(ram, c, 0x00); push_al(ram, c, 0x18);                     /* 02BF..02C4 */
    al(c, *b(ram, c, 6)); push(ram, c, c->ax);                        /* 02C5 MOV AL,[BP+6]; PUSH AX */
    lea_push(ram, c, -0x2B);                                          /* 02C9 LEA DI,[BP-2Bh]; PUSH SS; PUSH DI */
    CALL(0x02CE, PRINT, 0x0626);
    al(c, *b(ram, c, -0x2B)); *b(ram, c, -0x2E) = (uint8_t)c->ax;     /* 02D3 MOV AL,[BP-2Bh]; MOV [BP-2Eh],AL */
    al(c, *b(ram, c, -0x2E)); *b(ram, c, -0x2C) = (uint8_t)c->ax;     /* 02D9 MOV AL,[BP-2Eh]; MOV [BP-2Ch],AL */
    c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Cu));              /* 02DF MOV AX,[BP+0Ch] */
    c->dx = (uint16_t)zero(c, 16u);                                   /* 02E2 XOR DX,DX */
    push(ram, c, c->dx); push(ram, c, c->ax);                         /* 02E4 PUSH DX; 02E5 PUSH AX */
    c->ax = (uint16_t)zero(c, 16u); push(ram, c, c->ax);              /* 02E6 XOR AX,AX; PUSH AX */
    lea_push(ram, c, -0x34);                                          /* 02E9 LEA DI,[BP-34h]; PUSH SS; PUSH DI */
    c->ax = 0x0005u; push(ram, c, c->ax);                             /* 02EE MOV AX,5; PUSH AX */
    CALL(0x02F2, STR, 0x1366);
    *b(ram, c, -0x3B) = 0u;                                           /* 02F7 MOV BYTE [BP-3Bh],0 */
    for (;;) {
        unsigned key;
        if (loops == env->key_limit) { c->ip = 0x02FBu; d->site = 0x02FBu; return PR_U7F_NUMBER_LIMIT; }
        CALL(0x02FB, KEY, 0x0059);                                    /* 02FB LCALL 0899:0059 */
        *b(ram, c, -0x3C) = (uint8_t)c->ax;                           /* 0300 MOV [BP-3Ch],AL */
        al(c, *b(ram, c, -0x3C)); key = c->ax & 0xFFu;                /* 0303 MOV AL,[BP-3Ch] */
        {   int digit = 0;
            arith(c, key, 0x30u, 1, 8u);                              /* 0306 CMP AL,'0' */
            if (!flag(c, CF)) {                                       /* 0308 JAE 030D (else 030A JMP 03AB) */
                arith(c, key, 0x39u, 1, 8u);                          /* 030D CMP AL,'9' */
                digit = flag(c, CF) || flag(c, ZF);                   /* 030F JBE 0314 (else 0311 JMP 03AB) */
            }
            if (!digit) goto not_digit;
        }
        {
            lea_push(ram, c, -0x144);                                 /* 0314 LEA DI,[BP-144h]; PUSH SS; PUSH DI */
            lea_push(ram, c, -0x3B);                                  /* 031A LEA DI,[BP-3Bh]; PUSH SS; PUSH DI */
            CALL(0x031F, LOAD, 0x0B0B);
            lea_push(ram, c, -0x244);                                 /* 0324 LEA DI,[BP-244h]; PUSH SS; PUSH DI */
            al(c, *b(ram, c, -0x3C)); push(ram, c, c->ax);            /* 032A MOV AL,[BP-3Ch]; PUSH AX */
            CALL(0x032E, CHAR, 0x0C3B);
            CALL(0x0333, CAT, 0x0B98);
            lea_push(ram, c, -0x3B);                                  /* 0338 LEA DI,[BP-3Bh]; PUSH SS; PUSH DI */
            c->ax = 0x0006u; push(ram, c, c->ax);                     /* 033D MOV AX,6; PUSH AX */
            CALL(0x0341, STORE, 0x0B25);
            lea_push(ram, c, -0x3B);                                  /* 0346 LEA DI,[BP-3Bh]; PUSH SS; PUSH DI */
            lea_push(ram, c, -0x42);                                  /* 034B LEA DI,[BP-42h]; PUSH SS; PUSH DI */
            CALL(0x0350, VAL, 0x13B1);
            wr16(ram, c->ss, (uint16_t)(c->bp - 0x40), c->ax);        /* 0355 MOV [BP-40h],AX */
            wr16(ram, c->ss, (uint16_t)(c->bp - 0x3E), c->dx);        /* 0358 MOV [BP-3Eh],DX */
            c->ax = rd16(ram, c->ss, (uint16_t)(c->bp + 0x0Cu));      /* 035B MOV AX,[BP+0Ch] */
            c->dx = (uint16_t)zero(c, 16u);                           /* 035E XOR DX,DX */
            {   /* 0360 CMP DX,[BP-3Eh]; JG 036C; JL 0371; CMP AX,[BP-40h]; JB 0371 */
                int over;
                arith(c, c->dx, rd16(ram, c->ss, (uint16_t)(c->bp - 0x3E)), 1, 16u);
                if (!flag(c, ZF) && flag(c, SF) == flag(c, OF)) over = 0;          /* JG: max > value */
                else if (flag(c, SF) != flag(c, OF)) over = 1;                     /* JL */
                else { arith(c, c->ax, rd16(ram, c->ss, (uint16_t)(c->bp - 0x40)), 1, 16u); over = flag(c, CF); }
                if (!over) {
                    *b(ram, c, -0x2C) = (uint8_t)incdec8(c, *b(ram, c, -0x2C), 0);  /* 036C INC BYTE [BP-2Ch]; JMP 0395 */
                } else {
                    lea_push(ram, c, -0x34);                          /* 0371 LEA DI,[BP-34h]; PUSH SS; PUSH DI */
                    lea_push(ram, c, -0x3B);                          /* 0376 LEA DI,[BP-3Bh]; PUSH SS; PUSH DI */
                    c->ax = 0x0006u; push(ram, c, c->ax);             /* 037B MOV AX,6; PUSH AX */
                    CALL(0x037F, STORE, 0x0B25);
                    al(c, *b(ram, c, -0x34)); c->ax &= 0x00FFu; zero(c, 8u);   /* 0384 MOV AL,[BP-34h]; 0387 XOR AH,AH */
                    c->dx = c->ax;                                    /* 0389 MOV DX,AX */
                    al(c, *b(ram, c, -0x2E)); c->ax &= 0x00FFu; zero(c, 8u);   /* 038B MOV AL,[BP-2Eh]; 038E XOR AH,AH */
                    c->ax = (uint16_t)arith(c, c->ax, c->dx, 0, 16u); /* 0390 ADD AX,DX */
                    *b(ram, c, -0x2C) = (uint8_t)c->ax;               /* 0392 MOV [BP-2Ch],AL */
                }
            }
            al(c, *b(ram, c, -0x2E)); push(ram, c, c->ax);            /* 0395 MOV AL,[BP-2Eh]; PUSH AX */
            push_al(ram, c, 0x18); push_al(ram, c, 0x0F);             /* 0399..039E */
            lea_push(ram, c, -0x3B);                                  /* 039F LEA DI,[BP-3Bh]; PUSH SS; PUSH DI */
            CALL(0x03A4, PRINT, 0x0626);                              /* 03A9 JMP 03F3 */
            goto tail;
        }
not_digit:
        {
            arith(c, c->ax & 0xFFu, 0x08u, 1, 8u);                    /* 03AB CMP AL,8 */
            if (flag(c, ZF)) {                                        /* 03AD JNE 03F3 */
                arith(c, *b(ram, c, -0x3B), 0x00u, 1, 8u);            /* 03AF CMP BYTE [BP-3Bh],0 */
                if (!flag(c, CF) && !flag(c, ZF)) {                   /* 03B3 JBE 03F3 */
                    lea_push(ram, c, -0x144);                         /* 03B5 LEA DI,[BP-144h]; PUSH SS; PUSH DI */
                    lea_push(ram, c, -0x3B);                          /* 03BB LEA DI,[BP-3Bh]; PUSH SS; PUSH DI */
                    c->ax = 0x0001u; push(ram, c, c->ax);             /* 03C0 MOV AX,1; PUSH AX */
                    al(c, *b(ram, c, -0x3B)); c->ax &= 0x00FFu; zero(c, 8u);   /* 03C4 MOV AL,[BP-3Bh]; 03C7 XOR AH,AH */
                    {   unsigned a = c->ax, res = (a - 1u) & 0xFFFFu;   /* 03C9 DEC AX */
                        set(c, ARITH & ~CF, szp(res, 0x8000u) | ((a ^ 1u ^ res) & AF) | (a == 0x8000u ? OF : 0u)); c->ax = (uint16_t)res; }
                    push(ram, c, c->ax);                              /* 03CA PUSH AX */
                    CALL(0x03CB, COPYS, 0x0B57);
                    lea_push(ram, c, -0x3B);                          /* 03D0 LEA DI,[BP-3Bh]; PUSH SS; PUSH DI */
                    c->ax = 0x0006u; push(ram, c, c->ax);             /* 03D5 MOV AX,6; PUSH AX */
                    CALL(0x03D9, STORE, 0x0B25);
                    al(c, *b(ram, c, -0x2C)); push(ram, c, c->ax);    /* 03DE MOV AL,[BP-2Ch]; PUSH AX */
                    push_al(ram, c, 0x18); push_al(ram, c, 0x00); push_al(ram, c, 0x01);   /* 03E2..03EA */
                    CALL(0x03EB, SPACE, 0x05EB);
                    *b(ram, c, -0x2C) = (uint8_t)incdec8(c, *b(ram, c, -0x2C), 1);   /* 03F0 DEC BYTE [BP-2Ch] */
                }
            }
        }
tail:
        arith(c, *b(ram, c, -0x3C), 0x0Du, 1, 8u);                    /* 03F3 CMP BYTE [BP-3Ch],0Dh */
        if (flag(c, ZF)) break;                                       /* 03F7 JE 0402 */
        arith(c, *b(ram, c, -0x3C), 0x1Bu, 1, 8u);                    /* 03F9 CMP BYTE [BP-3Ch],1Bh */
        if (flag(c, ZF)) break;                                       /* 03FD JE 0402 */
        ++loops;                                                      /* 03FF JMP 02FB */
    }
    push_al(ram, c, 0x00); push_al(ram, c, 0x18); push_al(ram, c, 0x00); push_al(ram, c, 0x28);   /* 0402..040D */
    CALL(0x040E, SPACE, 0x05EB);
    arith(c, *b(ram, c, -0x3C), 0x1Bu, 1, 8u);                        /* 0413 CMP BYTE [BP-3Ch],1Bh */
    if (flag(c, ZF)) {                                                /* 0417 JNE 0420 */
        c->ax = (uint16_t)zero(c, 16u);                               /* 0419 XOR AX,AX */
        wr16(ram, c->ss, (uint16_t)(c->bp - 0x44), c->ax);            /* 041B MOV [BP-44h],AX; 041E JMP 0432 */
    } else {
        lea_push(ram, c, -0x3B);                                      /* 0420 LEA DI,[BP-3Bh]; PUSH SS; PUSH DI */
        lea_push(ram, c, -0x42);                                      /* 0425 LEA DI,[BP-42h]; PUSH SS; PUSH DI */
        CALL(0x042A, VAL, 0x13B1);
        wr16(ram, c->ss, (uint16_t)(c->bp - 0x44), c->ax);            /* 042F MOV [BP-44h],AX */
    }
    c->ax = rd16(ram, c->ss, (uint16_t)(c->bp - 0x44));               /* 0432 MOV AX,[BP-44h] */
    wr16(ram, c->ss, (uint16_t)(c->bp - 2), c->ax);                   /* 0435 MOV [BP-2],AX */
    c->ax = rd16(ram, c->ss, (uint16_t)(c->bp - 2));                  /* 0438 MOV AX,[BP-2] */
    c->sp = c->bp;                                                    /* 043B MOV SP,BP */
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u); /* 043D POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));   /* 043E RETF 8 */
    c->sp = (uint16_t)(c->sp + 4u + 8u);
    return PR_U7F_NUMBER_RETURN;
}
