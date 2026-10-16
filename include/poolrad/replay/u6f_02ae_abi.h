#ifndef POOLRAD_REPLAY_U6F_02AE_ABI_H
#define POOLRAD_REPLAY_U6F_02AE_ABI_H

#include "poolrad/recovered/slot_a_directory_fullstate.h"

typedef enum {
    PR_U6F_02AE_STEPPED = 0,
    PR_U6F_02AE_RETURNED,
    PR_U6F_02AE_BLOCKED_MESSAGE,
    PR_U6F_02AE_BLOCKED_DOS_SERVICE,
    PR_U6F_02AE_BLOCKED_MEMORY,
    PR_U6F_02AE_BLOCKED_PC,
    PR_U6F_02AE_BAD_EVENT,
    PR_U6F_02AE_ARGUMENT
} pr_u6f_02ae_status;

/* Bounded entry-to-IOResult path of original U6F +02AE, plus generic
 * 0/5 return and +031F retry decision. Other errors stop BEFORE +02E9
 * message setup. Uses the existing SYSTEM Copy instruction adapter and
 * directory service-event validator. One step is one source instruction;
 * REP and the explicit DOS event are atomic host steps.
 *
 * Loaded code segments are U6F=2A8E, SYSTEM=131C. RETF reads the actual
 * frame; no outer return PC is prescribed. State, journal and RAM must
 * be disjoint. RAM <= 1 MiB; spans must fit one segment and RAM without
 * A20 wrap, as required by the reused Copy adapter. Unsupported spans,
 * PCs and DF-set string instructions block without CPU/RAM mutation.
 * Logic AF is normalized to zero; it is undefined in the source.
 *
 * The service callback must obey pr_slot_a_directory_service's read-only
 * input and complete-event contract. No filesystem or DOS result is
 * inferred. No whole-function, native parity or strict credit is claimed.
 */
pr_u6f_02ae_status pr_u6f_02ae_step(
    pr_slot_a_directory_state *state,
    pr_slot_a_directory_service service, void *context,
    pr_slot_a_directory_write *journal, size_t journal_capacity);

#endif
