#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_BYTE_STORE_05ED_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_BYTE_STORE_05ED_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_05ED_ENTRY 0x05EDu
#define PR_U3C2_05ED_STOP 0x05F0u
typedef enum { PR_U3C2_05ED_OK = 0, PR_U3C2_05ED_ARGUMENT } pr_u3c2_byte_store_05ed_status;
typedef enum { PR_U3C2_05ED_AF_PRESERVE_SUPPLIED = 1 } pr_u3c2_05ed_af_policy;
/* MOV byte SS:[BP-3],AL only; stop before 05F0 byte load.
 * Raw 1 MiB RAM and distinct 14-word CPU; code immutable, no async/debug traps.
 * Mandatory host policy preserves all supplied FLAGS including AF, whose
 * upstream native value is unknown. Only the destination byte and IP change.
 * Effective offset wraps16 before physical A20; CPU must be disjoint from RAM.
 * Null/short RAM, wrong IP or unsupported AF policy reject before mutation.
 * Independent range-reject supplied state; no producer, successor or return claim. */
pr_u3c2_byte_store_05ed_status pr_u3c2_byte_store_05ed_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_05ed_af_policy af_policy);
#endif
