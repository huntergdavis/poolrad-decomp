/* GAME.OVR unit 0x25:+0F53..+1152, complete 512-byte store string (public
 * entry 12), composed with its near +06FB..+0755 address classifier (hook
 * and composed entries). */
#include "poolrad/recovered/overlay_u25_store_string_0f53.h"
#include "poolrad/recovered/system_strstore_fullstate_0b25.h"

#include <string.h>

#define RAM_MASK (PR_U25_0F53_RAM_SIZE - 1u)
#define ARITH_FLAGS 0x08D5u /* OF SF ZF AF PF CF */

#define ENTRY_IP 0x0F53u
#define STORE_ENTRY 0x0B25u

_Static_assert(sizeof(pr_u25_0f53_cpu) == sizeof(pr_system_0b25_cpu), "SYSTEM 0B25 CPU layout differs");

typedef struct {
    uint8_t *ram;
    size_t size;
    pr_u25_0f53_cpu *cpu;
    pr_u25_0f53_external external;  /* NULL: composed entry */
    void *context;
    uint16_t entry;
    int capped;
    unsigned max_loopbacks;
    int *callee_status;
    pr_u25_0f53_status stop;
} machine;

/* One element loop: the far pointer slot in DS, the element displacement,
 * and whether elements are words (classes 0-2) or bytes (class 3). */
typedef struct {
    uint16_t pointer, disp;
    int words;
    uint16_t inc;  /* original offset of the loop's INC BYTE [BP-0x103] */
} element_loop;

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & RAM_MASK;
}

static uint8_t read8(const machine *m, uint16_t seg, uint16_t off)
{
    return m->ram[physical(seg, off)];
}

static uint16_t read16(const machine *m, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    return (uint16_t)(m->ram[a] | ((uint16_t)m->ram[(a + 1u) & RAM_MASK] << 8));
}

static void write8(machine *m, uint16_t seg, uint16_t off, uint8_t value)
{
    m->ram[physical(seg, off)] = value;
}

static void write16(machine *m, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t a = physical(seg, off);
    m->ram[a] = (uint8_t)value;
    m->ram[(a + 1u) & RAM_MASK] = (uint8_t)(value >> 8);
}

static void push16(machine *m, uint16_t value)
{
    m->cpu->sp = (uint16_t)(m->cpu->sp - 2u);
    write16(m, m->cpu->ss, m->cpu->sp, value);
}

static uint16_t pop16(machine *m)
{
    uint16_t value = read16(m, m->cpu->ss, m->cpu->sp);
    m->cpu->sp = (uint16_t)(m->cpu->sp + 2u);
    return value;
}

static uint16_t read16_at(const machine *m, size_t a)
{
    return (uint16_t)(m->ram[a & RAM_MASK] | ((uint16_t)m->ram[(a + 1u) & RAM_MASK] << 8));
}

/* RETF n: the return CS is the next two physical bytes after the IP word. */
static void retf(machine *m, uint16_t args)
{
    pr_u25_0f53_cpu *cpu = m->cpu;
    size_t a = physical(cpu->ss, cpu->sp);
    cpu->ip = read16_at(m, a);
    cpu->cs = read16_at(m, a + 2u);
    cpu->sp = (uint16_t)(cpu->sp + 4u + args);
}

/* An original offset as the running code sees it: absolute in the hook
 * entry, relative to the entry IP in the composed entry. */
static uint16_t here(const machine *m, uint16_t offset)
{
    return m->external != NULL ? offset : (uint16_t)(m->entry + (uint16_t)(offset - ENTRY_IP));
}

static int parity(uint8_t value)
{
    value ^= (uint8_t)(value >> 4);
    value ^= (uint8_t)(value >> 2);
    value ^= (uint8_t)(value >> 1);
    return (value & 1u) == 0u;
}

/* ADD (subtract == 0) or SUB/CMP flags at the width given by sign. */
static uint16_t arith(pr_u25_0f53_cpu *cpu, uint16_t a, uint16_t b,
                      uint16_t sign, int subtract)
{
    uint16_t mask = (uint16_t)(sign | (sign - 1u));
    uint32_t wide = subtract ? (uint32_t)a - b : (uint32_t)a + b;
    uint16_t r = (uint16_t)(wide & mask);
    cpu->flags &= (uint16_t)~ARITH_FLAGS;
    if (subtract ? a < b : wide > mask) cpu->flags |= 0x0001u;
    if (parity((uint8_t)r)) cpu->flags |= 0x0004u;
    if ((a ^ b ^ r) & 0x10u) cpu->flags |= 0x0010u;
    if (r == 0u) cpu->flags |= 0x0040u;
    if (r & sign) cpu->flags |= 0x0080u;
    if ((subtract ? (uint16_t)(a ^ b) : (uint16_t)~(a ^ b)) & (a ^ r) & sign)
        cpu->flags |= 0x0800u;
    return r;
}

