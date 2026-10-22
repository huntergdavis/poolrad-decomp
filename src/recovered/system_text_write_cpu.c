/* START.EXE SYSTEM text output: Write(string) 0AF8:0919 and Writeln 0AF8:084A with their near helpers, raw CPU
 * (docs/re/SYSTEM_TEXT_WRITE_CPU.md). */
#include "poolrad/recovered/system_text_write_cpu.h"

#define MASK (PR_SYSTEM_TEXT_RAM_SIZE - 1u)

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_system_text_cpu *c;
    const pr_system_text_env *env;
    int *service_status;
} machine;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint16_t word_at(const uint8_t *ram, size_t at) { return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u)); }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off) { return word_at(ram, physical(seg, off)); }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)v; ram[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static void push(machine *m, uint16_t v) { m->c->sp = (uint16_t)(m->c->sp - 2u); wr16(m->ram, m->c->ss, m->c->sp, v); }
static uint16_t pop(machine *m) { uint16_t v = rd16(m->ram, m->c->ss, m->c->sp); m->c->sp = (uint16_t)(m->c->sp + 2u); return v; }
/* LES/LDS reg,seg:[off]: the pair read physically contiguous. */
static void load_pair(const uint8_t *ram, uint16_t seg, uint16_t off, uint16_t *reg, uint16_t *sreg)
{
    size_t at = physical(seg, off);
    *reg = word_at(ram, at); *sreg = word_at(ram, (at + 2u) & MASK);
}

static void szp(pr_system_text_cpu *c, uint16_t r, unsigned bits)
{
    uint8_t p = (uint8_t)r;
    p ^= (uint8_t)(p >> 4u); p ^= (uint8_t)(p >> 2u); p ^= (uint8_t)(p >> 1u);
    if ((p & 1u) == 0u) c->flags |= 0x04u;
    if ((r & (bits == 8u ? 0xFFu : 0xFFFFu)) == 0u) c->flags |= 0x40u;
    if (r & (bits == 8u ? 0x80u : 0x8000u)) c->flags |= 0x80u;
}
static uint16_t sub16(pr_system_text_cpu *c, uint16_t l, uint16_t r)
{
    uint16_t res = (uint16_t)(l - r);
    c->flags &= (uint16_t)~0x08D5u;
    if (l < r) c->flags |= 1u;
    if ((l ^ r ^ res) & 0x10u) c->flags |= 0x10u;
    if ((l ^ r) & (l ^ res) & 0x8000u) c->flags |= 0x800u;
    szp(c, res, 16u);
    return res;
}
static void logic(pr_system_text_cpu *c, uint16_t v, unsigned bits) { c->flags &= (uint16_t)~0x08D5u; szp(c, v, bits); }
static int zf(const pr_system_text_cpu *c) { return (c->flags & 0x40u) != 0u; }

/* A far call through the TextRec far pointer at ES:[DI+slot], from the LCALL at `site` (next instruction `back`). */
static int driver(machine *m, uint16_t slot, uint16_t back)
{
    pr_system_text_cpu *c = m->c, before, after;
    uint16_t off = rd16(m->ram, c->es, (uint16_t)(c->di + slot)), seg = rd16(m->ram, c->es, (uint16_t)(c->di + slot + 2u));
    uint16_t system_cs = c->cs;
    push(m, c->cs); push(m, back);
    c->cs = seg; c->ip = off;
    before = *c;
    if (m->env->driver(m->env->context, m->ram, &before, &after) != 0 || after.cs != system_cs || after.ip != back ||
        after.ss != before.ss || after.sp != (uint16_t)(before.sp + 8u)) {
        *c = before;
        if (m->service_status != NULL) *m->service_status = 1;
        return 0;
    }
    if (m->service_status != NULL) *m->service_status = 0;
    *c = after;
    return 1;
}

/* 07B8: the mode check, entered by a near CALL whose return word is on the stack. Returns ZF (1 = the file is open for
 * output and InOutRes was 0). */
static int mode_check(machine *m)
{
    pr_system_text_cpu *c = m->c;
    uint16_t res = rd16(m->ram, c->ds, 0x4426u);
    sub16(c, res, 0u);                                                /* 07B8 CMP WORD [4426],0; JNE 07C7 */
    if (res == 0u) {
        uint16_t mode = rd16(m->ram, c->es, (uint16_t)(c->di + 2u));
        sub16(c, mode, 0xD7B2u);                                      /* 07BF CMP WORD ES:[DI+2],D7B2h; JNE 07D4 */
        if (mode != 0xD7B2u)
            wr16(m->ram, c->ds, 0x4426u, 0x0069u);                    /* 07D4 MOV WORD [4426],69h; JMP 07C7 */
    }
    c->bx = rd16(m->ram, c->es, (uint16_t)(c->di + 8u));              /* 07C7 MOV BX,ES:[DI+8] */
    c->dx = rd16(m->ram, c->es, (uint16_t)(c->di + 4u));              /* 07CB MOV DX,ES:[DI+4] */
    load_pair(m->ram, c->es, (uint16_t)(c->di + 0x0Cu), &c->di, &c->es);  /* 07CF LES DI,ES:[DI+0Ch] */
    c->ip = pop(m);                                                   /* 07D3 RET */
    return zf(c);
}

/* 07DC: put AL, entered by a near CALL. Returns 0 on a driver refusal. */
static int put_char(machine *m)
{
    pr_system_text_cpu *c = m->c;
    uint16_t before;
    m->ram[physical(c->es, (uint16_t)(c->bx + c->di))] = (uint8_t)c->ax;  /* 07DC MOV ES:[BX+DI],AL */
    before = c->bx; c->bx = (uint16_t)(c->bx + 1u);                    /* 07DF INC BX: CF kept */
    c->flags &= (uint16_t)~0x08D4u;
    if ((before & 0x0Fu) == 0x0Fu) c->flags |= 0x10u;
    if (before == 0x7FFFu) c->flags |= 0x800u;
    szp(c, c->bx, 16u);
    sub16(c, c->bx, c->dx);                                           /* 07E0 CMP BX,DX; JE 07E5 */
    if (c->bx != c->dx) {
        c->ip = pop(m);                                               /* 07E4 RET */
        return 1;
    }
    c->di = c->sp;                                                    /* 07E5 MOV DI,SP */
    load_pair(m->ram, c->ss, (uint16_t)(c->di + 2u), &c->di, &c->es); /* 07E7 LES DI,SS:[DI+2] */
    push(m, c->cx); push(m, c->si); push(m, c->ds);                   /* 07EB PUSH CX; PUSH SI; PUSH DS */
    c->dx = rd16(m->ram, c->cs, 0x07EFu);                             /* 07EE MOV DX,0C7Ch (a loader relocation) */
    c->ds = c->dx;                                                    /* 07F1 MOV DS,DX */
    wr16(m->ram, c->es, (uint16_t)(c->di + 8u), c->bx);               /* 07F3 MOV ES:[DI+8],BX */
    push(m, c->es); push(m, c->di); push(m, c->es); push(m, c->di);   /* 07F7 PUSH ES; PUSH DI; PUSH ES; PUSH DI */
    if (!driver(m, 0x14u, 0x07FFu))                                   /* 07FB LCALL ES:[DI+14h] (InOutFunc) */
        return 0;
    logic(c, c->ax, 16u);                                             /* 07FF OR AX,AX; JE 0806 */
    if (c->ax != 0u)
        wr16(m->ram, c->ds, 0x4426u, c->ax);                          /* 0803 MOV [4426],AX */
    c->di = pop(m); c->es = pop(m);                                   /* 0806 POP DI; POP ES */
    c->ax = rd16(m->ram, c->es, (uint16_t)(c->di + 0x0Au));           /* 0808 MOV AX,ES:[DI+0Ah] */
    c->bx = rd16(m->ram, c->es, (uint16_t)(c->di + 8u));              /* 080C MOV BX,ES:[DI+8] */
    c->dx = rd16(m->ram, c->es, (uint16_t)(c->di + 4u));              /* 0810 MOV DX,ES:[DI+4] */
    load_pair(m->ram, c->es, (uint16_t)(c->di + 0x0Cu), &c->di, &c->es);  /* 0814 LES DI,ES:[DI+0Ch] */
    c->ds = pop(m); c->si = pop(m); c->cx = pop(m);                   /* 0818 POP DS; POP SI; POP CX */
    c->ip = pop(m);                                                   /* 081B RET */
    return 1;
}

/* 087B: the flush, entered by a near CALL. Returns 0 on a driver refusal. */
static int flush(machine *m)
{
    pr_system_text_cpu *c = m->c;
    uint16_t v = rd16(m->ram, c->es, (uint16_t)(c->di + 0x1Au));
    sub16(c, v, 0u);                                                  /* 087B CMP WORD ES:[DI+1Ah],0; JNE 0883 */
    if (v != 0u) {
        v = rd16(m->ram, c->ds, 0x4426u);
        sub16(c, v, 0u);                                              /* 0883 CMP WORD [4426],0; JNE 0897 */
        if (v == 0u) {
            push(m, c->es); push(m, c->di);                           /* 088A PUSH ES; PUSH DI */
            if (!driver(m, 0x18u, 0x0890u))                           /* 088C LCALL ES:[DI+18h] (FlushFunc) */
                return 0;
            logic(c, c->ax, 16u);                                     /* 0890 OR AX,AX; JE 0897 */
            if (c->ax != 0u)
                wr16(m->ram, c->ds, 0x4426u, c->ax);                  /* 0894 MOV [4426],AX */
        }
    }
    c->ip = pop(m);                                                   /* 0882/0897 RET */
    return 1;
}

static int ready(const uint8_t *ram, size_t size, const pr_system_text_cpu *c, const pr_system_text_env *env)
{
    return ram != NULL && c != NULL && env != NULL && env->driver != NULL && size == PR_SYSTEM_TEXT_RAM_SIZE;
}

pr_system_text_status pr_system_writeln_084a(uint8_t *ram, size_t ram_size, pr_system_text_cpu *c,
                                             const pr_system_text_env *env, int *service_status)
{
    machine m;
    if (!ready(ram, ram_size, c, env))
        return PR_SYSTEM_TEXT_ARGUMENT;
    m.ram = ram; m.size = ram_size; m.c = c; m.env = env; m.service_status = service_status;
    if (service_status != NULL) *service_status = -1;
    push(&m, c->bp);                                                  /* 084A PUSH BP */
    c->bp = c->sp;                                                    /* 084B MOV BP,SP */
    load_pair(ram, c->ss, (uint16_t)(c->bp + 6u), &c->di, &c->es);    /* 084D LES DI,[BP+6] */
    push(&m, c->es); push(&m, c->di);                                 /* 0850 PUSH ES; PUSH DI */
    push(&m, 0x0855u);                                                /* 0852 CALL 07B8 */
    if (mode_check(&m)) {                                             /* 0855 JNE 0861 */
        c->ax = (uint16_t)((c->ax & 0xFF00u) | 0x0Du);                /* 0857 MOV AL,0Dh; CALL 07DC */
        push(&m, 0x085Cu);
        if (!put_char(&m))
            return PR_SYSTEM_TEXT_SERVICE;
        c->ax = (uint16_t)((c->ax & 0xFF00u) | 0x0Au);                /* 085C MOV AL,0Ah; CALL 07DC */
        push(&m, 0x0861u);
        if (!put_char(&m))
            return PR_SYSTEM_TEXT_SERVICE;
    }
    c->di = pop(&m); c->es = pop(&m);                                 /* 0861 POP DI; POP ES */
    wr16(ram, c->es, (uint16_t)(c->di + 8u), c->bx);                  /* 0863 MOV ES:[DI+8],BX */
    push(&m, 0x086Au);                                                /* 0867 CALL 087B */
    if (!flush(&m))
        return PR_SYSTEM_TEXT_SERVICE;
    c->bp = pop(&m);                                                  /* 086A POP BP */
    c->ip = pop(&m); c->cs = pop(&m); c->sp = (uint16_t)(c->sp + 4u); /* 086B RETF 4 */
    return PR_SYSTEM_TEXT_RETURNED;
}

pr_system_text_status pr_system_write_str_0919(uint8_t *ram, size_t ram_size, pr_system_text_cpu *c,
                                               const pr_system_text_env *env, int *service_status)
{
    machine m;
    if (!ready(ram, ram_size, c, env))
        return PR_SYSTEM_TEXT_ARGUMENT;
    m.ram = ram; m.size = ram_size; m.c = c; m.env = env; m.service_status = service_status;
    if (service_status != NULL) *service_status = -1;
    push(&m, c->bp);                                                  /* 0919 PUSH BP */
    c->bp = c->sp;                                                    /* 091A MOV BP,SP */
    push(&m, c->ds);                                                  /* 091C PUSH DS */
    load_pair(ram, c->ss, (uint16_t)(c->bp + 0x0Cu), &c->di, &c->es); /* 091D LES DI,[BP+0Ch] */
    push(&m, c->es); push(&m, c->di);                                 /* 0920 PUSH ES; PUSH DI */
    push(&m, 0x0925u);                                                /* 0922 CALL 07B8 */
    if (mode_check(&m)) {                                             /* 0925 JNE 094C */
        load_pair(ram, c->ss, (uint16_t)(c->bp + 8u), &c->si, &c->ds);/* 0927 LDS SI,[BP+8] */
        c->cx = rd16(ram, c->ss, (uint16_t)(c->bp + 6u));             /* 092A MOV CX,[BP+6] */
        c->flags &= (uint16_t)~0x0400u;                               /* 092D CLD */
        c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, c->si)]);  /* 092E LODSB */
        c->si = (uint16_t)(c->si + 1u);
        c->ax &= 0x00FFu; logic(c, 0u, 8u);                           /* 092F XOR AH,AH */
        c->cx = sub16(c, c->cx, c->ax);                               /* 0931 SUB CX,AX; JLE 0941 */
        if (!(zf(c) || (((c->flags >> 7) & 1u) != ((c->flags >> 11) & 1u)))) {
            do {
                c->ax = (uint16_t)((c->ax & 0xFF00u) | 0x20u);        /* 0935 MOV AL,20h; CALL 07DC */
                push(&m, 0x093Au);
                if (!put_char(&m))
                    return PR_SYSTEM_TEXT_SERVICE;
                c->cx = (uint16_t)(c->cx - 1u);                       /* 093A LOOP 0935 */
            } while (c->cx != 0u);
            c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, (uint16_t)(c->si - 1u))]);  /* 093C MOV AL,[SI-1] */
            c->ax &= 0x00FFu; logic(c, 0u, 8u);                       /* 093F XOR AH,AH */
        }
        c->cx = c->ax;                                                /* 0941 MOV CX,AX; JCXZ 094C */
        while (c->cx != 0u) {
            c->flags &= (uint16_t)~0x0400u;                           /* 0945 CLD; LODSB; CALL 07DC */
            c->ax = (uint16_t)((c->ax & 0xFF00u) | ram[physical(c->ds, c->si)]);
            c->si = (uint16_t)(c->si + 1u);
            push(&m, 0x094Au);
            if (!put_char(&m))
                return PR_SYSTEM_TEXT_SERVICE;
            c->cx = (uint16_t)(c->cx - 1u);                           /* 094A LOOP 0945 */
        }
    }
    c->di = pop(&m); c->es = pop(&m);                                 /* 094C POP DI; POP ES */
    wr16(ram, c->es, (uint16_t)(c->di + 8u), c->bx);                  /* 094E MOV ES:[DI+8],BX */
    c->ds = pop(&m);                                                  /* 0952 POP DS */
    c->bp = pop(&m);                                                  /* 0953 POP BP */
    c->ip = pop(&m); c->cs = pop(&m); c->sp = (uint16_t)(c->sp + 6u); /* 0954 RETF 6 */
    return PR_SYSTEM_TEXT_RETURNED;
}
