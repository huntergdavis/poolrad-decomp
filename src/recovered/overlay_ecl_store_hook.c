/* Portable C for GAME.OVR unit 0x25 + 0x0A74, the class-1 ECL store hook
 * (docs/re/ECL_STORE_HOOK_0A74.md). The case chain below is in the
 * original's compare order; the ranges are disjoint so the order is not
 * observable. Word stores are little-endian as the `mov word ptr es:[..]`
 * of the original. */
#include "poolrad/recovered/overlay_ecl_store_hook.h"

static void store_word(uint8_t *at, uint16_t value)
{
    at[0] = (uint8_t)value;
    at[1] = (uint8_t)(value >> 8);
}

pr_u25_hook_status pr_u25_store_hook_0a74(uint16_t address, uint16_t value, uint8_t *record,
                                          pr_u25_hook_flags *flags,
                                          const pr_u25_hook_callouts *callouts,
                                          pr_u25_hook_report *report)
{
    pr_u25_hook_report local;
    pr_u25_hook_report *r = report ? report : &local;
    uint16_t rel = (uint16_t)(address - PR_U25_HOOK_WINDOW_BASE);   /* `sub ax, 0x6B00` */
    r->which = PR_U25_HOOK_CASE_NONE;
    r->record_offset = 0;
    r->callout_made = 0;
    r->callout_value = 0;
    r->callout_selector = 0;
    if (!flags) return PR_U25_HOOK_ARGUMENT;

    /* +0x0A86: rel == 0 -> if value == 0 then [0x84D3] = 1 */
    if (rel == 0) {
        r->which = PR_U25_HOOK_CASE_FLAG_84D3;
        if (value == 0) flags->b84d3 = 1;
        return PR_U25_HOOK_OK;
    }
    /* +0x0A99: 0x20 <= rel <= 0x70 -> record[0x17 + (uint8)(rel - 0x1F)] = low byte */
    if (rel >= 0x20u && rel <= 0x70u) {
        unsigned index = (uint8_t)(rel - 0x1Fu);
        r->which = PR_U25_HOOK_CASE_BYTE_RANGE;
        r->record_offset = 0x17u + index;
        if (!record) return PR_U25_HOOK_ARGUMENT;
        record[0x17u + index] = (uint8_t)value;
        return PR_U25_HOOK_OK;
    }
    /* +0x0AC1: rel == 0xB8 -> if value > 0xB2 then value -= 0x32; record[0x84] = low byte */
    if (rel == 0xB8u) {
        r->which = PR_U25_HOOK_CASE_B8;
        r->record_offset = 0x84u;
        if (!record) return PR_U25_HOOK_ARGUMENT;
        if (value > 0xB2u) value = (uint16_t)(value - 0x32u);
        record[0x84u] = (uint8_t)value;
        return PR_U25_HOOK_OK;
    }
    /* +0x0AE0..+0x0B55: word stores */
    if (rel == 0xBBu || rel == 0xBDu || rel == 0xBFu || rel == 0xC1u || rel == 0xC3u || rel == 0xF7u) {
        unsigned offset = (rel == 0xF7u) ? 0xB8u : 0x88u + (unsigned)(rel - 0xBBu);
        r->which = PR_U25_HOOK_CASE_WORD;
        r->record_offset = offset;
        if (!record) return PR_U25_HOOK_ARGUMENT;
        store_word(record + offset, value);
        return PR_U25_HOOK_OK;
    }
    /* +0x0B58: rel == 0xF9 -> record[0xBA] = low byte */
    if (rel == 0xF9u) {
        r->which = PR_U25_HOOK_CASE_BYTE_BA;
        r->record_offset = 0xBAu;
        if (!record) return PR_U25_HOOK_ARGUMENT;
        record[0xBAu] = (uint8_t)value;
        return PR_U25_HOOK_OK;
    }
    /* +0x0B6C: rel == 0x100 -> if value >= 0x80: record[0x10D] = 0 and, if value == 0x87,
     * record[0x10C] = 7; then if value == 0: [0x84D2] = 1 */
    if (rel == 0x100u) {
        r->which = PR_U25_HOOK_CASE_100;
        r->record_offset = 0x10Du;
        if (value >= 0x80u) {
            if (!record) return PR_U25_HOOK_ARGUMENT;
            record[0x10Du] = 0;
            if (value == 0x87u) record[0x10Cu] = 7;
        }
        if (value == 0) flags->b84d2 = 1;
        return PR_U25_HOOK_OK;
    }
    /* +0x0BA1: rel == 0x10C -> value 0: (0x10E, 0x10F) = (0, 0); 0x80: (0, 1); 0x81: (1, 1) */
    if (rel == 0x10Cu) {
        r->which = PR_U25_HOOK_CASE_10C;
        r->record_offset = 0x10Eu;
        if (value == 0 || value == 0x80u || value == 0x81u) {
            if (!record) return PR_U25_HOOK_ARGUMENT;
            record[0x10Eu] = (value == 0x81u) ? 1u : 0u;
            record[0x10Fu] = (value == 0) ? 0u : 1u;
        }
        return PR_U25_HOOK_OK;
    }
    /* +0x0BFA: rel == 0x312 -> mov al,[bp+6]; push ax; lcall 0x802:0x094A. AX still holds
     * rel (0x0312) when AL is loaded, so the pushed word is 0x0300 | low byte of value. */
    if (rel == 0x312u) {
        uint16_t pushed = (uint16_t)(0x0300u | (value & 0xFFu));
        r->which = PR_U25_HOOK_CASE_312;
        r->callout_value = pushed;
        if (!callouts || !callouts->res_802_094a) return PR_U25_HOOK_CALLOUT;
        callouts->res_802_094a(callouts->context, pushed);
        r->callout_made = 1;
        return PR_U25_HOOK_OK;
    }
    /* +0x0C0A/+0x0C2D/+0x0C50: rel 0x322/0x324/0x326 -> if value > 0x80: call unit 0x3bd
     * entry 6 ((value & 0x7F) low byte, selector 1/2/3) */
    if (rel == 0x322u || rel == 0x324u || rel == 0x326u) {
        r->which = PR_U25_HOOK_CASE_322;
        r->callout_selector = 1u + (unsigned)((rel - 0x322u) >> 1);
        if (value <= 0x80u) return PR_U25_HOOK_OK;
        value = (uint16_t)(value & 0x7Fu);
        r->callout_value = (uint8_t)value;
        if (!callouts || !callouts->u3bd_entry6) return PR_U25_HOOK_CALLOUT;
        callouts->u3bd_entry6(callouts->context, (uint8_t)value, (uint8_t)r->callout_selector);
        r->callout_made = 1;
        return PR_U25_HOOK_OK;
    }
    return PR_U25_HOOK_OK;
}
