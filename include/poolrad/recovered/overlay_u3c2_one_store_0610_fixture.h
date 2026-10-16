#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_ONE_STORE_0610_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_ONE_STORE_0610_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_0610_ENTRY 0x0610u
#define PR_U3C2_0610_STOP 0x0614u
typedef enum { PR_U3C2_0610_OK = 0, PR_U3C2_0610_ARGUMENT } pr_u3c2_one_store_0610_status;
typedef enum { PR_U3C2_0610_AF_PRESERVE_SUPPLIED = 1 } pr_u3c2_0610_af_policy;
/* MOV byte SS:[BP-1],1 only; stop before 0614 successor.
 * Raw 1 MiB RAM and distinct 14-word CPU; code immutable, no async/debug traps.
 * Mandatory host policy preserves all supplied FLAGS including AF, whose
 * upstream native value is unknown. Only the destination byte and IP change.
 * Effective offset wraps16 before physical A20; CPU must be disjoint from RAM.
 * Null/short RAM, wrong IP or unsupported AF policy reject before mutation.
 * Independent result-byte store from supplied state; no producer, successor or return claim. */
pr_u3c2_one_store_0610_status pr_u3c2_one_store_0610_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_0610_af_policy af_policy);
#endif
