#ifndef POOLRAD_RECOVERED_OVERLAY_U3C6_MOV_SP_BP_0062_H
#define POOLRAD_RECOVERED_OVERLAY_U3C6_MOV_SP_BP_0062_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
typedef enum { PR_U3C6_0062_NEXT_PENDING=0, PR_U3C6_0062_ARGUMENT } pr_u3c6_0062_status;
/* Independent supplied post-load CPU/IP0062. MOV SP,BP only; stop0064
 * BEFORE POP. OnlySP/IP change; BP/allFLAGS/otherCPU/RAM retained.
 * No predecessor/frame/return inference. Exactly1MiB, CPU separate,
 * immutable code. Bad args/IP reject unchanged; native parity unmeasured.
 */
pr_u3c6_0062_status pr_u3c6_mov_sp_bp_0062_fixture(
    uint8_t *ram,size_t ram_size,pr_u6c_3f31_cpu *cpu);
#endif
