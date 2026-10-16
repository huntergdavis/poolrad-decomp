#ifndef POOLRAD_REPLAY_COLD_START_PUBLISHER_RGB_H
#define POOLRAD_REPLAY_COLD_START_PUBLISHER_RGB_H

#include <stddef.h>
#include <stdint.h>

#define PR_COLD_PUBLISHER_WIDTH 640u
#define PR_COLD_PUBLISHER_HEIGHT 400u
#define PR_COLD_PUBLISHER_RGB24_BYTES \
    (PR_COLD_PUBLISHER_WIDTH * PR_COLD_PUBLISHER_HEIGHT * 3u)

typedef enum {
    PR_COLD_PUBLISHER_OK = 0,
    PR_COLD_PUBLISHER_ARGUMENT,
    PR_COLD_PUBLISHER_ARCHIVE,
    PR_COLD_PUBLISHER_BLOCK,
    PR_COLD_PUBLISHER_IMAGE,
    PR_COLD_PUBLISHER_OUTPUT_SIZE,
    PR_COLD_PUBLISHER_MEMORY
} pr_cold_publisher_status;

/* Decode TITLE.DAX block ID 1's single 320x200 EGA item with the standard
 * EGA RGB palette. The result is top-down, tightly packed 640x400 RGB24.
 * On OUTPUT_SIZE, written reports PR_COLD_PUBLISHER_RGB24_BYTES and no
 * output byte changes. Other errors set written to zero when supplied. */
pr_cold_publisher_status pr_cold_publisher_rgb24(const uint8_t *title_dax,
                                                 size_t title_dax_size,
                                                 uint8_t *rgb24,
                                                 size_t capacity,
                                                 size_t *written);
const char *pr_cold_publisher_status_string(pr_cold_publisher_status status);

#endif
