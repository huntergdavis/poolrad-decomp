/*
 * OVL_0038_0233 (U38 slot 7) C composition.
 *
 * Executes the pinned 324-byte handler body (119 instructions at CS:IP 30FF:0233) from the native
 * handler-entry state. The body's own bytes are interpreted. Each far call is not executed: the body's
 * lcall pushes its return CS:IP as the instruction does, then the declared callee effects from the
 * native trace (one CALL block: W writes, one R register line, one SP line) are applied in order.
 * The body bytes are read from a file that the caller extracts from the owner's media; no body bytes
 * are embedded in this source.
 *
 * Usage: compose BODY_BIN ENTRY_RAM ENTRY_REGS14 EFFECTS OUT_RAM [MUTANT]
 * MUTANT: none | retf-pop-0C | arg-byte-call2 | retaddr-call2 | arg-byte-call3
 * Output: OUT_RAM (655360 bytes below A0000) and OUT_RAM.regs (13 register fields, then FLAGS).
 *
 * Note: capstone prints opcodes 0x98 and 0x99 as cwde and cdq. In 16-bit code without an operand-size
 * prefix they are CBW and CWD, which is what this program implements.
 */
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define RAM_BELOW 655360u
#define MEM_SIZE 0x100000u
#define BODY_SEG 0x30FFu
#define BODY_BASE 0x0233u
#define BODY_LEN 324u
#define MAX_STEPS 100000u

enum { F_CF = 0x0001, F_PF = 0x0004, F_AF = 0x0010, F_ZF = 0x0040, F_SF = 0x0080, F_OF = 0x0800 };
enum { ALU_ADD, ALU_OR, ALU_SUB, ALU_XOR, ALU_CMP };

static uint8_t mem[MEM_SIZE];
static uint8_t body[BODY_LEN];
static uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, fl;
static int es_override, done, mut_retf0c, mut_ret2, call_k;
static FILE *eff;

static void die(const char *m) {
    fprintf(stderr, "compose: %s\n", m);
    exit(2);
}

static uint32_t lin(uint16_t seg, uint16_t off) { return (((uint32_t)seg << 4) + off) & 0xFFFFFu; }
static uint8_t rd8(uint32_t a) { return mem[a & 0xFFFFFu]; }
static uint16_t rd16(uint32_t a) { return (uint16_t)(rd8(a) | ((uint16_t)rd8(a + 1) << 8)); }

static void wr8(uint32_t a, uint8_t v) {
    a &= 0xFFFFFu;
    if (a >= RAM_BELOW) die("body wrote at or above A0000");
    mem[a] = v;
}
static void wr16(uint32_t a, uint16_t v) {
    wr8(a, (uint8_t)(v & 0xFFu));
    wr8(a + 1, (uint8_t)(v >> 8));
}

static uint16_t *reg16(int n) {
    switch (n) {
    case 0: return &ax;
    case 1: return &cx;
    case 2: return &dx;
    case 3: return &bx;
    case 4: return &sp;
    case 5: return &bp;
    case 6: return &si;
    default: return &di;
    }
}
static uint8_t get8(int n) {
    uint16_t v = *reg16(n & 3);
    return (n & 4) ? (uint8_t)(v >> 8) : (uint8_t)(v & 0xFFu);
}
static void set8(int n, uint8_t b) {
    uint16_t *p = reg16(n & 3);
    if (n & 4) *p = (uint16_t)((*p & 0x00FFu) | ((uint16_t)b << 8));
    else *p = (uint16_t)((*p & 0xFF00u) | b);
}

static uint8_t fetch8(void) {
    if (cs != BODY_SEG || ip < BODY_BASE || ip >= BODY_BASE + BODY_LEN) die("instruction fetch outside the body");
    uint8_t b = body[ip - BODY_BASE];
    ip = (uint16_t)(ip + 1);
    return b;
}
static uint16_t fetch16(void) {
    uint16_t lo = fetch8();
    uint16_t hi = fetch8();
    return (uint16_t)(lo | (hi << 8));
}

static void push16(uint16_t v) {
    sp = (uint16_t)(sp - 2);
    wr16(lin(ss, sp), v);
}
static uint16_t pop16(void) {
    uint16_t v = rd16(lin(ss, sp));
    sp = (uint16_t)(sp + 2);
    return v;
}

