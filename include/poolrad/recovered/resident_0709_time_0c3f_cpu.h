#ifndef POOLRAD_RECOVERED_RESIDENT_0709_TIME_0C3F_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_0709_TIME_0C3F_CPU_H
#include "poolrad/recovered/resident_08ae_time_0000_cpu.h"

/* 0709:0C3F clock, using the 08AE clock's CPU/status/diagnostic types.
 * The DOS service must obey its no-RAM-write contract. With an attached EGA,
 * reject before execution if any byte in the conservative stack footprint
 * overlaps A0000..AFFFF. Code operands are read through the live bus.
 * See docs/re/RES_0709_TIME_0C3F_CPU.md. */
pr_r08ae_time_cpu_status pr_r0709_time_0c3f_cpu(pr_bus *bus, size_t ram_size, pr_r08ae_time_cpu *cpu,
                                                pr_dos_gettime_service service, void *service_context,
                                                pr_r08ae_time_cpu_diagnostic *diagnostic);
#endif
