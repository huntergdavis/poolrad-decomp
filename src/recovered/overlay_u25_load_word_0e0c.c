/* DOS v1.3 unit 25 load word +0E0C, classifier +06FB, load hook +07BC.
 * Source order matters: arguments, globals, records and frame locals may
 * alias in the caller-owned RAM. Evidence: docs/re/U25_LOAD_WORD_0E0C.md. */
#include "poolrad/recovered/overlay_u25_load_word_0e0c.h"

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

static uint16_t arithmetic(pr_u25_load_word_cpu *c, uint16_t left,
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

static int eq16(pr_u25_load_word_cpu *c, uint16_t value)
{
    (void)arithmetic(c, c->ax, value, 16u, 1);
    return (c->flags & 0x40u) != 0u;
}

static int eq8(pr_u25_load_word_cpu *c, uint8_t value)
{
    (void)arithmetic(c, (uint8_t)c->ax, value, 8u, 1);
    return (c->flags & 0x40u) != 0u;
}

static void al(pr_u25_load_word_cpu *c, uint8_t value)
{
    c->ax = (uint16_t)((c->ax & 0xFF00u) | value);
}

static void zero_ah(pr_u25_load_word_cpu *c)
{
    c->ax &= 0xFFu;
    c->flags = (uint16_t)((c->flags & ~ARITH) | 0x44u);
}

static void zero_ax(pr_u25_load_word_cpu *c)
{
    c->ax = 0u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | 0x44u);
}

static void cbw(pr_u25_load_word_cpu *c)
{
    c->ax = (uint16_t)((c->ax & 0x80u) ? c->ax | 0xFF00u : c->ax & 0xFFu);
}

static void shift_ax(pr_u25_load_word_cpu *c, int right)
{
    uint16_t old = c->ax;
    uint16_t flags;
    c->ax = right ? old >> 1 : (uint16_t)(old << 1);
    flags = szp(c->ax, 16u);
    if (right ? old & 1u : old & 0x8000u) flags |= 1u;
    if (right ? old & 0x8000u : (old ^ c->ax) & 0x8000u) flags |= 0x800u;
    c->flags = (uint16_t)((c->flags & ~ARITH) | flags);
}

static void les(const uint8_t *ram, pr_u25_load_word_cpu *c,
                uint16_t seg, uint16_t off)
{
    size_t at = physical(seg, off);
    c->di = word_at(ram, at);
    c->es = word_at(ram, (at + 2u) & MASK);
}

static void push(uint8_t *ram, pr_u25_load_word_cpu *c, uint16_t value)
{
    c->sp = (uint16_t)(c->sp - 2u);
    w16(ram, c->ss, c->sp, value);
}

static void frame(uint8_t *ram, pr_u25_load_word_cpu *c, uint16_t locals)
{
    push(ram, c, c->bp);
    c->bp = c->sp;
    c->sp = arithmetic(c, c->sp, locals, 16u, 1);
}

static void finish(const uint8_t *ram, pr_u25_load_word_cpu *c, uint16_t args)
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

static void result(uint8_t *ram, const pr_u25_load_word_cpu *c, uint16_t value)
{
    w16(ram, c->ss, (uint16_t)(c->bp - 2u), value);
}

static void classify(uint8_t *ram, pr_u25_load_word_cpu *c)
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

static void record(const uint8_t *ram, pr_u25_load_word_cpu *c)
{
    les(ram, c, c->ds, 0x5D92u);
}