static void set_flag(uint16_t bit, int on) {
    if (on) fl = (uint16_t)(fl | bit);
    else fl = (uint16_t)(fl & ~bit);
}
static int parity_even(uint8_t b) {
    int n = 0;
    for (int i = 0; i < 8; i++) n += (b >> i) & 1;
    return (n % 2) == 0;
}
static void set_szp(uint32_t r, int bits) {
    uint32_t mask = (bits == 8) ? 0xFFu : 0xFFFFu;
    uint32_t sign = (bits == 8) ? 0x80u : 0x8000u;
    set_flag(F_ZF, (r & mask) == 0);
    set_flag(F_SF, (r & sign) != 0);
    set_flag(F_PF, parity_even((uint8_t)(r & 0xFFu)));
}

static uint16_t alu(int op, uint32_t a, uint32_t b, int bits) {
    uint32_t mask = (bits == 8) ? 0xFFu : 0xFFFFu;
    uint32_t sign = (bits == 8) ? 0x80u : 0x8000u;
    uint32_t r;
    a &= mask;
    b &= mask;
    switch (op) {
    case ALU_ADD: {
        uint32_t full = a + b;
        r = full & mask;
        set_flag(F_CF, full > mask);
        set_flag(F_OF, ((~(a ^ b)) & (a ^ r) & sign) != 0);
        set_flag(F_AF, ((a ^ b ^ r) & 0x10u) != 0);
        break;
    }
    case ALU_SUB:
    case ALU_CMP:
        r = (a - b) & mask;
        set_flag(F_CF, a < b);
        set_flag(F_OF, ((a ^ b) & (a ^ r) & sign) != 0);
        set_flag(F_AF, ((a ^ b ^ r) & 0x10u) != 0);
        break;
    case ALU_OR:
        r = (a | b) & mask;
        set_flag(F_CF, 0);
        set_flag(F_OF, 0);
        set_flag(F_AF, 0);
        break;
    default:
        r = (a ^ b) & mask;
        set_flag(F_CF, 0);
        set_flag(F_OF, 0);
        set_flag(F_AF, 0);
        break;
    }
    set_szp(r, bits);
    return (uint16_t)r;
}

typedef struct {
    int reg;      /* register index when mod == 3, else -1 */
    int regf;     /* reg field of the ModRM byte */
    uint16_t off; /* effective offset (for LEA) */
    uint32_t addr; /* linear address when memory */
} Modrm;

static Modrm modrm(void) {
    Modrm o;
    uint8_t m = fetch8();
    int mod = m >> 6;
    int rm = m & 7;
    int use_bp = 0;
    uint16_t base = 0;
    uint16_t disp = 0;
    o.regf = (m >> 3) & 7;
    o.reg = -1;
    o.off = 0;
    o.addr = 0;
    if (mod == 3) {
        o.reg = rm;
        return o;
    }
    switch (rm) {
    case 0: base = (uint16_t)(bx + si); break;
    case 1: base = (uint16_t)(bx + di); break;
    case 2: base = (uint16_t)(bp + si); use_bp = 1; break;
    case 3: base = (uint16_t)(bp + di); use_bp = 1; break;
    case 4: base = si; break;
    case 5: base = di; break;
    case 6:
        if (mod == 0) {
            base = fetch16();
        } else {
            base = bp;
            use_bp = 1;
        }
        break;
    default: base = bx; break;
    }
    if (mod == 1) disp = (uint16_t)(int16_t)(int8_t)fetch8();
    else if (mod == 2) disp = fetch16();
    o.off = (uint16_t)(base + disp);
    o.addr = lin(es_override ? es : (use_bp ? ss : ds), o.off);
    return o;
}

static uint16_t rd_op(Modrm o, int bits) {
    if (o.reg >= 0) return (bits == 8) ? get8(o.reg) : *reg16(o.reg);
    return (bits == 8) ? rd8(o.addr) : rd16(o.addr);
}
static void wr_op(Modrm o, int bits, uint16_t v) {
    if (o.reg >= 0) {
        if (bits == 8) set8(o.reg, (uint8_t)v);
        else *reg16(o.reg) = v;
        return;
    }
    if (bits == 8) wr8(o.addr, (uint8_t)v);
    else wr16(o.addr, v);
}
static uint16_t get_g(Modrm o, int bits) { return (bits == 8) ? get8(o.regf) : *reg16(o.regf); }
static void set_g(Modrm o, int bits, uint16_t v) {
    if (bits == 8) set8(o.regf, (uint8_t)v);
    else *reg16(o.regf) = v;
}

