#ifndef POOLRAD_REPLAY_RESIDENT_1037_GUARDED_H
#define POOLRAD_REPLAY_RESIDENT_1037_GUARDED_H
#include "poolrad/recovered/memory_bus.h"
#include "poolrad/recovered/resident_00ba_byte_string_1037_cpu.h"
typedef enum { PR_RES_1037_GUARDED_OK=0, PR_RES_1037_GUARDED_ARGUMENT, PR_RES_1037_GUARDED_CODE, PR_RES_1037_GUARDED_CALLEE, PR_RES_1037_GUARDED_TRANSFER, PR_RES_1037_GUARDED_PROFILE } pr_res_1037_guarded_status;
/* Flat-leaf bounded profile: exact 1 MiB, ordinary caller CS/SS, SP
 * [0200,FFF0]. With EGA, each reached flat call requires ordinary live CS;
 * StrStore also requires an ordinary live destination. Stops retain effects.
 * See docs/re/RES_290B_RECORD_BRANCH.md. */
pr_res_1037_guarded_status pr_res_00ba_1037_guarded(pr_bus *bus, size_t size, pr_res_1037_cpu *cpu, pr_res_1037_callee *callee, int *callee_status);
#endif