static pr_u25_load_word_status load_hook(uint8_t *ram, size_t size,
                                        pr_u25_load_word_cpu *c)
{
    static const uint16_t fields[][3] = {
        {0xB8u, 0x84u, 0u}, {0xBBu, 0x88u, 2u},
        {0xBDu, 0x8Au, 2u}, {0xBFu, 0x8Cu, 2u},
        {0xC1u, 0x8Eu, 2u}, {0xC3u, 0x90u, 2u},
        {0xD8u, 0xA0u, 1u}, {0xF7u, 0xB8u, 2u},
        {0xF9u, 0xBAu, 0u}
    };
    unsigned i;
    frame(ram, c, 4u);
    les(ram, c, c->ss, (uint16_t)(c->bp + 6u));
    w8(ram, c->es, c->di, 1u);
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 10u));
    c->ax = arithmetic(c, c->ax, 0x6B00u, 16u, 1);
    w16(ram, c->ss, (uint16_t)(c->bp + 10u), c->ax);
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 10u));
    if (eq16(c, 0x18u)) {
        record(ram, c);
        al(c, r8(ram, c->es, (uint16_t)(c->di + 0x14u)));
        zero_ah(c);
        result(ram, c, c->ax);
        goto done;
    }
    if (eq16(c, 0x9Bu)) {
        record(ram, c);
        al(c, r8(ram, c->es, (uint16_t)(c->di + 0x6Eu)));
        zero_ah(c);
        result(ram, c, c->ax);
        goto done;
    }
    if (eq16(c, 0xA0u)) {
        record(ram, c);
        al(c, r8(ram, c->es, (uint16_t)(c->di + 0x73u)));
        cbw(c);
        result(ram, c, c->ax);
        goto done;
    }
    (void)arithmetic(c, c->ax, 0xA5u, 16u, 1);
    if (!(c->flags & 1u)) {
        (void)arithmetic(c, c->ax, 0xACu, 16u, 1);
        if (c->flags & 0x41u) {
            c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 10u));
            c->ax = arithmetic(c, c->ax, 0xA4u, 16u, 1);
            w8(ram, c->ss, (uint16_t)(c->bp - 3u), (uint8_t)c->ax);
            al(c, r8(ram, c->ss, (uint16_t)(c->bp - 3u)));
            zero_ah(c);
            record(ram, c);
            c->di = arithmetic(c, c->di, c->ax, 16u, 0);
            al(c, r8(ram, c->es, (uint16_t)(c->di + 0x76u)));
            zero_ah(c);
            result(ram, c, c->ax);
            goto done;
        }
    }
    for (i = 0u; i < sizeof fields / sizeof fields[0]; ++i) {
        if (!eq16(c, fields[i][0])) continue;
        record(ram, c);
        if (fields[i][2] == 2u) {
            c->ax = r16(ram, c->es, (uint16_t)(c->di + fields[i][1]));
        } else {
            al(c, r8(ram, c->es, (uint16_t)(c->di + fields[i][1])));
            if (fields[i][2]) cbw(c);
            else zero_ah(c);
        }
        result(ram, c, c->ax);
        goto done;
    }
    if (eq16(c, 0x100u)) {
        record(ram, c);
        (void)arithmetic(c, r8(ram, c->es, (uint16_t)(c->di + 0x10Du)), 0u, 8u, 1);
        result(ram, c, c->flags & 0x40u ? 0x80u : 1u);
        goto done;
    }
    if (eq16(c, 0x10Cu)) {
        record(ram, c);
        (void)arithmetic(c, r8(ram, c->es, (uint16_t)(c->di + 0x10Eu)), 0u, 8u, 1);
        if (c->flags & 0x40u) {
            record(ram, c);
            (void)arithmetic(c, r8(ram, c->es, (uint16_t)(c->di + 0x10Fu)), 0u, 8u, 1);
            if (!(c->flags & 0x40u)) {
                result(ram, c, 0x80u);
                goto done;
            }
        }
        record(ram, c);
        (void)arithmetic(c, r8(ram, c->es, (uint16_t)(c->di + 0x10Eu)), 1u, 8u, 1);
        if (c->flags & 0x40u) result(ram, c, 0x81u);
        else {
            zero_ax(c);
            result(ram, c, c->ax);
        }
        goto done;
    }
    if (eq16(c, 0x10Du)) goto done; /* flag stays 1, result local unwritten */
    if (eq16(c, 0x11Bu)) {
        record(ram, c);
        al(c, r8(ram, c->es, (uint16_t)(c->di + 0x11Cu)));
        zero_ah(c);
        result(ram, c, c->ax);
        goto done;
    }
    if (eq16(c, 0x2B1u)) {
        uint16_t cs = c->cs;
        push(ram, c, r16(ram, c->ds, 0x5D94u));
        push(ram, c, r16(ram, c->ds, 0x5D92u));
        push(ram, c, c->cs);
        push(ram, c, 0x0984u);
        c->ip = 0x0756u;
        if (pr_u25_party_index_0756(ram, size, c) != PR_U25_0756_OK)
            return PR_U25_LOAD_WORD_ARGUMENT;
        if (c->cs != cs || c->ip != 0x0984u) return PR_U25_LOAD_WORD_TRANSFER;
        zero_ah(c);
        result(ram, c, c->ax);
        goto done;
    }
    if (eq16(c, 0x2B4u)) {
        uint16_t cs = c->cs;
        push(ram, c, r16(ram, c->ds, 0x5D94u));
        push(ram, c, r16(ram, c->ds, 0x5D92u));
        push(ram, c, c->cs);
        push(ram, c, 0x099Du);
        c->ip = 0x0756u;
        if (pr_u25_party_index_0756(ram, size, c) != PR_U25_0756_OK)
            return PR_U25_LOAD_WORD_ARGUMENT;
        if (c->cs != cs || c->ip != 0x099Du) return PR_U25_LOAD_WORD_TRANSFER;
        zero_ah(c);
        result(ram, c, c->ax);
        goto done;
    }
    if (eq16(c, 0x2CFu)) {
        record(ram, c);
        al(c, r8(ram, c->es, (uint16_t)(c->di + 0x15u)));
        if (eq8(c, 3u)) {
            zero_ax(c);
            result(ram, c, c->ax);
        } else if (eq8(c, 4u)) result(ram, c, 5u);
        else if (eq8(c, 5u)) result(ram, c, 10u);
        else if (eq8(c, 6u)) result(ram, c, 15u);
        else if (eq8(c, 7u)) result(ram, c, 20u);
        else {
            (void)arithmetic(c, (uint8_t)c->ax, 8u, 8u, 1);
            if (!(c->flags & 1u)) {
                (void)arithmetic(c, (uint8_t)c->ax, 12u, 8u, 1);
                if (c->flags & 0x41u) {
                    result(ram, c, 25u);
                    goto done;
                }
            }
            if (eq8(c, 13u)) result(ram, c, 30u);
            else if (eq8(c, 14u)) result(ram, c, 35u);
            else if (eq8(c, 15u)) result(ram, c, 40u);
            else if (eq8(c, 16u)) result(ram, c, 50u);
            else if (eq8(c, 17u)) result(ram, c, 55u);
            else {
                (void)arithmetic(c, (uint8_t)c->ax, 18u, 8u, 1);
                if (!(c->flags & 1u)) {
                    (void)arithmetic(c, (uint8_t)c->ax, 25u, 8u, 1);
                    if (c->flags & 0x41u) result(ram, c, 60u);
                }
            }
        }
        goto done;
    }
    if (eq16(c, 0x312u)) {
        al(c, r8(ram, c->ds, 0x5376u));
        zero_ah(c);
        result(ram, c, c->ax);
        goto done;
    }
    if (eq16(c, 0x33Eu)) {
        les(ram, c, c->ds, 0x49D6u);
        c->ax = r16(ram, c->es, (uint16_t)(c->di + 0x67Cu));
        result(ram, c, c->ax);
        goto done;
    }
    les(ram, c, c->ss, (uint16_t)(c->bp + 6u));
    w8(ram, c->es, c->di, 0u);
