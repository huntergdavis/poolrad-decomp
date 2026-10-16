#ifndef POOLRAD_RECOVERED_OVERLAY_U6F_0010_READ_LIST_H
#define POOLRAD_RECOVERED_OVERLAY_U6F_0010_READ_LIST_H

#include <stddef.h>
#include <stdint.h>

/* GAME.OVR unit 0x6F + 0x0010: public entry 1 (run-time stub 0x7C:0x25),
 * executable [0010,0197), 391 bytes, 147 instructions, 18 call sites, one
 * far argument (a var list head) consumed by `retf 4`. Evidence and offsets:
 * docs/re/U6F_0010_READ_LIST.md. It is the inverse of entry 2 (+01B8, the
 * writer): it reads the roster text file into a linked list of 46-byte nodes
 * and marks the names that belong to the current party. Semantics:
 *   +0017  head^ := nil (both words of the far argument);
 *   +0023  near call +02AE (the directory-ensuring routine, no arguments);
 *   +0027  path := the live Pascal string DS:0x537A followed by the
 *          12-character constant at CS:0x0000 (0xAF8:0x0B0B, 0x0B98);
 *   +0041  resident 0x802:0x02EB probes that path; AL = 0 returns at once
 *          (head stays nil, nothing opened, nothing closed);
 *   +004D  the same path is rebuilt; 0xAF8:0x04F7 binds the Text record at
 *          SS:BP-0x10C to it; 0xAF8:0x0567 opens it for input;
 *   +007D  while not Eof(f) (0xAF8:0x0A04, AL = 0):
 *            node := GetMem(0x2E) (0xAF8:0x023F); FillChar(node^, 0x2E, 0)
 *            (0xAF8:0x16A8); if head^ = nil then head^ := node, tail := head^
 *            else tail^.next := node, tail := tail^.next (far pointer at
 *            node+0x2A); ReadLn(f, tail^.name) with at most 0x28 characters
 *            (0xAF8:0x08E1 then 0x081C); p := the party head DS:0x5D96/0x5D98;
 *            while p <> nil and not (p^.name = tail^.name) (0xAF8:0x0C10)
 *            do p := p^.link (far pointer at p+0x104); if p <> nil then
 *            tail^.name := Copy(CS:0x000D (2 characters) + tail^.name, 1, 0x28)
 *            (0xAF8:0x0B0B, 0x0B98, 0x0B25 with limit 0x28);
 *   +0186  0xAF8:0x05C1 closes the record; +0191 frame restore; retf 4.
 * The runtime text-file services, the heap allocation, the resident selector
 * and +02AE are the boundaries of this model and are supplied as callbacks;
 * the pure string operations (load, concatenate, compare, bounded store,
 * fill) are translated inline; the private CS literals are supplied by the
 * caller with their exact original lengths. The engine's two loops are
 * unbounded (a read loop that ends only on Eof and a party walk that never
 * terminates on a cycle); host guards bound them and report LINE_LIMIT or
 * PARTY_LIMIT, which the engine cannot return. */

#define PR_U6F_0010_NAME_LIMIT 0x28u                 /* characters kept by ReadLn and by the marked copy */
#define PR_U6F_0010_NAME_BYTES (PR_U6F_0010_NAME_LIMIT + 1u)   /* length byte + 40 characters: node+0x00..+0x28 */

/* One heap node as the engine lays it out (46 bytes there: the string[40] at
 * +0, one unused byte at +0x29, the next far pointer at +0x2A). The host
 * supplies nodes through `alloc_node`; the model zero-fills them (FillChar)
 * before linking, so a host pool needs no initialization. */
typedef struct pr_u6f_0010_node {
    uint8_t name[PR_U6F_0010_NAME_BYTES];            /* Pascal string, node+0x00 */
    uint8_t reserved;                                /* node+0x29, zero-filled, never read */
    struct pr_u6f_0010_node *next;                   /* the far pointer at node+0x2A (nil = NULL) */
} pr_u6f_0010_node;

