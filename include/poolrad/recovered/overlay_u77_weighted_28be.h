#ifndef POOLRAD_RECOVERED_OVERLAY_U77_WEIGHTED_28BE_H
#define POOLRAD_RECOVERED_OVERLAY_U77_WEIGHTED_28BE_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    PR_U77_28BE_OK = 0,
    PR_U77_28BE_ARGUMENT,
    PR_U77_28BE_DIVIDE_ERROR
} pr_u77_28be_status;

/* Unit 0x77 entry 11. The record begins at the far pointer passed by the
 * caller. Weights are the five dwords at DS:0D24; weights[3] is also the
 * divisor at DS:0D30. The returned dword is the DOS DX:AX result.
 * This API models the routine's value, not its scratch stack or flags. */
pr_u77_28be_status pr_u77_weighted_28be(
    const uint8_t *record, size_t record_size, const uint32_t weights[5],
    uint32_t *result);

#endif