done:
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp - 2u));
    finish(ram, c, 6u);
    return PR_U25_LOAD_WORD_OK;
}

static void window(uint8_t *ram, pr_u25_load_word_cpu *c,
                   uint16_t pointer, uint16_t bias)
{
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 6u));
    shift_ax(c, 0);
    les(ram, c, c->ds, pointer);
    c->di = arithmetic(c, c->di, c->ax, 16u, 0);
    c->ax = r16(ram, c->es, (uint16_t)(c->di + bias));
    result(ram, c, c->ax);
}

pr_u25_load_word_status pr_u25_load_word_0e0c(
    uint8_t *ram, size_t ram_size, pr_u25_load_word_cpu *c)
{
    uint16_t cs;
    if (ram == NULL || ram_size < PR_U25_0756_RAM_SIZE ||
        c == NULL || c->ip != 0x0E0Cu)
        return PR_U25_LOAD_WORD_ARGUMENT;
    frame(ram, c, 6u);
    push(ram, c, r16(ram, c->ss, (uint16_t)(c->bp + 6u)));
    cs = c->cs;
    push(ram, c, c->cs);
    push(ram, c, 0x0E19u);
    c->ip = 0x06FBu;
    classify(ram, c);
    if (c->cs != cs || c->ip != 0x0E19u) return PR_U25_LOAD_WORD_TRANSFER;
    w8(ram, c->ss, (uint16_t)(c->bp - 3u), (uint8_t)c->ax);
    w8(ram, c->ss, (uint16_t)(c->bp - 4u), 0u);
    al(c, r8(ram, c->ss, (uint16_t)(c->bp - 3u)));
    if (eq8(c, 0u)) {
        window(ram, c, 0x49D2u, 0x6E00u);
        goto done;
    }
    if (eq8(c, 1u)) {
        pr_u25_load_word_status status;
        push(ram, c, r16(ram, c->ss, (uint16_t)(c->bp + 6u)));
        c->di = (uint16_t)(c->bp - 4u);
        push(ram, c, c->ss);
        push(ram, c, c->di);
        cs = c->cs;
        push(ram, c, c->cs);
        push(ram, c, 0x0E4Du);
        c->ip = 0x07BCu;
        status = load_hook(ram, ram_size, c);
        if (status != PR_U25_LOAD_WORD_OK) return status;
        if (c->cs != cs || c->ip != 0x0E4Du) return PR_U25_LOAD_WORD_TRANSFER;
        result(ram, c, c->ax);
        (void)arithmetic(c, r8(ram, c->ss, (uint16_t)(c->bp - 4u)), 0u, 8u, 1);
        if (c->flags & 0x40u) window(ram, c, 0x49D6u, 0x2A00u);
        goto done;
    }
    if (eq8(c, 2u)) {
        window(ram, c, 0x49DAu, 0xD200u);
        goto done;
    }
    if (eq8(c, 3u)) {
        c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 6u));
        les(ram, c, c->ds, 0x49DEu);
        c->di = arithmetic(c, c->di, c->ax, 16u, 0);
        al(c, r8(ram, c->es, (uint16_t)(c->di + 0x6700u)));
        zero_ah(c);
        result(ram, c, c->ax);
        goto done;
    }
    if (!eq8(c, 4u)) goto done;
    (void)arithmetic(c, r16(ram, c->ss, (uint16_t)(c->bp + 6u)), 0xC04Bu, 16u, 1);
    if (c->flags & 1u) {
        c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 6u));
        if (eq16(c, 0xB1u)) {
            c->ax = r16(ram, c->ds, 0x6F6Eu);
            result(ram, c, c->ax);
        } else if (eq16(c, 0xFBu)) {
            c->ax = r16(ram, c->ds, 0x6F6Au);
            result(ram, c, c->ax);
        } else if (eq16(c, 0xFCu)) {
            c->ax = r16(ram, c->ds, 0x6F6Cu);
            result(ram, c, c->ax);
        } else if (eq16(c, 0x33Du)) {
            al(c, r8(ram, c->ds, 0x6AAFu));
            zero_ah(c);
            result(ram, c, c->ax);
        } else (void)eq16(c, 0x35Fu);
        goto done;
    }
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 6u));
    c->ax = arithmetic(c, c->ax, 0xC04Bu, 16u, 1);
    w16(ram, c->ss, (uint16_t)(c->bp + 6u), c->ax);
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp + 6u));
    if (eq16(c, 0u)) {
        al(c, r8(ram, c->ds, 0x6AADu));
        cbw(c);
        result(ram, c, c->ax);
    } else if (eq16(c, 1u)) {
        al(c, r8(ram, c->ds, 0x6AAEu));
        cbw(c);
        result(ram, c, c->ax);
    } else if (eq16(c, 2u)) {
        al(c, r8(ram, c->ds, 0x6AAFu));
        zero_ah(c);
        shift_ax(c, 1);
        result(ram, c, c->ax);
    } else if (eq16(c, 3u)) {
        al(c, r8(ram, c->ds, 0x6AB0u));
        zero_ah(c);
        result(ram, c, c->ax);
    } else if (eq16(c, 4u)) {
        al(c, r8(ram, c->ds, 0x6AB1u));
        zero_ah(c);
        result(ram, c, c->ax);
    } else (void)eq16(c, 0xEu);
done:
    c->ax = r16(ram, c->ss, (uint16_t)(c->bp - 2u));
    finish(ram, c, 2u);
    return PR_U25_LOAD_WORD_OK;
}
