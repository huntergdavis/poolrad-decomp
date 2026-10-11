#include "poolrad/recovered/resident_route_290b.h"

static pr_rec_290b_push pushed(pr_rec_290b_push_kind kind, uint16_t value)
{
    pr_rec_290b_push result = {kind, value};
    return result;
}

static pr_rec_290b_status invoke(const pr_rec_290b_io *io, uint16_t site,
                                  pr_rec_290b_target target,
                                  const pr_rec_290b_push *pushes,
                                  uint8_t count, uint8_t *observed_length)
{
    pr_rec_290b_call call = {0};
    uint8_t i;
    if (io->call == 0) return PR_REC_290B_BOUNDARY;
    call.image_site = site;
    call.target = target;
    call.push_count = count;
    for (i = 0; i < count; ++i) call.pushes[i] = pushes[i];
    return io->call(io->context, &call, observed_length) ?
        PR_REC_290B_BOUNDARY : PR_REC_290B_OK;
}

static pr_rec_290b_status ds_byte(const pr_rec_290b_io *io,
                                   uint16_t offset, uint8_t *value)
{
    if (io->read_ds_byte == 0 ||
        io->read_ds_byte(io->context, offset, value))
        return PR_REC_290B_BOUNDARY;
    return PR_REC_290B_OK;
}

static pr_rec_290b_status far_byte(const pr_rec_290b_io *io,
                                    uint16_t slot, uint16_t displacement,
                                    uint8_t *value)
{
    pr_rec_290b_far_ptr base;
    if (io->read_ds_far_ptr == 0 ||
        io->read_ds_far_ptr(io->context, slot, &base) ||
        io->read_far_byte == 0 ||
        io->read_far_byte(io->context, base,
                          (uint16_t)(base.offset + displacement), value))
        return PR_REC_290B_BOUNDARY;
    return PR_REC_290B_OK;
}

static pr_rec_290b_status far_word(const pr_rec_290b_io *io,
                                    uint16_t slot, uint16_t displacement,
                                    uint16_t *value)
{
    pr_rec_290b_far_ptr base;
    if (io->read_ds_far_ptr == 0 ||
        io->read_ds_far_ptr(io->context, slot, &base) ||
        io->read_far_word == 0 ||
        io->read_far_word(io->context, base,
                          (uint16_t)(base.offset + displacement), value))
        return PR_REC_290B_BOUNDARY;
    return PR_REC_290B_OK;
}

#define WORD(x) pushed(PR_REC_290B_WORD, (x))
#define LOW(x) pushed(PR_REC_290B_AX_LOW_BYTE, (x))
#define LOCAL(x) pushed(PR_REC_290B_SS_LOCAL, (x))
#define CODE(x) pushed(PR_REC_290B_CS_OFFSET, (x))
#define DATA(x) pushed(PR_REC_290B_DS_OFFSET, (x))
#define CALL(site, target, observed, ...) do { \
    const pr_rec_290b_push args[] = {__VA_ARGS__}; \
    if (invoke(io, (site), (target), args, \
               (uint8_t)(sizeof args / sizeof args[0]), (observed)) != \
        PR_REC_290B_OK) return PR_REC_290B_BOUNDARY; \
} while (0)

