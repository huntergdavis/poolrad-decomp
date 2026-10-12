/* Host adapter for the unit 0x6f entry 6 block reader hook; see include/poolrad/recovered/overlay_u6f_reader_adapter.h. */
#include "poolrad/recovered/overlay_u6f_reader_adapter.h"

#include <string.h>

int pr_u6f_ra_init(pr_u6f_ra *ra, pr_rec_res_ecl_dax_io *io, pr_u25_e5_object *objects, size_t slot_count, uint16_t key_segment_base)
{
    size_t i;
    if (!ra || !io || !objects || slot_count == 0u || slot_count > PR_U6F_RA_MAX_SLOTS || key_segment_base == 0u)
        return PR_U6F_RA_STOP_ARGUMENT;
    memset(ra, 0, sizeof *ra);
    ra->io = io; ra->objects = objects; ra->slot_count = slot_count; ra->key_segment_base = key_segment_base;
    for (i = 0; i < slot_count; i++) {
        objects[i].segment = NULL; objects[i].segment_size = 0u;
        objects[i].offset = 0u; objects[i].segment_id = (uint16_t)(key_segment_base + i);
    }
    return PR_U6F_RA_OK;
}

/* a Pascal string as a C string: length byte, then that many bytes, none NUL */
static int to_c(const uint8_t *p, char *out)
{
    size_t i;
    if (!p) return 0;
    for (i = 0; i < p[0]; i++)
        if (p[1 + i] == 0u) return 0;
    memcpy(out, p + 1, p[0]);
    out[p[0]] = '\0';
    return 1;
}

int pr_u6f_ra_block_hook(void *ctx, const pr_u6f_e6_env *env, const uint8_t *name, const uint8_t *ext, uint8_t id, uint16_t *size, pr_far16 *ptr)
{
    pr_u6f_ra *ra = ctx;
    char cname[256], cext[256];
    const uint8_t *block = NULL;
    uint16_t sz = 0u;
    size_t i;
    int rs;
    (void)env;
    if (!ra || !size || !ptr) return PR_U6F_RA_STOP_ARGUMENT;
    *size = 0u; ptr->off = 0u; ptr->seg = 0u;
    if (!to_c(name, cname) || !to_c(ext, cext)) return PR_U6F_RA_STOP_STRING;
    for (i = 0; i < ra->slot_count && ra->slots[i].live; i++) {}
    if (i == ra->slot_count) return PR_U6F_RA_STOP_NO_SLOT;                  /* before the reader runs: nothing to clean up */
    rs = pr_rec_res_0802_0716(ra->io, cname, cext, id, &sz, &block);
    ra->last_reader_status = rs;
    ra->loads++;
    if (rs == PR_REC_RES_ECL_DAX_OK) {
        if (sz == 0u || !block) {                                           /* not a DOS outcome: release what the reader returned, then stop */
            if (block) pr_rec_res_0802_0716_release(ra->io, block, sz);
            return PR_U6F_RA_STOP_ZERO_OK;
        }
        ra->slots[i].block = block; ra->slots[i].size = sz; ra->slots[i].live = 1u; ra->slots[i].released = 0u;
        ra->objects[i].segment = (uint8_t *)block;                         /* a borrowed view: the u6f model only reads block objects */
        ra->objects[i].segment_size = sz;
        *size = sz;
        ptr->off = ra->objects[i].offset; ptr->seg = ra->objects[i].segment_id;
        return PR_U6F_RA_OK;
    }
    if (rs == PR_REC_RES_ECL_DAX_MISSING_FILE || rs == PR_REC_RES_ECL_DAX_MISSING_ID)
        return PR_U6F_RA_OK;                                                /* zero outputs, normal return */
    if (rs == PR_REC_RES_ECL_DAX_PACKED_LIMIT_ERROR)
        return PR_U6F_RA_STOP_PACKED_LIMIT;
    return PR_U6F_RA_STOP_HOST_BASE + rs;
}

int pr_u6f_ra_free_hook(void *ctx, const pr_u6f_e6_env *env, pr_far16 ptr, uint16_t size)
{
    pr_u6f_ra *ra = ctx;
    size_t i;
    (void)env;
    if (!ra) return PR_U6F_RA_STOP_ARGUMENT;
    for (i = 0; i < ra->slot_count; i++) {
        if (ra->objects[i].offset != ptr.off || ra->objects[i].segment_id != ptr.seg)
            continue;
        if (!ra->slots[i].live || ra->slots[i].size != size)
            return PR_U6F_RA_STOP_FREE_MISMATCH;                            /* double release or wrong size: nothing is released */
        pr_rec_res_0802_0716_release(ra->io, ra->slots[i].block, ra->slots[i].size);
        ra->slots[i].live = 0u; ra->slots[i].released = 1u; ra->slots[i].block = NULL;
        ra->objects[i].segment = NULL; ra->objects[i].segment_size = 0u;     /* a stale key is now VIEW */
        ra->releases++;
        return PR_U6F_RA_OK;
    }
    return PR_U6F_RA_STOP_FREE_MISMATCH;
}

unsigned pr_u6f_ra_teardown(pr_u6f_ra *ra)
{
    size_t i;
    unsigned n = 0u;
    if (!ra) return 0u;
    for (i = 0; i < ra->slot_count; i++) {
        if (!ra->slots[i].live) continue;
        pr_rec_res_0802_0716_release(ra->io, ra->slots[i].block, ra->slots[i].size);
        ra->slots[i].live = 0u; ra->slots[i].block = NULL;
        ra->objects[i].segment = NULL; ra->objects[i].segment_size = 0u;
        n++;
    }
    ra->teardown_releases += n;
    return n;
}
