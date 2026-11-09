#ifndef POOLRAD_RECOVERED_RESIDENT_00BA_WORD_STRING_1073_CPU_H
#define POOLRAD_RECOVERED_RESIDENT_00BA_WORD_STRING_1073_CPU_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_00ba_byte_string_1037_cpu.h"
typedef pr_res_1037_cpu pr_res_1073_cpu;
typedef pr_res_1037_callee pr_res_1073_callee;
typedef enum { PR_RES_1073_OK=0,PR_RES_1073_ARGUMENT,PR_RES_1073_CODE,
    PR_RES_1073_CALLEE,PR_RES_1073_TRANSFER,PR_RES_1073_PROFILE } pr_res_1073_status;
/* Real 00BA:1073 word formatter. 1 MiB bus; SS and caller CS ordinary,
 * SP [0200,FFF0]. With EGA attached, Str's live CS and StrStore's live CS and destination must be ordinary. See RES_00BA_WORD_STRING_1073.md. */
pr_res_1073_status pr_res_00ba_1073_cpu(pr_bus *bus,size_t ram_size,pr_res_1073_cpu *cpu,
    pr_res_1073_callee *callee,int *callee_status);
#endif
