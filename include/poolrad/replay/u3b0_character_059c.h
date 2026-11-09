#ifndef POOLRAD_REPLAY_U3B0_CHARACTER_059C_H
#define POOLRAD_REPLAY_U3B0_CHARACTER_059C_H
#include "poolrad/recovered/resident_0899_readkey_0059_cpu.h"
#include <stddef.h>
#include <stdint.h>
typedef enum {
    PR_U3B0_CHARACTER_ESCAPE = 0, /* stopped at +06CF */
    PR_U3B0_CHARACTER_ENTER_ENABLED, /* stopped at +06CF */
    PR_U3B0_CHARACTER_ENTER_DISABLED, /* stopped at +06CF */
    PR_U3B0_CHARACTER_OTHER, /* stopped before +05C4 */
    PR_U3B0_CHARACTER_ARGUMENT
} pr_u3b0_character_status;
/* Caller supplies live +059C CPU/frame and 1 MiB A20-off RAM, including all
 * prior prompt effects and stable source-bound code. Those prerequisites are
 * not proved here. No drawing, loop, prompt return or entry2 hook is executed.
 * Guest state is changed in source order; invalid arguments change nothing. */
pr_u3b0_character_status pr_u3b0_character_059c(uint8_t *ram, size_t size,
                                                pr_r0899_cpu *cpu);
#endif
