#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_TARGET_RESET_2637_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_TARGET_RESET_2637_CPU_H
#include <stddef.h>
#include <stdint.h>
#define PR_RES_2637_RAM_SIZE (1u<<20)
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_res_2637_cpu;
typedef enum { PR_RES_2637_OK=0,PR_RES_2637_ARGUMENT } pr_res_2637_status;
/* Raw resident [2637,267D), 70 bytes/20 instructions. After far CALL,
 * return IP/CS then character offset/segment; RETF4. Reload the character
 * argument and its +108 far pointer before each zero write to +3,+0,+7,+6.
 * Write local [BP-1]=1, load AL (AH survives), and restore the live frame.
 * FLAGS are the original SUB SP,2 result, not a normalized success flag.
 * Exact1MiB A20off RAM; IP2637, SP[0004,FFF8]; CPU outside RAM; immutable
 * code, no interrupts/faults; every reached return frame nonwrapping even
 * after aliases. LES/word operands physically contiguous, offsets16bit.
 * ARGUMENT changes nothing. The earlier scalar callback API is unchanged.
 */
pr_res_2637_status pr_res_00ba_target_reset_2637_cpu(uint8_t *ram,size_t size,pr_res_2637_cpu *cpu);
#endif
