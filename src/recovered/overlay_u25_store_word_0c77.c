/* DOS v1.3 unit 25 store word +0C77, classifier +06FB, local store hook +0A74.
 * Source order matters: arguments, globals, records and frame locals may
 * alias in the caller-owned RAM. Evidence: docs/re/U25_STORE_WORD_0C77.md. */
#include "poolrad/recovered/overlay_u25_store_word_0c77.h"

#define MASK (PR_U25_0756_RAM_SIZE - 1u)
#define ARITH 0x08D5u

static size_t physical(uint16_t seg, uint16_t off)
{
    return (((size_t)seg << 4) + off) & MASK;
}

static uint8_t r8(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return ram[physical(seg, off)];
}

static uint16_t word_at(const uint8_t *ram, size_t at)
{
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & MASK] << 8));
}

static uint16_t r16(const uint8_t *ram, uint16_t seg, uint16_t off)
{
    return word_at(ram, physical(seg, off));
}

static void w8(uint8_t *ram, uint16_t seg, uint16_t off, uint8_t value)
{
    ram[physical(seg, off)] = value;
}

static void w16(uint8_t *ram, uint16_t seg, uint16_t off, uint16_t value)
{
    size_t at = physical(seg, off);
    ram[at] = (uint8_t)value;
    ram[(at + 1u) & MASK] = (uint8_t)(value >> 8);
}

static uint16_t szp(uint16_t result, unsigned bits)
{
    uint8_t parity = (uint8_t)result;
    uint16_t flags = 0u;
    parity ^= (uint8_t)(parity >> 4);
    parity ^= (uint8_t)(parity >> 2);
    parity ^= (uint8_t)(parity >> 1);
    if (!(parity & 1u)) flags |= 4u;
    if (!result) flags |= 0x40u;
    if (result & (bits == 8u ? 0x80u : 0x8000u)) flags |= 0x80u;
    return flags;
}

static uint16_t arithmetic(pr_u25_store_word_cpu *c, uint16_t left,
                           uint16_t right, unsigned bits, int subtract)
{
    uint32_t wide = subtract ? (uint32_t)left - right : (uint32_t)left + right;
    uint16_t mask = bits == 8u ? 0xFFu : 0xFFFFu;
    uint16_t sign = bits == 8u ? 0x80u : 0x8000u;
    uint16_t result = (uint16_t)wide & mask;
    uint16_t flags = szp(result, bits);
    if (subtract ? left < right : wide > mask) flags |= 1u;
    if ((left ^ right ^ result) & 0x10u) flags |= 0x10u;
    if ((subtract ? (left ^ right) : (uint16_t)~(left ^ right)) &
        (left ^ result) & sign) flags |= 0x800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | flags);
    return result;
}

static int eq16(pr_u25_store_word_cpu *c, uint16_t value)
{
    (void)arithmetic(c, c->ax, value, 16u, 1);
    return (c->flags & 0x40u) != 0u;
}

static int eq8(pr_u25_store_word_cpu *c, uint8_t value)
{
    (void)arithmetic(c, (uint8_t)c->ax, value, 8u, 1);
    return (c->flags & 0x40u) != 0u;
}

static void al(pr_u25_store_word_cpu *c, uint8_t value)
{
    c->ax = (uint16_t)((c->ax & 0xFF00u) | value);
}

static void zero_ah(pr_u25_store_word_cpu *c)
{
    c->ax &= 0xFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | 0x44u);
}



static void shift_ax(pr_u25_store_word_cpu *c, int right)
{
    uint16_t old = c->ax;
    uint16_t flags;
    c->ax = right ? old >> 1 : (uint16_t)(old << 1);
    flags = (uint16_t)(szp(c->ax, 16u) | 0x10u); /* pinned: AF set for a non-zero count */
    if (right ? old & 1u : old & 0x8000u) flags |= 1u;
    if (right ? old & 0x8000u : (old ^ c->ax) & 0x8000u) flags |= 0x800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | flags);
}

