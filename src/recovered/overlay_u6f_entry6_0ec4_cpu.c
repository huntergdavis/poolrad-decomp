/* Unit 0x6F [0EC4,1262), the monster-record loader, raw CPU (docs/re/OVL_006F_0EC4_CPU.md). */
#include "poolrad/recovered/overlay_u6f_entry6_0ec4_cpu.h"
#include "poolrad/recovered/overlay_u9e_entry8_fullstate_compose_0de3.h"
#include "poolrad/recovered/system_freemem_0254_cpu.h"
#include "poolrad/recovered/system_getmem_023f_cpu.h"
#include "poolrad/recovered/system_str_1366_cpu.h"
#include "poolrad/recovered/system_string_leaves_cpu.h"
#include "poolrad/recovered/system_string_move_leaves_cpu.h"
#include <string.h>

#define MASK (PR_BUS_RAM_SIZE - 1u)
#define COPY(dst, src) do { (void)sizeof(char[sizeof *(dst) == sizeof *(src) ? 1 : -1]); memcpy((dst), (src), sizeof *(dst)); } while (0)
#define CONTINUE (-1)
typedef pr_u6f_0ec4_cpu cpu_t;
typedef struct {
    uint8_t *ram;
    pr_bus *bus;
    size_t size;
    cpu_t *c;
    const pr_u6f_0ec4_env *e;
    pr_u6f_0ec4_diagnostic *d;
} machine;

static size_t physical(uint16_t seg, uint16_t off) { return (((size_t)seg << 4u) + off) & MASK; }
static uint8_t rd8(const uint8_t *ram, uint16_t seg, uint16_t off) { return ram[physical(seg, off)]; }
static uint16_t rd16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8u));
}
static void wr8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t v) { ram[physical(seg, off)] = v; }
static void wr16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t v)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)v; ram[(at + 1u) & MASK] = (uint8_t)(v >> 8u);
}
static void push(machine *m, uint16_t v) { m->c->sp = (uint16_t)(m->c->sp - 2u); wr16(m->ram, m->c->ss, m->c->sp, v); }
static void set_al(cpu_t *c, uint8_t v) { c->ax = (uint16_t)((c->ax & 0xFF00u) | v); }
static uint16_t bp16(machine *m, int off) { return rd16(m->ram, m->c->ss, (uint16_t)(m->c->bp + off)); }
static uint8_t bp8(machine *m, int off) { return rd8(m->ram, m->c->ss, (uint16_t)(m->c->bp + off)); }
static void set_bp16(machine *m, int off, uint16_t v) { wr16(m->ram, m->c->ss, (uint16_t)(m->c->bp + off), v); }
static void lea_di(machine *m, int off) { m->c->di = (uint16_t)(m->c->bp + off); }
static void les_bp(machine *m, int off)                                            /* LES DI,SS:[BP+off] */
{
    m->c->di = bp16(m, off); m->c->es = bp16(m, off + 2);
}
static void push_ss_di(machine *m) { push(m, m->c->ss); push(m, m->c->di); }
static void push_cs_di(machine *m, uint16_t imm) { m->c->di = imm; push(m, m->c->cs); push(m, m->c->di); }   /* MOV DI,imm; PUSH CS; PUSH DI */
static unsigned parity(uint8_t v) { v ^= (uint8_t)(v >> 4); v ^= (uint8_t)(v >> 2); v ^= (uint8_t)(v >> 1); return (v & 1u) ? 0u : 0x04u; }
#define ARITH 0x08D5u
static void sub_flags(cpu_t *c, unsigned a, unsigned b, unsigned msb)              /* SUB / CMP of width msb*2: the six arithmetic flags */
{
    unsigned mask = msb * 2u - 1u, r = (a - b) & mask;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? 0x40u : 0u) | (r & msb ? 0x80u : 0u) | parity((uint8_t)r) | ((a & mask) < (b & mask) ? 0x01u : 0u) |
                          ((a ^ b ^ r) & 0x10u) | (((a ^ b) & (a ^ r) & msb) ? 0x800u : 0u));
}
static void add16_flags(cpu_t *c, unsigned a, unsigned b)
{
    unsigned r = (a + b) & 0xFFFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | (r == 0u ? 0x40u : 0u) | (r & 0x8000u ? 0x80u : 0u) | parity((uint8_t)r) | (a + b > 0xFFFFu ? 0x01u : 0u) |
                          ((a ^ b ^ r) & 0x10u) | ((~(a ^ b) & (a ^ r) & 0x8000u) ? 0x800u : 0u));
}
static void inc8_flags(cpu_t *c, unsigned a)                                       /* INC byte: CF kept */
{
    unsigned r = (a + 1u) & 0xFFu;
    c->flags = (uint16_t)((c->flags & ~(ARITH & ~0x01u)) | (r == 0u ? 0x40u : 0u) | (r & 0x80u ? 0x80u : 0u) | parity((uint8_t)r) | ((a ^ 1u ^ r) & 0x10u) |
                          (r == 0x80u ? 0x800u : 0u));
}
static void zero_flags(cpu_t *c) { c->flags = (uint16_t)((c->flags & ~ARITH) | 0x44u); }   /* XOR r,r: CF = OF = AF = 0, ZF, PF */

