#ifndef POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY_01DA_CPU_H
#define POOLRAD_RECOVERED_OVERLAY_U3B0_ENTRY_01DA_CPU_H
#include "poolrad/recovered/resident_0709_glyph_01df_cpu.h"
typedef struct { uint16_t ax,bx,cx,dx,si,di,bp,sp,cs,ip,ds,es,ss,flags; } pr_u3b0_01da_cpu;
typedef struct { size_t caller_loopbacks,glyph_loopbacks; } pr_u3b0_01da_limits;
typedef struct { int callee,status;pr_r0709_glyph_cpu_diagnostic glyph; } pr_u3b0_01da_diagnostic;
typedef enum { PR_U3B0_01DA_SET=0,PR_U3B0_01DA_GLYPH } pr_u3b0_01da_callee;
typedef enum { PR_U3B0_01DA_OK=0,PR_U3B0_01DA_ARGUMENT,PR_U3B0_01DA_CODE,
    PR_U3B0_01DA_CALLEE,PR_U3B0_01DA_TRANSFER,PR_U3B0_01DA_LIMIT } pr_u3b0_01da_status;
/* Complete U3B0 [01DA,037D), RETF4; following queue bytes are data.
 * SS:[entry SP+4] context L in SS, [SP+6] selection low byte. Length L-64,
 * start column L-65, character i at L+i-53, one-based. Selection range bytes
 * L+2*selection-8F/-8E; selected/set-member color L+0E, normal color L+10.
 * Zero length returns without clearing. Nonzero length draws at row24; if
 * live length+column <39, issue black-space count40-length-column from that
 * sum. No clipping to the legacy bounded page adapter's column38 limit.
 * CS:01BA is the live32-byte set. Full raw SYSTEM0DC0 and glyph01DF compose.
 * Exact1MiB A20off, EGA attached, IP01DA, SP[0040,FFF4]; initial DS/SS
 * windows and caller/flat-RAM callee operands exclude A0000..AFFFF. Drawing
 * uses pr_bus. Code/bindings/set immutable outside aperture, writes avoid
 * them, reached frames nonwrapping; other aliases live. CPU/bus/EGA/limits/
 * diag/RAM storage distinct; callbacks observe/refuse OUT only. Normal/386,
 * synchronous; no IRQ/fault/self-modification/arbitrary-PC resume.
 * Caller budget counts taken JMP0312, LIMIT before jump; glyph budget fresh
 * per invocation. Root far opcode/offset checked before CALL; segments live.
 * CALLEE retains nested stops, TRANSFER unexpected returns, root RETF live.
 * No rollback. ARGUMENT preserves CPU/RAM/EGA and resets optional diag.
 * Modes0/2 ordinary-memory effects do not establish CGA/Tandy closure. */
pr_u3b0_01da_status pr_u3b0_entry_01da_cpu(pr_bus *bus,size_t ram_size,
    pr_u3b0_01da_cpu *cpu,const pr_u3b0_01da_limits *limits,pr_u3b0_01da_diagnostic *diagnostic);
#endif
