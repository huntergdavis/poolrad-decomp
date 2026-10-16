#ifndef POOLRAD_REPLAY_COLD_START_TITLE_CREDITS_FIXTURE_H
#define POOLRAD_REPLAY_COLD_START_TITLE_CREDITS_FIXTURE_H

#include <stddef.h>
#include <stdint.h>

#define PR_COLD_TITLE_CREDITS_FRAMES 2u
#define PR_COLD_TITLE_CREDITS_FRAME_BYTES (640u * 400u * 3u)
#define PR_COLD_TITLE_CREDITS_BYTES \
    (PR_COLD_TITLE_CREDITS_FRAMES * PR_COLD_TITLE_CREDITS_FRAME_BYTES)

typedef enum {
    PR_COLD_TITLE_CREDITS_OK = 0,
    PR_COLD_TITLE_CREDITS_ARGUMENT,
    PR_COLD_TITLE_CREDITS_OUTPUT_SIZE,
    PR_COLD_TITLE_CREDITS_MEMORY,
    PR_COLD_TITLE_CREDITS_TITLE,
    PR_COLD_TITLE_CREDITS_CREDITS
} pr_cold_title_credits_status;

/* Deterministic two-state visual fixture. Frame 0 is the merged synthetic
 * title renderer's RGB24 output; frame 1 is the merged synthetic credits
 * renderer's RGB24 output. The returned boundary index is 1. These frames
 * are adjacent by fixture design; no native duration, intermediate frame,
 * route reachability, or first-writer assertion follows from this API.
 * The caller owns all buffers. A failure leaves output untouched. */
pr_cold_title_credits_status pr_cold_title_credits_fixture(
    const uint8_t *title_dax, size_t title_dax_size,
    const uint8_t *credits_dax, size_t credits_dax_size,
    const uint8_t *ovr, size_t ovr_size,
    uint8_t *frames_rgb24, size_t capacity,
    size_t *written, size_t *boundary_frame);

const char *pr_cold_title_credits_status_string(
    pr_cold_title_credits_status status);

#endif
