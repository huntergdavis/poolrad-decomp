#ifndef POOLRAD_RECOVERED_OVERLAY_U68_MAP_UPDATE_H
#define POOLRAD_RECOVERED_OVERLAY_U68_MAP_UPDATE_H

/* DOS v1.3 GAME.OVR unit 0x68 internal +0x0109..+0x0235.
 * The three Pascal word arguments are consumed as low bytes:
 * x=[bp+0xA], y=[bp+8], selector=[bp+6]. In-range signed-byte x/y
 * and selectors 6/4/2/0 update one two-bit field in ES:[map base +
 * 0x300 + 16*y + x]. The source of the far map pointer is DS:[0x6A5C].
 * The caller supplies its offset and a full 64 KiB ES segment view.
 * Invalid coordinates/selectors return OK without reading that view;
 * a reached missing/short view returns ARGUMENT before any write.
 * See docs/re/U68_MAP_UPDATE_0109.md. */

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PR_U68_MAP_UPDATE_OK = 0,
    PR_U68_MAP_UPDATE_ARGUMENT
} pr_u68_map_update_status;

pr_u68_map_update_status pr_u68_map_update(uint8_t x, uint8_t y,
                                           uint8_t selector,
                                           uint16_t map_base_offset,
                                           uint8_t *map_segment,
                                           size_t map_segment_size);

#endif
