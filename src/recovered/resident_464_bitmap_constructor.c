#include "poolrad/recovered/resident_464_bitmap_constructor.h"

#include <string.h>

#define REACHED(r, call) ((r)->calls[(r)->call_count++] = (call))

static void put_word(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static pr_r464_ctor_status stopped(pr_r464_ctor_report *r,
                                    pr_r464_ctor_call call, int result) {
    r->stopped_at = call;
    r->callback_status = result;
    return result ? PR_R464_CTOR_CALLBACK : PR_R464_CTOR_HOOK;
}

pr_r464_ctor_status pr_r464_bitmap_construct(
    const pr_r464_ctor_env *env, uint16_t word_0, uint16_t word_2,
    uint8_t byte_8, uint8_t arg_a, pr_r464_block_object **destination,
    pr_r464_ctor_report *report) {
    pr_r464_block_object *primary = NULL, *auxiliary = NULL;
    uint16_t span_word, data_size, primary_size, auxiliary_size;
    uint8_t video;
    int result;

    if (!env || !destination || !report) return PR_R464_CTOR_ARGUMENT;
    memset(report, 0, sizeof *report);

    /* +0x2494..+0x249F: low AX of unsigned MUL, then SHL AX. */
    span_word = (uint16_t)((uint32_t)word_0 * word_2);
    span_word = (uint16_t)((uint32_t)span_word << 1);
    report->span_word = span_word;
    /* +0x249F..+0x24AE: nonzero DS video byte doubles the low WORD. */
    if (!env->read_video)
        return stopped(report, PR_R464_CTOR_READ_VIDEO_MAIN, 0);
    REACHED(report, PR_R464_CTOR_READ_VIDEO_MAIN);
    result = env->read_video(env->ctx, &video);
    if (result) return stopped(report, PR_R464_CTOR_READ_VIDEO_MAIN, result);
    report->video_main = video;
    if (video) span_word = (uint16_t)((uint32_t)span_word << 1);
    report->span_word = span_word;

    /* +0x24AE..+0x24C5: low AX of byte_8 * span_word, then +0x17
     * modulo 65536, passed to GetMem. */
    data_size = (uint16_t)((uint32_t)byte_8 * span_word);
    primary_size = (uint16_t)(data_size + 0x17u);
    report->data_size = data_size;
    report->primary_size = primary_size;
    if (!env->allocate)
        return stopped(report, PR_R464_CTOR_ALLOC_MAIN, 0);
    REACHED(report, PR_R464_CTOR_ALLOC_MAIN);
    result = env->allocate(env->ctx, primary_size, &primary);
    report->primary = primary;
    if (result) return stopped(report, PR_R464_CTOR_ALLOC_MAIN, result);
    *destination = primary; /* +0x24C5..+0x24CF */
    if (!primary || primary_size < 0x17u || !primary->bytes ||
        primary->capacity < primary_size) {
        report->stopped_at = PR_R464_CTOR_FILL_MAIN;
        return PR_R464_CTOR_BOUNDS;
    }

    /* +0x24CF..+0x2519: FillChar then four direct header writes. */
    if (!env->fill_zero)
        return stopped(report, PR_R464_CTOR_FILL_MAIN, 0);
    REACHED(report, PR_R464_CTOR_FILL_MAIN);
    result = env->fill_zero(env->ctx, primary, primary_size);
    if (result) return stopped(report, PR_R464_CTOR_FILL_MAIN, result);
    put_word(primary->bytes, word_0);
    put_word(primary->bytes + 2, word_2);
    put_word(primary->bytes + 0x11, span_word);
    primary->bytes[8] = byte_8;

    /* +0x2519..+0x2522: only bit 0 of [BP+0x0A] is retained. */
    if (!(arg_a & 1u)) return PR_R464_CTOR_OK;

    /* +0x2522..+0x2562: video state is read again. Exactly 1 selects
     * data_size >> 2; every other value selects data_size. */
    if (!env->read_video)
        return stopped(report, PR_R464_CTOR_READ_VIDEO_AUX, 0);
    REACHED(report, PR_R464_CTOR_READ_VIDEO_AUX);
    result = env->read_video(env->ctx, &video);
    if (result) return stopped(report, PR_R464_CTOR_READ_VIDEO_AUX, result);
    report->video_aux = video;
    auxiliary_size = video == 1u ? (uint16_t)(data_size >> 2) : data_size;
    report->auxiliary_size = auxiliary_size;
    if (!env->allocate)
        return stopped(report, PR_R464_CTOR_ALLOC_AUX, 0);
    REACHED(report, PR_R464_CTOR_ALLOC_AUX);
    result = env->allocate(env->ctx, auxiliary_size, &auxiliary);
    report->auxiliary = auxiliary;
    if (result) return stopped(report, PR_R464_CTOR_ALLOC_AUX, result);

    /* +0x2536..+0x255B or +0x256A..+0x258A: store far pointer at +0x13,
     * then FillChar the auxiliary allocation. Host pointer mapping is
     * deliberately delegated to attach_aux. */
    if (!env->attach_aux)
        return stopped(report, PR_R464_CTOR_ATTACH_AUX, 0);
    REACHED(report, PR_R464_CTOR_ATTACH_AUX);
    result = env->attach_aux(env->ctx, primary, auxiliary);
    if (result) return stopped(report, PR_R464_CTOR_ATTACH_AUX, result);
    if (!auxiliary || (auxiliary_size &&
        (!auxiliary->bytes || auxiliary->capacity < auxiliary_size))) {
        report->stopped_at = PR_R464_CTOR_FILL_AUX;
        return PR_R464_CTOR_BOUNDS;
    }
    if (!env->fill_zero)
        return stopped(report, PR_R464_CTOR_FILL_AUX, 0);
    REACHED(report, PR_R464_CTOR_FILL_AUX);
    result = env->fill_zero(env->ctx, auxiliary, auxiliary_size);
    if (result) return stopped(report, PR_R464_CTOR_FILL_AUX, result);
    return PR_R464_CTOR_OK;
}
