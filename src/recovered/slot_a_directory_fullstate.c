#include "poolrad/recovered/slot_a_directory_fullstate.h"

#define SYSTEM_SEG 0x131cu
#define CALLER_SEG 0x2a8eu
#define CF 0x0001u

static int address(const pr_slot_a_directory_state *s, uint16_t seg,
                   uint16_t off, size_t bytes, size_t *linear)
{
    size_t p = ((size_t)seg << 4) + off;
    if (p > 0xfffffu || p > s->ram_size || bytes > s->ram_size - p ||
        bytes > 0x100000u - p)
        return 0;
    *linear = p;
    return 1;
}

static int read_word(const pr_slot_a_directory_state *s, uint16_t seg,
                     uint16_t off, uint16_t *value)
{
    size_t p;
    if (!address(s, seg, off, 2u, &p)) return 0;
    *value = (uint16_t)(s->ram[p] | ((uint16_t)s->ram[p + 1u] << 8));
    return 1;
}

static int write_word(pr_slot_a_directory_state *s, uint16_t seg,
                      uint16_t off, uint16_t value)
{
    size_t p;
    if (!address(s, seg, off, 2u, &p)) return 0;
    s->ram[p] = (uint8_t)value;
    s->ram[p + 1u] = (uint8_t)(value >> 8);
    return 1;
}

static int push_word(pr_slot_a_directory_state *s, uint16_t value)
{
    uint16_t next = (uint16_t)(s->cpu.sp - 2u);
    if (!write_word(s, s->cpu.ss, next, value)) return 0;
    s->cpu.sp = next;
    return 1;
}

static int valid_journal(const pr_slot_a_directory_state *s,
                         const pr_slot_a_directory_write *w, size_t count)
{
    size_t i, j;
    for (i = 0; i < count; ++i) {
        uint8_t previous;
        if (w[i].linear >= s->ram_size || w[i].linear >= 0x100000u)
            return 0;
        previous = s->ram[w[i].linear];
        for (j = 0; j < i; ++j)
            if (w[j].linear == w[i].linear) previous = w[j].new_byte;
        if (previous != w[i].old_byte) return 0;
    }
    return 1;
}

pr_slot_a_directory_status pr_slot_a_directory_step(
    pr_slot_a_directory_state *s, pr_slot_a_directory_service service,
    void *context, pr_slot_a_directory_write *journal, size_t capacity)
{
    pr_slot_a_directory_cpu *c, after;
    pr_slot_a_directory_status result;
    uint16_t value, ip, cs, frame;
    size_t count, i, linear;

    if (!s || !s->ram || s->ram_size > 0x100000u ||
        (capacity != 0u && !journal)) return PR_SLOT_A_DIRECTORY_BOUNDARY;
    c = &s->cpu;
    if (c->cs == SYSTEM_SEG && c->ip == 0x1821u) {
        if ((c->ax >> 8) != 0x39u || c->ds != c->ss ||
            !address(s, c->ds, c->dx, 1u, &linear))
            return PR_SLOT_A_DIRECTORY_BOUNDARY;
        if (!service) return PR_SLOT_A_DIRECTORY_BLOCKED_DOS_SERVICE;
        after = *c;
        count = 0u;
        result = service(context, c, s->ram, s->ram_size, &after,
                         journal, capacity, &count);
        if (result != PR_SLOT_A_DIRECTORY_OK) return result;
        if (count > capacity || after.cs != SYSTEM_SEG ||
            after.ip != 0x1823u || !valid_journal(s, journal, count))
            return PR_SLOT_A_DIRECTORY_BAD_EVENT;
        for (i = 0; i < count; ++i)
            s->ram[journal[i].linear] = journal[i].new_byte;
        *c = after;
        return PR_SLOT_A_DIRECTORY_OK;
    }
    if (c->cs == SYSTEM_SEG && c->ip == 0x1823u) {
        if (!read_word(s, c->ss, c->sp, &value))
            return PR_SLOT_A_DIRECTORY_BOUNDARY;
        c->ds = value;                  /* POP DS */
        c->sp = (uint16_t)(c->sp + 2u);
        c->ip = (c->flags & CF) ? 0x1826u : 0x1829u; /* JAE */
        return PR_SLOT_A_DIRECTORY_OK;
    }
    if (c->cs == SYSTEM_SEG && c->ip == 0x1826u) {
        if (!write_word(s, c->ds, 0x4426u, c->ax))
            return PR_SLOT_A_DIRECTORY_BOUNDARY;
        c->ip = 0x1829u;
        return PR_SLOT_A_DIRECTORY_OK;
    }
    if (c->cs == SYSTEM_SEG && c->ip == 0x1829u) {
        /* RET to +17E5; MOV SP,BP; POP BP; RETF 4 to +02D5. Validate
         * all stack words before changing any state. */
        frame = c->bp;
        if (!read_word(s, c->ss, c->sp, &ip) || ip != 0x17e5u ||
            !read_word(s, c->ss, frame, &value) ||
            !read_word(s, c->ss, (uint16_t)(frame + 2u), &ip) ||
            ip != 0x02d5u ||
            !read_word(s, c->ss, (uint16_t)(frame + 4u), &cs) ||
            cs != CALLER_SEG)
            return PR_SLOT_A_DIRECTORY_BOUNDARY;
        c->bp = value;
        c->sp = (uint16_t)(frame + 10u);
        c->cs = cs; c->ip = ip;
        return PR_SLOT_A_DIRECTORY_OK;
    }
    if (c->cs == CALLER_SEG && c->ip == 0x02d5u) {
        /* Caller CALL FAR to the reached IOResult entry. */
        if (!address(s, c->ss, (uint16_t)(c->sp - 4u), 4u, &linear))
            return PR_SLOT_A_DIRECTORY_BOUNDARY;
        if (!push_word(s, c->cs) || !push_word(s, 0x02dau))
            return PR_SLOT_A_DIRECTORY_BOUNDARY;
        c->cs = SYSTEM_SEG; c->ip = 0x04a2u;
        return PR_SLOT_A_DIRECTORY_OK;
    }
    if (c->cs == SYSTEM_SEG && c->ip == 0x04a2u) {
        if (!read_word(s, c->ds, 0x4426u, &value))
            return PR_SLOT_A_DIRECTORY_BOUNDARY;
        /* XOR AX,AX: OF/CF/AF cleared, SF=0, ZF/PF=1; AF undefined on
         * 8086 and is normalized to zero here. XCHG does not change flags. */
        c->flags = (uint16_t)((c->flags & (uint16_t)~0x08d5u) | 0x0044u);
        c->ax = value;
        write_word(s, c->ds, 0x4426u, 0u);
        c->ip = 0x04a8u;
        return PR_SLOT_A_DIRECTORY_OK;
    }
    if (c->cs == SYSTEM_SEG && c->ip == 0x04a8u) {
        if (!read_word(s, c->ss, c->sp, &ip) ||
            !read_word(s, c->ss, (uint16_t)(c->sp + 2u), &cs) ||
            ip != 0x02dau || cs != CALLER_SEG)
            return PR_SLOT_A_DIRECTORY_BOUNDARY;
        c->ip = ip; c->cs = cs; c->sp = (uint16_t)(c->sp + 4u);
        return PR_SLOT_A_DIRECTORY_OK;
    }
    return PR_SLOT_A_DIRECTORY_BOUNDARY;
}