static const uint16_t ENTRY_OF[] = {0x1366u, 0x0B0Bu, 0x0B98u, 0x1685u, 0x023Fu, 0x0254u, 0x0716u, 0x0BF5u, 0x0000u, 0x0104u, 0x0048u};

static int leaf_stop(int s) { return s == PR_SYSTEM_LEAF_RETURNED ? CONTINUE : (int)PR_U6F_0EC4_CALLEE; }

/* LCALL at `site`: opcode and offset are checked before the return frame is pushed; the segment word is live. */
static int far_call(machine *m, uint16_t site, pr_u6f_0ec4_callee kind)
{
    cpu_t *c = m->c;
    pr_u6f_0ec4_diagnostic *d = m->d;
    uint8_t *ram = m->ram;
    uint16_t home = c->cs, back = (uint16_t)(site + 5u), seg, ip = ENTRY_OF[kind];
    int s = CONTINUE;
    d->site = site; d->callee = (int)kind; d->callee_status = -1; c->ip = site;
    if (rd8(ram, home, site) != 0x9Au || rd16(ram, home, (uint16_t)(site + 1u)) != ip) return (int)PR_U6F_0EC4_CODE;
    seg = rd16(ram, home, (uint16_t)(site + 3u));
    if (kind == PR_U6F_0EC4_ROLL) {                      /* the stub entry: a far JMP to 0DE3 in the loaded unit 0x9E */
        if (rd8(ram, seg, 0x48u) != 0xEAu || rd16(ram, seg, 0x49u) != 0x0DE3u) return (int)PR_U6F_0EC4_CODE;
        ip = 0x0DE3u; seg = rd16(ram, seg, 0x4Bu);
    }
    push(m, home); push(m, back);
    c->cs = seg; c->ip = ip;
    switch (kind) {
    case PR_U6F_0EC4_STR: case PR_U6F_0EC4_STRLOAD: case PR_U6F_0EC4_STRCAT: case PR_U6F_0EC4_MOVE: {
        pr_system_leaf_cpu x; int r;
        COPY(&x, c);
        r = kind == PR_U6F_0EC4_STR ? (int)pr_system_str_1366(ram, m->size, &x)
          : kind == PR_U6F_0EC4_STRLOAD ? (int)pr_system_strload_0b0b(ram, m->size, &x)
          : kind == PR_U6F_0EC4_STRCAT ? (int)pr_system_strcat_0b98(ram, m->size, &x) : (int)pr_system_move_1685(ram, m->size, &x);
        COPY(c, &x); d->callee_status = r; s = leaf_stop(r);
        break;
    }
    case PR_U6F_0EC4_GETMEM: {
        pr_system_getmem_cpu x; int r;
        COPY(&x, c); r = (int)pr_system_getmem_023f(ram, m->size, &x, m->e->heap_steps); COPY(c, &x);
        d->callee_status = r;
        s = r == PR_SYSTEM_GETMEM_RETURNED ? CONTINUE : r == PR_SYSTEM_GETMEM_RUNERROR ? (int)PR_U6F_0EC4_RUNERROR
          : r == PR_SYSTEM_GETMEM_TRANSFER ? (int)PR_U6F_0EC4_TRANSFER : r == PR_SYSTEM_GETMEM_LIMIT ? (int)PR_U6F_0EC4_LIMIT : (int)PR_U6F_0EC4_CALLEE;
        break;
    }
    case PR_U6F_0EC4_FREEMEM: {
        pr_system_freemem_cpu x; int r;
        COPY(&x, c); r = (int)pr_system_freemem_0254(ram, m->size, &x, m->e->heap_steps); COPY(c, &x);
        d->callee_status = r;
        s = r == PR_SYSTEM_FREEMEM_RETURNED ? CONTINUE : r == PR_SYSTEM_FREEMEM_RUNERROR ? (int)PR_U6F_0EC4_RUNERROR
          : r == PR_SYSTEM_FREEMEM_TRANSFER ? (int)PR_U6F_0EC4_TRANSFER : r == PR_SYSTEM_FREEMEM_LIMIT ? (int)PR_U6F_0EC4_LIMIT : (int)PR_U6F_0EC4_CALLEE;
        break;
    }
    case PR_U6F_0EC4_LOAD: {
        pr_r0802_0716c_cpu x; int r;
        COPY(&x, c); r = (int)pr_r0802_0716_composed(ram, m->size, &x, &m->e->load, &d->load); COPY(c, &x);
        d->callee_status = r;
        s = r == PR_R0802_0716C_RETURNED ? CONTINUE : r == PR_R0802_0716C_EXIT ? (int)PR_U6F_0EC4_EXIT : r == PR_R0802_0716C_RUNERROR ? (int)PR_U6F_0EC4_RUNERROR
          : r == PR_R0802_0716C_DIVIDE ? (int)PR_U6F_0EC4_DIVIDE : r == PR_R0802_0716C_BREAK ? (int)PR_U6F_0EC4_BREAK
          : r == PR_R0802_0716C_STALLED ? (int)PR_U6F_0EC4_STALLED : r == PR_R0802_0716C_LIMIT ? (int)PR_U6F_0EC4_LIMIT
          : r == PR_R0802_0716C_SERVICE ? (int)PR_U6F_0EC4_SERVICE : (int)PR_U6F_0EC4_CALLEE;
        break;
    }
    case PR_U6F_0EC4_MSG: {
        pr_r0709_0bf5_cpu x; int r;
        COPY(&x, c); r = (int)pr_r0709_msgbox_0bf5(m->bus, m->size, &x, &m->e->msg, &d->msg); COPY(c, &x);
        d->callee_status = r;
        if (r == PR_R0709_0BF5_RETURNED) s = CONTINUE;
        else if (r == PR_R0709_0BF5_KEY) {
            int k = d->msg.key_status;
            s = k == PR_R0899_KEY_EXIT ? (int)PR_U6F_0EC4_EXIT : k == PR_R0899_KEY_BREAK ? (int)PR_U6F_0EC4_BREAK
              : k == PR_R0899_KEY_LIMIT ? (int)PR_U6F_0EC4_LIMIT : k == PR_R0899_KEY_CRT ? (int)PR_U6F_0EC4_SERVICE : (int)PR_U6F_0EC4_CALLEE;
        } else s = (int)PR_U6F_0EC4_CALLEE;
        break;
    }
    case PR_U6F_0EC4_QUIT: {
        pr_r0899_cpu x; int r;
        COPY(&x, c); r = (int)pr_r0899_quit_0000_cpu(ram, m->size, &x, &m->e->msg.key.quit, &d->quit); COPY(c, &x);
        d->callee_status = r;
        return r == PR_R0899_QUIT_EXIT ? (int)PR_U6F_0EC4_EXIT : (int)PR_U6F_0EC4_CALLEE;     /* Halt never returns */
    }
    case PR_U6F_0EC4_IDLE: {
        pr_r0899_idle_cpu x; int r;
        COPY(&x, c); r = (int)pr_r0899_idle_0104_cpu(ram, m->size, &x); COPY(c, &x);
        d->callee_status = r; s = r == PR_R0899_IDLE_OK ? CONTINUE : (int)PR_U6F_0EC4_CALLEE;
        break;
    }
    case PR_U6F_0EC4_ROLL: {
        pr_u9e_entry8_compose_cpu x; int r;
        COPY(&x, c); r = (int)pr_u9e_entry8_fullstate_compose_0de3(ram, m->size, &x, m->e->roll_calls); COPY(c, &x);
        d->callee_status = r;
        s = r == PR_U9E_ENTRY8_COMPOSE_RETURNED ? CONTINUE : r == PR_U9E_ENTRY8_COMPOSE_LIMIT ? (int)PR_U6F_0EC4_LIMIT : (int)PR_U6F_0EC4_CALLEE;
        break;
    }
    default: return (int)PR_U6F_0EC4_CALLEE;
    }
    if (s != CONTINUE) return s;
    if (c->cs != home || c->ip != back) return (int)PR_U6F_0EC4_TRANSFER;
    ++d->calls; if (kind == PR_U6F_0EC4_ROLL) ++d->rolls;
    d->site = 0; d->callee = -1; d->callee_status = -1;
    return CONTINUE;
}

