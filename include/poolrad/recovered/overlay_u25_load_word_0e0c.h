#ifndef POOLRAD_RECOVERED_OVERLAY_U25_LOAD_WORD_0E0C_H
#define POOLRAD_RECOVERED_OVERLAY_U25_LOAD_WORD_0E0C_H

#include "poolrad/recovered/overlay_u25_party_index_0756.h"

/* Shared raw-RAM register ABI; the party-index callee uses this exact type. */
typedef pr_u25_0756_cpu pr_u25_load_word_cpu;

typedef enum {
    PR_U25_LOAD_WORD_OK = 0,
    PR_U25_LOAD_WORD_ARGUMENT,
    PR_U25_LOAD_WORD_TRANSFER
} pr_u25_load_word_status;

/* Entry at CS:0E0C after the far CALL. SS:SP holds return IP/CS and the
 * address argument. Executes the classifier and complete class-1 hook,
 * reusing +0756 for its two party-list calls. RAM is at least 1 MiB and
 * uses A20-off physical wrapping, with immutable routine code.
 *
 * Unwritten result locals are read from RAM, never invented or replaced
 * by a host default. A cyclic party list does not return, as in DOS.
 * TRANSFER exposes an internal callee's altered return frame exactly;
 * the enclosing continuation is not executed after such a transfer. */
pr_u25_load_word_status pr_u25_load_word_0e0c(
    uint8_t *ram, size_t ram_size, pr_u25_load_word_cpu *cpu);

/* Entry at CS:07BC after the far CALL (or PUSH CS; CALL): the class-1 load
 * hook alone, the SAME code the +0E0C entry runs (no second copy). SS:SP
 * holds return IP/CS, the far pointer to the found flag, and the address
 * word; RETF 6. The flag byte is set to 1 before the address is read and
 * cleared on the not-found path; AX is the result local [BP-2], which some
 * paths never write (it is read from RAM, as in DOS). Its +0756 calls
 * return TRANSFER on an altered frame. Same RAM contract as above. */
pr_u25_load_word_status pr_u25_load_hook_07bc(
    uint8_t *ram, size_t ram_size, pr_u25_load_word_cpu *cpu);

#endif
