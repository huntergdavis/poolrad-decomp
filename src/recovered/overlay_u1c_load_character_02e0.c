/* Translation of unit 0x1c +0x02E0 (ECL opcode 0x0A, LOAD CHARACTER). File 0x1026, 212 bytes, 70 instructions, RETF.
 * Each block below names the file offsets it translates (unit-relative, +0x02E0 base). The disassembly was taken with capstone in
 * 16-bit mode. Claim and checks: docs/re/U1C_LOAD_CHARACTER_02E0.md. */
#include "poolrad/recovered/overlay_u1c_load_character_02e0.h"

static uint16_t ds_word(const uint8_t *ds, uint16_t off)
{
    return (uint16_t)(ds[off] | ((uint16_t)ds[(uint16_t)(off + 1u)] << 8));
}

static void ds_put_word(uint8_t *ds, uint16_t off, uint16_t value)
{
    ds[off] = (uint8_t)(value & 0xFFu);
    ds[(uint16_t)(off + 1u)] = (uint8_t)(value >> 8);
}

pr_u1c_lc02e0_status pr_u1c_load_character_02e0(const pr_u1c_lc02e0_env *env, uint8_t entry_ah)
{
    const pr_u1c_lc02e0_hooks *h;
    uint8_t *ds;
    void *ctx;
    uint16_t ret_a, ret_b;
    uint16_t p_off, p_seg, n_off, n_seg, w;
    uint8_t ah, i, count, count_raw, hi_flag;

    if (env == NULL || env->ds == NULL || env->ds_size < PR_U1C_LC02E0_DS_SIZE) {
        return PR_U1C_LC02E0_ARGUMENT;
    }
    h = &env->hooks;
    if (h->call_2b_2a == NULL || h->call_2b_25 == NULL || h->call_78_2a == NULL || h->call_ba_767 == NULL
        || h->far_read16 == NULL) {
        return PR_U1C_LC02E0_ARGUMENT;
    }
    ds = env->ds;
    ctx = env->ctx;

    /* +0x02E6  mov byte [0x4430], 1 */
    ds[0x4430] = 1u;

    /* +0x02EB..+0x02EE  mov al,1 ; push ax (AX = AH:01, AH from the caller) ; lcall 0x2B:0x2A
     * The returned AX is kept only as its high byte, which is the next argument's AH. */
    ret_a = h->call_2b_2a(ctx, (uint16_t)(((uint16_t)entry_ah << 8) | 1u));
    ah = (uint8_t)(ret_a >> 8);

    /* +0x02F3  mov byte [bp-7], 0   (loop index i) */
    i = 0u;

    /* +0x02F7..+0x02FF  mov al,1 ; push ax (AX = AH:01) ; lcall 0x2B:0x25 ; mov [bp-6], al   (count_raw) */
    ret_b = h->call_2b_25(ctx, (uint16_t)(((uint16_t)ah << 8) | 1u));
    count_raw = (uint8_t)(ret_b & 0xFFu);

    /* +0x0302..+0x030C  [bp-4]:[bp-2] := [0x5D96]:[0x5D98] (offset word, segment word) */
    p_off = ds_word(ds, 0x5D96u);
    p_seg = ds_word(ds, 0x5D98u);

    /* +0x030F..+0x031C  [bp-8] := count_raw & 0x80 ; [bp-6] := count_raw & 0x7F */
    hi_flag = (uint8_t)(count_raw & 0x80u);
    count = (uint8_t)(count_raw & 0x7Fu);

    /* +0x031F..+0x034B  loop: while count != 0 and i < count (unsigned) and ptr != 0:
     *                    ptr := ES:[ptr + 0x104] (offset word), with ES = ptr's segment and the segment word from +0x106; i++ */
    while (count != 0u && i < count && (p_off | p_seg) != 0u) {
        if (h->far_read16(ctx, p_seg, (uint16_t)(p_off + 0x104u), &n_off) != 0) {
            return PR_U1C_LC02E0_FAR_READ;
        }
        if (h->far_read16(ctx, p_seg, (uint16_t)(p_off + 0x106u), &n_seg) != 0) {
            return PR_U1C_LC02E0_FAR_READ;
        }
        p_off = n_off;
        p_seg = n_seg;
        i = (uint8_t)(i + 1u);
    }

    /* +0x034D..+0x035E  if ptr != 0 then [0x5D92]:[0x5D94] := ptr */
    if ((p_off | p_seg) != 0u) {
        ds_put_word(ds, 0x5D92u, p_off);
        ds_put_word(ds, 0x5D94u, p_seg);
    }

    /* +0x0362..+0x0374  the block below runs only when hi_flag != 0 and [0x84D2] != 0 and [0x84D3] != 0; otherwise every
     *                    jump (je 0x3B0) goes straight to the frame teardown, past the calls and the clears. */
    if (hi_flag != 0u && ds[0x84D2u] != 0u && ds[0x84D3u] != 0u) {
        /* +0x0376..+0x0389  AX := [0x4436] (so AH := its high byte); if [0x4438] = [0x5D94] and [0x4436] = [0x5D92]
         *                    then [0x4430] := 0 */
        w = ds_word(ds, 0x4436u);
        ah = (uint8_t)(w >> 8);
        if (ds_word(ds, 0x4438u) == ds_word(ds, 0x5D94u) && w == ds_word(ds, 0x5D92u)) {
            ds[0x4430u] = 0u;
        }

        /* +0x038E..+0x0394  mov al,0 ; push ax (AH:00) ; mov al,1 ; push ax (AH:01) ; lcall 0x78:0x2A */
        h->call_78_2a(ctx, (uint16_t)((uint16_t)ah << 8), (uint16_t)(((uint16_t)ah << 8) | 1u));

        /* +0x0399..+0x03A1  push [0x5D94] (segment, pushed first) ; push [0x5D92] (offset, top of stack) ; lcall 0xBA:0x767
         * The words are read after the previous call returns, since that callee may have changed them. */
        h->call_ba_767(ctx, ds_word(ds, 0x5D92u), ds_word(ds, 0x5D94u));

        /* +0x03A6..+0x03AB  mov byte [0x84D2], 0 ; mov byte [0x84D3], 0 */
        ds[0x84D2u] = 0u;
        ds[0x84D3u] = 0u;
    }

    /* +0x03B0..+0x03B3  mov sp,bp ; pop bp ; retf: the frame is discarded; the translation has no SP or BP to restore. */
    return PR_U1C_LC02E0_OK;
}