#define CALL(site, kind) do { int r_ = far_call(&m, (site), (kind)); if (r_ != CONTINUE) return (pr_u6f_0ec4_status)r_; } while (0)
#define FAR_OBJECT_LES() les_bp(&m, -0x18)

/* The block's name and extension, then 0802:0716 (id byte [BP+0Ah], &size [BP-2], &block [BP-6]). */
static int load_block(machine *m, const uint16_t sites[5], uint16_t suffix)
{
    cpu_t *c = m->c;
    lea_di(m, -0x114); push_ss_di(m); push_cs_di(m, 0x0E98u);
    { int r = far_call(m, sites[0], PR_U6F_0EC4_STRLOAD); if (r != CONTINUE) return r; }
    lea_di(m, -0x12); push_ss_di(m);
    { int r = far_call(m, sites[1], PR_U6F_0EC4_STRCAT); if (r != CONTINUE) return r; }
    push_cs_di(m, suffix);
    { int r = far_call(m, sites[2], PR_U6F_0EC4_STRCAT); if (r != CONTINUE) return r; }
    lea_di(m, -0x119); push_ss_di(m); push_cs_di(m, 0x0EA0u);
    { int r = far_call(m, sites[3], PR_U6F_0EC4_STRLOAD); if (r != CONTINUE) return r; }
    set_al(c, bp8(m, 0x0A)); push(m, c->ax);
    lea_di(m, -2); push_ss_di(m); lea_di(m, -6); push_ss_di(m);
    return far_call(m, sites[4], PR_U6F_0EC4_LOAD);
}

