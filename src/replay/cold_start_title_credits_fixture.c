#include "poolrad/replay/cold_start_title_credits_fixture.h"

#include "poolrad/replay/cold_start_credits_rgb.h"
#include "poolrad/replay/cold_start_title_rgb.h"

#include <stdlib.h>
#include <string.h>

pr_cold_title_credits_status pr_cold_title_credits_fixture(
    const uint8_t *title_dax, size_t title_dax_size,
    const uint8_t *credits_dax, size_t credits_dax_size,
    const uint8_t *ovr, size_t ovr_size,
    uint8_t *frames_rgb24, size_t capacity,
    size_t *written, size_t *boundary_frame) {
    uint8_t *staged;
    size_t frame_size = 0u;
    pr_cold_title_credits_status result = PR_COLD_TITLE_CREDITS_OK;

    if (!written || !boundary_frame) return PR_COLD_TITLE_CREDITS_ARGUMENT;
    *written = 0u;
    *boundary_frame = 0u;
    if (!title_dax || !credits_dax || !ovr)
        return PR_COLD_TITLE_CREDITS_ARGUMENT;
    if (!frames_rgb24 || capacity < PR_COLD_TITLE_CREDITS_BYTES) {
        *written = PR_COLD_TITLE_CREDITS_BYTES;
        return PR_COLD_TITLE_CREDITS_OUTPUT_SIZE;
    }
    staged = malloc(PR_COLD_TITLE_CREDITS_BYTES);
    if (!staged) return PR_COLD_TITLE_CREDITS_MEMORY;
    if (pr_cold_title_rgb24(title_dax, title_dax_size, staged,
                            PR_COLD_TITLE_CREDITS_FRAME_BYTES,
                            &frame_size) != PR_COLD_TITLE_OK ||
        frame_size != PR_COLD_TITLE_CREDITS_FRAME_BYTES) {
        result = PR_COLD_TITLE_CREDITS_TITLE;
        goto done;
    }
    if (pr_cold_credits_rgb24(credits_dax, credits_dax_size, ovr,
                              ovr_size,
                              staged + PR_COLD_TITLE_CREDITS_FRAME_BYTES,
                              PR_COLD_TITLE_CREDITS_FRAME_BYTES,
                              &frame_size) != PR_COLD_CREDITS_OK ||
        frame_size != PR_COLD_TITLE_CREDITS_FRAME_BYTES) {
        result = PR_COLD_TITLE_CREDITS_CREDITS;
        goto done;
    }
    memcpy(frames_rgb24, staged, PR_COLD_TITLE_CREDITS_BYTES);
    *written = PR_COLD_TITLE_CREDITS_BYTES;
    *boundary_frame = 1u;
done:
    free(staged);
    return result;
}

const char *pr_cold_title_credits_status_string(
    pr_cold_title_credits_status status) {
    switch (status) {
    case PR_COLD_TITLE_CREDITS_OK: return "ok";
    case PR_COLD_TITLE_CREDITS_ARGUMENT: return "invalid argument";
    case PR_COLD_TITLE_CREDITS_OUTPUT_SIZE: return "RGB24 sequence buffer too small";
    case PR_COLD_TITLE_CREDITS_MEMORY: return "allocation failure";
    case PR_COLD_TITLE_CREDITS_TITLE: return "title frame failed";
    case PR_COLD_TITLE_CREDITS_CREDITS: return "credits frame failed";
    default: return "unknown error";
    }
}
