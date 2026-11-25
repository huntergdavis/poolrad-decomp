/* Translation of unit 0x3B4 +0x0000 (the unit's entry in ledger slot 2). File 0x2DB87, 199 bytes, 72 instructions, RETF. Each block names the
 * unit-relative offsets it translates. AX is a full 16-bit register from the first use: entry AX is not read. Claim and checks:
 * docs/re/U3B4_ENTRY2_0000.md. */
#include "poolrad/recovered/overlay_u3b4_entry2_0000.h"

static uint16_t ds_word(const uint8_t *ds, uint16_t off)
{
    return (uint16_t)(ds[off] | ((uint16_t)ds[(uint16_t)(off + 1u)] << 8));
}

/* LES DI,[0x49D2] and a word read at ES:[DI + field]. The far pointer is read from DS at each LES. */
static int rec_read(const pr_u3b4_entry2_env *env, uint16_t field, uint16_t *out)
{
    uint16_t off = ds_word(env->ds, 0x49D2u);
    uint16_t seg = ds_word(env->ds, 0x49D4u);
    return env->hooks.far_read16(env->ctx, seg, (uint16_t)(off + field), out);
}

pr_u3b4_entry2_status pr_u3b4_entry2_0000(const pr_u3b4_entry2_env *env)
{
    const pr_u3b4_entry2_hooks *h;
    uint8_t *ds;
    uint16_t ax, rec, h_ah, w1, w2, w3, w4, w5;
    uint8_t b1, b2, val;

    if (env == NULL || env->ds == NULL || env->ds_size < PR_U3B4_ENTRY2_DS_SIZE) {
        return PR_U3B4_ENTRY2_ARGUMENT;
    }
    h = &env->hooks;
    if (h->call_3e9_39 == NULL || h->call_464_1b52 == NULL || h->far_read16 == NULL) {
        return PR_U3B4_ENTRY2_ARGUMENT;
    }
    ds = env->ds;

    /* +0000..+0005  push bp ; mov bp,sp ; sub sp,2: [bp-1] and [bp-2] are locals. */

    /* +0006..+000B  mov al,[0x49FA] ; xor ah,ah ; mov di,ax: the index is the full byte, AH is 0 */
    ax = ds[0x49FAu];

    /* +000D..+0011  mov al,[di+0x3C76] ; xor ah,ah: AX is the table byte */
    ax = ds[(uint16_t)(0x3C76u + ax)];

    /* +0013..+0017  les di,[0x49D2] ; add ax,es:[di+0x186]: a full-width add, so AH may change */
    if (rec_read(env, 0x186u, &rec) != 0) {
        return PR_U3B4_ENTRY2_FAR_READ;
    }
    ax = (uint16_t)(ax + rec);

    /* +001C..+0023  mov [bp-1],al ; cmp [bp-1],2 ; jge: the byte is compared as a signed value.
     * +0027..+0031: a value of 2 or more and below 0x2A becomes 2 (+002D).
     * +0033..+0039: 0x2A becomes 3 (+0039). +003F: 0x2B or more becomes 4.
     * A value below 2 (signed; negative values included) is kept. */
    b1 = (uint8_t)(ax & 0xFFu);
    if ((int8_t)b1 >= 2) {
        if ((int8_t)b1 < 0x2A) {
            b1 = 2u;
        } else if ((int8_t)b1 < 0x2B) {
            b1 = 3u;
        } else {
            b1 = 4u;
        }
    }

    /* +0043..+004D  les di,[0x49D2] ; cmp word es:[di+0x188],2 ; jae +005D (unsigned).
     * +004F..+0058  below 2: [bp-2] := the low byte of the word (mov al,byte es:[di+0x188]).
     * +005D..+006D  2 up to 0x21: [bp-2] := 2 (the 0x22 test at +0061 is unsigned).
     * +006F..+007F  0x22: [bp-2] := 3; 0x23 or more: [bp-2] := 4. */
    if (rec_read(env, 0x188u, &rec) != 0) {
        return PR_U3B4_ENTRY2_FAR_READ;
    }
    if (rec >= 2u) {
        if (rec >= 0x22u) {
            b2 = (rec >= 0x23u) ? 4u : 3u;
        } else {
            b2 = 2u;
        }
    } else {
        b2 = (uint8_t)(rec & 0xFFu);
    }

    /* +0085..+0093  mov al,[bp-1] ; push ax ; mov al,[bp-2] ; push ax ; mov al,[0x6AAF] ; push ax.
     * Each MOV AL changes only AL, so every word carries the high byte of AX as the record add left it. */
    h_ah = ax >> 8;
    w1 = (uint16_t)((h_ah << 8) | b1);
    w2 = (uint16_t)((h_ah << 8) | b2);
    w3 = (uint16_t)((h_ah << 8) | ds[0x6AAFu]);

    /* +0094..+009D  mov al,[0x6DDC] ; xor ah,ah ; cdq ; mov cx,2 ; idiv cx ; xchg dx,ax ; push ax: w4 is the remainder of the byte by 2.
     * The byte is read before the callee, so the callee's writes do not reach it. */
    val = ds[0x6DDCu];
    w4 = (uint16_t)(val % 2u);

    /* +009E..+00A8  mov al,[0x6DDC] ; xor ah,ah ; shr ax,1 ; add ax,9 ; push ax: w5 = (byte >> 1) + 9 */
    w5 = (uint16_t)((uint16_t)(val >> 1) + 9u);

    /* +00A9  lcall 0x3E9:0x39 with the five words; the callee's return is not used */
    h->call_3e9_39(env->ctx, w1, w2, w3, w4, w5);

    /* +00AE..+00BB  mov al,[0x6DDC] ; xor ah,ah ; inc ax ; cdq ; mov cx,6 ; idiv cx ; xchg dx,ax ; mov [0x6DDC],al.
     * The byte is read after the callee returns. The value is (byte + 1) mod 6, stored in the low byte. */
    val = ds[0x6DDCu];
    ds[0x6DDCu] = (uint8_t)(((uint16_t)val + 1u) % 6u);

    /* +00BE  lcall 0x464:0x1B52 with no arguments */
    h->call_464_1b52(env->ctx);

    /* +00C3..+00C6  mov sp,bp ; pop bp ; retf: SP and BP are restored, so the translation has nothing else to return. */
    return PR_U3B4_ENTRY2_OK;
}
