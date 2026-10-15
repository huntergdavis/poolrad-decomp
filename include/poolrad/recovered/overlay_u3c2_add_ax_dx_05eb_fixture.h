#ifndef POOLRAD_RECOVERED_OVERLAY_U3C2_ADD_AX_DX_05EB_FIXTURE_H
#define POOLRAD_RECOVERED_OVERLAY_U3C2_ADD_AX_DX_05EB_FIXTURE_H
#include "poolrad/recovered/overlay_u6c_record_table_sum_3f31.h"
#define PR_U3C2_05EB_ENTRY 0x05EBu
#define PR_U3C2_05EB_STOP 0x05EDu
typedef enum { PR_U3C2_05EB_OK = 0, PR_U3C2_05EB_ARGUMENT } pr_u3c2_add_ax_dx_05eb_status;
typedef enum { PR_U3C2_05EB_AF_ADD_DEFINED = 1 } pr_u3c2_05eb_af_policy;
/* ADD AX,DX only; stop before the 05ED byte store. AF is defined by addition.
 * Raw 1 MiB RAM and distinct 14-word CPU; code immutable, no async/debug traps.
 * Independent AX/DX/FLAGS include wider synthetic states without CBW provenance.
 * CF/PF/AF/ZF/SF/OF are defined; other supplied flags are retained.
 * Null/short RAM, wrong IP or unsupported mandatory AF policy reject unchanged.
 * No predecessor reach, successor effects or return is claimed. */
pr_u3c2_add_ax_dx_05eb_status pr_u3c2_add_ax_dx_05eb_fixture(
    uint8_t *ram, size_t ram_size, pr_u6c_3f31_cpu *cpu, pr_u3c2_05eb_af_policy af_policy);
#endif
