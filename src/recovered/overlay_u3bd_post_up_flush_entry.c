#include "poolrad/recovered/overlay_u3bd_post_up_flush_entry.h"

static size_t physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4) + offset) & 0xFFFFFu;
}

static int get_byte(const pr_u3bd_post_up_flush_entry_state *s,
                    uint16_t segment, uint16_t offset, uint8_t *out)
{
    size_t at = physical(segment, offset);
    if (s->ram == NULL || at >= s->ram_size) return 0;
    *out = s->ram[at];
    return 1;
}

static int get_word(const pr_u3bd_post_up_flush_entry_state *s,
                    uint16_t segment, uint16_t offset, uint16_t *out)
{
    uint8_t lo, hi;
    if (!get_byte(s, segment, offset, &lo) ||
        !get_byte(s, segment, (uint16_t)(offset + 1u), &hi)) return 0;
    *out = (uint16_t)(lo | ((uint16_t)hi << 8));
    return 1;
}

static int push_word(pr_u3bd_post_up_flush_entry_state *s, uint16_t value)
{
    uint16_t next = (uint16_t)(s->sp - 2u);
    size_t lo = physical(s->ss, next);
    size_t hi = physical(s->ss, (uint16_t)(next + 1u));
    if (s->ram == NULL || lo >= s->ram_size || hi >= s->ram_size) return 0;
    s->ram[lo] = (uint8_t)value;
    s->ram[hi] = (uint8_t)(value >> 8);
    s->sp = next;
    return 1;
}

pr_u3bd_post_up_flush_entry_status pr_u3bd_post_up_flush_entry(
    pr_u3bd_post_up_flush_entry_state *s,
    pr_u3bd_post_up_flush_entry_report *report)
{
    pr_u3bd_post_up_flush_entry_report r = {0};
    uint16_t target_ip, target_cs;

    if (s == NULL || s->ip != 0x0DAAu)
        return PR_U3BD_POST_UP_FLUSH_ENTRY_ARGUMENT;

    /* +0DAA PUSH CS; +0DAB near CALL +05F3. */
    if (!push_word(s, s->cs)) return PR_U3BD_POST_UP_FLUSH_ENTRY_BOUNDARY;
    s->ip = 0x0DABu;
    r.near_return_ip = 0x0DAEu;
    if (!push_word(s, r.near_return_ip))
        return PR_U3BD_POST_UP_FLUSH_ENTRY_BOUNDARY;
    s->ip = 0x05F3u;

    /* +05F3 PUSH BP; +05F4 MOV BP,SP. */
    if (!push_word(s, s->bp)) return PR_U3BD_POST_UP_FLUSH_ENTRY_BOUNDARY;
    s->ip = 0x05F4u;
    s->bp = s->sp;
    s->ip = 0x05F6u;

    /* +05F6 far CALL uses the relocated immediate in loaded CS. */
    if (!get_word(s, s->cs, 0x05F7u, &target_ip) ||
        !get_word(s, s->cs, 0x05F9u, &target_cs))
        return PR_U3BD_POST_UP_FLUSH_ENTRY_BOUNDARY;
    r.target_ip = target_ip;
    r.target_cs = target_cs;
    if (!push_word(s, s->cs)) return PR_U3BD_POST_UP_FLUSH_ENTRY_BOUNDARY;
    r.far_return_ip = 0x05FBu;
    if (!push_word(s, r.far_return_ip))
        return PR_U3BD_POST_UP_FLUSH_ENTRY_BOUNDARY;
    s->cs = target_cs;
    s->ip = target_ip;
    if (report != NULL) *report = r;
    return PR_U3BD_POST_UP_FLUSH_ENTRY_OK;
}