static void les(const uint8_t *ram, pr_u25_store_word_cpu *c,
                uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    c->di = word_at(ram, at);
    c->es = word_at(ram, (at + 2u) & MASK);
}

static void push(uint8_t *ram, pr_u25_store_word_cpu *c, uint16_t value)
{
    c->sp = (uint16_t)(c->sp - 2u);
    w16(ram, c->ss, c->sp, value);
}

static void frame(uint8_t *ram, pr_u25_store_word_cpu *c, uint16_t locals)
{
    push(ram, c, c->bp);
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, locals, 16u, 1);
}

static void finish(const uint8_t *ram, pr_u25_store_word_cpu *c, uint16_t args)
{
    size_t at;
    c->sp = c->bp;
    c->bp = r16(ram, c->ss, c->sp);
    c->sp = (uint16_t)(c->sp + 2u);
    at = physical(c->ss, c->sp);
    c->ip = word_at(ram, at);
    c->cs = word_at(ram, (at + 2u) & MASK);
    c->sp = (uint16_t)(c->sp + 4u + args);
}


static void classify(uint8_t *ram, pr_u25_store_word_cpu *c)
{
    static const uint16_t ranges[4][2] = {
        {0x4900u, 0x4CFFu}, {0x6B00u, 0x6EFFu},
        {0x9700u, 0x98FFu}, {0x9900u, 0xB6FFu}
    };
    unsigned i;
    frame(ram, c, 2u);
    w8(ram, c->ss, (uint16_t)(c->bp - 1u), 4u);
    for (i = 0u; i < 4u; ++i) {
        (void)arithmetic(c, r16(ram, c->ss, (uint16_t)(c->bp + 6u)), ranges[i][0], 16u, 1);
        if (c->flags & 1u) continue;
        (void)arithmetic(c, r16(ram, c->ss, (uint16_t)(c->bp + 6u)), ranges[i][1], 16u, 1);
        if (!(c->flags & 0x41u)) continue;
        w8(ram, c->ss, (uint16_t)(c->bp - 1u), (uint8_t)i);
    }
    al(c, r8(ram, c->ss, (uint16_t)(c->bp - 1u)));
    finish(ram, c, 2u);
}

static uint16_t value(const uint8_t *ram, const pr_u25_store_word_cpu *c)
{
    return r16(ram, c->ss, (uint16_t)(c->bp + 6u));
}

static void record(const uint8_t *ram, pr_u25_store_word_cpu *c)
{
    les(ram, c, c->ds, 0x5D92u);
}

/* Complete raw ABI of the existing RES_0802:094A selector-push leaf.
 * Its DS stores can alias its frame, so the pushed byte is read only
 * AFTER copying the prior selector to 5377. No SUB SP, hence no new flags. */
static void selector_push(uint8_t *ram, pr_u25_store_word_cpu *c)
{
    push(ram, c, c->bp);
    c->bp = c->sp;
    al(c, r8(ram, c->ds, 0x5376u));
    w8(ram, c->ds, 0x5377u, (uint8_t)c->ax);
    al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
    w8(ram, c->ds, 0x5376u, (uint8_t)c->ax);
    finish(ram, c, 2u);
}

