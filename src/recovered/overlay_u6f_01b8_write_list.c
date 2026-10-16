#include "poolrad/recovered/overlay_u6f_01b8_write_list.h"

/* Pascal string helpers: buffers hold a length byte then up to 255 bytes. */
static int pascal_set(uint8_t *dst, const uint8_t *src, size_t src_size) {
    size_t len;
    if (src_size == 0u) {
        return -1;
    }
    len = src[0];
    if (len + 1u > src_size) {
        return -1;                                              /* the length byte overruns its span */
    }
    dst[0] = (uint8_t)len;
    for (size_t i = 0; i < len; ++i) {
        dst[1u + i] = src[1u + i];
    }
    return 0;
}

static int pascal_append(uint8_t *dst, const uint8_t *src, size_t count) {
    size_t len = dst[0];
    if (len + count > 255u) {
        return -1;                                              /* the runtime would truncate at 255 */
    }
    for (size_t i = 0; i < count; ++i) {
        dst[1u + len + i] = src[i];
    }
    dst[0] = (uint8_t)(len + count);
    return 0;
}

pr_u6f_01b8_status pr_u6f_01b8_write_list(
    const pr_u6f_01b8_node *list,
    const uint8_t *live_path, size_t live_path_size,
    const uint8_t *file_name, size_t file_name_size,
    const uint8_t *message_prefix, size_t prefix_size,
    const uint8_t *message_suffix, size_t suffix_size,
    const pr_u6f_01b8_hooks *hooks,
    size_t max_open_attempts, size_t max_nodes,
    pr_u6f_01b8_result *out) {
    uint8_t path[256];                                          /* SS:BP-0x206 */
    uint8_t message[256];                                       /* SS:BP-0x306 */
    uint16_t io = 0u;                                           /* SS:BP-0x106 */
    const pr_u6f_01b8_node *node;                               /* SS:BP-4 */
    pr_u6f_01b8_result r = {0u, 0u, 0u, 0u, 0u, 0u, 0u, 0x01B8u};
    if (out != NULL) {
        *out = r;
    }
    if (out == NULL || hooks == NULL || live_path == NULL || live_path_size == 0u ||
        file_name == NULL || file_name_size != PR_U6F_01B8_FILE_NAME_LENGTH ||
        message_prefix == NULL || prefix_size != PR_U6F_01B8_MESSAGE_PREFIX_LENGTH ||
        message_suffix == NULL || suffix_size != PR_U6F_01B8_MESSAGE_SUFFIX_LENGTH) {
        return PR_U6F_01B8_ARGUMENT;
    }
    if (live_path_size < 2u || (size_t)live_path[0] + 1u > live_path_size) {
        return PR_U6F_01B8_ARGUMENT;                            /* the byte after the length (DS:0x537B) and the
                                                                 * whole string must lie inside the supplied span */
    }
    /* +01BF: push cs; call +02AE */
    r.next_offset = 0x01BFu;
    if (hooks->ensure_directory == NULL) { *out = r; return PR_U6F_01B8_NEED_HOOK; }
    if (hooks->ensure_directory(hooks->context) != 0) { *out = r; return PR_U6F_01B8_HOST_FAILURE; }
    for (;;) {
        /* +01C3..+01DE: path := DS:0x537A + CS:0x0197 (StrLoad, StrCat) */
        r.next_offset = 0x01C3u;
        if (pascal_set(path, live_path, live_path_size) != 0) { *out = r; return PR_U6F_01B8_ARGUMENT; }
        if (pascal_append(path, file_name, PR_U6F_01B8_FILE_NAME_LENGTH) != 0) { *out = r; return PR_U6F_01B8_BOUNDS; }
        if (r.open_attempts >= max_open_attempts) { *out = r; return PR_U6F_01B8_RETRY_LIMIT; }
        r.open_attempts++;
        /* +01E3: 0xAF8:0x04F7 text-record initialization with the path */
        r.next_offset = 0x01E3u;
        if (hooks->text_assign == NULL) { *out = r; return PR_U6F_01B8_NEED_HOOK; }
        if (hooks->text_assign(hooks->context, path, (size_t)path[0] + 1u) != 0) { *out = r; return PR_U6F_01B8_HOST_FAILURE; }
        /* +01EE: 0xAF8:0x056C open for output */
        r.next_offset = 0x01EEu;
        if (hooks->text_rewrite == NULL) { *out = r; return PR_U6F_01B8_NEED_HOOK; }
        if (hooks->text_rewrite(hooks->context) != 0) { *out = r; return PR_U6F_01B8_HOST_FAILURE; }
        /* +01F3..+01F8: IOResult into SS:BP-0x106 */
        r.next_offset = 0x01F3u;
        if (hooks->io_result == NULL) { *out = r; return PR_U6F_01B8_NEED_HOOK; }
        if (hooks->io_result(hooks->context, &io) != 0) { *out = r; return PR_U6F_01B8_HOST_FAILURE; }
        r.last_io_result = io;
        /* +01FC..+0201: zero skips the message */
        if (io != 0u) {
            /* +0203..+022F: message := CS:0x01A4 + Chr(DS:0x537B) + CS:0x01B6; color AL = 0x0E */
            r.next_offset = 0x0203u;
            message[0] = 0u;
            {
                uint8_t first = live_path[1];                   /* DS:0x537B: the byte after the length */
                if (pascal_append(message, message_prefix, PR_U6F_01B8_MESSAGE_PREFIX_LENGTH) != 0 ||
                    pascal_append(message, &first, 1u) != 0 ||
                    pascal_append(message, message_suffix, PR_U6F_01B8_MESSAGE_SUFFIX_LENGTH) != 0) {
                    *out = r; return PR_U6F_01B8_BOUNDS;
                }
            }
            /* +0234: resident 0x709:0x0BF5 (clear row, print, wait for a key) */
            r.next_offset = 0x0234u;
            if (hooks->display_and_input == NULL) { *out = r; return PR_U6F_01B8_NEED_HOOK; }
            r.displays++;
            if (hooks->display_and_input(hooks->context, message, (size_t)message[0] + 1u, PR_U6F_01B8_COLOR) != 0) { *out = r; return PR_U6F_01B8_HOST_FAILURE; }
        }
        /* +0239..+023E: re-read the live IOResult; non-zero retries from +01C3 */
        if (io == 0u) {
            break;
        }
    }
    /* +0240..+0249: node := argument */
    node = list;
    r.next_offset = 0x024Cu;
    for (;;) {
        /* +024C..+0252: nil ends the walk */
        if (node == NULL) {
            break;
        }
        if (r.nodes_visited >= max_nodes) { *out = r; return PR_U6F_01B8_NODE_LIMIT; }
        r.nodes_visited++;
        if (node->pascal_name == NULL || node->name_size < 2u) { *out = r; return PR_U6F_01B8_ARGUMENT; }
        /* +0254..+025C: the byte at node+1 (first character) against 0x2A, whatever the length byte */
        if (node->pascal_name[1] == PR_U6F_01B8_SKIP_CHARACTER) {
            r.nodes_skipped++;
        } else {
            /* +025E..+0271: 0x0919 (string, width 0) then 0x084A (end of line) */
            r.next_offset = 0x025Eu;
            if ((size_t)node->pascal_name[0] + 1u > node->name_size) { *out = r; return PR_U6F_01B8_ARGUMENT; }   /* length + 1 bytes are forwarded */
            if (hooks->write_line == NULL) { *out = r; return PR_U6F_01B8_NEED_HOOK; }
            if (hooks->write_line(hooks->context, node->pascal_name, (size_t)node->pascal_name[0] + 1u) != 0) { *out = r; return PR_U6F_01B8_HOST_FAILURE; }
            r.lines_written++;
        }
        /* +0276..+0287: node := the far pointer at node+0x2A; jump +024C */
        node = node->next;
        r.next_offset = 0x024Cu;
    }
    /* +0289..+028F: 0x05C1 close */
    r.next_offset = 0x0289u;
    if (hooks->text_close == NULL) { *out = r; return PR_U6F_01B8_NEED_HOOK; }
    if (hooks->text_close(hooks->context) != 0) { *out = r; return PR_U6F_01B8_HOST_FAILURE; }
    r.closed = 1u;
    r.next_offset = PR_U6F_01B8_END_OFFSET;                     /* +0294..+0297: mov sp,bp; pop bp; retf 4 */
    *out = r;
    return PR_U6F_01B8_OK;
}
