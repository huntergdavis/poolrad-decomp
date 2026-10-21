#ifndef POOLRAD_RECOVERED_RESIDENT_08AE_TIME_0000_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_08AE_TIME_0000_CPU_H
#include <stddef.h>
#include <stdint.h>
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/dos_gettime_00a2_cpu.h"

/* Raw-CPU, full-state model of START.EXE resident 08AE:[0000,006C) (108 bytes,
 * 48 instructions, RETF), a DRIVER_INTERNAL member candidate (never a credit,
 * docs/re/DRIVER_INTERNAL_CLASS.md): the time since midnight in hundredths of a
 * second. It calls Dos.GetTime 0A32:00A2 with four SS locals (hour [BP-6],
 * minute [BP-8], second [BP-0Ah], sec100 [BP-0Ch]), then forms DX:AX :=
 * LongMul(hour, 360000) + minute*6000 + second*100 + sec100 (the SYSTEM LongInt
 * multiply 0AF8:0A5B; 16-bit MULs for the other terms, each zero-extended and
 * added with carry into DX), stores it in [BP-4]/[BP-2] and returns it.
 * GetTime's INT 21h/2Ch is the caller-supplied declared service (passed through).
 * See docs/re/RES_08AE_TIME_0000_CPU.md for the contract and evidence. */
typedef struct {
    uint16_t ax, bx, cx, dx, si, di;
    uint16_t bp, sp, cs, ip, ds, es, ss, flags;
} pr_r08ae_time_cpu;

typedef enum {
    PR_R08AE_TIME_CPU_OK = 0,
    PR_R08AE_TIME_CPU_ARGUMENT,  /* host arguments or entry state rejected (incl. an attached EGA); nothing changed */
    PR_R08AE_TIME_CPU_CALLEE,    /* GetTime or LongMul did not return; the diagnostic names which and its status */
    PR_R08AE_TIME_CPU_PORT,      /* unused (no OUT); shared numbering */
    PR_R08AE_TIME_CPU_TRANSFER,  /* a callee returned somewhere other than the call's next IP */
    PR_R08AE_TIME_CPU_LIMIT      /* unused (no backward jump); shared numbering */
} pr_r08ae_time_cpu_status;

typedef struct {
    int gettime_status;  /* last pr_dos_gettime_status, -1 when not called */
    int longmul_status;  /* last pr_system_longmul_status, -1 when not called */
} pr_r08ae_time_cpu_diagnostic;

pr_r08ae_time_cpu_status pr_r08ae_time_0000_cpu(pr_bus *bus, size_t ram_size, pr_r08ae_time_cpu *cpu,
                                                pr_dos_gettime_service service, void *service_context,
                                                pr_r08ae_time_cpu_diagnostic *diagnostic);
#endif