/* FreeMem(block [BP-6], size [BP-2]): PUSH [BP-4]; PUSH [BP-6]; PUSH [BP-2]. */
static int free_block(machine *m, uint16_t site)
{
    push(m, bp16(m, -4)); push(m, bp16(m, -6)); push(m, bp16(m, -2));
    return far_call(m, site, PR_U6F_0EC4_FREEMEM);
}

/* The unit chain of blocks 2 and 3: `unit`-byte GetMem/Move cells from R+field, links at +link, cursor at [BP+cur]. */
static int unit_chain(machine *m, uint16_t unit, uint16_t field, uint16_t link, int cur, const uint16_t sites[4])
{
    cpu_t *c = m->c;
    uint8_t *ram = m->ram;
    int r;
    c->ax = 0; zero_flags(c); set_bp16(m, -8, c->ax);                                   /* XOR AX,AX; MOV [BP-8],AX */
    c->ax = unit; push(m, c->ax);
    if ((r = far_call(m, sites[0], PR_U6F_0EC4_GETMEM)) != CONTINUE) return r;
    les_bp(m, 6); wr16(ram, c->es, (uint16_t)(c->di + field), c->ax); wr16(ram, c->es, (uint16_t)(c->di + field + 2u), c->dx);
    les_bp(m, 6); c->ax = rd16(ram, c->es, (uint16_t)(c->di + field)); c->dx = rd16(ram, c->es, (uint16_t)(c->di + field + 2u));
    set_bp16(m, cur, c->ax); set_bp16(m, cur + 2, c->dx);
    for (;;) {                                                                          /* 10FC / 11E6 */
        c->ax = bp16(m, -8); les_bp(m, -6);
        add16_flags(c, c->di, c->ax); c->di = (uint16_t)(c->di + c->ax);               /* ADD DI,AX */
        push(m, c->es); push(m, c->di);
        les_bp(m, cur); push(m, c->es); push(m, c->di);
        c->ax = unit; push(m, c->ax);
        if ((r = far_call(m, sites[1], PR_U6F_0EC4_MOVE)) != CONTINUE) return r;
        les_bp(m, cur); c->ax = 0; zero_flags(c);
        wr16(ram, c->es, (uint16_t)(c->di + link), c->ax); wr16(ram, c->es, (uint16_t)(c->di + link + 2u), c->ax);
        { unsigned a = bp16(m, -8); add16_flags(c, a, unit); set_bp16(m, -8, (uint16_t)(a + unit)); }   /* ADD word [BP-8],unit */
        c->ax = bp16(m, -8); sub_flags(c, c->ax, bp16(m, -2), 0x8000u);               /* CMP AX,[BP-2]; JAE */
        if (!(c->flags & 1u)) {
            /* 1152: the same compare again */
        } else {
            c->ax = unit; push(m, c->ax);
            if ((r = far_call(m, sites[2], PR_U6F_0EC4_GETMEM)) != CONTINUE) return r;
            les_bp(m, cur); wr16(ram, c->es, (uint16_t)(c->di + link), c->ax); wr16(ram, c->es, (uint16_t)(c->di + link + 2u), c->dx);
            les_bp(m, cur); c->ax = rd16(ram, c->es, (uint16_t)(c->di + link)); c->dx = rd16(ram, c->es, (uint16_t)(c->di + link + 2u));
            set_bp16(m, cur, c->ax); set_bp16(m, cur + 2, c->dx);
        }
        c->ax = bp16(m, -8); sub_flags(c, c->ax, bp16(m, -2), 0x8000u);               /* CMP AX,[BP-2]; JB loop */
        if (!(c->flags & 1u)) break;
    }
    return free_block(m, sites[3]);
}

