#ifndef POOLRAD_RECOVERED_SYSTEM_BLOCKREAD_14FB_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_BLOCKREAD_14FB_CPU_H
#include <stddef.h>
#include <stdint.h>

#define PR_BLOCKREAD_RAM_SIZE (1u << 20)

/* Raw-CPU, full-state model of Turbo Pascal SYSTEM BlockRead(var f: file; var buf; count: word; result: ^word),
 * START.EXE 0AF8:[14FB,1563) (104 bytes, 45 instructions, RETF 0Eh), BlockRead entry only: the BlockWrite entry
 * 1502h/1504h (MOV BL,40h; MOV CX,65h) is a separate endpoint and excluded. The open check CALL 14B6 composes par-d's
 * shared model (system_filecheck_14b6_cpu). Runtime coverage only, no identity claim (docs/re/SYSTEM_BLOCKREAD_14FB_CPU.md).
 *
 * Entered after a far call: [SP] IP, [SP+2] CS, [SP+4] the far result pointer, [SP+8] count, [SP+0Ah] the far buffer,
 * [SP+0Eh] the far address of f. BL := 3Fh, CX := 100 (the short-read InOutRes). Not open: InOutRes := 103 (the check).
 * count = 0: no INT, AX := 0. Otherwise CX := the LOW WORD of count * recsize (recsize = f+4; the MUL's high word DX is
 * DROPPED), INT 21h AH=3Fh with BX = the handle f+0, DS:DX = buf. Carry: InOutRes := AX. Success: AX := bytes read DIV
 * recsize (DX := 0 first). Then *result := AX (0 after an error or when not open) if result is not nil; when result is
 * nil and AX != count, InOutRes := 100. POP BP; RETF 0Eh.
 *
 * Declared environment input: INT 21h AH=3Fh (fail-closed list: any other AH is ENV). The service receives the CPU at the
 * INT (IP 1529h) and returns the complete CPU after DOS returns at 152Bh (same CS, SS, SP, BP; on no carry AX <= CX, a read
 * returns at most what was asked); otherwise SERVICE, stopped at the INT. The read WRITES RAM, and the contract PERMITS
 * exactly the buffer: the service is handed the CX bytes at linear DS*16+DX (contiguous, 1 MiB wrap, as DOSBox-X's
 * MEM_BlockWrite) and may change any of them, on success or error; the model writes them back. Nothing else may change.
 *
 * DIVIDE: recsize = 0 with count != 0 makes the DIV at 1531h raise INT 0 (Turbo Pascal's handler then ends the program
 * with RunError 200): an EXIT-class stop, with the CPU at the faulting DIV (IP 1531h, DX = 0) and nothing after it run.
 * TRANSFER: the check's RET rereads its slot, so its InOutRes store landing there (SS = DS, entry SP 442Ah) returns elsewhere;
 * CS:IP is where it returned. MUL's undefined SF/ZF/AF/PF are set as DOSBox-X's 386 core does and reach only the service's
 * input flags; DIV's undefined flags are left unchanged; both are overwritten before the body reads flags.
 * Exactly 1 MiB A20-off RAM; IP = 14FBh; entry SP in [0006h,FFEEh] (pushes reach entry SP - 6, arguments entry SP + 11h). */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_blockread_cpu;

typedef enum {
    PR_BLOCKREAD_RETURNED = 0,  /* RETF 0Eh executed */
    PR_BLOCKREAD_ARGUMENT,      /* null RAM/CPU/service, RAM not exactly 1 MiB, IP not 14FBh, SP outside [0006h,FFEEh]; nothing changed */
    PR_BLOCKREAD_SERVICE,       /* the INT 21h service declined or returned a non-continuation; stopped at the INT (IP 1529h) */
    PR_BLOCKREAD_ENV,           /* an INT 21h function other than 3Fh would be issued (structural guard) */
    PR_BLOCKREAD_DIVIDE,        /* EXIT-class: DIV by recsize 0 at 1531h raises INT 0 (RunError 200); CPU at the DIV */
    PR_BLOCKREAD_TRANSFER       /* the composed check returned somewhere other than 1510h; CS:IP is where it returned */
} pr_blockread_status;

/* buffer/length: the permitted write, the CX bytes at linear DS*16+DX as they are at the INT. */
typedef int (*pr_blockread_dos_service)(void *context, const pr_blockread_cpu *before, pr_blockread_cpu *after,
                                        uint8_t *buffer, size_t length);

pr_blockread_status pr_system_blockread_14fb(uint8_t *ram, size_t ram_size, pr_blockread_cpu *cpu,
                                             pr_blockread_dos_service service, void *context);
#endif