static pr_u25_store_word_status store_hook(uint8_t *ram, pr_u25_store_word_cpu *c)
{
    static const uint16_t fields[][2] = {
        {0xBBu, 0x88u}, {0xBDu, 0x8Au}, {0xBFu, 0x8Cu},
        {0xC1u, 0x8Eu}, {0xC3u, 0x90u}, {0xF7u, 0xB8u}
    };
    static const uint16_t calls[3][2] = {
        {0x322u, 0x0C26u}, {0x324u, 0x0C49u}, {0x326u, 0x0C6Cu}
    };
    unsigned i;
    frame(ram, c, 2u);
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 8u));
    c->ax = arithmetic(c, c->ax, 0x6B00u, 16u, 1);
    w16(ram, c->ss, (uint16_t)(c->bp + 8u), c->ax);
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 8u));
    if (eq16(c, 0u)) {
        (void)arithmetic(c, value(ram, c), 0u, 16u, 1);
        if (c->flags & 0x40u) w8(ram, c->ds, 0x84D3u, 1u);
        goto done;
    }
    (void)arithmetic(c, c->ax, 0x20u, 16u, 1);
    if (!(c->flags & 1u)) {
        (void)arithmetic(c, c->ax, 0x70u, 16u, 1);
        if (c->flags & 0x41u) {
            c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 8u));
            c->ax = arithmetic(c, c->ax, 0x1Fu, 16u, 1);
            w8(ram, c->ss, (uint16_t)(c->bp - 1u), (uint8_t)c->ax);
            c->dx = (uint16_t)((c->dx & 0xFF00u) | r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
            al(c, r8(ram, c->ss, (uint16_t)(c->bp - 1u)));
            zero_ah(c);
            record(ram, c);
            c->di = arithmetic(c, c->di, c->ax, 16u, 0);
            w8(ram, c->es, (uint16_t)(c->di + 0x17u), (uint8_t)c->dx);
            goto done;
        }
    }
    if (eq16(c, 0xB8u)) {
        (void)arithmetic(c, value(ram, c), 0xB2u, 16u, 1);
        if (!(c->flags & 0x41u))
            w16(ram, c->ss, (uint16_t)(c->bp + 6u), arithmetic(c, value(ram, c), 0x32u, 16u, 1));
        al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
        record(ram, c);
        w8(ram, c->es, (uint16_t)(c->di + 0x84u), (uint8_t)c->ax);
        goto done;
    }
    for (i = 0u; i < sizeof fields / sizeof fields[0]; ++i) {
        if (!eq16(c, fields[i][0])) continue;
        c->ax = value(ram, c);
        record(ram, c);
        w16(ram, c->es, (uint16_t)(c->di + fields[i][1]), c->ax);
        goto done;
    }
    if (eq16(c, 0xF9u)) {
        al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
        record(ram, c);
        w8(ram, c->es, (uint16_t)(c->di + 0xBAu), (uint8_t)c->ax);
        goto done;
    }
    if (eq16(c, 0x100u)) {
        (void)arithmetic(c, value(ram, c), 0x80u, 16u, 1);
        if (!(c->flags & 1u)) {
            record(ram, c);
            w8(ram, c->es, (uint16_t)(c->di + 0x10Du), 0u);
            (void)arithmetic(c, value(ram, c), 0x87u, 16u, 1);
            if (c->flags & 0x40u) {
                record(ram, c);
                w8(ram, c->es, (uint16_t)(c->di + 0x10Cu), 7u);
            }
        }
        (void)arithmetic(c, value(ram, c), 0u, 16u, 1);
        if (c->flags & 0x40u) w8(ram, c->ds, 0x84D2u, 1u);
        goto done;
    }
    if (eq16(c, 0x10Cu)) {
        c->ax = value(ram, c);
        if (eq16(c, 0u)) {
            record(ram, c);
            w8(ram, c->es, (uint16_t)(c->di + 0x10Eu), 0u);
            record(ram, c);
            w8(ram, c->es, (uint16_t)(c->di + 0x10Fu), 0u);
        } else if (eq16(c, 0x80u)) {
            record(ram, c);
            w8(ram, c->es, (uint16_t)(c->di + 0x10Eu), 0u);
            record(ram, c);
            w8(ram, c->es, (uint16_t)(c->di + 0x10Fu), 1u);
        } else if (eq16(c, 0x81u)) {
            record(ram, c);
            w8(ram, c->es, (uint16_t)(c->di + 0x10Eu), 1u);
            record(ram, c);
            w8(ram, c->es, (uint16_t)(c->di + 0x10Fu), 1u);
        }
        goto done;
    }
    if (eq16(c, 0x312u)) {
        uint16_t cs, target_cs, target_ip;
        al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
        push(ram, c, c->ax);
        cs = c->cs;
        /* Fetch the actual far operand, including its loader relocation. */
        target_ip = r16(ram, cs, 0x0C04u);
        target_cs = r16(ram, cs, 0x0C06u);
        push(ram, c, c->cs);
        push(ram, c, 0x0C08u);
        c->cs = target_cs;
        c->ip = target_ip;
        selector_push(ram, c);
        if (c->cs != cs || c->ip != 0x0C08u) return PR_U25_STORE_WORD_TRANSFER;
        goto done;
    }
    for (i = 0u; i < 3u; ++i) {
        if (!eq16(c, calls[i][0])) continue;
        (void)arithmetic(c, value(ram, c), 0x80u, 16u, 1);
        if (c->flags & 0x41u) goto done;
        c->ax = value(ram, c) & 0x7Fu;
        c->flags = (uint16_t)((c->flags & ~ARITH) | szp(c->ax, 16u));
        w16(ram, c->ss, (uint16_t)(c->bp + 6u), c->ax);
        al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
        push(ram, c, c->ax);
        al(c, (uint8_t)(i + 1u));
        push(ram, c, c->ax);
        c->ip = calls[i][1];
        return PR_U25_STORE_WORD_DEPENDENCY;
    }
