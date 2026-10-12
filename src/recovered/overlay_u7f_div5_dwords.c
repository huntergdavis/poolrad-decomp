/* Portable C translation of GAME.OVR unit 0x7f entry 11 (+0x0185); see
 * docs/re/U7F_DIV5_DWORDS_ENTRY11.md. Every instruction path of the 86
 * reachable code bytes is reproduced in the original's order. The routine
 * is a leaf: no calls, no reads from the block, fourteen DS word stores
 * (ten in the loop, four after the divisions) covering DS 0x67F4..0x6807. */
#include "poolrad/recovered/overlay_u7f_div5_dwords.h"

#include <string.h>

/* `mov word ptr [addr], ax`: little-endian, as the 8086 stores it. */
static void pr_u7f_dwords_store_word(uint8_t *at, uint16_t value)
{
    at[0] = (uint8_t)(value & 0xFFu);
    at[1] = (uint8_t)(value >> 8);
}

pr_u7f_dwords_status pr_u7f_entry11(uint8_t *block, size_t block_size, uint16_t value,
                                    pr_u7f_dwords_report *report)
{
    pr_u7f_dwords_report local;
    pr_u7f_dwords_report *r = report ? report : &local;
    uint8_t i;
    uint16_t quotient, remainder;
    memset(r, 0, sizeof *r);
    /* The original has no check here; see the header for the policy. */
    if (!block || block_size < PR_U7F_DWORDS_WRITE_END) return PR_U7F_DWORDS_ARGUMENT;

    /* +0x018B..+0x01AC: [bp-1] = 0; jmp +0x0194; body; cmp [bp-1],4; jne +0x0191 (inc, body).
     * Body: al = [bp-1]; cbw; di = ax; shl di,1; shl di,1; ax = 0;
     * [di+0x67F4] = ax; [di+0x67F6] = ax. The counter is 0..4, so cbw never sign-extends. */
    i = 0;
    for (;;) {
        uint8_t *slot = block + 4u * (unsigned)i;
        pr_u7f_dwords_store_word(slot, 0);          /* +0x01A0: mov [di+0x67F4], ax */
        pr_u7f_dwords_store_word(slot + 2, 0);      /* +0x01A4: mov [di+0x67F6], ax */
        r->cleared_slots++;
        if (i == PR_U7F_DWORDS_CLEAR_SLOTS - 1u) break;   /* cmp byte ptr [bp-1], 4; jne */
        i++;                                              /* inc byte ptr [bp-1] */
    }

    /* +0x01AE..+0x01C0: ax = [bp+6]; xor dx,dx; cx = 5; div cx (unsigned 32/16 with DX:AX = 0:value,
     * so the quotient fits 16 bits and no #DE can occur); xor dx,dx; [0x6804] = ax; [0x6806] = dx */
    quotient = (uint16_t)(value / PR_U7F_DWORDS_DIVISOR);
    pr_u7f_dwords_store_word(block + PR_U7F_DWORDS_QUOTIENT_OFFSET, quotient);
    pr_u7f_dwords_store_word(block + PR_U7F_DWORDS_QUOTIENT_OFFSET + 2, 0);

    /* +0x01C1..+0x01D4: the same load and division again; xchg ax,dx (ax = remainder); xor dx,dx;
     * [0x6800] = ax; [0x6802] = dx */
    remainder = (uint16_t)(value % PR_U7F_DWORDS_DIVISOR);
    pr_u7f_dwords_store_word(block + PR_U7F_DWORDS_REMAINDER_OFFSET, remainder);
    pr_u7f_dwords_store_word(block + PR_U7F_DWORDS_REMAINDER_OFFSET + 2, 0);

    /* +0x01D5..+0x01D8: mov sp,bp; pop bp; retf 2. AX holds the remainder and DX is 0 on return;
     * no caller reads either. */
    r->quotient = quotient;
    r->remainder = remainder;
    r->first_offset = 0;
    r->last_offset = PR_U7F_DWORDS_WRITE_END - 1u;
    return PR_U7F_DWORDS_OK;
}
