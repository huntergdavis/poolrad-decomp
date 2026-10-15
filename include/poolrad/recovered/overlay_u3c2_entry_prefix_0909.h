#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_ENTRY_PREFIX_0909_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_ENTRY_PREFIX_0909_H

#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"

#define PR_U3C2_0909_PREFIX_ENTRY 0x0909u
#define PR_U3C2_0909_PREFIX_STOP 0x0913u

typedef enum {
    PR_U3C2_0909_PREFIX_OK = 0,
    PR_U3C2_0909_PREFIX_ARGUMENT
} pr_u3c2_0909_prefix_status;

/* Four instructions at +0909..+0912; stop before +0913 JMP.
 * Save BP, establish BP, subtract 32 from SP and clear SS:[BP-3].
 * All SUB-defined flags are modeled; other flags are preserved.
 * Offsets/SP wrap at 16 bits, word stores are contiguous physical bytes
 * with A20 disabled. Code must be immutable; no asynchronous traps.
 * No branch execution, whole-caller or natural/native-frame claim.
 */
pr_u3c2_0909_prefix_status pr_u3c2_entry_prefix_0909(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu);

#endif
