/* Translation of unit 0x1c +0x2B33 (ECL opcode 0x39, WHO). File 0x3879, 75 bytes, 36 instructions, RETF. Each block names the
 * unit-relative offsets it translates (the disassembly of the owner bytes, 16-bit mode). Claim and checks: docs/re/U1C_WHO_2B33.md. */
#include "poolrad/recovered/overlay_u1c_who_2b33.h"

#define WHO_FRAME_SIZE 0x100u
#define WHO_SRC_OFF 0x70C8u
#define WHO_STR_MAX 0x00FFu
#define WHO_PTR_OFF 0x5D92u

pr_u1c_who_status pr_u1c_who_2b33(const pr_u1c_who_env *env, uint8_t entry_ah)
{
    const pr_u1c_who_hooks *h;
    uint16_t ret_a, ret_c, dst_off, ah_word;
    uint8_t ah;

    if (env == NULL) {
        return PR_U1C_WHO_ARGUMENT;
    }
    h = &env->hooks;
    if (h->call_2b_2a == NULL || h->call_709_0dd3 == NULL || h->call_af8_0b25 == NULL || h->call_ba_2c48 == NULL) {
        return PR_U1C_WHO_ARGUMENT;
    }
    /* +0x2B33..+0x2B36  push bp ; mov bp,sp ; sub sp,0x100: BP = entry_sp - 2, the frame is [BP-0x100, BP). A frame address
     * below zero would wrap, so the translation rejects it before any call. */
    if (env->entry_sp < WHO_FRAME_SIZE + 2u) {
        return PR_U1C_WHO_ARGUMENT;
    }
    dst_off = (uint16_t)(env->entry_sp - 2u - WHO_FRAME_SIZE);

    /* +0x2B3A..+0x2B3D  mov al,1 ; push ax (AX = AH:01, AH from the caller) ; lcall 0x2B:0x2A
     * The returned AX is kept only as its high byte, which is the next four words' AH. */
    ret_a = h->call_2b_2a(env->ctx, (uint16_t)(((uint16_t)entry_ah << 8) | 1u));
    ah = (uint8_t)(ret_a >> 8);

    /* +0x2B42..+0x2B4E  four words, each mov al,imm ; push ax with AH unchanged: 1, 0x11, 0x26, 0x16 ; lcall 0x709:0x0DD3 */
    h->call_709_0dd3(env->ctx,
                     (uint16_t)(((uint16_t)ah << 8) | 0x01u),
                     (uint16_t)(((uint16_t)ah << 8) | 0x11u),
                     (uint16_t)(((uint16_t)ah << 8) | 0x26u),
                     (uint16_t)(((uint16_t)ah << 8) | 0x16u));

    /* +0x2B53..+0x2B62  push ds ; push 0x70C8 ; push ss ; push frame address ; mov ax,0xFF ; push ax ; lcall 0xAF8:0x0B25.
     * AX is loaded with 0xFF here, so the last word's AH is 0. The returned AX is kept for the final call. */
    ret_c = h->call_af8_0b25(env->ctx, env->ds_seg, WHO_SRC_OFF, env->ss_seg, dst_off, WHO_STR_MAX);
    ah = (uint8_t)(ret_c >> 8);

    /* +0x2B6D..+0x2B75  mov al,0 ; push ax (AH:00, AH from the StrStore's return) ; mov di,0x5D92 ; push ds ; push di ;
     *                    lcall 0xBA:0x2C48 */
    ah_word = (uint16_t)(((uint16_t)ah << 8) | 0x00u);
    h->call_ba_2c48(env->ctx, ah_word, env->ds_seg, WHO_PTR_OFF);

    /* +0x2B7A..+0x2B7D  mov sp,bp ; pop bp ; retf: SP and BP are restored, so the translation has nothing else to return. */
    return PR_U1C_WHO_OK;
}
