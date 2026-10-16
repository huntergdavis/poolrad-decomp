#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_01B8_WRITE_LIST_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_01B8_WRITE_LIST_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x6F + 0x01B8: public entry 2 (run-time stub 0x7C:0x2A),
 * executable [01B8,029A), 226 bytes, 84 instructions, 14 call sites, one
 * far argument (a list head) consumed by `retf 4`. Evidence and offsets:
 * docs/re/U6F_01B8_WRITE_LIST.md. Semantics, instruction by instruction:
 *   +01BF  near call +02AE (the directory-ensuring routine, no arguments);
 *   +01C3  path := the live Pascal string DS:0x537A followed by the
 *          12-character constant at CS:0x0197 (0xAF8:0x0B0B, 0x0B98);
 *          0xAF8:0x04F7 binds the Text record at SS:BP-0x104 to that name;
 *          0xAF8:0x056C opens it for output; 0xAF8:0x04A2 reads IOResult
 *          into SS:BP-0x106;
 *   +01FC  a non-zero IOResult builds the message CS:0x01A4 (17 chars) +
 *          Chr(DS:0x537B) + CS:0x01B6 (1 char) and calls resident
 *          0x709:0x0BF5 with (color word AL=0x0E, message); then +0239
 *          re-reads the live IOResult and retries from +01C3 while non-zero;
 *   +0240  node := the argument; while node <> nil: if the byte at node+1
 *          (the first character of the Pascal string at node+0) is not 0x2A,
 *          0xAF8:0x0919 writes the string (width 0) and 0xAF8:0x084A ends
 *          the line; node := the far pointer at node+0x2A;
 *   +0289  0xAF8:0x05C1 closes the record; frame restore; retf 4.
 * The runtime text-file services, the resident prompt and +02AE are the
 * boundaries of this model and are supplied as callbacks; the private CS
 * literals are supplied by the caller (as in overlay_u6f_postinsert_02ae).
 * The engine's two loops are unbounded (a retry loop and a list walk that
 * never terminates on a cycle); host guards bound them and report
 * RETRY_LIMIT or NODE_LIMIT, which the engine cannot return. */

typedef struct pr_u6f_01b8_node {
    const uint8_t *pascal_name;              /* length byte then characters, as at node+0 */
    size_t name_size;                        /* supplied span of pascal_name: the engine reads
                                              * byte 1 whatever the length byte says (>= 2 needed)
                                              * and writes length + 1 bytes (>= length + 1 needed) */
    const struct pr_u6f_01b8_node *next;     /* the far pointer at node+0x2A (nil = NULL) */
} pr_u6f_01b8_node;

typedef struct {
    void *context;
    int (*ensure_directory)(void *context);                               /* +02AE */
    int (*text_assign)(void *context, const uint8_t *pascal_path, size_t span);   /* 0x04F7 */
    int (*text_rewrite)(void *context);                                   /* 0x056C */
    int (*io_result)(void *context, uint16_t *result);                    /* 0x04A2 */
    int (*display_and_input)(void *context, const uint8_t *pascal_message,
                             size_t span, uint8_t color);                 /* 0x709:0x0BF5 */
    int (*write_line)(void *context, const uint8_t *pascal_string, size_t span); /* 0x0919 + 0x084A */
    int (*text_close)(void *context);                                     /* 0x05C1 */
} pr_u6f_01b8_hooks;

typedef struct {
    size_t open_attempts;      /* assign+rewrite+ioresult rounds, including failures */
    size_t displays;           /* resident prompt calls */
    size_t nodes_visited;
    size_t lines_written;
    size_t nodes_skipped;      /* first character 0x2A */
    uint16_t last_io_result;
    uint8_t closed;            /* the close service was reached */
    uint16_t next_offset;      /* source offset the model stopped at, or 0x029A after retf */
} pr_u6f_01b8_result;

typedef enum {
    PR_U6F_01B8_OK = 0,
    PR_U6F_01B8_ARGUMENT,
    PR_U6F_01B8_BOUNDS,          /* path or message would exceed 255 characters */
    PR_U6F_01B8_NEED_HOOK,       /* a reached boundary has no callback */
    PR_U6F_01B8_HOST_FAILURE,    /* a callback returned non-zero */
    PR_U6F_01B8_RETRY_LIMIT,     /* host guard on the open retry loop */
    PR_U6F_01B8_NODE_LIMIT       /* host guard on the list walk */
} pr_u6f_01b8_status;

/* live_path: the Pascal string at DS:0x537A (length byte first), read for the
 * path and, on the failed-open message, for the raw byte after the length
 * (DS:0x537B, read even when the length is 0): the supplied span must hold
 * that byte (live_path_size >= 2) and the whole string (>= length + 1).
 * A node whose supplied name span is too short for the byte-1 read or for
 * the write reports ARGUMENT at that node, after the earlier nodes' effects.
 * file_name: the
 * 12-character CS:0x0197 constant; message_prefix / message_suffix: the
 * CS:0x01A4 (17) and CS:0x01B6 (1) constants. Lengths are checked against
 * the originals' lengths. max_open_attempts and max_nodes are host guards
 * (0 means unlimited is NOT offered: 0 reports the limit at once). */
pr_u6f_01b8_status pr_u6f_01b8_write_list(
    const pr_u6f_01b8_node *list,
    const uint8_t *live_path, size_t live_path_size,
    const uint8_t *file_name, size_t file_name_size,
    const uint8_t *message_prefix, size_t prefix_size,
    const uint8_t *message_suffix, size_t suffix_size,
    const pr_u6f_01b8_hooks *hooks,
    size_t max_open_attempts, size_t max_nodes,
    pr_u6f_01b8_result *out);

#define PR_U6F_01B8_FILE_NAME_LENGTH 12u
#define PR_U6F_01B8_MESSAGE_PREFIX_LENGTH 17u
#define PR_U6F_01B8_MESSAGE_SUFFIX_LENGTH 1u
#define PR_U6F_01B8_COLOR 0x0Eu
#define PR_U6F_01B8_SKIP_CHARACTER 0x2Au
#define PR_U6F_01B8_NEXT_OFFSET 0x2Au
#define PR_U6F_01B8_RETF_OFFSET 0x0297u
#define PR_U6F_01B8_END_OFFSET 0x029Au

#endif
