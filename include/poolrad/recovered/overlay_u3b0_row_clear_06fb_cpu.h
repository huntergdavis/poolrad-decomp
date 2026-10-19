#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ROW_CLEAR_06FB_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ROW_CLEAR_06FB_CPU_H
#include "poolrad/recovered/resident_0709_space_05eb_cpu.h"
typedef pr_r0709_space_cpu pr_u3b0_06fb_cpu;
typedef enum {
    PR_U3B0_06FB_OK=0, PR_U3B0_06FB_ARGUMENT,
    PR_U3B0_06FB_CODE, PR_U3B0_06FB_CALLEE, PR_U3B0_06FB_TRANSFER
} pr_u3b0_06fb_status;
typedef struct {
    int space_status;
    pr_r0709_space_cpu_diagnostic space;
} pr_u3b0_06fb_diagnostic;
/* Raw U3B0 [06FB,0711), RETF0, no arguments. Calls 0709:05EB with
 * count40,color0,y24,x0, preserving AH in each argument word.
 * Exactly1MiB A20off; attached EGA; IP06FB; SP[003A,FFFC]. DS/SS
 * complete windows avoid EGA. Immutable loaded original code outside EGA;
 * writes avoid code and reached far-return frames do not cross SS:FFFF.
 * CPU/bus/EGA/diagnostic are separate host objects outside RAM.
 * Inherits the space/glyph bus, flags, mode and budget contracts.
 * CALL070A checks opcode and offset05EB before pushing its return frame;
 * its live segment operand selects the loaded resident body. No callback
 * substitutes for any function. Ports may observe/refuse without mutation.
 * CALLEE retains nested status and partial CPU/RAM/EGA; TRANSFER retains an
 * unexpected successful callee return. No arbitrary-PC resume or interrupts.
 * Invalid host arguments preserve CPU/RAM/EGA and reset diagnostics to -1.
 * Native hardware scope is EGA; packed modes alone do not prove CGA/Tandy.
 */
pr_u3b0_06fb_status pr_u3b0_row_clear_06fb_cpu(pr_bus *bus, size_t ram_size,
    pr_u3b0_06fb_cpu *cpu, size_t max_loopbacks, pr_u3b0_06fb_diagnostic *diagnostic);
#endif
