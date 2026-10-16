#ifndef POOLRAD_REPLAY_COLD_START_TITLE_RGB_H
#define POOLRAD_REPLAY_COLD_START_TITLE_RGB_H

#include <stddef.h>
#include <stdint.h>

#define PR_COLD_TITLE_WIDTH 640u
#define PR_COLD_TITLE_HEIGHT 400u
#define PR_COLD_TITLE_RGB24_BYTES \
    (PR_COLD_TITLE_WIDTH * PR_COLD_TITLE_HEIGHT * 3u)

typedef enum {
    PR_COLD_TITLE_OK = 0,
    PR_COLD_TITLE_ARGUMENT,
    PR_COLD_TITLE_ARCHIVE,
    PR_COLD_TITLE_BLOCK,
    PR_COLD_TITLE_IMAGE,
    PR_COLD_TITLE_OUTPUT_SIZE,
    PR_COLD_TITLE_MEMORY
} pr_cold_title_status;

/* Decode TITLE.DAX block ID 2's single 320x200 EGA item and render it with
 * the standard 16-colour EGA RGB palette. Pixels are doubled horizontally
 * and vertically into top-down, tightly packed 640x400 RGB24. The caller
 * retains the archive and output buffers. On OUTPUT_SIZE, written reports
 * PR_COLD_TITLE_RGB24_BYTES and no output byte is changed. */
pr_cold_title_status pr_cold_title_rgb24(const uint8_t *title_dax,
                                         size_t title_dax_size,
                                         uint8_t *rgb24, size_t capacity,
                                         size_t *written);
const char *pr_cold_title_status_string(pr_cold_title_status status);

#endif
