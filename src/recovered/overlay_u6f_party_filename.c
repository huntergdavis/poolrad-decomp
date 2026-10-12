#include "poolrad/recovered/overlay_u6f_party_filename.h"

#include <string.h>

pr_u6f_party_filename_status pr_u6f_party_filename_first(
    const pr_u6f_party_string_frame *prior,
    const uint8_t *prefix, size_t prefix_size,
    const uint8_t *suffix, size_t suffix_size,
    const pr_rec_dax_selector_02eb_io *io,
    pr_u6f_party_filename_result *out)
{
    pr_u6f_party_filename_result result;
    pr_rec_dax_selector_02eb_string probe_name = {0};
    pr_rec_dax_selector_02eb_status probe_status;
    size_t base_len, length;
    uint16_t ax = 0u;

    if (!prior || !prefix || !suffix || !out)
        return PR_U6F_PARTY_FILENAME_ARGUMENT;
    if (prior->next_pc != 0x1983u || prior->basename[0] > 40u ||
        prior->basename[0] != (prior->after_0425[0] < 40u
                               ? prior->after_0425[0] : 40u) ||
        memcmp(prior->basename + 1u, prior->after_0425 + 1u,
               prior->basename[0]) != 0)
        return PR_U6F_PARTY_FILENAME_PRIOR;
    if (prefix_size == 0u || suffix_size == 0u ||
        prefix_size - 1u < prefix[0] || suffix_size - 1u < suffix[0])
        return PR_U6F_PARTY_FILENAME_BOUNDS;

    base_len = prior->basename[0];
    length = (size_t)prefix[0] + base_len + suffix[0];
    if (length + 1u > PR_U6F_PARTY_FILENAME_LOCAL_SIZE)
        return PR_U6F_PARTY_FILENAME_BOUNDS;

    memset(&result, 0, sizeof result);
    /* +0x1983..+0x1992: SYSTEM 0x0B0B copies DS:0x537A to -0x312 and
     * retains the destination far pointer. Both SYSTEM 0x0B98 calls append
     * to that same destination and return with RETF 4. */
    result.filename[0] = (uint8_t)length;
    memcpy(result.filename + 1u, prefix + 1u, prefix[0]);
    memcpy(result.filename + 1u + prefix[0], prior->basename + 1u,
           base_len);                         /* +0x1993..+0x199D */
    memcpy(result.filename + 1u + prefix[0] + base_len,
           suffix + 1u, suffix[0]);           /* +0x199E..+0x19A7 */
    result.next_pc = 0x19a8u;

    /* +0x19A8 is the first file effect, via the merged resident 0x02EB
     * model. The missing host I/O boundary is explicit and reached only
     * after the filename exists. */
    if (!io || !io->assign_temp || !io->reset_temp || !io->close_temp ||
        !io->io_result || !io->store_ds_word) {
        *out = result;
        return PR_U6F_PARTY_FILENAME_NEED_IO;
    }
    probe_name.length = (uint8_t)length;
    memcpy(probe_name.bytes, result.filename + 1u, length);
    probe_status = pr_rec_res_0802_02eb(io, &probe_name, &ax);
    if (probe_status != PR_REC_DAX_SELECTOR_02EB_OK) {
        *out = result;
        return PR_U6F_PARTY_FILENAME_IO_BOUNDARY;
    }
    result.ax = ax;
    /* +0x19AD OR AL,AL; +0x19AF JE +0x19EA. AH is not tested. */
    result.next_pc = (uint8_t)ax != 0u ? 0x19b1u : 0x19eau;
    *out = result;
    return PR_U6F_PARTY_FILENAME_OK;
}
