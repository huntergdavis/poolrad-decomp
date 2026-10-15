#ifndef POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_COMPOSED_08AE_001F_H
#define POOLRAD_RECOVERED_RESIDENT_TIME_CALLER_COMPOSED_08AE_001F_H

#include "poolrad/recovered/resident_time_caller_prefix_08ae_001f.h"

#include <stdint.h>

/* Called at the relocated arithmetic entry. It must supply the returned
 * CPU state at the caller's +002F and apply any helper-owned RAM effects
 * through ctx. This adapter does not implement the helper. */
typedef void (*pr_rec_res_08ae_001f_helper_return)(
    pr_rec_res_08ae_001f_cpu *cpu, void *ctx);

/* Compose the reached +001F..+0032 caller operations, stopping at +0033.
 * local_word maps SS:[BP-6], return_frame maps SS:[SP-4..SP-1], and
 * target_cs is the relocated arithmetic segment. */
void pr_rec_res_08ae_001f_to_0033(
    pr_rec_res_08ae_001f_cpu *cpu, const uint8_t local_word[2],
    uint8_t return_frame[4], uint16_t target_cs,
    pr_rec_res_08ae_001f_helper_return helper_return, void *ctx);

#endif
