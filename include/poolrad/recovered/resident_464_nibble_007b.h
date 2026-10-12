#ifndef POOLRAD_RECOVERED_RESIDENT_464_NIBBLE_007B_H
#define POOLRAD_RECOVERED_RESIDENT_464_NIBBLE_007B_H

#include <stddef.h>
#include <stdint.h>

/* Bounded RES_0464:007B model. The DOS frame supplies a far source
 * pointer [BP+6] and a selector byte [BP+0x0A], and returns AL after
 * copying eight bytes to its local frame with runtime Move.
 * All four audited callers pass a selector in 0..15. Higher values
 * address outside that local copy, so this host API rejects them. */
typedef enum {
    PR_R464_NIBBLE_OK = 0,
    PR_R464_NIBBLE_ARGUMENT,
    PR_R464_NIBBLE_BOUNDS
} pr_r464_nibble_status;

pr_r464_nibble_status pr_r464_nibble_007b(const uint8_t *source,
                                           size_t source_size,
                                           uint8_t selector,
                                           uint8_t *result);

#endif
