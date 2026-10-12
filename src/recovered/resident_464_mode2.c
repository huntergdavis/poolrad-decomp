#include "poolrad/recovered/resident_464_mode2.h"

#include <string.h>

static uint16_t word(const uint8_t *p) {
    return (uint16_t)((uint16_t)p[0] | ((uint16_t)p[1] << 8));
}

static int auxiliary_present(const uint8_t *object) {
    return object[0x13] || object[0x14] ||
           object[0x15] || object[0x16];
}

static pr_r464_mode2_status stop(pr_r464_mode2_report *r,
                                  pr_r464_mode2_stop site,
                                  pr_r464_mode2_status status) {
    r->stopped_at = site;
    return status;
}

pr_r464_mode2_status pr_r464_postprocess_mode2(
    const uint8_t *source, size_t source_size, uint16_t source_offset,
    uint8_t raw_unused, uint8_t selector, pr_r464_block_object *object,
    pr_r464_block_object *auxiliary, pr_r464_mode2_report *report) {
    unsigned layer;
    uint8_t count;

    (void)raw_unused; /* The decoded +0x02A5 body never reads [BP+0x0C]. */
    if (!object || !report) return PR_R464_MODE2_ARGUMENT;
    memset(report, 0, sizeof *report);
    report->source_offset = source_offset;
    if (!object->bytes || object->capacity < 9u)
        return stop(report, PR_R464_MODE2_STOP_HEADER,
                    PR_R464_MODE2_BOUNDS);

    /* +0x07B3..+0x07C7: count zero skips the entire route. */
    count = object->bytes[8];
    for (layer = 1; layer <= count; ++layer) {
        uint16_t stride;
        size_t src = report->source_offset;
        size_t dst = (size_t)report->data_offset + 0x17u;
        uint16_t end;
        uint16_t index;

        report->layer = (uint8_t)layer;
        if (object->capacity < 0x13u)
            return stop(report, PR_R464_MODE2_STOP_HEADER,
                        PR_R464_MODE2_BOUNDS);
        stride = word(object->bytes + 0x11);
        /* +0x07CF..+0x07EE: source and object addresses use 16-bit
         * offset arithmetic; runtime Move copies stride bytes. The host
         * only admits a contiguous, nonwrapping transfer. */
        if (stride && (!source || src + stride > source_size ||
                       src + stride > 0x10000u ||
                       dst + stride > object->capacity ||
                       dst + stride > 0x10000u))
            return stop(report, PR_R464_MODE2_STOP_MOVE,
                        PR_R464_MODE2_BOUNDS);
        if (stride) memmove(object->bytes + dst, source + src, stride);
        ++report->move_calls;

        /* +0x07F3..+0x081F: skip the auxiliary loop if its far pointer
         * words are both zero. The inclusive WORD end wraps like AX. */
        if (object->capacity < 0x17u)
            return stop(report, PR_R464_MODE2_STOP_HEADER,
                        PR_R464_MODE2_BOUNDS);
        if (auxiliary_present(object->bytes)) {
            end = (uint16_t)(report->data_offset + stride - 1u);
            if (report->data_offset <= end) {
                index = report->data_offset;
                for (;;) {
                    size_t primary_index = (size_t)index + 0x17u;
                    uint8_t value;
                    report->mask_index = index;
                    if (primary_index >= object->capacity ||
                        primary_index >= 0x10000u)
                        return stop(report, PR_R464_MODE2_STOP_PRIMARY_READ,
                                    PR_R464_MODE2_BOUNDS);
                    value = object->bytes[primary_index];

                    /* +0x0824..+0x0852: high nibble hit overwrites
                     * auxiliary[index] with 0xF0. */
                    if ((uint8_t)(value >> 4) == selector) {
                        if (!auxiliary || !auxiliary->bytes)
                            return stop(report, PR_R464_MODE2_STOP_AUX_WRITE,
                                        PR_R464_MODE2_AUX_MAPPING);
                        if (index >= auxiliary->capacity)
                            return stop(report, PR_R464_MODE2_STOP_AUX_WRITE,
                                        PR_R464_MODE2_BOUNDS);
                        auxiliary->bytes[index] = 0xF0u;
                        ++report->aux_writes;
                    }
                    /* +0x0852..+0x0875: low nibble hit adds 0x0F to
                     * the current auxiliary byte modulo 256. */
                    value = object->bytes[primary_index];
                    if ((uint8_t)(value & 0x0Fu) == selector) {
                        if (!auxiliary || !auxiliary->bytes)
                            return stop(report, PR_R464_MODE2_STOP_AUX_WRITE,
                                        PR_R464_MODE2_AUX_MAPPING);
                        if (index >= auxiliary->capacity)
                            return stop(report, PR_R464_MODE2_STOP_AUX_WRITE,
                                        PR_R464_MODE2_BOUNDS);
                        auxiliary->bytes[index] =
                            (uint8_t)(auxiliary->bytes[index] + 0x0Fu);
                        ++report->aux_writes;
                    }
                    /* +0x0875..+0x08A4: this read occurs even when
                     * neither comparison matched. */
                    if (!auxiliary || !auxiliary->bytes)
                        return stop(report, PR_R464_MODE2_STOP_AUX_READ,
                                    PR_R464_MODE2_AUX_MAPPING);
                    if (index >= auxiliary->capacity)
                        return stop(report, PR_R464_MODE2_STOP_AUX_READ,
                                    PR_R464_MODE2_BOUNDS);
                    object->bytes[primary_index] &=
                        (uint8_t)~auxiliary->bytes[index];
                    ++report->primary_writes;
                    ++report->mask_steps;
                    if (index == end) break;
                    index = (uint16_t)(index + 1u);
                }
            }
        }

        /* +0x08AF..+0x08C9: the two frame offsets advance after all
         * reached auxiliary work. The header stride is read twice. */
        report->data_offset =
            (uint16_t)(report->data_offset + word(object->bytes + 0x11));
        report->source_offset =
            (uint16_t)(report->source_offset + word(object->bytes + 0x11));
    }
    return PR_R464_MODE2_OK;
}
