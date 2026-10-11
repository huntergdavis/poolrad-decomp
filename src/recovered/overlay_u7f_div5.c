/* Portable C translation of GAME.OVR unit 0x7f entry 10 (+0x0130); see
 * docs/re/U7F_DIV5_ENTRY10.md. Every instruction path is reproduced in the
 * original's order. The routine is a leaf: no calls, no reads from the
 * record, one data-segment far pointer ([0x5D92], loaded three times). */
#include "poolrad/recovered/overlay_u7f_div5.h"

#include <string.h>

/* `mov word ptr es:[di + k], ax`: little-endian, as the 8086 stores it. */
static void pr_u7f_div5_store_word(uint8_t *at, uint16_t value)
{
    at[0] = (uint8_t)(value & 0xFFu);
    at[1] = (uint8_t)(value >> 8);
}

pr_u7f_div5_status pr_u7f_entry10(uint8_t *record, size_t record_size, uint16_t value,
                                  pr_u7f_div5_report *report)
{
    pr_u7f_div5_report local;
    pr_u7f_div5_report *r = report ? report : &local;
    uint8_t i;
    uint16_t quotient, remainder;
    memset(r, 0, sizeof *r);
    /* The original has no check here; see the header for the policy. */
    if (!record || record_size < PR_U7F_DIV5_WRITE_END) return PR_U7F_DIV5_ARGUMENT;

    /* +0x0136..+0x0156: [bp-1] = 0; jmp +0x013F; body; cmp [bp-1],4; jne +0x013C (inc, body).
     * Body: al = [bp-1]; cbw; shl ax,1; les di,[0x5D92]; di += ax; ax = 0;
     * es:[di + 0x88] = ax. The counter is 0..4, so cbw never sign-extends. */
    i = 0;
    for (;;) {
        pr_u7f_div5_store_word(record + PR_U7F_DIV5_CLEAR_BASE + 2u * (unsigned)i, 0);
        r->cleared_words++;
        if (i == PR_U7F_DIV5_CLEAR_WORDS - 1u) break;   /* cmp byte ptr [bp-1], 4; jne */
        i++;                                           /* inc byte ptr [bp-1] */
    }

    /* +0x0158..+0x016A: ax = [bp+6]; xor dx,dx; cx = 5; div cx (unsigned 32/16, DX:AX = 0:value,
     * so the quotient fits 16 bits and no #DE can occur); les di,[0x5D92]; es:[di + 0x90] = ax */
    quotient = (uint16_t)(value / PR_U7F_DIV5_DIVISOR);
    pr_u7f_div5_store_word(record + PR_U7F_DIV5_QUOTIENT_OFFSET, quotient);

    /* +0x016B..+0x017E: the same load and division again; xchg ax,dx (ax = remainder);
     * les di,[0x5D92]; es:[di + 0x8E] = ax */
    remainder = (uint16_t)(value % PR_U7F_DIV5_DIVISOR);
    pr_u7f_div5_store_word(record + PR_U7F_DIV5_REMAINDER_OFFSET, remainder);

    /* +0x017F..+0x0182: mov sp,bp; pop bp; retf 2. AX holds the remainder on return; no
     * caller reads it. */
    r->quotient = quotient;
    r->remainder = remainder;
    r->first_offset = PR_U7F_DIV5_CLEAR_BASE;
    r->last_offset = PR_U7F_DIV5_WRITE_END - 1u;
    return PR_U7F_DIV5_OK;
}
