#ifndef POOLRAD_RECOVERED_OVERLAY_U1C_TEXT_2E7D_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U1C_TEXT_2E7D_CPU_H
#include "poolrad/recovered/memory_bus.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u1c_2e7d_cpu;
typedef struct {
    uint16_t system_cs;
    size_t max_operands,max_string_iterations,max_loopbacks,max_glyph_loopbacks;
} pr_u1c_2e7d_limits;
typedef struct { int callee,status,glyph_callee,glyph_status; } pr_u1c_2e7d_diagnostic;
typedef enum { PR_U1C_2E7D_FILLCHAR=0,PR_U1C_2E7D_LOADER,PR_U1C_2E7D_WORD,
    PR_U1C_2E7D_LOAD_WORD,PR_U1C_2E7D_GLYPH } pr_u1c_2e7d_callee;
typedef enum { PR_U1C_2E7D_OK=0,PR_U1C_2E7D_ARGUMENT,PR_U1C_2E7D_CODE,
    PR_U1C_2E7D_CALLEE,PR_U1C_2E7D_TRANSFER,PR_U1C_2E7D_LIMIT } pr_u1c_2e7d_status;
/* Full U1C [2E7D,2F2D), RETF, no arguments. Clears DS:84C8/84C9 and
 * DS:84E0, loads one operand, assembles slot1 into a base address, then
 * reads successive addresses through U25 +0E0C. Low byte of (value+1)
 * <=1 ends the loop; otherwise draw one glyph at DS:5E24/5E25, color15,
 * raw character, and increment x and the byte index. y increments on exit.
 * Entry IP2E7D; exact1MiB A20-off RAM; attached EGA; SP[023A,FFFC];
 * initial DS/SS 64KiB windows exclude A0000..AFFFF. ALL accesses by flat-RAM
 * SYSTEM/U25 callees must also exclude that aperture. Glyph accesses use
 * pr_bus. Code/bindings immutable outside aperture; nonwrapping reached
 * return frames; CPU/bus/EGA/limits/diagnostic distinct from RAM/each other.
 * Loader/string budgets positive. max_loopbacks counts taken +2F23->2EDB
 * edges, stopping before the branch with LIMIT; zero permits one glyph.
 * Glyph budget resets each invocation. U25 +0E0C retains its existing
 * acyclic-party-list precondition (no invented party-list budget).
 * Reached LCALL/stub opcodes and offsets are checked; segment bindings
 * read live. CODE precedes CALL. CALLEE preserves the actual stopped
 * callee status; TRANSFER preserves altered far returns. No rollback.
 * ARGUMENT leaves CPU/RAM/EGA unchanged; optional diagnostic resets.
 * Normal/386 flags, synchronous execution; no IRQs/faults/self-modification.
 * Port callbacks may only observe or refuse OUT, never mutate state.
 */
pr_u1c_2e7d_status pr_u1c_text_2e7d_cpu(pr_bus *bus,size_t ram_size,
    pr_u1c_2e7d_cpu *cpu,const pr_u1c_2e7d_limits *limits,
    pr_u1c_2e7d_diagnostic *diagnostic);
#endif
