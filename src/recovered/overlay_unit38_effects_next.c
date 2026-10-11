/* Portable C for GAME.OVR unit 0x38 public entries 6, 75, 82 and 97
 * (docs/re/OVERLAY_UNIT38_EFFECT_ENTRIES_NEXT.md). Byte arithmetic wraps
 * modulo 256 as the originals' byte adds and `inc/dec byte` do. */
#include "poolrad/recovered/overlay_unit38_effects_next.h"

static pr_u38_far_ptr read_far_ptr(const uint8_t *bytes)
{
    pr_u38_far_ptr p;
    p.offset = (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
    p.segment = (uint16_t)(bytes[2] | ((uint16_t)bytes[3] << 8));
    return p;
}

static void put(unsigned *out, unsigned value)
{
    if (out) *out = value;
}

/* +0x01C3: les di,[bp+0xC]; les di,es:[di+0x108]; mov ax,es:[di+0xA];
 * mov dx,es:[di+0xC]; mov [0x6826],ax; mov [0x6828],dx; les di,[0x6826];
 * mov al,es:[di+0x9F]; cmp al,0xA; jne L1; mov [bp-1],1; jmp T;
 * L1: cmp al,9; je L2; cmp al,0xC; jne L3; L2: mov [bp-1],2; jmp T;
 * L3: cmp al,4; jne L4; mov [bp-1],3; jmp T; L4: mov [bp-1],0;
 * T: add [0x6822],[bp-1]; add [0x6818],[bp-1]; mov byte [0x6819],9. */
pr_u38_status pr_u38_entry6(pr_u38n_state *state, const uint8_t *record,
                            pr_u38_resolve_fn resolve, void *context, unsigned *class_out)
{
    const uint8_t *object;
    const uint8_t *target;
    uint8_t value, class_value;
    put(class_out, 0);
    if (!state || !record || !resolve) return PR_U38_ARGUMENT;
    object = resolve(context, read_far_ptr(record + PR_U38_RECORD_SLOT_108), 0x0Eu);
    if (!object) return PR_U38_ARGUMENT;
    state->scratch_6826 = read_far_ptr(object + 0x0Au);
    target = resolve(context, state->scratch_6826, 0xA0u);
    if (!target) return PR_U38_ARGUMENT;
    value = target[0x9Fu];
    if (value == 0x0Au) class_value = 1;
    else if (value == 0x09u || value == 0x0Cu) class_value = 2;
    else if (value == 0x04u) class_value = 3;
    else class_value = 0;
    state->b6822 = (uint8_t)(state->b6822 + class_value);
    state->b6818 = (uint8_t)(state->b6818 + class_value);
    state->b6819 = 9;
    put(class_out, class_value);
    return PR_U38_OK;
}

/* +0x24D9: cmp byte [0x682A],0; jne done; les di,[bp+0xC]; mov al,es:[di+0x14];
 * five range tests (jb/ja pairs) each storing 1..5 into [bp-1] and jumping to
 * the add; no store when none matches; add: mov al,[bp-1]; xor ah,ah;
 * mov dx,ax; mov al,[0x6816]; xor ah,ah; add ax,dx; mov [0x6816],al. */
pr_u38_status pr_u38_entry75(pr_u38n_state *state, const uint8_t *record,
                             uint8_t stack_residue, unsigned *paths, unsigned *bracket_out)
{
    uint8_t value, bracket;
    unsigned taken = 0;
    put(paths, 0);
    put(bracket_out, 0);
    if (!state || !record) return PR_U38_ARGUMENT;
    if (state->b682a != 0) return PR_U38_OK;
    value = record[PR_U38_RECORD_BYTE_14];
    if (value >= 4u && value <= 6u) bracket = 1;
    else if (value >= 7u && value <= 10u) bracket = 2;
    else if (value >= 11u && value <= 13u) bracket = 3;
    else if (value >= 14u && value <= 17u) bracket = 4;
    else if (value >= 18u && value <= 20u) bracket = 5;
    else { bracket = stack_residue; taken |= PR_U38_PATH_SECOND; }   /* [bp-1] never written */
    state->b6816 = (uint8_t)(state->b6816 + bracket);
    taken |= PR_U38_PATH_EFFECT;
    put(paths, taken);
    put(bracket_out, bracket);
    return PR_U38_OK;
}

/* +0x2700: les di,[bp+0xC]; add byte es:[di+0x11B],3; les di,[bp+0xC];
 * mov al,es:[di+0x11B]; les di,[bp+0xC]; cmp al,es:[di+0x32]; jbe done;
 * les di,[bp+0xC]; mov al,es:[di+0x32]; les di,[bp+0xC]; mov es:[di+0x11B],al. */
pr_u38_status pr_u38_entry82(uint8_t *record, unsigned *paths)
{
    put(paths, 0);
    if (!record) return PR_U38_ARGUMENT;
    record[PR_U38_RECORD_BYTE_11B] = (uint8_t)(record[PR_U38_RECORD_BYTE_11B] + 3u);
    if (record[PR_U38_RECORD_BYTE_11B] > record[PR_U38_RECORD_BYTE_32]) {
        record[PR_U38_RECORD_BYTE_11B] = record[PR_U38_RECORD_BYTE_32];
        put(paths, PR_U38_PATH_EFFECT);
    }
    return PR_U38_OK;
}

/* +0x2A28: mov al,[0x6819]; and al,1; or al,al; je done; mov al,[0x681C];
 * mov [bp-2],al; mov al,1; cmp al,[bp-2]; ja done; mov [bp-1],al; jmp body;
 * next: inc byte [bp-1]; body: dec byte [0x6818]; mov al,[0x6818];
 * cmp al,[0x681C]; jae keep; mov al,[0x681C]; mov [0x6818],al;
 * keep: mov al,[bp-1]; cmp al,[bp-2]; jne next; done. */
pr_u38_status pr_u38_entry97(pr_u38n_state *state, unsigned *iterations_out)
{
    uint8_t limit, counter;
    unsigned iterations = 0;
    put(iterations_out, 0);
    if (!state) return PR_U38_ARGUMENT;
    if ((state->b6819 & 0x01u) == 0) return PR_U38_OK;
    limit = state->b681c;
    if (limit < 1u) return PR_U38_OK;
    counter = 1;
    for (;;) {
        state->b6818 = (uint8_t)(state->b6818 - 1u);
        if (state->b6818 < state->b681c) state->b6818 = state->b681c;
        iterations++;
        if (counter == limit) break;
        counter++;
    }
    put(iterations_out, iterations);
    return PR_U38_OK;
}
