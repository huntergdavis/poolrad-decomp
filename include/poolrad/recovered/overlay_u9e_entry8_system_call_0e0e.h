#ifndef POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_SYSTEM_CALL_0E0E_H
#define POOLRAD_RECOVERED_OVERLAY_U9E_ENTRY8_SYSTEM_CALL_0E0E_H

#include "poolrad/recovered/system_random_fullstate_11c0.h"

typedef pr_system_11c0_cpu pr_u9e_entry8_call_cpu;

typedef enum {
    PR_U9E_ENTRY8_CALL_RETURNED = 0,
    PR_U9E_ENTRY8_CALL_ARGUMENT,
    PR_U9E_ENTRY8_CALL_SYSTEM_ARGUMENT,
    PR_U9E_ENTRY8_CALL_SYSTEM_FAILED,
    PR_U9E_ENTRY8_CALL_SYSTEM_TRANSFER,
    PR_U9E_ENTRY8_CALL_RETURN_TRANSFER
} pr_u9e_entry8_call_status;

/* Execute only U9E entry8 +0E0E LCALL 0AF8:11C0 and its merged SYSTEM
 * callee on the same live 1 MiB RAM/CPU14. A normal return stops at the
 * original U9E CS:+0E13. Other live destinations remain explicit transfers.
 * Loaded U9E and SYSTEM instruction bodies must contain original bytes. */
pr_u9e_entry8_call_status pr_u9e_entry8_system_call_0e0e(
    uint8_t *ram, size_t ram_size, pr_u9e_entry8_call_cpu *cpu);

#endif
