#ifndef POOLRAD_RECOVERED_OVERLAY_U68_PARTY_BYTE_0236_H
#define POOLRAD_RECOVERED_OVERLAY_U68_PARTY_BYTE_0236_H

/* Portable C translation of DOS v1.3 GAME.OVR unit 0x68 internal routine
 * +0x0236..+0x0293 (file 0x16901, exactly 94 bytes, 32 instructions, `retf 2`,
 * no calls, no trailing data: the next prologue is +0x0294). Near-called
 * from entry 1 (+0x0D6D) at +0x0DDE (argument 6), +0x0F1D (6) and +0x0F57 (5);
 * those are all of its callers in the unit. Evidence:
 * docs/re/U68_PARTY_BYTE_0236.md.
 *
 * One word argument [bp+6], read as a byte and sign-extended (`cbw`).
 * Result in AL (also stored to [bp-1]):
 *   found := 0; node := far [0x5D96]
 *   while node <> nil and not found:
 *       if signed byte at node + 0x96 + k > 0 then found := 1       (+0x025B..+0x026C: `jle` skips)
 *       node := far pointer at node + 0x104                          (+0x0270..+0x0283: also after a hit)
 *   AL := found
 * The next pointer is read once more after a hit, before the loop test
 * ends it; that read is not observable here beyond the node being valid.
 * `k` is signed, so 128..255 index below +0x96 (down to +0x16). The
 * address arithmetic is the original's 16-bit one: `add di, ax` (di = far
 * offset + cbw(k)) wraps modulo 65536 and so does the effective address
 * di + 0x96 of the read. Each node is a 64 KiB-or-smaller host view of its
 * ES segment plus the far offset, and the read is view[(offset + k + 0x96)
 * mod 65536]; an address at or past the view's size is a host guard.
 * What the byte means is not asserted.
 *
 * Classification: full translation of all 32 instructions: the routine
 * makes no calls and writes nothing but its own locals. The party list and
 * the node records are explicit host state (far pointers as host structs);
 * the only guards are host-side (a record too short for the index, a
 * cyclic list). No game data is embedded. */

#include <stddef.h>
#include <stdint.h>

#define PR_U68_0236_BYTE_BASE 0x96u         /* es:[di + 0x96] */
#define PR_U68_0236_NEXT_OFFSET 0x104u      /* far next pointer at es:[di + 0x104]..+0x107 (host: node->next) */
#define PR_U68_0236_NEXT_BYTES 4u
#define PR_U68_0236_LIST_CAP 65536u         /* host guard: the original has no cycle check */

typedef struct pr_u68_0236_node {
    const uint8_t *segment;                 /* host bytes of the node's ES segment, from offset 0 */
    size_t segment_size;                    /* at most 0x10000; reads at or past it are a host guard */
    uint16_t offset;                        /* the far pointer's offset word */
    const struct pr_u68_0236_node *next;    /* far pointer at +0x104 (NULL = both words zero) */
} pr_u68_0236_node;

typedef enum {
    PR_U68_0236_OK = 0,
    PR_U68_0236_ARGUMENT   /* null result; a visited node whose segment is null or too small for the byte
                              address or the four link bytes; or a list longer than the cap. Result
                              untouched. */
} pr_u68_0236_status;

typedef struct {
    unsigned visited;                       /* nodes whose byte was tested */
    const pr_u68_0236_node *hit;            /* the node that set found, or NULL */
    int index;                              /* 0x96 + sign-extended k (before adding a node's offset) */
    uint16_t hit_address;                   /* effective offset of the byte read at the hit (valid when hit) */
} pr_u68_0236_report;

/* The routine. `head` = far [0x5D96], `k` = the low byte of [bp+6]. On OK, *result = AL (0 or 1). */
pr_u68_0236_status pr_u68_0236(const pr_u68_0236_node *head, uint8_t k, uint8_t *result,
                               pr_u68_0236_report *report);

#endif
