/* Translation of unit 0x1c +0x2E10 (ECL opcode 0x38, PROGRAM). File 0x3B56, 109 bytes, 40 instructions, RETF. Each block names the
 * unit-relative offsets it translates (16-bit disassembly of the owner bytes). Claim and checks: docs/re/U1C_PROGRAM_2E10.md. */
#include "poolrad/recovered/overlay_u1c_program_2e10.h"

static uint16_t ds_word(const uint8_t *ds, uint16_t off)
{
    return (uint16_t)(ds[off] | ((uint16_t)ds[(uint16_t)(off + 1u)] << 8));
}

static void ds_put_word(uint8_t *ds, uint16_t off, uint16_t value)
{
    ds[off] = (uint8_t)(value & 0xFFu);
    ds[(uint16_t)(off + 1u)] = (uint8_t)(value >> 8);
}

pr_u1c_program_status pr_u1c_program_2e10(const pr_u1c_program_env *env, uint8_t entry_ah)
{
    const pr_u1c_program_hooks *h;
    uint8_t *ds;
    uint16_t w, ret_a, ret_b, saved;
    uint8_t ah, sel;

    if (env == NULL || env->ds == NULL || env->ds_size < PR_U1C_PROGRAM_DS_SIZE) {
        return PR_U1C_PROGRAM_ARGUMENT;
    }
    h = &env->hooks;
    if (h->call_2b_2a == NULL || h->call_2b_25 == NULL || h->call_78_25 == NULL || h->call_82_25 == NULL
        || h->call_ba_27d9 == NULL || h->call_near_2dd3 == NULL || h->call_near_0001 == NULL) {
        return PR_U1C_PROGRAM_ARGUMENT;
    }
    ds = env->ds;

    /* +2E10..+2E15  push bp ; mov bp,sp ; sub sp,4: four bytes of locals, no memory effect of the translation. */

    /* +2E16..+2E2B  cmp byte [0x4430],0 ; je +2E30 ; mov ax,[0x4436] ; mov dx,[0x4438] ; mov [0x5D92],ax ; mov [0x5D94],dx ;
     *                mov byte [0x4430],0.
     * On the flag path AX is loaded in full from [0x4436], so its high byte is the AH of the first argument word. */
    if (ds[0x4430u] != 0u) {
        w = ds_word(ds, 0x4436u);
        ds_put_word(ds, 0x5D92u, w);
        ds_put_word(ds, 0x5D94u, ds_word(ds, 0x4438u));
        ds[0x4430u] = 0u;
        ah = (uint8_t)(w >> 8);
    } else {
        ah = entry_ah;
    }

    /* +2E30..+2E33  mov al,1 ; push ax (AH:01) ; lcall 0x2B:0x2A. The returned AX keeps only its high byte: the next AH. */
    ret_a = h->call_2b_2a(env->ctx, (uint16_t)(((uint16_t)ah << 8) | 1u));
    ah = (uint8_t)(ret_a >> 8);

    /* +2E38..+2E40  mov al,1 ; push ax (AH:01) ; lcall 0x2B:0x25 ; mov [bp-1],al. The selector is the returned AL. */
    ret_b = h->call_2b_25(env->ctx, (uint16_t)(((uint16_t)ah << 8) | 1u));
    sel = (uint8_t)(ret_b & 0xFFu);

    /* +2E43..+2E48  mov al,[bp-1] ; cmp al,0 ; jne +2E56 */
    if (sel == 0u) {
        /* +2E4A..+2E4F  lcall 0x78:0x25 ; lcall 0xBA:0x27D9 (no arguments), then jmp +2E79 */
        h->call_78_25(env->ctx);
        h->call_ba_27d9(env->ctx);
    } else if (sel == 8u) {
        /* +2E56..+2E5F  cmp al,8 ; jne +2E61 ; lcall 0x82:0x25 ; jmp +2E79 */
        h->call_82_25(env->ctx);
    } else if (sel == 9u) {
        /* +2E61..+2E63  cmp al,9 ; jne +2E79 (taken for any other value: no call).
         * +2E65..+2E6A  mov ax,[0x49ED] ; mov [bp-3],ax ; push cs ; call near +2DD3
         * +2E6F..+2E72  mov ax,[bp-3] ; mov [0x49ED],ax   (the saved PC is restored whatever the callee left there)
         * +2E75..+2E76  push cs ; call near +0001 */
        saved = ds_word(ds, 0x49EDu);
        h->call_near_2dd3(env->ctx);
        ds_put_word(ds, 0x49EDu, saved);
        h->call_near_0001(env->ctx);
    }

    /* +2E79..+2E7C  mov sp,bp ; pop bp ; retf: SP and BP are restored, so the translation has nothing else to return. */
    return PR_U1C_PROGRAM_OK;
}
