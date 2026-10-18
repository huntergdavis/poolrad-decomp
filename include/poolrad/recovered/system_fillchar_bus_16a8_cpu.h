#ifndef POOLRAD_RECOVERED_SYSTEM_FILLCHAR_BUS_16A8_CPU_H
#define POOLRAD_RECOVERED_SYSTEM_FILLCHAR_BUS_16A8_CPU_H

#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/system_runtime_leaves_cpu.h"

/* The SYSTEM FillChar at START.EXE 0AF8:16A8..16BB (20 bytes, 7 instructions) with every memory access on a pr_bus, so
 * that a fill of video memory goes through the EGA write pipeline: MOV BX,SP; LES DI,SS:[BX+8]; MOV CX,SS:[BX+6]; MOV
 * AL,SS:[BX+4]; CLD; REP STOSB; RETF 8. Its CPU effects are those of the covered pr_system_fillchar in
 * system_runtime_leaves_cpu.h: BX = the entry SP, ES:DI past the fill (DI wraps at 16 bits), CX = 0, AL = the value (AH
 * kept), DF clear, no other flag changed. Null bus, RAM or CPU returns ARGUMENT before any change. */
pr_system_leaf_status pr_system_fillchar_bus_16a8(pr_bus *bus, pr_system_leaf_cpu *cpu);

#endif