static int zf(const pr_u25_0f53_cpu *cpu)
{
    return (cpu->flags & 0x0040u) != 0u;
}

/* XOR AH,AH / XOR AX,AX. AF is architecturally undefined; the reference
 * emulator's cleared AF is used. */
static void xor_ah_ah(pr_u25_0f53_cpu *cpu)
{
    cpu->ax &= 0x00FFu;
    cpu->flags = (uint16_t)((cpu->flags & ~ARITH_FLAGS) | 0x0044u);
}

static void set_al(pr_u25_0f53_cpu *cpu, uint8_t value)
{
    cpu->ax = (uint16_t)((cpu->ax & 0xFF00u) | value);
}

/* LES DI,[seg:off]: the segment word is the next two physical bytes. */
static void les_di(machine *m, uint16_t seg, uint16_t off)
{
    size_t a = physical(seg, off);
    m->cpu->di = read16_at(m, a);
    m->cpu->es = read16_at(m, a + 2u);
}

static uint8_t local8(const machine *m, uint16_t disp)
{
    return read8(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp));
}

static void set_local8(machine *m, uint16_t disp, uint8_t value)
{
    write8(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp), value);
}

static uint16_t arg16(const machine *m, uint16_t disp)
{
    return read16(m, m->cpu->ss, (uint16_t)(m->cpu->bp + disp));
}

/* The LCALL at original offset `site`, with the five argument words on SS:SP. The hook entry hands it
 * to the hook and removes the arguments; the composed entry reads 9A 25 0B seg from RAM, pushes CS and
 * the next IP and runs the raw-CPU string store, whose RETF 0Ah removes the frame and the arguments. */
static int far_call(machine *m, pr_u25_0f53_call call, uint16_t site)
{
    pr_u25_0f53_cpu *cpu = m->cpu;
    uint16_t at = here(m, site), return_ip = (uint16_t)(at + 5u), caller_cs = cpu->cs;
    pr_system_0b25_cpu leaf;
    pr_system_0b25_status status;
    cpu->ip = at;
    if (m->external != NULL) {
        if (!m->external(m->context, m->ram, m->size, cpu, call)) {
            m->stop = PR_U25_0F53_CALL_ABORT;
            return 0;
        }
        cpu->sp = (uint16_t)(cpu->sp + 0x0Au);
        cpu->ip = return_ip;
        return 1;
    }
    if (read8(m, cpu->cs, at) != 0x9Au || read16(m, cpu->cs, (uint16_t)(at + 1u)) != STORE_ENTRY) {
        m->stop = PR_U25_0F53_CODE;
        return 0;
    }
    push16(m, cpu->cs);
    push16(m, return_ip);
    cpu->cs = read16(m, caller_cs, (uint16_t)(at + 3u));
    cpu->ip = STORE_ENTRY;
    memcpy(&leaf, cpu, sizeof leaf);
    status = pr_system_strstore_0b25(m->ram, m->size, &leaf);
    memcpy(cpu, &leaf, sizeof leaf);
    if (m->callee_status != NULL)
        *m->callee_status = (int)status;
    if (cpu->ip != return_ip || cpu->cs != caller_cs) {
        m->stop = PR_U25_0F53_TRANSFER;
        return 0;
    }
    return 1;
}

/* +06FB..+0755, entered by PUSH word [bp+0xA]; PUSH CS; CALL +06FB. Its
 * only frame write is SS:[BP'-1], so its RETF 2 always resumes at +0F75. */
static void classify(machine *m)
{
    static const uint16_t ranges[4][2] = {
        {0x4900u, 0x4CFFu}, {0x6B00u, 0x6EFFu},
        {0x9700u, 0x98FFu}, {0x9900u, 0xB6FFu}
    };
    pr_u25_0f53_cpu *cpu = m->cpu;
    unsigned i;
    push16(m, arg16(m, 0x0Au));                            /* +0F6E */
    push16(m, cpu->cs);
    push16(m, here(m, 0x0F75u));
    cpu->ip = here(m, 0x06FBu);
    push16(m, cpu->bp);                                    /* +06FB */
    cpu->bp = cpu->sp;
    cpu->sp = arith(cpu, cpu->sp, 2u, 0x8000u, 1);
    set_local8(m, 0xFFFFu, 4u);
    for (i = 0u; i < 4u; ++i) {
        (void)arith(cpu, arg16(m, 6u), ranges[i][0], 0x8000u, 1);
        if (cpu->flags & 0x0001u) continue;                /* JB */
        (void)arith(cpu, arg16(m, 6u), ranges[i][1], 0x8000u, 1);
        if ((cpu->flags & 0x0041u) == 0u) continue;        /* JA */
        set_local8(m, 0xFFFFu, (uint8_t)i);
    }
    set_al(cpu, local8(m, 0xFFFFu));                       /* +074D */
    cpu->sp = cpu->bp;
    cpu->bp = pop16(m);
    retf(m, 2u);
}

