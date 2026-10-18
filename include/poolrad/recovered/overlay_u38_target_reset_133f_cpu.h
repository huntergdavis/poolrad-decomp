#ifndef POOLRAD_RECOVERED_OVERLAY_U38_TARGET_RESET_133F_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U38_TARGET_RESET_133F_CPU_H
#include "poolrad/recovered/resident_00ba_target_reset_2637_cpu.h"
typedef pr_res_2637_cpu pr_u38_133f_cpu;
typedef enum { PR_U38_133F_OK=0,PR_U38_133F_ARGUMENT,PR_U38_133F_CODE,PR_U38_133F_CALLEE,PR_U38_133F_TRANSFER } pr_u38_133f_status;
/* U38 [133F,1355),22 bytes/9 instructions, composing raw resident2637.
 * After far CALL: return IP/CS, three unused words, character off/seg;
 * RETF10. Resident segment comes from the reached LCALL's live fixup word.
 * CODE stops at1348 before the call; TRANSFER preserves a changed resident
 * return instead of continuing134D. The JE at134F targets its own fallthrough.
 * Exact1MiB A20off RAM, IP133F, SP[0010,FFF2], CPU outside RAM, immutable
 * code/bindings; no interrupts/faults. Reached return frames must remain
 * nonwrapping even after aliases. See raw resident contract. Logical AF=0.
 */
pr_u38_133f_status pr_u38_target_reset_133f_cpu(uint8_t *ram,size_t size,pr_u38_133f_cpu *cpu);
#endif
