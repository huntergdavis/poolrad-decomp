#include "poolrad/recovered/resident_464_block_wrapper.h"

#include <string.h>

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static void put_word(uint8_t *p, uint16_t value) {
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
}

static pr_r464_wrapper_status stopped(pr_r464_wrapper_report *r,
                                       pr_r464_wrapper_call call, int result) {
    r->stopped_at = call;
    r->callback_status = result;
    return result ? PR_R464_WRAPPER_CALLBACK : PR_R464_WRAPPER_HOOK;
}

#define REACHED(r, call) ((r)->calls[(r)->call_count++] = (call))

pr_r464_wrapper_status pr_r464_block_wrapper(
    const pr_r464_wrapper_env *env, const uint8_t name[256],
    uint16_t id_word, uint8_t arg_a, uint8_t arg_c,
    pr_r464_block_object **destination, pr_r464_wrapper_report *report) {
    char short_name[21];
    size_t i, length;
    uint16_t size = 0;
    const uint8_t *block = NULL;
    pr_r464_block_object *object;
    int result;

    if (!env || !name || !destination || !report || !env->extension)
        return PR_R464_WRAPPER_ARGUMENT;
    memset(report, 0, sizeof *report);

    /* 0x0118..0x012C: StrStore cap 255; 0x012C..0x0141: old bitmap release.
     * The merged reader later caps the name to 20, so copy that prefix here. */
    length = name[0] < 20u ? name[0] : 20u;
    if (*destination) {
        if (!env->release_prior)
            return stopped(report, PR_R464_CALL_RELEASE_PRIOR, 0);
        REACHED(report, PR_R464_CALL_RELEASE_PRIOR);
        result = env->release_prior(env->ctx, destination);
        if (result) return stopped(report, PR_R464_CALL_RELEASE_PRIOR, result);
    }

    /* The merged reader callback uses C strings. Embedded NULs in the
     * Pascal prefix cannot be represented; stop only when reader is reached. */
    for (i = 0; i < length; ++i) {
        if (!name[i + 1]) {
            report->stopped_at = PR_R464_CALL_READ_BLOCK;
            return PR_R464_WRAPPER_NAME;
        }
        short_name[i] = (char)name[i + 1];
    }
    short_name[length] = '\0';

    /* 0x0141..0x016C: load the private extension, low-byte ID, and reader
     * outputs. The host supplies the extension without publishing its text. */
    if (!env->read_block)
        return stopped(report, PR_R464_CALL_READ_BLOCK, 0);
    REACHED(report, PR_R464_CALL_READ_BLOCK);
    result = env->read_block(env->ctx, short_name, env->extension,
                             (uint8_t)id_word, &size, &block);
    report->reader_status = result;
    report->block_size = size;
    report->held_block = block;
    if (result) {
        report->stopped_at = PR_R464_CALL_READ_BLOCK;
        return PR_R464_WRAPPER_READER;
    }
    /* 0x016C..0x0176: size zero jumps directly to RETF 0x0E. */
    if (!size) return PR_R464_WRAPPER_OK;
    if (!block || size < 17u) {
        report->stopped_at = PR_R464_CALL_MAKE_OBJECT;
        return PR_R464_WRAPPER_BLOCK;
    }

    /* 0x0176..0x0222: six Move calls select four words, one byte and
     * eight bytes; 0x248E consumes the first two words and the byte. */
    if (!env->make_object)
        return stopped(report, PR_R464_CALL_MAKE_OBJECT, 0);
    REACHED(report, PR_R464_CALL_MAKE_OBJECT);
    result = env->make_object(env->ctx, word(block), word(block + 2),
                              block[8], arg_c, destination);
    if (result) return stopped(report, PR_R464_CALL_MAKE_OBJECT, result);
    object = *destination;
    if (!object || !object->bytes || object->capacity < 17u) {
        report->stopped_at = PR_R464_CALL_MAKE_OBJECT;
        return PR_R464_WRAPPER_BLOCK;
    }

    /* 0x0222..0x0261: two words and the eight-byte field are installed
     * after the constructor returns. Its own allocations stay in the hook. */
    put_word(object->bytes + 4, word(block + 4));
    put_word(object->bytes + 6, word(block + 6));
    memcpy(object->bytes + 9, block + 9, 8);

    /* 0x0261..0x0289: helper gets block pointer, offset 0x11, caller
     * bytes [BP+0x0C]/[BP+0x0A], and destination's current object pointer. */
    if (!env->process_block)
        return stopped(report, PR_R464_CALL_PROCESS_BLOCK, 0);
    REACHED(report, PR_R464_CALL_PROCESS_BLOCK);
    result = env->process_block(env->ctx, block, size,
                                PR_R464_BLOCK_DATA_OFFSET, arg_c, arg_a,
                                object);
    if (result) return stopped(report, PR_R464_CALL_PROCESS_BLOCK, result);

    /* 0x0289..0x029F: FreeMem(size), then key-flush runtime call. */
    if (!env->release_block)
        return stopped(report, PR_R464_CALL_RELEASE_BLOCK, 0);
    REACHED(report, PR_R464_CALL_RELEASE_BLOCK);
    env->release_block(env->ctx, block, size);
    report->held_block = NULL;
    if (!env->flush_899_0104)
        return stopped(report, PR_R464_CALL_FLUSH, 0);
    REACHED(report, PR_R464_CALL_FLUSH);
    result = env->flush_899_0104(env->ctx);
    if (result) return stopped(report, PR_R464_CALL_FLUSH, result);
    return PR_R464_WRAPPER_OK;
}
