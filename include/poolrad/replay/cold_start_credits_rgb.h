#ifndef POOLRAD_REPLAY_COLD_START_CREDITS_RGB_H
#define POOLRAD_REPLAY_COLD_START_CREDITS_RGB_H

#include <stddef.h>
#include <stdint.h>

#define PR_COLD_CREDITS_WIDTH 640u
#define PR_COLD_CREDITS_HEIGHT 400u
#define PR_COLD_CREDITS_RGB24_BYTES \
    (PR_COLD_CREDITS_WIDTH * PR_COLD_CREDITS_HEIGHT * 3u)

typedef enum {
    PR_COLD_CREDITS_OK = 0,
    PR_COLD_CREDITS_ARGUMENT,
    PR_COLD_CREDITS_ARCHIVE,
    PR_COLD_CREDITS_FONT,
    PR_COLD_CREDITS_TILES,
    PR_COLD_CREDITS_SCRIPT,
    PR_COLD_CREDITS_OUTPUT_SIZE,
    PR_COLD_CREDITS_MEMORY
} pr_cold_credits_status;

/* Source-level model of the DOS v1.3 unit-0x17 credits frame. Reads font
 * block 201, border block 202, and the 33 fixed-form printer call sites from
 * the original GAME.OVR image. No original text, glyph, or tile is embedded.
 * The 320x200 EGA index frame is doubled to top-down 640x400 RGB24. This is
 * a synthetic frame composer, not a replay of the startup call graph.
 * On OUTPUT_SIZE, written reports the required size and rgb24 is unchanged. */
pr_cold_credits_status pr_cold_credits_rgb24(
    const uint8_t *dax, size_t dax_size, const uint8_t *ovr, size_t ovr_size,
    uint8_t *rgb24, size_t capacity, size_t *written);

const char *pr_cold_credits_status_string(pr_cold_credits_status status);

#endif