pr_u6f_0ec4_status pr_u6f_entry6_0ec4_cpu(pr_bus *bus, size_t ram_size, pr_u6f_0ec4_cpu *c, const pr_u6f_0ec4_env *env,
                                          pr_u6f_0ec4_diagnostic *diagnostic)
{
    static const uint16_t block1[5] = {0x0EF0u, 0x0EFAu, 0x0F04u, 0x0F14u, 0x0F27u};
    static const uint16_t block2[5] = {0x108Bu, 0x1095u, 0x109Fu, 0x10AFu, 0x10C2u};
    static const uint16_t block3[5] = {0x1173u, 0x117Du, 0x1187u, 0x1197u, 0x11AAu};
    static const uint16_t chain2[4] = {0x10D9u, 0x110Fu, 0x1131u, 0x1163u};
    static const uint16_t chain3[4] = {0x11C1u, 0x11F9u, 0x121Bu, 0x124Du};
    pr_u6f_0ec4_diagnostic local, *d = diagnostic ? diagnostic : &local;
    machine m;
    uint8_t *ram;
    unsigned idx_end, k, a;
    int r;
    if (bus == NULL || bus->ram == NULL || c == NULL || env == NULL || ram_size != PR_BUS_RAM_SIZE || c->ip != 0x0EC4u || c->sp < 0x09F4u || c->sp > 0xFFF4u)
        return PR_U6F_0EC4_ARGUMENT;
    ram = bus->ram;
    memset(d, 0, sizeof *d); d->callee = -1; d->callee_status = -1;
    m.ram = ram; m.bus = bus; m.size = ram_size; m.c = c; m.e = env; m.d = d;
    push(&m, c->bp); c->bp = c->sp;                                                    /* 0EC4 PUSH BP; MOV BP,SP */
    { unsigned x = c->sp; c->sp = (uint16_t)(x - 0x11Au); sub_flags(c, x, 0x11Au, 0x8000u); }   /* 0EC7 SUB SP,11Ah */
    set_al(c, rd8(ram, c->ds, 0x5376u)); c->ax &= 0x00FFu; zero_flags(c);              /* 0ECB MOV AL,[5376]; XOR AH,AH */
    c->dx = 0; zero_flags(c); push(&m, c->dx); push(&m, c->ax);                        /* 0ED0 XOR DX,DX; PUSH DX; PUSH AX */
    c->ax = 0; zero_flags(c); push(&m, c->ax);                                         /* 0ED4 XOR AX,AX; PUSH AX: Width 0 */
    lea_di(&m, -0x12); push_ss_di(&m); c->ax = 1; push(&m, c->ax);                    /* 0ED7 S := [BP-12h], M := 1 */
    CALL(0x0EE0u, PR_U6F_0EC4_STR);
    r = load_block(&m, block1, 0x0E9Cu);                                                /* 0EE5..0F27 */
    if (r != CONTINUE) return (pr_u6f_0ec4_status)r;
    sub_flags(c, bp16(&m, -2), 0u, 0x8000u);                                           /* 0F2C CMP word [BP-2],0 */
    if (bp16(&m, -2) == 0u) {
        set_al(c, 0x0Fu); push(&m, c->ax);                                             /* 0F32 MOV AL,0Fh; PUSH AX: the colour */
        lea_di(&m, -0x2B); push_ss_di(&m); push_cs_di(&m, 0x0EA5u);
        CALL(0x0F3Fu, PR_U6F_0EC4_STRLOAD);
        CALL(0x0F44u, PR_U6F_0EC4_MSG);
        CALL(0x0F49u, PR_U6F_0EC4_QUIT);
        return PR_U6F_0EC4_TRANSFER;                                                    /* unreachable: QUIT never returns */
    }
    les_bp(&m, -6); push(&m, c->es); push(&m, c->di);                                  /* 0F4E LES DI,[BP-6]; PUSH ES; PUSH DI */
    les_bp(&m, 6); push(&m, c->es); push(&m, c->di);                                   /* 0F53 LES DI,[BP+6]; PUSH ES; PUSH DI */
    c->ax = 0x011Du; push(&m, c->ax);
    CALL(0x0F5Cu, PR_U6F_0EC4_MOVE);
    les_bp(&m, 6); set_bp16(&m, -0x18, c->di); set_bp16(&m, -0x16, c->es);            /* 0F61..0F6A R copy := [BP+6] */
    {
        static const uint16_t clear[4][2] = {{0xC8u, 0xCAu}, {0x7Fu, 0x81u}, {0x104u, 0x106u}, {0x108u, 0x10Au}};
        for (k = 0; k < 4u; ++k) {                                                     /* 0F6A..0FA0 LES; XOR AX,AX; two stores */
            FAR_OBJECT_LES(); c->ax = 0; zero_flags(c);
            wr16(ram, c->es, (uint16_t)(c->di + clear[k][0]), c->ax); wr16(ram, c->es, (uint16_t)(c->di + clear[k][1]), c->ax);
        }
    }
    FAR_OBJECT_LES(); set_al(c, rd8(ram, c->es, (uint16_t)(c->di + 0x2Eu)));          /* 0FA5 MOV AL,ES:[DI+2Eh] */
    {
        unsigned al = c->ax & 0xFFu;
        int one = 0, two = 0;
        sub_flags(c, al, 1u, 0x80u);
        if (c->flags & 0x40u) one = 1;
        else { sub_flags(c, al, 3u, 0x80u);
            if (c->flags & 0x40u) one = 1;
            else { sub_flags(c, al, 5u, 0x80u); if (c->flags & 0x40u) one = 1; } }
        if (!one) {
            sub_flags(c, al, 2u, 0x80u);
            if (c->flags & 0x40u) two = 1;
            else { sub_flags(c, al, 4u, 0x80u);
                if (c->flags & 0x40u) two = 1;
                else { sub_flags(c, al, 6u, 0x80u);
                    if (c->flags & 0x40u) two = 1;
                    else { sub_flags(c, al, 7u, 0x80u); if (c->flags & 0x40u) two = 1; } } }
        }
        if (one) { FAR_OBJECT_LES(); wr8(ram, c->es, (uint16_t)(c->di + 0xC0u), 1u); }
        else if (two) { FAR_OBJECT_LES(); wr8(ram, c->es, (uint16_t)(c->di + 0xC0u), 2u); }
    }
    for (idx_end = 3u, k = 0; k < 2u; ++k, idx_end = 6u) {                              /* 0FDC..1067: the two roll loops (indices 1..3, 5..6) */
        if (k == 1u) { FAR_OBJECT_LES(); wr8(ram, c->es, (uint16_t)(c->di + 0xC4u), 0xC4u); }   /* 1020 R+C4 := C4h */
        wr8(ram, c->ss, (uint16_t)(c->bp - 0x13), k == 0u ? 1u : 5u);                           /* 0FDC / 1029 index := 1 / 5 */
        for (;;) {
            set_al(c, 1u); push(&m, c->ax); set_al(c, 6u); push(&m, c->ax);            /* MOV AL,1; PUSH AX; MOV AL,6; PUSH AX */
            CALL(k == 0u ? 0x0FEBu : 0x1038u, PR_U6F_0EC4_ROLL);
            wr8(ram, c->ss, (uint16_t)(c->bp - 0x14), (uint8_t)c->ax);                 /* MOV [BP-14h],AL */
            a = bp8(&m, -0x14);
            c->ax = (uint16_t)a; zero_flags(c);                                        /* MOV AL,[BP-14h]; XOR AH,AH */
            c->ax = (uint16_t)(c->ax + 8u);                                            /* ADD AX,8 (flags overwritten by the SHL and XOR) */
            c->cx = 4u; c->ax = (uint16_t)(c->ax << 4u);                               /* MOV CX,4; SHL AX,CL (flags dead: XOR AH,AH follows) */
            c->dx = c->ax;
            c->ax = (uint16_t)a; zero_flags(c);
            c->ax = (uint16_t)(c->ax + c->dx);                                         /* ADD AX,DX (flags dead) */
            c->dx = (uint16_t)((c->dx & 0xFF00u) | (c->ax & 0xFFu));                   /* MOV DL,AL */
            set_al(c, bp8(&m, -0x13)); c->ax &= 0x00FFu; zero_flags(c);                /* MOV AL,[BP-13h]; XOR AH,AH */
            FAR_OBJECT_LES();
            add16_flags(c, c->di, c->ax); c->di = (uint16_t)(c->di + c->ax);           /* ADD DI,AX */
            wr8(ram, c->es, (uint16_t)(c->di + 0xC0u), (uint8_t)c->dx);
            sub_flags(c, bp8(&m, -0x13), idx_end, 0x80u);                              /* CMP byte [BP-13h],end; JNE */
            if (bp8(&m, -0x13) == idx_end) break;
            inc8_flags(c, bp8(&m, -0x13)); wr8(ram, c->ss, (uint16_t)(c->bp - 0x13), (uint8_t)(bp8(&m, -0x13) + 1u));
        }
    }
    r = free_block(&m, 0x1076u);                                                        /* 106D..1076 FreeMem(block, size) */
    if (r != CONTINUE) return (pr_u6f_0ec4_status)r;
    wr8(ram, c->ds, 0x557Cu, 1u);                                                       /* 107B MOV byte [557C],1 */
    r = load_block(&m, block2, 0x0EBCu);
    if (r != CONTINUE) return (pr_u6f_0ec4_status)r;
    sub_flags(c, bp16(&m, -2), 0u, 0x8000u);                                           /* 10C7 CMP word [BP-2],0; JNE 10D0; JMP 1168 */
    if (bp16(&m, -2) != 0u) {
        r = unit_chain(&m, 9u, 0x7Fu, 5u, -0x10, chain2);
        if (r != CONTINUE) return (pr_u6f_0ec4_status)r;
    }
    r = load_block(&m, block3, 0x0EC0u);
    if (r != CONTINUE) return (pr_u6f_0ec4_status)r;
    sub_flags(c, bp16(&m, -2), 0u, 0x8000u);                                           /* 11AF CMP word [BP-2],0; JNE 11B8; JMP 1252 */
    if (bp16(&m, -2) != 0u) {
        r = unit_chain(&m, 0x3Fu, 0xC8u, 0x2Au, -0xC, chain3);
        if (r != CONTINUE) return (pr_u6f_0ec4_status)r;
    }
    wr8(ram, c->ds, 0x557Cu, 0u);                                                       /* 1252 MOV byte [557C],0 */
    CALL(0x1257u, PR_U6F_0EC4_IDLE);
    c->sp = c->bp;                                                                      /* 125C MOV SP,BP */
    c->bp = rd16(ram, c->ss, c->sp); c->sp = (uint16_t)(c->sp + 2u);                    /* 125E POP BP */
    c->ip = rd16(ram, c->ss, c->sp); c->cs = rd16(ram, c->ss, (uint16_t)(c->sp + 2u));  /* 125F RETF 6 */
    c->sp = (uint16_t)(c->sp + 4u + 6u);
    return PR_U6F_0EC4_RETURNED;
}
