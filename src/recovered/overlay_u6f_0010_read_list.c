#include "poolrad/recovered/overlay_u6f_0010_read_list.h"

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

/* 0xAF8:0x0C10 as the PM decoded it: equal lengths and equal bytes. */
static int pascal_equal(const uint8_t *a, const uint8_t *b) {
    if (a[0] != b[0]) {
        return 0;
    }
    for (size_t i = 1; i <= a[0]; ++i) {
        if (a[i] != b[i]) {
            return 0;
        }
    }
    return 1;
}

/* 0xAF8:0x16A8 FillChar(node^, 0x2E, 0) over the model's node. */
static void node_clear(pr_u6f_0010_node *node) {
    for (size_t i = 0; i < PR_U6F_0010_NAME_BYTES; ++i) {
        node->name[i] = 0u;
    }
    node->reserved = 0u;
    node->next = NULL;
}

pr_u6f_0010_status pr_u6f_0010_read_list(
    pr_u6f_0010_node **head,
    const uint8_t *live_path, size_t live_path_size,
    const uint8_t *file_name, size_t file_name_size,
    const uint8_t *marker, size_t marker_size,
    const pr_u6f_0010_party *party,
    const pr_u6f_0010_hooks *hooks,
    size_t max_nodes, size_t max_party_steps,
    pr_u6f_0010_result *out) {
    uint8_t temp[256];                                          /* SS:BP-0x20C: the path, later the marked copy */
    pr_u6f_0010_node *tail = NULL;                              /* SS:BP-8 */
    pr_u6f_0010_node *published = NULL;                         /* what this call last stored through head */
    pr_u6f_0010_result r = {0u, 0u, 0u, 0u, 0u, 0x0010u};
    if (out != NULL) {
        *out = r;
    }
    if (out == NULL || head == NULL || hooks == NULL || live_path == NULL || live_path_size == 0u ||
        file_name == NULL || file_name_size != PR_U6F_0010_FILE_NAME_LENGTH ||
        marker == NULL || marker_size != PR_U6F_0010_MARKER_LENGTH) {
        return PR_U6F_0010_ARGUMENT;
    }
    if ((size_t)live_path[0] + 1u > live_path_size) {
        return PR_U6F_0010_ARGUMENT;                            /* the whole live string must lie inside the span */
    }
    /* +0017..+001F: head^ := nil */
    *head = NULL;
    /* +0023: push cs; +0024 call +02AE */
    r.next_offset = 0x0024u;
    if (hooks->ensure_directory == NULL) { *out = r; return PR_U6F_0010_NEED_HOOK; }
    if (hooks->ensure_directory(hooks->context) != 0) { *out = r; return PR_U6F_0010_HOST_FAILURE; }
    /* +0027..+0040: temp := DS:0x537A + CS:0x0000 (StrLoad, StrCat) */
    r.next_offset = 0x0027u;
    if (pascal_set(temp, live_path, live_path_size) != 0) { *out = r; return PR_U6F_0010_ARGUMENT; }
    if (pascal_append(temp, file_name, PR_U6F_0010_FILE_NAME_LENGTH) != 0) { *out = r; return PR_U6F_0010_BOUNDS; }
    /* +0041: resident 0x802:0x02EB with the retained temp; +0046..+004A: AL = 0 jumps to +0191 */
    r.next_offset = 0x0041u;
    if (hooks->select_file == NULL) { *out = r; return PR_U6F_0010_NEED_HOOK; }
    {
        uint8_t al = 0u;
        if (hooks->select_file(hooks->context, temp, (size_t)temp[0] + 1u, &al) != 0) { *out = r; return PR_U6F_0010_HOST_FAILURE; }
        if (al == 0u) {
            r.next_offset = PR_U6F_0010_END_OFFSET;             /* +0191..+0194: frame restore; retf 4; nothing opened */
            *out = r;
            return PR_U6F_0010_OK;
        }
        r.selected = 1u;
    }
    /* +004D..+006D: the path is rebuilt from the same two strings (unchanged by the
     * selector) and 0xAF8:0x04F7 binds the Text record at SS:BP-0x10C to it */
    r.next_offset = 0x006Du;
    if (hooks->text_assign == NULL) { *out = r; return PR_U6F_0010_NEED_HOOK; }
    if (hooks->text_assign(hooks->context, temp, (size_t)temp[0] + 1u) != 0) { *out = r; return PR_U6F_0010_HOST_FAILURE; }
    /* +0072..+0078: 0xAF8:0x0567 open for input (mode 0xD7B1) */
    r.next_offset = 0x0078u;
    if (hooks->text_reset == NULL) { *out = r; return PR_U6F_0010_NEED_HOOK; }
    if (hooks->text_reset(hooks->context) != 0) { *out = r; return PR_U6F_0010_HOST_FAILURE; }
    for (;;) {
        pr_u6f_0010_node *node;                                 /* SS:BP-4 */
        const pr_u6f_0010_party *p;                             /* SS:BP-0xC */
        size_t steps;
        /* +007D..+008C: Eof(f); AL <> 0 jumps to +0186 */
        r.next_offset = 0x0083u;
        if (hooks->eof == NULL) { *out = r; return PR_U6F_0010_NEED_HOOK; }
        {
            uint8_t al = 0u;
            if (hooks->eof(hooks->context, &al) != 0) { *out = r; return PR_U6F_0010_HOST_FAILURE; }
            if (al != 0u) {
                break;
            }
        }
        if (r.nodes >= max_nodes) { *out = r; return PR_U6F_0010_LINE_LIMIT; }
        /* +008F..+009B: node := GetMem(0x2E) */
        r.next_offset = 0x0093u;
        if (hooks->alloc_node == NULL) { *out = r; return PR_U6F_0010_NEED_HOOK; }
        node = NULL;
        if (hooks->alloc_node(hooks->context, &node) != 0) { *out = r; return PR_U6F_0010_HOST_FAILURE; }
        if (node == NULL) { *out = r; return PR_U6F_0010_ARGUMENT; }   /* the engine never tests GetMem's result */
        /* +009E..+00AA: FillChar(node^, 0x2E, 0) */
        node_clear(node);
        r.nodes++;
        /* +00AF..+00FC: the engine rereads head^ here: an empty list takes the node as
         * head and tail; otherwise tail^.next := node and tail := tail^.next. The
         * engine's tail is only valid for the head this call published, so a head
         * changed by any callback (before or after the first link) fails closed. */
        r.next_offset = 0x00AFu;
        if (*head != published) { *out = r; return PR_U6F_0010_ARGUMENT; }   /* host contract: a callback moved *head */
        if (*head == NULL) {
            *head = node;
            tail = *head;
        } else {
            tail->next = node;
            tail = tail->next;
        }
        published = *head;
        /* +00FF..+0113: ReadLn(f, tail^.name) with at most 0x28 characters: 0x08E1
         * stores the characters and the length byte; 0x081C ends the line */
        r.next_offset = 0x010Eu;
        if (hooks->read_line == NULL) { *out = r; return PR_U6F_0010_NEED_HOOK; }
        {
            size_t count = 0u;
            if (hooks->read_line(hooks->context, tail->name + 1, PR_U6F_0010_NAME_LIMIT, &count) != 0) { *out = r; return PR_U6F_0010_HOST_FAILURE; }
            if (count > PR_U6F_0010_NAME_LIMIT) { *out = r; return PR_U6F_0010_ARGUMENT; }   /* host contract: the service never stores more */
            tail->name[0] = (uint8_t)count;
        }
        /* +0118..+0151: p := DS:0x5D96/0x5D98; while p <> nil: equal names break; p := p^+0x104 */
        r.next_offset = 0x0125u;
        p = party;
        steps = 0u;
        while (p != NULL) {
            if (steps >= max_party_steps) { *out = r; return PR_U6F_0010_PARTY_LIMIT; }
            steps++;
            r.party_steps++;
            if (p->pascal_name == NULL || p->name_size == 0u || (size_t)p->pascal_name[0] + 1u > p->name_size) {
                r.next_offset = 0x0137u;
                *out = r;
                return PR_U6F_0010_ARGUMENT;                    /* the compare reads the whole record name */
            }
            if (pascal_equal(p->pascal_name, tail->name)) {
                break;
            }
            p = p->next;
        }
        /* +0153..+0159: nil skips the marking */
        if (p != NULL) {
            /* +015B..+017E: temp := CS:0x000D + tail^.name; tail^.name := Copy(temp, 1, 0x28) */
            r.next_offset = 0x015Bu;
            temp[0] = 0u;
            (void)pascal_append(temp, marker, PR_U6F_0010_MARKER_LENGTH);    /* 2 + at most 40: never past 255 */
            (void)pascal_append(temp, tail->name + 1, tail->name[0]);
            {
                size_t keep = temp[0] > PR_U6F_0010_NAME_LIMIT ? PR_U6F_0010_NAME_LIMIT : temp[0];
                for (size_t i = 0; i < keep; ++i) {
                    tail->name[1u + i] = temp[1u + i];
                }
                tail->name[0] = (uint8_t)keep;
            }
            r.marked++;
        }
        /* +0183: jmp +007D */
        r.next_offset = 0x007Du;
    }
    /* +0186..+018C: 0x05C1 close */
    r.next_offset = 0x018Cu;
    if (hooks->text_close == NULL) { *out = r; return PR_U6F_0010_NEED_HOOK; }
    if (hooks->text_close(hooks->context) != 0) { *out = r; return PR_U6F_0010_HOST_FAILURE; }
    r.closed = 1u;
    r.next_offset = PR_U6F_0010_END_OFFSET;                     /* +0191..+0194: mov sp,bp; pop bp; retf 4 */
    *out = r;
    return PR_U6F_0010_OK;
}
