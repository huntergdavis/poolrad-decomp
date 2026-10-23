/* Plain-argument shim over the ECL helper models in src/recovered/ecl_helpers.c, for the original-byte audit
 * tools/overlay/ecl_helpers_original_audit.py (the loader state is too large to mirror in ctypes). Each function builds a
 * zeroed state, sets the fields the body reads, calls the model and copies out the fields the body writes. */
#include "poolrad/recovered/ecl_helpers.h"
#include <stdlib.h>
#include <string.h>

uint16_t shim_operand_word(uint8_t low, uint8_t high) { return pr_rec_ecl_operand_word(low, high); }
uint8_t shim_char_remap(uint8_t code) { return pr_rec_ecl_char_remap(code); }
uint8_t shim_address_class(uint16_t address) { return pr_rec_ecl_address_class(address); }

int shim_compare_words(uint16_t a, uint16_t b, const uint8_t flags_in[6], uint8_t flags_out[6])
{
    pr_rec_ecl_loader_state *s = calloc(1, sizeof *s);
    int status;
    if (s == NULL) return -1;
    memcpy(s->cmp_flags, flags_in, 6);
    status = (int)pr_rec_ecl_compare_words(s, a, b);
    memcpy(flags_out, s->cmp_flags, 6);
    free(s);
    return status;
}

int shim_compare_strings(const char *a, size_t a_len, const char *b, size_t b_len, const uint8_t flags_in[6], uint8_t flags_out[6])
{
    pr_rec_ecl_loader_state *s = calloc(1, sizeof *s);
    int status;
    if (s == NULL) return -1;
    memcpy(s->cmp_flags, flags_in, 6);
    status = (int)pr_rec_ecl_compare_strings(s, a, a_len, b, b_len);
    memcpy(flags_out, s->cmp_flags, 6);
    free(s);
    return status;
}

/* in/out: pc, b84df, b5e24, b5e25. */
int shim_print_return(uint16_t *pc, uint8_t *b84df, uint8_t *b5e24, uint8_t *b5e25)
{
    pr_rec_ecl_loader_state *s = calloc(1, sizeof *s);
    int status;
    if (s == NULL) return -1;
    s->pc = *pc; s->b84df = *b84df; s->b5e24 = *b5e24; s->b5e25 = *b5e25;
    status = (int)pr_rec_ecl_op_print_return(s);
    *pc = s->pc; *b84df = s->b84df; *b5e24 = s->b5e24; *b5e25 = s->b5e25;
    free(s);
    return status;
}

/* u38 entries 8, 9 (read record byte +A0h), 10, 30, 31: modifiers in/out as [6816, 6822, 6818, 6819]. */
int shim_u38_entry(int which, uint8_t mods_io[4], const uint8_t *record, unsigned *applied)
{
    pr_rec_modifier_state m;
    int status;
    m.b6816 = mods_io[0]; m.b6822 = mods_io[1]; m.b6818 = mods_io[2]; m.b6819 = mods_io[3];
    *applied = 0u;
    switch (which) {
    case 8: status = (int)pr_rec_u38_entry8(&m, record, applied); break;
    case 9: status = (int)pr_rec_u38_entry9(&m, record, applied); break;
    case 10: status = (int)pr_rec_u38_entry10(&m, applied); break;
    case 30: status = (int)pr_rec_u38_entry30(&m); break;
    case 31: status = (int)pr_rec_u38_entry31(&m); break;
    default: return -1;
    }
    mods_io[0] = m.b6816; mods_io[1] = m.b6822; mods_io[2] = m.b6818; mods_io[3] = m.b6819;
    return status;
}
