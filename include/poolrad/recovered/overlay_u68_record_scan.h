#ifndef POOLRAD_RECOVERED_OVERLAY_U68_RECORD_SCAN_H
#define POOLRAD_RECOVERED_OVERLAY_U68_RECORD_SCAN_H

/* DOS v1.3 GAME.OVR unit 0x68 internal +0x05AB..+0x05ED.
 * The 67-byte, call-free body scans 21 bytes at ES:(record_offset+0x17)..
 * ES:(record_offset+0x2B), with 16-bit offset wrap, for the low byte of
 * value_word. It returns AX=0x0000..0x0014 for the first hit, or AX=0x00FF.
 * The original accepts a far record pointer and a word argument, reads no
 * other state, writes only its stack frame, and ends with retf 6.
 *
 * The host view covers the full 64 KiB DOS segment so every wrapped offset
 * is representable. A missing/short view or null result is rejected before
 * any read or result write. See docs/re/U68_RECORD_SCAN_05AB.md. */

#include <stddef.h>
#include <stdint.h>

#define PR_U68_RECORD_SEGMENT_SIZE 65536u
#define PR_U68_RECORD_SCAN_COUNT 21u
#define PR_U68_RECORD_SCAN_START 0x17u
#define PR_U68_RECORD_SCAN_MISS 0x00FFu

typedef enum {
    PR_U68_RECORD_SCAN_OK = 0,
    PR_U68_RECORD_SCAN_ARGUMENT
} pr_u68_record_scan_status;

pr_u68_record_scan_status pr_u68_record_scan(const uint8_t *segment,
                                             size_t segment_size,
                                             uint16_t record_offset,
                                             uint16_t value_word,
                                             uint16_t *ax_out);

#endif
