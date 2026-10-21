#ifndef POOLRAD_RECOVERED_DOS_INTR_000B_CPU_H
#define POOLRAD_RECOVERED_DOS_INTR_000B_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_DOS_INTR_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of the Turbo Pascal Dos unit's Intr(IntNo; var Regs) at START.EXE
 * 0A32:[000B,006C) (97 bytes, 69 instructions, RETF 6), readiness coverage only (no identity claim).
 * Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] Regs offset, [SP+6] Regs segment, [SP+8] IntNo
 * (low byte). The body pushes BP, DS and a fake INT frame (FLAGS, CS, 0045h), reads the vector at
 * 0000:IntNo*4, loads AX BX CX DX BP SI DI DS ES from the Regs record, CLI, and RETFs into the handler.
 * The handler's return lands at 0045h, which stores AX BX CX DX BP SI DI DS ES FLAGS back into Regs,
 * then POP DS; POP BP; RETF 6. The RETF at +0044 is the transfer, not the return.
 *
 * The handler is a DECLARED environment input: the service receives RAM and the CPU at handler entry
 * (CS:IP = the vector, SP at the fake INT frame, IF clear, DF clear) and returns the complete CPU at
 * the continuation 0A32:0045 (same CS as the body, IP 0045h, same SS, SP = entry SP + 6). Any other
 * result, or a refusal, stops with SERVICE at the handler entry (nothing after it applied). The service
 * may write RAM (a real handler does); the model rereads every operand after it in source order.
 *
 * Flags handed to the handler: those of the second SHL BX,1 (CF OF SF 0, ZF and PF from IntNo*4,
 * AF = 1 as DOSBox-X computes it for a non-zero shift; Unicorn gives 0: declared split), DF and IF
 * clear, TF and the reserved bits as on entry. Words are linear-contiguous with 1 MiB wrap; the
 * entry SP guard [0014h,FFF6h] keeps the whole frame inside SS without crossing offset FFFFh. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_dos_intr_cpu;

typedef enum {
    PR_DOS_INTR_RETURNED = 0,  /* RETF 6 executed */
    PR_DOS_INTR_ARGUMENT,      /* null RAM/CPU/service, RAM not exactly 1 MiB, IP not 000Bh, or SP outside
                                  [0014h,FFF6h]; nothing changed */
    PR_DOS_INTR_SERVICE        /* the service refused or returned a non-continuation; stopped at handler entry */
} pr_dos_intr_status;

/* Returns 0 with *after filled (at 0A32:0045), or nonzero to refuse. */
typedef int (*pr_dos_intr_service)(void *context, uint8_t *ram, const pr_dos_intr_cpu *before, pr_dos_intr_cpu *after);

pr_dos_intr_status pr_dos_intr_000b(uint8_t *ram, size_t ram_size, pr_dos_intr_cpu *cpu,
                                    pr_dos_intr_service service, void *context);
#endif
