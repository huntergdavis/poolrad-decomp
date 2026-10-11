/* Portable C for sixteen bounded DOS v1.3 GAME.OVR unit 0x38 dispatch entries.
 * Each function mirrors the decoded instruction sequence of its original
 * (docs/re/OVERLAY_UNIT38_EFFECT_ENTRIES.md): byte arithmetic wraps modulo
 * 256 as `add/sub/inc/dec byte` do, and the `shr ax,1` shifts act on the
 * zero-extended byte so the stored byte is the old value halved. */
#include "poolrad/recovered/overlay_unit38_effects.h"

static void set_paths(unsigned *paths, unsigned value)
{
    if (paths) *paths = value;
}

static pr_u38_far_ptr read_far_ptr(const uint8_t *bytes)
{
    pr_u38_far_ptr p;
    p.offset = (uint16_t)(bytes[0] | ((uint16_t)bytes[1] << 8));
    p.segment = (uint16_t)(bytes[2] | ((uint16_t)bytes[3] << 8));
    return p;
}

/* +0x010D: add byte [0x6825],5; inc byte [0x6822]. */
pr_u38_status pr_u38_entry2(pr_u38_state *state)
{
    if (!state) return PR_U38_ARGUMENT;
    state->b6825 = (uint8_t)(state->b6825 + 5u);
    state->b6822 = (uint8_t)(state->b6822 + 1u);
    return PR_U38_OK;
}

/* +0x011D: cmp byte [0x6825],5; jae sub; mov byte [0x6825],0; jmp done;
 * sub: sub byte [0x6825],5; done: dec byte [0x6822]. */
pr_u38_status pr_u38_entry3(pr_u38_state *state, unsigned *paths)
{
    set_paths(paths, 0);
    if (!state) return PR_U38_ARGUMENT;
    if (state->b6825 >= 5u) {
        state->b6825 = (uint8_t)(state->b6825 - 5u);
        set_paths(paths, PR_U38_PATH_EFFECT);
    } else {
        state->b6825 = 0;
    }
    state->b6822 = (uint8_t)(state->b6822 - 1u);
    return PR_U38_OK;
}

/* +0x06E4: mov al,[0x6819]; and al,1; or al,al; je skip;
 * mov al,[0x6818]; xor ah,ah; shr ax,1; mov [0x6818],al; add byte [0x6816],3. */
pr_u38_status pr_u38_entry18(pr_u38_state *state, unsigned *paths)
{
    set_paths(paths, 0);
    if (!state) return PR_U38_ARGUMENT;
    if (state->b6819 & 0x01u) {
        state->b6818 = (uint8_t)(state->b6818 >> 1);
        state->b6816 = (uint8_t)(state->b6816 + 3u);
        set_paths(paths, PR_U38_PATH_EFFECT);
    }
    return PR_U38_OK;
}

/* +0x0A3D: mov al,[0x6818]; xor ah,ah; shr ax,1; shr ax,1; mov dx,ax;
 * mov al,[0x6818]; xor ah,ah; sub ax,dx; mov [0x6818],al. */
pr_u38_status pr_u38_entry25(pr_u38_state *state)
{
    unsigned quarter, value;
    if (!state) return PR_U38_ARGUMENT;
    quarter = (unsigned)state->b6818 >> 2;
    value = (unsigned)state->b6818 - quarter;
    state->b6818 = (uint8_t)value;
    return PR_U38_OK;
}

/* +0x10C1: mov al,[0x681A]; xor ah,ah; shr ax,1; mov [0x681A],al. */
pr_u38_status pr_u38_entry36(pr_u38_state *state)
{
    if (!state) return PR_U38_ARGUMENT;
    state->b681a = (uint8_t)(state->b681a >> 1);
    return PR_U38_OK;
}

/* +0x171F: mov byte [0x681E],1; sub byte [0x6822],4. */
pr_u38_status pr_u38_entry59(pr_u38_state *state)
{
    if (!state) return PR_U38_ARGUMENT;
    state->b681e = 1;
    state->b6822 = (uint8_t)(state->b6822 - 4u);
    return PR_U38_OK;
}

