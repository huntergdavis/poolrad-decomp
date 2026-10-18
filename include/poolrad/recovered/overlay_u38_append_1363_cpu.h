#ifndef POOLRAD_RECOVERED_OVERLAY_U38_APPEND_1363_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_APPEND_1363_CPU_H
#include "poolrad/recovered/overlay_u9e_list_append_0e52_cpu.h"
#define PR_U38_1363_RAM_SIZE PR_U9E_0E52_RAM_SIZE
typedef pr_u9e_0e52_cpu pr_u38_1363_cpu;
typedef struct { unsigned max_heap_steps;size_t max_links; } pr_u38_1363_limits;
typedef struct { int append_status,getmem_status; } pr_u38_1363_diagnostic;
typedef enum { PR_U38_1363_OK=0,PR_U38_1363_ARGUMENT,PR_U38_1363_CODE,
    PR_U38_1363_CALLEE,PR_U38_1363_TRANSFER } pr_u38_1363_status;
/* Complete U38 entry46 [1363,1382), RETF0Ah. Dispatch type56.
 * Five argument words; record offset/segment at entry SS:[SP+0Ah]/[SP+0Ch].
 * Other three arguments ignored, including mode. Always invokes raw U9E0E52
 * with b=19h,a=1,c=0Ch,d=0; GetMem and list writes remain live.
 * Exact1MiB A20-off RAM; IP1363, SP[0040,FFF2], immutable code/bindings.
 * Reached callee contracts apply; return frames must remain nonwrapping.
 * CPU/limits/diagnostic distinct from RAM and each other; no IRQs/faults.
 * Budgets may be zero. CODE stops before malformed reached call/stub.
 * CALLEE preserves partial CPU/RAM and both raw statuses. TRANSFER preserves
 * a changed direct return. POP BP uses the actual returned SP, not cached BP.
 * ARGUMENT leaves CPU/RAM unchanged; diagnostic initializes -1/-1.
 * A null diagnostic is allowed. No rollback and no callback.
 */
pr_u38_1363_status pr_u38_append_1363_cpu(uint8_t *ram,size_t size,
    pr_u38_1363_cpu *cpu,const pr_u38_1363_limits *limits,pr_u38_1363_diagnostic *diagnostic);
#endif