/* mov al,[bp-0x103 or -0x102]; xor ah,ah; add ax,[bp+0xA]; [shl ax,1];
 * les di,[ptr]; add di,ax. The ADD AX and SHL flags are dead; ADD DI's
 * flags reach the return on the terminator path of class 3. */
static void element(machine *m, const element_loop *loop, uint16_t index)
{
    pr_u25_0f53_cpu *cpu = m->cpu;
    set_al(cpu, local8(m, index));
    xor_ah_ah(cpu);
    cpu->ax = (uint16_t)(cpu->ax + arg16(m, 0x0Au));
    if (loop->words) cpu->ax = (uint16_t)(cpu->ax << 1);
    les_di(m, cpu->ds, loop->pointer);
    cpu->di = arith(cpu, cpu->di, cpu->ax, 0x8000u, 0);
}

/* The four element loops (+0F89, +1017, +1084, +10F0). With a zero
 * length, DEC gives 0xFF, JA is never taken (0 is never above a byte), and
 * all 256 local-buffer bytes are written before the terminator. */
static int store_elements(machine *m, const element_loop *loop)
{
    pr_u25_0f53_cpu *cpu = m->cpu;
    unsigned loopbacks = 0u;
    set_al(cpu, local8(m, 0xFEFEu));                       /* [bp-0x102] */
    xor_ah_ah(cpu);
    cpu->ax = (uint16_t)(cpu->ax - 1u);  /* DEC AX: flags dead */
    set_local8(m, 0xFEFCu, (uint8_t)cpu->ax);              /* [bp-0x104] */
    set_al(cpu, 0u);
    (void)arith(cpu, 0u, local8(m, 0xFEFCu), 0x80u, 1);
    if ((cpu->flags & 0x0041u) == 0u) goto terminator;     /* JA: unreachable */
    set_local8(m, 0xFEFDu, (uint8_t)cpu->ax);              /* [bp-0x103] */
    for (;;) {
        set_al(cpu, local8(m, 0xFEFDu));
        xor_ah_ah(cpu);
        cpu->ax = (uint16_t)(cpu->ax + 1u);  /* INC AX: flags dead */
        cpu->di = cpu->ax;
        if (loop->words) {
            set_al(cpu, read8(m, cpu->ss, (uint16_t)(cpu->bp + cpu->di - 0x100u)));
            xor_ah_ah(cpu);
            cpu->dx = cpu->ax;
        } else {
            cpu->dx = (uint16_t)((cpu->dx & 0xFF00u) |
                read8(m, cpu->ss, (uint16_t)(cpu->bp + cpu->di - 0x100u)));
        }
        element(m, loop, 0xFEFDu);
        if (loop->words)
            write16(m, cpu->es, (uint16_t)(cpu->di + loop->disp), cpu->dx);
        else
            write8(m, cpu->es, (uint16_t)(cpu->di + loop->disp), (uint8_t)cpu->dx);
        set_al(cpu, local8(m, 0xFEFDu));
        (void)arith(cpu, (uint8_t)cpu->ax, local8(m, 0xFEFCu), 0x80u, 1);
        if (zf(cpu)) break;
        if (m->capped && loopbacks++ == m->max_loopbacks) {
            cpu->ip = here(m, loop->inc);
            m->stop = PR_U25_0F53_LIMIT;
            return 0;
        }
        /* INC byte [bp-0x103]: flags dead (XOR AH,AH follows). */
        set_local8(m, 0xFEFDu, (uint8_t)(local8(m, 0xFEFDu) + 1u));
    }
terminator:
    element(m, loop, 0xFEFEu);
    if (loop->words) {
        cpu->ax = 0u;  /* XOR AX,AX */
        cpu->flags = (uint16_t)((cpu->flags & ~ARITH_FLAGS) | 0x0044u);
        write16(m, cpu->es, (uint16_t)(cpu->di + loop->disp), cpu->ax);
    } else {
        write8(m, cpu->es, (uint16_t)(cpu->di + loop->disp), 0u);
    }
    return 1;
}