/* Shared body of +0x25A2, +0x2A6D and +0x2B63: mov al,[0x6819]; and al,mask;
 * or al,al; je skip; mov al,[0x6818]; xor ah,ah; shr ax,1; mov [0x6818],al. */
static pr_u38_status halve_6818_if_bit(pr_u38_state *state, uint8_t mask, unsigned *paths)
{
    set_paths(paths, 0);
    if (!state) return PR_U38_ARGUMENT;
    if (state->b6819 & mask) {
        state->b6818 = (uint8_t)(state->b6818 >> 1);
        set_paths(paths, PR_U38_PATH_EFFECT);
    }
    return PR_U38_OK;
}

pr_u38_status pr_u38_entry77(pr_u38_state *state, unsigned *paths)
{
    return halve_6818_if_bit(state, 0x01u, paths);
}

pr_u38_status pr_u38_entry98(pr_u38_state *state, unsigned *paths)
{
    return halve_6818_if_bit(state, 0x04u, paths);
}

pr_u38_status pr_u38_entry102(pr_u38_state *state, unsigned *paths)
{
    return halve_6818_if_bit(state, 0x02u, paths);
}

/* +0x05D6: les di,[bp+8]; mov al,es:[di+3]; les di,[bp+0xC]; mov es:[di+0x15],al. */
pr_u38_status pr_u38_entry13(uint8_t *record, const uint8_t *source)
{
    if (!record || !source) return PR_U38_ARGUMENT;
    record[PR_U38_RECORD_BYTE_15] = source[3];
    return PR_U38_OK;
}

/* +0x064E: les di,[bp+0xC]; cmp byte es:[di+0x111],0x39; jae next;
 * les di,[bp+0xC]; mov byte es:[di+0x111],0x39; next: inc byte [0x6816];
 * cmp byte [0x681B],0xF; jne done; mov byte [0x6818],0. */
pr_u38_status pr_u38_entry16(pr_u38_state *state, uint8_t *record, unsigned *paths)
{
    unsigned taken = 0;
    set_paths(paths, 0);
    if (!state || !record) return PR_U38_ARGUMENT;
    if (record[PR_U38_RECORD_BYTE_111] < 0x39u) {
        record[PR_U38_RECORD_BYTE_111] = 0x39u;
        taken |= PR_U38_PATH_EFFECT;
    }
    state->b6816 = (uint8_t)(state->b6816 + 1u);
    if (state->b681b == 0x0Fu) {
        state->b6818 = 0;
        taken |= PR_U38_PATH_SECOND;
    }
    set_paths(paths, taken);
    return PR_U38_OK;
}

/* +0x0BAC: sub byte [0x6822],4; les di,[bp+0xC]; sub byte es:[di+0x111],4;
 * les di,[bp+0xC]; sub byte es:[di+0x112],4; sub byte [0x6816],4. */
pr_u38_status pr_u38_entry28(pr_u38_state *state, uint8_t *record)
{
    if (!state || !record) return PR_U38_ARGUMENT;
    state->b6822 = (uint8_t)(state->b6822 - 4u);
    record[PR_U38_RECORD_BYTE_111] = (uint8_t)(record[PR_U38_RECORD_BYTE_111] - 4u);
    record[PR_U38_RECORD_BYTE_112] = (uint8_t)(record[PR_U38_RECORD_BYTE_112] - 4u);
    state->b6816 = (uint8_t)(state->b6816 - 4u);
    return PR_U38_OK;
}

/* +0x0C26: les di,[bp+0xC]; les di,es:[di+0x108]; cmp byte es:[di+3],0;
 * jle done; mov byte [0x681E],1; mov byte [0x6822],0xFF. */