pr_rec_290b_status pr_rec_res_00ba_290b(const pr_rec_290b_io *io)
{
    uint8_t first, second, mode, length, index;
    uint16_t value, other, flags;
    if (io == 0) return PR_REC_290B_ARGUMENT;

    /* [BP-0x31] begins as an empty Pascal string. No other local contents
     * are claimed; the host owns the effects of the string/runtime calls. */
    if (ds_byte(io, 0x6aadu, &first) != PR_REC_290B_OK ||
        ds_byte(io, 0x6aaeu, &second) != PR_REC_290B_OK ||
        ds_byte(io, 0x49fau, &mode) != PR_REC_290B_OK)
        return PR_REC_290B_BOUNDARY;
    if (mode > 1u) {
        if (far_byte(io, 0x49d2u, 0x0186u, &first) != PR_REC_290B_OK ||
            far_byte(io, 0x49d2u, 0x0188u, &second) != PR_REC_290B_OK ||
            ds_byte(io, 0x49fau, &mode) != PR_REC_290B_OK)
            return PR_REC_290B_BOUNDARY;
        if (mode == 3u) first = (uint8_t)(first + 13u);
        if (ds_byte(io, 0x49fau, &mode) != PR_REC_290B_OK)
            return PR_REC_290B_BOUNDARY;
        if (mode == 4u) first = (uint8_t)(first + 26u);
    }

    if (far_word(io, 0x49d2u, 0x0192u, &value) != PR_REC_290B_OK)
        return PR_REC_290B_BOUNDARY;
    CALL(0x3507u, PR_REC_290B_LOCAL_1C13, 0, LOCAL(0x0131u), WORD(value));
    CALL(0x3513u, PR_REC_290B_SYSTEM_0B25, &length,
         LOCAL(0x0005u), WORD(2u));
    if (length < 2u) {
        CALL(0x3529u, PR_REC_290B_SYSTEM_0B0B, 0,
             LOCAL(0x0131u), CODE(0x28f2u));
        CALL(0x3533u, PR_REC_290B_SYSTEM_0B98, 0, LOCAL(0x0005u));
        CALL(0x3541u, PR_REC_290B_SYSTEM_0B25, 0,
             LOCAL(0x0005u), WORD(2u));
    }

    if (far_word(io, 0x49d2u, 0x0190u, &value) != PR_REC_290B_OK ||
        far_word(io, 0x49d2u, 0x018eu, &other) != PR_REC_290B_OK)
        return PR_REC_290B_BOUNDARY;
    value = (uint16_t)((uint32_t)value * 10u + other);
    CALL(0x3563u, PR_REC_290B_LOCAL_1C13, 0, LOCAL(0x0131u), WORD(value));
    CALL(0x356fu, PR_REC_290B_SYSTEM_0B25, &length,
         LOCAL(0x0008u), WORD(2u));
    if (length < 2u) {
        CALL(0x3585u, PR_REC_290B_SYSTEM_0B0B, 0,
             LOCAL(0x0131u), CODE(0x28f2u));
        CALL(0x358fu, PR_REC_290B_SYSTEM_0B98, 0, LOCAL(0x0008u));
        CALL(0x359du, PR_REC_290B_SYSTEM_0B25, 0,
             LOCAL(0x0008u), WORD(2u));
    }

    if (far_word(io, 0x49d2u, 0x01f6u, &value) != PR_REC_290B_OK)
        return PR_REC_290B_BOUNDARY;
    if (value == 0u) {
        CALL(0x35b9u, PR_REC_290B_LOCAL_1BD7, 0,
             LOCAL(0x0131u), LOW(first));
        CALL(0x35c1u, PR_REC_290B_SYSTEM_0B98, 0, CODE(0x28f4u));
        CALL(0x35d1u, PR_REC_290B_LOCAL_1BD7, 0,
             LOCAL(0x0231u), LOW(second));
        if (invoke(io, 0x35d4u, PR_REC_290B_SYSTEM_0B98,
                   0, 0, 0) != PR_REC_290B_OK)
            return PR_REC_290B_BOUNDARY;
        CALL(0x35deu, PR_REC_290B_SYSTEM_0B98, 0, CODE(0x28f6u));
        CALL(0x35ecu, PR_REC_290B_SYSTEM_0B25, 0,
             LOCAL(0x0031u), WORD(0x28u));
    }

    CALL(0x35fcu, PR_REC_290B_SYSTEM_0B0B, 0,
         LOCAL(0x0131u), LOCAL(0x0031u));
    if (ds_byte(io, 0x6aafu, &index) != PR_REC_290B_OK)
        return PR_REC_290B_BOUNDARY;
    CALL(0x3614u, PR_REC_290B_SYSTEM_0B98, 0,
         DATA((uint16_t)(0x25a8u + 3u * index)));
    CALL(0x361eu, PR_REC_290B_SYSTEM_0B98, 0, CODE(0x28f6u));
    CALL(0x362cu, PR_REC_290B_SYSTEM_0B25, 0,
         LOCAL(0x0031u), WORD(0x28u));
    CALL(0x363cu, PR_REC_290B_SYSTEM_0B0B, 0,
         LOCAL(0x0131u), LOCAL(0x0031u));
    CALL(0x3646u, PR_REC_290B_SYSTEM_0B98, 0, LOCAL(0x0005u));
    CALL(0x3650u, PR_REC_290B_SYSTEM_0B98, 0, CODE(0x28f8u));
    CALL(0x365au, PR_REC_290B_SYSTEM_0B98, 0, LOCAL(0x0008u));
    CALL(0x3668u, PR_REC_290B_SYSTEM_0B25, 0,
         LOCAL(0x0031u), WORD(0x28u));

    if (ds_byte(io, 0x49f3u, &mode) != PR_REC_290B_OK)
        return PR_REC_290B_BOUNDARY;
    if (mode == 2u) {
        CALL(0x367fu, PR_REC_290B_SYSTEM_0B0B, 0,
             LOCAL(0x0131u), LOCAL(0x0031u));
        CALL(0x3689u, PR_REC_290B_SYSTEM_0B98, 0, CODE(0x28fau));
        CALL(0x3697u, PR_REC_290B_SYSTEM_0B25, 0,
             LOCAL(0x0031u), WORD(0x28u));
    } else {
        if (far_word(io, 0x49d6u, 0x0594u, &flags) != PR_REC_290B_OK)
            return PR_REC_290B_BOUNDARY;
        if ((flags & 1u) != 0u) {
            CALL(0x36b9u, PR_REC_290B_SYSTEM_0B0B, 0,
                 LOCAL(0x0131u), LOCAL(0x0031u));
            CALL(0x36c3u, PR_REC_290B_SYSTEM_0B98, 0, CODE(0x2903u));
            CALL(0x36d1u, PR_REC_290B_SYSTEM_0B25, 0,
                 LOCAL(0x0031u), WORD(0x28u));
        }
    }
    CALL(0x36e2u, PR_REC_290B_RES_03F1_0137, 0,
         LOW(0x11u), LOW(0x0fu), LOW(0x26u), LOW(0x0fu));
    CALL(0x36f5u, PR_REC_290B_RES_0709_0626, 0,
         LOW(0x11u), LOW(0x0fu), LOW(0x0au), LOCAL(0x0031u));
    return PR_REC_290B_OK;
}

#undef WORD
#undef LOW
#undef LOCAL
#undef CODE
#undef DATA
#undef CALL
