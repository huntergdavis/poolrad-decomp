#ifndef POOLRAD_RECOVERED_SYSTEM_STRSTORE_FULLSTATE_0B25_H
#define POOLRAD_RECOVERED_SYSTEM_STRSTORE_FULLSTATE_0B25_H

#include <stddef.h>
#include <stdint.h>

#define PR_SYSTEM_0B25_RAM_SIZE (1u << 20)

typedef struct {
    uint16_t ax, bx, cx, dx, si, di, bp, sp, cs, ip, ds, es, ss, flags;
} pr_system_0b25_cpu;

typedef enum {
    PR_SYSTEM_0B25_RETURNED = 0,
    PR_SYSTEM_0B25_ARGUMENT  /* null RAM/CPU or RAM that is not exactly 1 MiB; nothing changed */
} pr_system_0b25_status;

/* Raw-CPU, full-state model of the Turbo Pascal SYSTEM string store at
 * loaded 0AF8:0B25..0B48 (START.EXE expanded image, 36 bytes, 16
 * instructions), entered after the far CALL with SS:[SP] return IP, [SP+2]
 * return CS, [SP+4] the maximum length word m, [SP+6]/[SP+8] the
 * destination far pointer D and [SP+0Ah]/[SP+0Ch] the source far pointer S.
 *
 * CLD (DF := 0); BX := SP; DX := DS; LDS SI,S; LES DI,D; CX := m; LODSB
 * reads the source length byte; AL := min(that byte, CL) (CMP AL,CL; JBE);
 * STOSB stores it; CX := AL; REP MOVSB copies that many bytes forward one
 * at a time, so an overlapping destination is read back as written; SI and
 * DI wrap at 16 bits in their segments; DS := DX; RETF 0Ah. AH is kept;
 * BX = the entry SP, CX = 0, DX = the entry DS, ES = D's segment. The final
 * flags are XOR CH,CH's (ZF and PF set; CF, OF, SF clear; AF clear as both
 * oracles leave it). Physical addresses wrap at 1 MiB; LDS/LES pairs and
 * the RETF frame are read physically contiguous. */
pr_system_0b25_status pr_system_strstore_0b25(uint8_t *ram, size_t ram_size, pr_system_0b25_cpu *cpu);

#endif