pr_u38_status pr_u38_entry32(pr_u38_state *state, const uint8_t *record,
                             pr_u38_resolve_fn resolve, void *context, unsigned *paths)
{
    const uint8_t *object;
    set_paths(paths, 0);
    if (!state || !record || !resolve) return PR_U38_ARGUMENT;
    object = resolve(context, read_far_ptr(record + PR_U38_RECORD_SLOT_108), 4u);
    if (!object) return PR_U38_ARGUMENT;
    if ((int8_t)object[3] > 0) {
        state->b681e = 1;
        state->b6822 = 0xFFu;
        set_paths(paths, PR_U38_PATH_EFFECT);
    }
    return PR_U38_OK;
}

/* +0x1452: les di,[bp+0xC]; les di,es:[di+0x108]; mov byte es:[di+6],0;
 * cmp byte [0x681D],0; je done; mov byte [0x681A],0. */
pr_u38_status pr_u38_entry48(pr_u38_state *state, const uint8_t *record,
                             pr_u38_resolve_fn resolve, void *context, unsigned *paths)
{
    uint8_t *object;
    set_paths(paths, 0);
    if (!state || !record || !resolve) return PR_U38_ARGUMENT;
    object = resolve(context, read_far_ptr(record + PR_U38_RECORD_SLOT_108), 7u);
    if (!object) return PR_U38_ARGUMENT;
    object[6] = 0;
    if (state->b681d != 0) {
        state->b681a = 0;
        set_paths(paths, PR_U38_PATH_EFFECT);
    }
    return PR_U38_OK;
}

/* +0x2490: cmp byte [0x6DE3],0; jne mark; cmp byte [0x6822],0; jne mark;
 * les di,[bp+8]; mov al,es:[di+3]; and al,0xF; les di,[bp+8]; mov es:[di+3],al;
 * jmp done; mark: les di,[bp+8]; mov al,es:[di+3]; and al,0x10; or al,al;
 * jne done; mov byte [0x6822],0xFF; les di,[bp+8]; mov al,es:[di+3];
 * or al,0x10; les di,[bp+8]; mov es:[di+3],al. */
pr_u38_status pr_u38_entry74(pr_u38_state *state, uint8_t *source, unsigned *paths)
{
    set_paths(paths, 0);
    if (!state || !source) return PR_U38_ARGUMENT;
    if (state->b6de3 == 0 && state->b6822 == 0) {
        source[3] = (uint8_t)(source[3] & 0x0Fu);
        set_paths(paths, PR_U38_PATH_EFFECT);
    } else if ((source[3] & 0x10u) == 0) {
        state->b6822 = 0xFFu;
        source[3] = (uint8_t)(source[3] | 0x10u);
        set_paths(paths, PR_U38_PATH_SECOND);
    }
    return PR_U38_OK;
}

/* +0x013B: les di,[bp+0xC]; les di,es:[di+0x108]; mov ax,es:[di+0xA];
 * mov dx,es:[di+0xC]; mov [0x6826],ax; mov [0x6828],dx; les di,[0x6826];
 * cmp byte es:[di+0x9F],4; jne done; add byte [0x6822],2; add byte [0x6818],2. */
pr_u38_status pr_u38_entry4(pr_u38_state *state, const uint8_t *record,
                            pr_u38_resolve_fn resolve, void *context, unsigned *paths)
{
    const uint8_t *object;
    const uint8_t *target;
    set_paths(paths, 0);
    if (!state || !record || !resolve) return PR_U38_ARGUMENT;
    object = resolve(context, read_far_ptr(record + PR_U38_RECORD_SLOT_108), 0x0Eu);
    if (!object) return PR_U38_ARGUMENT;
    state->scratch_6826 = read_far_ptr(object + 0x0Au);
    target = resolve(context, state->scratch_6826, 0xA0u);
    if (!target) return PR_U38_ARGUMENT;
    if (target[0x9Fu] == 4u) {
        state->b6822 = (uint8_t)(state->b6822 + 2u);
        state->b6818 = (uint8_t)(state->b6818 + 2u);
        set_paths(paths, PR_U38_PATH_EFFECT);
    }
    return PR_U38_OK;
}
