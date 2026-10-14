#ifndef POOLRAD_RECOVERED_OVERLAY_U25_STORE_WORD_0C77_H
#define POOLRAD_RECOVERED_OVERLAY_U25_STORE_WORD_0C77_H

#include "poolrad/recovered/overlay_u25_party_index_0756.h"

typedef pr_u25_0756_cpu pr_u25_store_word_cpu;

typedef enum {
    PR_U25_STORE_WORD_OK = 0,
    PR_U25_STORE_WORD_ARGUMENT,
    PR_U25_STORE_WORD_TRANSFER,
    PR_U25_STORE_WORD_DEPENDENCY
} pr_u25_store_word_status;

/* Raw A20-off RAM model of CS:0C77, after the far CALL. Return IP/CS,
 * value word and address word occupy SS:SP. Includes +06FB and the local
 * +0A74 store-hook effects and the complete resident selector-push leaf.
 * Code must remain immutable, with valid loader-relocated call operands.
 *
 * DEPENDENCY stops BEFORE an untranslated unit-3BD far CALL, with IP at
 * 0C26, 0C49 or 0C6C and its argument words already pushed. A host executor
 * must execute the actual (possibly relocated) instruction and remaining
 * continuation. No callee result or post-call effect is invented.
 * TRANSFER exposes an altered internal return. The class-4 subtract-four
 * loop preserves the original's repeated writes and possible nontermination
 * under adversarial RAM aliases. No whole-entry strict-C claim. */
pr_u25_store_word_status pr_u25_store_word_0c77(
    uint8_t *ram, size_t ram_size, pr_u25_store_word_cpu *cpu);

#endif
