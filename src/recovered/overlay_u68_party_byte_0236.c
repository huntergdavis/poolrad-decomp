/* Portable C translation of GAME.OVR unit 0x68 +0x0236; see
 * docs/re/U68_PARTY_BYTE_0236.md. */
#include "poolrad/recovered/overlay_u68_party_byte_0236.h"

/* +0x025B..+0x0262: di := (offset + cbw(k)) mod 65536; the read is es:[(di + 0x96) mod 65536] */
static uint16_t pr_u68_0236_address(uint16_t offset, uint8_t k)
{
    uint16_t sext = (uint16_t)(k < 0x80u ? k : 0xFF00u | k);       /* cbw */
    uint16_t di = (uint16_t)(offset + sext);
    return (uint16_t)(di + PR_U68_0236_BYTE_BASE);
}

/* The host guard for one visited node: the byte and the four link bytes must be inside the view. */
static int pr_u68_0236_readable(const pr_u68_0236_node *n, uint16_t byte_addr)
{
    uint16_t link = (uint16_t)(n->offset + PR_U68_0236_NEXT_OFFSET);
    unsigned i;
    if (!n->segment || n->segment_size > 0x10000u || byte_addr >= n->segment_size)
        return 0;
    for (i = 0; i < PR_U68_0236_NEXT_BYTES; i++)
        if ((uint16_t)(link + i) >= n->segment_size)
            return 0;
    return 1;
}

pr_u68_0236_status pr_u68_0236(const pr_u68_0236_node *head, uint8_t k, uint8_t *result,
                               pr_u68_0236_report *report)
{
    pr_u68_0236_report local;
    pr_u68_0236_report *r = report ? report : &local;
    const pr_u68_0236_node *n;
    const pr_u68_0236_node *hit = NULL;
    unsigned visited = 0;
    uint16_t hit_address = 0;
    uint8_t found = 0;                      /* [bp-6] */
    r->visited = 0;
    r->hit = NULL;
    r->hit_address = 0;
    r->index = (int)PR_U68_0236_BYTE_BASE + (k < 0x80u ? (int)k : (int)k - 256);
    if (!result)
        return PR_U68_0236_ARGUMENT;
    /* host guard pass first, so nothing is reported half-way: every node the loop will visit must be readable
       (the loop stops after the first positive byte, whose own link is still read) */
    for (n = head; n; n = n->next) {
        uint16_t addr = pr_u68_0236_address(n->offset, k);
        uint8_t b;
        if (++visited > PR_U68_0236_LIST_CAP || !pr_u68_0236_readable(n, addr))
            return PR_U68_0236_ARGUMENT;
        b = n->segment[addr];
        if (b > 0u && b < 0x80u)            /* signed byte > 0, written without a signed cast */
            break;
    }
    visited = 0;
    /* +0x023C..+0x0283 */
    for (n = head; n && !found; n = n->next) {
        uint16_t addr = pr_u68_0236_address(n->offset, k);
        uint8_t b = n->segment[addr];       /* +0x0264: cmp byte es:[di + 0x96], 0 */
        visited++;
        if (b > 0u && b < 0x80u) {          /* +0x026A: jle skips; positive as a signed byte */
            found = 1;
            hit = n;
            hit_address = addr;
        }
    }
    r->visited = visited;
    r->hit = hit;
    r->hit_address = hit_address;
    *result = found;                        /* +0x0285..+0x028B */
    return PR_U68_0236_OK;
}