static void grp1(Modrm o, int bits, uint16_t imm) {
    uint16_t r;
    switch (o.regf) {
    case 0: r = alu(ALU_ADD, rd_op(o, bits), imm, bits); wr_op(o, bits, r); break;
    case 1: r = alu(ALU_OR, rd_op(o, bits), imm, bits); wr_op(o, bits, r); break;
    case 5: r = alu(ALU_SUB, rd_op(o, bits), imm, bits); wr_op(o, bits, r); break;
    case 7: (void)alu(ALU_CMP, rd_op(o, bits), imm, bits); break;
    default: die("unsupported group-1 member");
    }
}

static uint8_t inc8(uint8_t v) {
    uint16_t saved = (uint16_t)(fl & F_CF);
    uint16_t r = alu(ALU_ADD, v, 1, 8);
    fl = (uint16_t)((fl & ~F_CF) | saved);
    return (uint8_t)r;
}

static void mul16(Modrm o) {
    uint32_t p = (uint32_t)ax * (uint32_t)rd_op(o, 16);
    ax = (uint16_t)(p & 0xFFFFu);
    dx = (uint16_t)(p >> 16);
    set_flag(F_CF, dx != 0);
    set_flag(F_OF, dx != 0);
}

static void idiv16(Modrm o) {
    int32_t num = (int32_t)(((uint32_t)dx << 16) | ax);
    int32_t den = (int16_t)rd_op(o, 16);
    int32_t q, rem;
    if (den == 0) die("idiv by zero");
    q = num / den;
    rem = num % den;
    if (q < -32768 || q > 32767) die("idiv quotient overflow");
    ax = (uint16_t)(int16_t)q;
    dx = (uint16_t)(int16_t)rem;
}

static void shl16_by1(Modrm o) {
    uint16_t v = rd_op(o, 16);
    uint16_t r = (uint16_t)(v << 1);
    set_flag(F_CF, (v & 0x8000u) != 0);
    set_flag(F_OF, ((r & 0x8000u) != 0) != ((v & 0x8000u) != 0));
    set_szp(r, 16);
    wr_op(o, 16, r);
}

static void jrel8(int taken) {
    int8_t rel = (int8_t)fetch8();
    if (taken) ip = (uint16_t)(ip + (int16_t)rel);
}

/* Apply one declared callee block from the native trace: CALL k, W lines, R line, SP line. */
static void apply_effects_block(void) {
    char line[512];
    int k = -1;
    if (!fgets(line, sizeof line, eff)) die("effects: missing CALL block");
    if (sscanf(line, "CALL %d", &k) != 1 || k != call_k) die("effects: CALL index does not match the far call");
    for (;;) {
        if (!fgets(line, sizeof line, eff)) die("effects: truncated block");
        if (line[0] == 'W') {
            unsigned a, v;
            if (sscanf(line, "W %x %x", &a, &v) != 2) die("effects: bad W line");
            if (a >= RAM_BELOW) die("effects: write at or above A0000");
            mem[a] = (uint8_t)v;
        } else if (line[0] == 'R') {
            unsigned v[10];
            if (sscanf(line, "R %x %x %x %x %x %x %x %x %x %x", &v[0], &v[1], &v[2], &v[3], &v[4], &v[5], &v[6], &v[7], &v[8], &v[9]) != 10)
                die("effects: bad R line");
            ax = (uint16_t)v[0];
            bx = (uint16_t)v[1];
            cx = (uint16_t)v[2];
            dx = (uint16_t)v[3];
            si = (uint16_t)v[4];
            di = (uint16_t)v[5];
            bp = (uint16_t)v[6];
            es = (uint16_t)v[7];
            ds = (uint16_t)v[8];
            fl = (uint16_t)v[9];
        } else if (strncmp(line, "SP ", 3) == 0) {
            unsigned v;
            if (sscanf(line, "SP %x", &v) != 1) die("effects: bad SP line");
            sp = (uint16_t)v;
            return;
        } else {
            die("effects: unexpected line");
        }
    }
}

static void lcall(void) {
    static const uint16_t tgt[3][2] = { { 0x0AF8u, 0x0B0Bu }, { 0x00BAu, 0x16ECu }, { 0x00B0u, 0x0052u } };
    uint16_t off = fetch16();
    uint16_t seg = fetch16();
    uint16_t ret = ip;
    if (call_k >= 3) die("more far calls than declared callees");
    if (seg != tgt[call_k][0] || off != tgt[call_k][1]) die("far call target differs from the declared callee");
    if (mut_ret2 && call_k == 1) ret = (uint16_t)(ret - 1); /* retaddr-call2: wrong pushed return IP */
    push16(cs);
    push16(ret);
    apply_effects_block();
    call_k++;
}

