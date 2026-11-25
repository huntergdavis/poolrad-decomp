/* Translation of unit 0x3B4 +0x016D (view refresh, public entry 1). File 0x2DCF4, 307 bytes, 114 instructions, RETF. Each block names
 * the unit-relative offsets it translates. AX is kept as a full 16-bit register: pushes take the whole value, and only the stated
 * instructions change AH. Claim and checks: docs/re/U3B4_VIEW_REFRESH_016D.md. */
#include "poolrad/recovered/overlay_u3b4_view_refresh_016d.h"

static uint16_t ds_word(const uint8_t *ds, uint16_t off)
{
    return (uint16_t)(ds[off] | ((uint16_t)ds[(uint16_t)(off + 1u)] << 8));
}

/* LES DI,[0x49D2]; then a word read at ES:[DI + field]. The far pointer is read from DS at each LES. */
static int rec_read(const pr_u3b4_016d_env *env, uint16_t field, uint16_t *out)
{
    uint16_t off = ds_word(env->ds, 0x49D2u);
    uint16_t seg = ds_word(env->ds, 0x49D4u);
    return env->hooks.far_read16(env->ctx, seg, (uint16_t)(off + field), out);
}

pr_u3b4_016d_status pr_u3b4_view_refresh_016d(const pr_u3b4_016d_env *env, uint8_t entry_ah)
{
    const pr_u3b4_016d_hooks *h;
    uint8_t *ds;
    uint16_t ax, w1, w2, w3, w4, rec, field;
    uint8_t flag, b1, b2, r;

    if (env == NULL || env->ds == NULL || env->ds_size < PR_U3B4_016D_DS_SIZE) {
        return PR_U3B4_016D_ARGUMENT;
    }
    h = &env->hooks;
    if (h->call_3d5_34 == NULL || h->call_3d5_25 == NULL || h->call_3d5_39 == NULL || h->near_00c7 == NULL
        || h->near_00fb == NULL || h->near_012f == NULL || h->call_3cd_25 == NULL || h->near_0000 == NULL
        || h->far_read16 == NULL) {
        return PR_U3B4_016D_ARGUMENT;
    }
    ds = env->ds;

    /* +016D..+0170  push bp ; mov bp,sp ; sub sp,4: the frame holds [bp-1], [bp-2] and the flag [bp-3]. */
    ax = (uint16_t)((uint16_t)entry_ah << 8);   /* AL is not read on entry */

    /* +0173..+0189  flag := [0x49FA]; les di,[0x49D2]; if es:[di+0x1CC] != 0 then flag := 1 */
    flag = ds[0x49FAu];
    if (rec_read(env, 0x1CCu, &rec) != 0) {
        return PR_U3B4_016D_FAR_READ;
    }
    if (rec != 0u) {
        flag = 1u;
    }

    if (flag == 1u) {
        /* +0193..+019B  mov al,[0x6AAD] ; push ax ; mov al,[0x6AAE] ; push ax ; lcall 0x3D5:0x34.
         * Each MOV AL changes only AL, so the words carry the entry AH. */
        ax = (uint16_t)((ax & 0xFF00u) | ds[0x6AADu]);
        w1 = ax;
        ax = (uint16_t)((ax & 0xFF00u) | ds[0x6AAEu]);
        w2 = ax;
        ax = h->call_3d5_34(env->ctx, w1, w2);

        /* +01A0..+01A8  mov [0x6AB1],al ; cmp [0x6AB1],0x7F ; jbe +01BC (unsigned) */
        ds[0x6AB1u] = (uint8_t)(ax & 0xFFu);
        field = (ds[0x6AB1u] <= 0x7Fu) ? 0x1FAu : 0x1FCu;
        /* +01AA..+01B7 or +01BC..+01C9  les di,[0x49D2] ; mov di,es:[di+field] ; mov al,[di+0xC26] ; mov [0x6A60],al.
         * AL is loaded from DS at (di + 0xC26), 16-bit wrap; AH keeps the callee's AH. */
        if (rec_read(env, field, &rec) != 0) {
            return PR_U3B4_016D_FAR_READ;
        }
        ax = (uint16_t)((ax & 0xFF00u) | ds[(uint16_t)(rec + 0xC26u)]);
        ds[0x6A60u] = (uint8_t)(ax & 0xFFu);

        /* +01CC..+01F4  if [0x84E5] != 0: three case-map calls, each with the byte pushed by MOV AL / PUSH AX. The near callees
         * return AL and keep AH, so AH stays the callee-return AH from the far call. */
        if (ds[0x84E5u] != 0u) {
            ax = (uint16_t)((ax & 0xFF00u) | ds[0x6F69u]);
            r = h->near_00c7(env->ctx, ax);
            ax = (uint16_t)((ax & 0xFF00u) | r);
            ds[0x6A61u] = r;

            ax = (uint16_t)((ax & 0xFF00u) | ds[0x6F69u]);
            r = h->near_00fb(env->ctx, ax);
            ax = (uint16_t)((ax & 0xFF00u) | r);
            ds[0x6A62u] = r;

            ax = (uint16_t)((ax & 0xFF00u) | ds[0x6F68u]);
            r = h->near_012f(env->ctx, ax);
            ax = (uint16_t)((ax & 0xFF00u) | r);
            ds[0x6A63u] = r;
        }

        /* +01F7..+0206  four words from [0x6A60..0x6A63] (each with the current AH), then lcall 0x3D5:0x25 */
        ax = (uint16_t)((ax & 0xFF00u) | ds[0x6A60u]);
        w1 = ax;
        ax = (uint16_t)((ax & 0xFF00u) | ds[0x6A61u]);
        w2 = ax;
        ax = (uint16_t)((ax & 0xFF00u) | ds[0x6A62u]);
        w3 = ax;
        ax = (uint16_t)((ax & 0xFF00u) | ds[0x6A63u]);
        w4 = ax;
        ax = h->call_3d5_25(env->ctx, w1, w2, w3, w4);

        /* +020C..+0218  les di,[0x49D2] ; cmp es:[di+0x1F6],0 ; je +021D ; mov byte [0x6AAC],0 */
        if (rec_read(env, 0x1F6u, &rec) != 0) {
            return PR_U3B4_016D_FAR_READ;
        }
        if (rec != 0u) {
            ds[0x6AACu] = 0u;
        }

        /* +021D..+0229  mov al,[0x6AAD] ; push ax ; mov al,[0x6AAE] ; push ax ; mov al,[0x6AAF] ; push ax ; lcall 0x3D5:0x39.
         * The words are read from DS after the previous callee returned, and carry that return's AH. */
        ax = (uint16_t)((ax & 0xFF00u) | ds[0x6AADu]);
        w1 = ax;
        ax = (uint16_t)((ax & 0xFF00u) | ds[0x6AAEu]);
        w2 = ax;
        ax = (uint16_t)((ax & 0xFF00u) | ds[0x6AAFu]);
        w3 = ax;
        (void)h->call_3d5_39(env->ctx, w1, w2, w3);

        /* +022E  jmp +029C: to the frame teardown and RETF */
        return PR_U3B4_016D_OK;
    }

    /* +0230..+0236  cmp al,2 ; jb +029C ; cmp al,4 ; ja +029C: the flag must be 2, 3 or 4 for the coordinate path. */
    if (flag < 2u || flag > 4u) {
        return PR_U3B4_016D_OK;
    }

    /* +0238..+023F  mov al,[bp-3] ; xor ah,ah ; mov di,ax ; mov al,[di+0x3C76]: AX is 0 .. 4 then the table byte; AH is 0 */
    ax = (uint16_t)flag;
    ax = ds[(uint16_t)(0x3C76u + flag)];

    /* +0243..+024E  xor ah,ah ; les di,[0x49D2] ; add ax,es:[di+0x186] ; dec ax ; dec ax */
    ax = (uint16_t)(ax & 0x00FFu);
    if (rec_read(env, 0x186u, &rec) != 0) {
        return PR_U3B4_016D_FAR_READ;
    }
    ax = (uint16_t)(ax + rec);
    ax = (uint16_t)(ax - 2u);

    /* +0250..+0263  mov [bp-1],al ; cmp [bp-1],0 ; jge ; mov [bp-1],0 ; cmp [bp-1],0x27 ; jle ; mov [bp-1],0x27 (signed bytes) */
    b1 = (uint8_t)(ax & 0xFFu);
    if ((int8_t)b1 < 0) {
        b1 = 0u;
    }
    if ((int8_t)b1 > 0x27) {
        b1 = 0x27u;
    }

    /* +0267..+0285  les di,[0x49D2] ; mov ax,es:[di+0x188] ; dec ax ; dec ax ; mov [bp-2],al ; signed clamp to 0 .. 0x1F */
    if (rec_read(env, 0x188u, &rec) != 0) {
        return PR_U3B4_016D_FAR_READ;
    }
    ax = (uint16_t)(rec - 2u);
    b2 = (uint8_t)(ax & 0xFFu);
    if ((int8_t)b2 < 0) {
        b2 = 0u;
    }
    if ((int8_t)b2 > 0x1F) {
        b2 = 0x1Fu;
    }

    /* +0289..+0298  mov al,[bp-1] ; cwde (CBW: AX := sign-extension of AL) ; push ax ; mov al,[bp-2] ; cwde ; push ax ;
     *                lcall 0x3CD:0x25 ; push cs ; call near 0x0000 */
    ax = (uint16_t)(int16_t)(int8_t)b1;
    w1 = ax;
    ax = (uint16_t)(int16_t)(int8_t)b2;
    w2 = ax;
    h->call_3cd_25(env->ctx, w1, w2);
    h->near_0000(env->ctx);

    /* +029C..+029F  mov sp,bp ; pop bp ; retf */
    return PR_U3B4_016D_OK;
}