static pr_u25_0f53_status run(const machine *init)
{
    static const element_loop loops[4] = {
        {0x49D2u, 0x6E00u, 1, 0x0FA2u}, {0x49D6u, 0x2A00u, 1, 0x1030u},
        {0x49DAu, 0xD200u, 1, 0x109Du}, {0x49DEu, 0x6700u, 0, 0x1109u}
    };
    machine m = *init;
    pr_u25_0f53_cpu *cpu = m.cpu;
    uint8_t klass;
    push16(&m, cpu->bp);                                   /* +0F53 */
    cpu->bp = cpu->sp;
    cpu->sp = arith(cpu, cpu->sp, 0x0104u, 0x8000u, 1);
    les_di(&m, cpu->ss, (uint16_t)(cpu->bp + 6u));         /* +0F5A */
    push16(&m, cpu->es);
    push16(&m, cpu->di);
    cpu->di = (uint16_t)(cpu->bp - 0x100u);
    push16(&m, cpu->ss);
    push16(&m, cpu->di);
    cpu->ax = 0x00FFu;
    push16(&m, cpu->ax);
    if (!far_call(&m, PR_U25_0F53_STRSTORE, 0x0F69u))
        return m.stop;
    classify(&m);
    set_local8(&m, 0xFEFFu, (uint8_t)cpu->ax);             /* +0F75 */
    set_al(cpu, local8(&m, 0xFF00u));
    set_local8(&m, 0xFEFEu, (uint8_t)cpu->ax);
    set_al(cpu, local8(&m, 0xFEFFu));
    klass = (uint8_t)cpu->ax;
    (void)arith(cpu, klass, 0u, 0x80u, 1);
    if (zf(cpu)) {
        if (!store_elements(&m, &loops[0])) return m.stop;
    } else if ((void)arith(cpu, klass, 1u, 0x80u, 1), zf(cpu)) {
        (void)arith(cpu, arg16(&m, 0x0Au), 0x6B00u, 0x8000u, 1); /* +0FF9 */
        if (zf(cpu)) {
            cpu->di = (uint16_t)(cpu->bp - 0x100u);
            push16(&m, cpu->ss);
            push16(&m, cpu->di);
            les_di(&m, cpu->ds, 0x5D92u);
            push16(&m, cpu->es);
            push16(&m, cpu->di);
            cpu->ax = 0x000Fu;
            push16(&m, cpu->ax);
            if (!far_call(&m, PR_U25_0F53_STRSTORE, 0x1010u))
                return m.stop;
        } else {
            if (!store_elements(&m, &loops[1])) return m.stop;
        }
    } else if ((void)arith(cpu, klass, 2u, 0x80u, 1), zf(cpu)) {
        if (!store_elements(&m, &loops[2])) return m.stop;
    } else if ((void)arith(cpu, klass, 3u, 0x80u, 1), zf(cpu)) {
        if (!store_elements(&m, &loops[3])) return m.stop;
    }
    cpu->sp = cpu->bp;                                     /* +114D */
    cpu->bp = pop16(&m);
    retf(&m, 6u);
    return PR_U25_0F53_OK;
}

pr_u25_0f53_status pr_u25_store_string_0f53(
    uint8_t *ram, size_t ram_size, pr_u25_0f53_cpu *cpu,
    pr_u25_0f53_external external, void *context)
{
    machine m;
    if (ram == NULL || cpu == NULL || external == NULL ||
        ram_size < PR_U25_0F53_RAM_SIZE)
        return PR_U25_0F53_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu;
    m.external = external; m.context = context;
    m.entry = cpu->ip; m.stop = PR_U25_0F53_OK;
    return run(&m);
}

pr_u25_0f53_status pr_u25_store_string_0f53_composed(
    uint8_t *ram, size_t ram_size, pr_u25_0f53_cpu *cpu,
    unsigned max_loopbacks, int *callee_status)
{
    machine m;
    if (callee_status != NULL)
        *callee_status = -1;
    if (ram == NULL || cpu == NULL || ram_size != PR_U25_0F53_RAM_SIZE)
        return PR_U25_0F53_ARGUMENT;
    memset(&m, 0, sizeof m);
    m.ram = ram; m.size = ram_size; m.cpu = cpu;
    m.external = NULL; m.context = NULL;
    m.entry = cpu->ip; m.capped = 1; m.max_loopbacks = max_loopbacks;
    m.callee_status = callee_status; m.stop = PR_U25_0F53_OK;
    return run(&m);
}