static void retf_n(void) {
    uint16_t imm = fetch16();
    if (mut_retf0c) imm = 0x000Cu;
    ip = pop16();
    cs = pop16();
    sp = (uint16_t)(sp + imm);
    done = 1;
}

static void step(void) {
    uint8_t op = fetch8();
    while (op == 0x26) {
        es_override = 1;
        op = fetch8();
    }
    if (op >= 0x50 && op <= 0x57) {
        push16(*reg16(op - 0x50));
    } else if (op >= 0x58 && op <= 0x5F) {
        *reg16(op - 0x58) = pop16();
    } else if (op == 0x0E) {
        push16(cs);
    } else if (op == 0x16) {
        push16(ss);
    } else if (op >= 0xB0 && op <= 0xB7) {
        set8(op - 0xB0, fetch8());
    } else if (op >= 0xB8 && op <= 0xBF) {
        *reg16(op - 0xB8) = fetch16();
    } else {
        switch (op) {
        case 0x00: { Modrm o = modrm(); wr_op(o, 8, alu(ALU_ADD, rd_op(o, 8), get_g(o, 8), 8)); break; }
        case 0x03: { Modrm o = modrm(); set_g(o, 16, alu(ALU_ADD, get_g(o, 16), rd_op(o, 16), 16)); break; }
        case 0x2B: { Modrm o = modrm(); set_g(o, 16, alu(ALU_SUB, get_g(o, 16), rd_op(o, 16), 16)); break; }
        case 0x2D: { uint16_t imm = fetch16(); ax = alu(ALU_SUB, ax, imm, 16); break; }
        case 0x30: { Modrm o = modrm(); wr_op(o, 8, alu(ALU_XOR, rd_op(o, 8), get_g(o, 8), 8)); break; }
        case 0x80: { Modrm o = modrm(); uint8_t imm = fetch8(); grp1(o, 8, imm); break; }
        case 0x83: { Modrm o = modrm(); uint8_t imm = fetch8(); grp1(o, 16, (uint16_t)(int16_t)(int8_t)imm); break; }
        case 0x88: { Modrm o = modrm(); wr_op(o, 8, get_g(o, 8)); break; }
        case 0x89: { Modrm o = modrm(); wr_op(o, 16, get_g(o, 16)); break; }
        case 0x8A: { Modrm o = modrm(); set_g(o, 8, rd_op(o, 8)); break; }
        case 0x8B: { Modrm o = modrm(); set_g(o, 16, rd_op(o, 16)); break; }
        case 0x8D: { Modrm o = modrm(); set_g(o, 16, o.off); break; }
        case 0x98: ax = (uint16_t)(int16_t)(int8_t)(uint8_t)(ax & 0xFFu); break; /* CBW */
        case 0x99: dx = (ax & 0x8000u) ? 0xFFFFu : 0x0000u; break;             /* CWD */
        case 0xC4: { Modrm o = modrm(); uint16_t seg_val = rd16(o.addr + 2); set_g(o, 16, rd16(o.addr)); es = seg_val; break; }
        case 0xC6: { Modrm o = modrm(); uint8_t imm = fetch8(); if (o.regf != 0) die("unsupported C6 member"); wr_op(o, 8, imm); break; }
        case 0xD1: { Modrm o = modrm(); if (o.regf != 4) die("unsupported D1 member"); shl16_by1(o); break; }
        case 0xE9: { int16_t rel = (int16_t)fetch16(); ip = (uint16_t)(ip + rel); break; }
        case 0xEB: jrel8(1); break;
        case 0xF7: {
            Modrm o = modrm();
            if (o.regf == 4) mul16(o);
            else if (o.regf == 7) idiv16(o);
            else die("unsupported F7 member");
            break;
        }
        case 0xFE: { Modrm o = modrm(); if (o.regf != 0) die("unsupported FE member"); wr_op(o, 8, inc8(rd_op(o, 8))); break; }
        case 0xFF: { Modrm o = modrm(); if (o.regf != 6) die("unsupported FF member"); push16(rd_op(o, 16)); break; }
        case 0x72: jrel8((fl & F_CF) != 0); break;
        case 0x73: jrel8((fl & F_CF) == 0); break;
        case 0x74: jrel8((fl & F_ZF) != 0); break;
        case 0x75: jrel8((fl & F_ZF) == 0); break;
        case 0x76: jrel8((fl & (F_CF | F_ZF)) != 0); break;
        case 0x7E: jrel8(((fl & F_ZF) != 0) || (((fl & F_SF) != 0) != ((fl & F_OF) != 0))); break;
        case 0x9A: lcall(); break;
        case 0xCA: retf_n(); break;
        default: {
            char msg[64];
            snprintf(msg, sizeof msg, "unsupported opcode 0x%02X at %04X", (unsigned)op, (unsigned)(ip - 1));
            die(msg);
        }
        }
    }
    es_override = 0;
}