/* One party character record as the engine reads it: the Pascal name at +0
 * and the far link at +0x104. The supplied span must hold the whole name
 * (name_size >= length + 1); the compare reads nothing else. */
typedef struct pr_u6f_0010_party {
    const uint8_t *pascal_name;
    size_t name_size;
    const struct pr_u6f_0010_party *next;
} pr_u6f_0010_party;

typedef struct {
    void *context;
    int (*ensure_directory)(void *context);                                   /* +02AE */
    int (*select_file)(void *context, const uint8_t *pascal_path, size_t span,
                       uint8_t *al);                                          /* 0x802:0x02EB, Boolean in AL */
    int (*text_assign)(void *context, const uint8_t *pascal_path, size_t span); /* 0x04F7 */
    int (*text_reset)(void *context);                                         /* 0x0567 (input mode) */
    int (*eof)(void *context, uint8_t *al);                                   /* 0x0A04, non-zero AL = end of file */
    int (*alloc_node)(void *context, pr_u6f_0010_node **node);                /* 0x023F GetMem(0x2E) */
    int (*read_line)(void *context, uint8_t *chars, size_t max_count,
                     size_t *count);                                          /* 0x08E1 (max_count = 0x28) + 0x081C */
    int (*text_close)(void *context);                                         /* 0x05C1 */
} pr_u6f_0010_hooks;

typedef struct {
    uint8_t selected;          /* the selector's AL was non-zero */
    uint8_t closed;            /* the close service was reached */
    size_t nodes;              /* nodes allocated and linked (= lines read) */
    size_t party_steps;        /* party records compared, over all lines */
    size_t marked;             /* names found in the party and prefixed */
    uint16_t next_offset;      /* source offset the model stopped at, or 0x0197 after retf */
} pr_u6f_0010_result;

typedef enum {
    PR_U6F_0010_OK = 0,
    PR_U6F_0010_ARGUMENT,        /* also a host contract break: a read longer than max_count, a NULL node,
                                  * or *head changed by a callback (detected at the engine's reread, +00AF) */
    PR_U6F_0010_BOUNDS,          /* the path would exceed 255 characters */
    PR_U6F_0010_NEED_HOOK,       /* a reached boundary has no callback; next_offset = the call instruction */
    PR_U6F_0010_HOST_FAILURE,    /* a callback returned non-zero */
    PR_U6F_0010_LINE_LIMIT,      /* host guard on the read loop (max_nodes) */
    PR_U6F_0010_PARTY_LIMIT      /* host guard on the party walk (max_party_steps per line) */
} pr_u6f_0010_status;

/* head: the var argument; set to NULL before anything else, then to the
 * first node. live_path: the Pascal string at DS:0x537A (length byte first;
 * the span must hold length + 1 bytes). file_name: the 12-character CS:0x0000
 * constant; marker: the 2-character CS:0x000D constant. party: the current
 * party list head (DS:0x5D96/0x5D98), NULL for an empty party. Lengths are
 * checked against the originals' lengths. max_nodes and max_party_steps are
 * host guards (0 reports the limit at once). Nodes keep whatever the host
 * allocated; on any non-OK status the list built so far stays reachable from
 * *head, as the engine's heap state would. */
pr_u6f_0010_status pr_u6f_0010_read_list(
    pr_u6f_0010_node **head,
    const uint8_t *live_path, size_t live_path_size,
    const uint8_t *file_name, size_t file_name_size,
    const uint8_t *marker, size_t marker_size,
    const pr_u6f_0010_party *party,
    const pr_u6f_0010_hooks *hooks,
    size_t max_nodes, size_t max_party_steps,
    pr_u6f_0010_result *out);

#define PR_U6F_0010_FILE_NAME_LENGTH 12u
#define PR_U6F_0010_MARKER_LENGTH 2u
#define PR_U6F_0010_NODE_SIZE 0x2Eu
#define PR_U6F_0010_NEXT_OFFSET 0x2Au
#define PR_U6F_0010_PARTY_LINK_OFFSET 0x104u
#define PR_U6F_0010_RETF_OFFSET 0x0194u
#define PR_U6F_0010_END_OFFSET 0x0197u

#endif