done:
    finish(ram, c, 4u);
    return PR_U25_STORE_WORD_OK;
}

static void window(uint8_t *ram, pr_u25_store_word_cpu *c,
                   uint16_t pointer_offset, uint16_t bias)
{
    c->dx = value(ram, c);
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 8u));
    shift_ax(c, 0);
    les(ram, c, c->ds, pointer_offset);
    c->di = arithmetic(c, c->di, c->ax, 16u, 0);
    w16(ram, c->es, (uint16_t)(c->di + bias), c->dx);
}

pr_u25_store_word_status pr_u25_store_word_0c77(
    uint8_t *ram, size_t ram_size, pr_u25_store_word_cpu *c)
{
    uint16_t cs;
    if (ram == NULL || ram_size < PR_U25_0756_RAM_SIZE ||
        c == NULL || c->ip != 0x0C77u)
        return PR_U25_STORE_WORD_ARGUMENT;
    frame(ram, c, 2u);
    push(ram, c, r16(ram, c->ss, (uint16_t)(c->bp + 8u)));
    cs = c->cs;
    push(ram, c, c->cs);
    push(ram, c, 0x0C84u);
    c->ip = 0x06FBu;
    classify(ram, c);
    if (c->cs != cs || c->ip != 0x0C84u) return PR_U25_STORE_WORD_TRANSFER;
    w8(ram, c->ss, (uint16_t)(c->bp - 1u), (uint8_t)c->ax);
    al(c, r8(ram, c->ss, (uint16_t)(c->bp - 1u)));
    if (eq8(c, 0u)) {
        window(ram, c, 0x49D2u, 0x6E00u);
        (void)arithmetic(c, r16(ram, c->ss, (uint16_t)(c->bp + 8u)), 0x49FDu, 16u, 1);
        if (!(c->flags & 0x40u))
            (void)arithmetic(c, r16(ram, c->ss, (uint16_t)(c->bp + 8u)), 0x49FEu, 16u, 1);
        if (c->flags & 0x40u) w8(ram, c->ds, 0x84E8u, 1u);
        goto done;
    }
    if (eq8(c, 1u)) {
        pr_u25_store_word_status status;
        window(ram, c, 0x49D6u, 0x2A00u);
        push(ram, c, r16(ram, c->ss, (uint16_t)(c->bp + 8u)));
        push(ram, c, value(ram, c));
        cs = c->cs;
        push(ram, c, c->cs);
        push(ram, c, 0x0CD8u);
        c->ip = 0x0A74u;
        status = store_hook(ram, c);
        if (status != PR_U25_STORE_WORD_OK) return status;
        if (c->cs != cs || c->ip != 0x0CD8u) return PR_U25_STORE_WORD_TRANSFER;
        goto done;
    }
    if (eq8(c, 2u)) {
        window(ram, c, 0x49DAu, 0xD200u);
        goto done;
    }
    if (eq8(c, 3u)) {
        c->dx = (uint16_t)((c->dx & 0xFF00u) | r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
        c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 8u));
        les(ram, c, c->ds, 0x49DEu);
        c->di = arithmetic(c, c->di, c->ax, 16u, 0);
        w8(ram, c->es, (uint16_t)(c->di + 0x6700u), (uint8_t)c->dx);
        goto done;
    }
    if (!eq8(c, 4u)) goto done;
    (void)arithmetic(c, r16(ram, c->ss, (uint16_t)(c->bp + 8u)), 0xBF68u, 16u, 1);
    if (c->flags & 1u) {
        c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 8u));
        if (eq16(c, 0xFBu)) goto done;
        if (eq16(c, 0xFCu)) goto done;
        if (eq16(c, 0xB1u)) goto done;
        if (eq16(c, 0x3DEu)) {
            c->ax = value(ram, c);
            w16(ram, c->ds, 0x84CCu, c->ax);
        } else if (eq16(c, 0xB8u)) {
            c->ax = value(ram, c);
            w16(ram, c->ds, 0x84CEu, c->ax);
        } else if (eq16(c, 0xB9u)) {
            c->ax = value(ram, c);
            w16(ram, c->ds, 0x84D0u, c->ax);
        }
        goto done;
    }
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 8u));
    c->ax = arithmetic(c, c->ax, 0xBF68u, 16u, 1);
    w16(ram, c->ss, (uint16_t)(c->bp + 8u), c->ax);
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 8u));
    if (eq16(c, 0xE3u)) {
        w8(ram, c->ds, 0x84E6u, 1u);
        al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
        w8(ram, c->ds, 0x6AADu, (uint8_t)c->ax);
        goto done;
    }
    if (eq16(c, 0xE4u)) {
        al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
        w8(ram, c->ds, 0x6AAEu, (uint8_t)c->ax);
        w8(ram, c->ds, 0x84E6u, 1u);
        goto done;
    }
    if (eq16(c, 0xE5u)) {
        for (;;) {
            w8(ram, c->ss, (uint16_t)(c->bp - 2u), 1u);
            c->ax = value(ram, c);
            if (eq16(c, 0u)) w8(ram, c->ds, 0x6AAFu, 0u);
            else if (eq16(c, 1u)) w8(ram, c->ds, 0x6AAFu, 2u);
            else if (eq16(c, 2u)) w8(ram, c->ds, 0x6AAFu, 4u);
            else if (eq16(c, 3u)) w8(ram, c->ds, 0x6AAFu, 6u);
            else {
                w8(ram, c->ss, (uint16_t)(c->bp - 2u), 0u);
                w16(ram, c->ss, (uint16_t)(c->bp + 6u), arithmetic(c, value(ram, c), 4u, 16u, 1));
            }
            (void)arithmetic(c, r8(ram, c->ss, (uint16_t)(c->bp - 2u)), 1u, 8u, 1);
            if (c->flags & 0x40u) break;
        }
        w8(ram, c->ds, 0x84E6u, 1u);
        goto done;
    }
    if (eq16(c, 0xF1u)) {
        al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
        w8(ram, c->ds, 0x6F68u, (uint8_t)c->ax);
        w8(ram, c->ds, 0x84E5u, 1u);
        goto done;
    }
    if (eq16(c, 0xF7u)) {
        al(c, r8(ram, c->ss, (uint16_t)(c->bp + 6u)));
        w8(ram, c->ds, 0x6F69u, (uint8_t)c->ax);
        w8(ram, c->ds, 0x84E5u, 1u);
        goto done;
    }
    (void)eq16(c, 0x10BAu);
done:
    finish(ram, c, 4u);
    return PR_U25_STORE_WORD_OK;
}