int main(int argc, char **argv) {
    const char *mutant = "none";
    FILE *f;
    unsigned v[14];
    unsigned steps = 0;
    int i;
    if (argc != 6 && argc != 7) {
        fprintf(stderr, "usage: compose BODY_BIN ENTRY_RAM ENTRY_REGS14 EFFECTS OUT_RAM [MUTANT]\n");
        return 2;
    }
    if (argc == 7) mutant = argv[6];

    f = fopen(argv[1], "rb");
    if (!f || fread(body, 1, BODY_LEN, f) != BODY_LEN) die("cannot read body (324 bytes)");
    fclose(f);

    if (strcmp(mutant, "none") == 0) {
        /* baseline: the pinned bytes */
    } else if (strcmp(mutant, "retf-pop-0C") == 0) {
        mut_retf0c = 1;
    } else if (strcmp(mutant, "retaddr-call2") == 0) {
        mut_ret2 = 1;
    } else if (strcmp(mutant, "arg-byte-call2") == 0) {
        if (body[0x24F - BODY_BASE] != 0x0A) die("arg-byte-call2: expected AL 0Ah at +024F");
        body[0x24F - BODY_BASE] = 0x0B;
    } else if (strcmp(mutant, "arg-byte-call3") == 0) {
        if (body[0x271 - BODY_BASE] != 0x3E) die("arg-byte-call3: expected AL 3Eh at +0271");
        body[0x271 - BODY_BASE] = 0x3F;
    } else {
        die("unknown mutant");
    }

    f = fopen(argv[2], "rb");
    if (!f) die("cannot open entry RAM");
    if (fread(mem, 1, RAM_BELOW, f) != RAM_BELOW) die("entry RAM must be 655360 bytes");
    if (fgetc(f) != EOF) die("entry RAM longer than 655360 bytes");
    fclose(f);

    f = fopen(argv[3], "r");
    if (!f) die("cannot open entry regs");
    for (i = 0; i < 14; i++)
        if (fscanf(f, "%x", &v[i]) != 1) die("entry regs: need 14 hex fields");
    fclose(f);
    ax = (uint16_t)v[0];
    bx = (uint16_t)v[1];
    cx = (uint16_t)v[2];
    dx = (uint16_t)v[3];
    si = (uint16_t)v[4];
    di = (uint16_t)v[5];
    bp = (uint16_t)v[6];
    sp = (uint16_t)v[7];
    cs = (uint16_t)v[8];
    ip = (uint16_t)v[9];
    ds = (uint16_t)v[10];
    es = (uint16_t)v[11];
    ss = (uint16_t)v[12];
    fl = (uint16_t)v[13];
    if (cs != BODY_SEG || ip != BODY_BASE) die("entry state is not at 30FF:0233");

    eff = fopen(argv[4], "r");
    if (!eff) die("cannot open effects");

    while (!done) {
        if (++steps > MAX_STEPS) die("step limit reached");
        step();
    }
    if (call_k != 3) die("the body did not make the three declared far calls");
    {
        int extra = fgetc(eff);
        if (extra != EOF) die("effects file has unconsumed content");
    }
    fclose(eff);

    f = fopen(argv[5], "wb");
    if (!f) die("cannot open output RAM");
    if (fwrite(mem, 1, RAM_BELOW, f) != RAM_BELOW) die("short write of output RAM");
    fclose(f);

    {
        char regs_path[4096];
        if (snprintf(regs_path, sizeof regs_path, "%s.regs", argv[5]) >= (int)sizeof regs_path) die("path too long");
        f = fopen(regs_path, "w");
        if (!f) die("cannot open output regs");
        fprintf(f, "%04X %04X %04X %04X %04X %04X %04X %04X %04X %04X %04X %04X %04X %04X\n",
                ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, fl);
        fclose(f);
    }
    printf("mutant=%s instructions=%u calls=%d final CS:IP=%04X:%04X SP=%04X FLAGS=%04X\n",
           mutant, steps, call_k, cs, ip, sp, fl);
    return 0;
}
