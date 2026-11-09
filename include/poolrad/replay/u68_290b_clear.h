#ifndef POOLRAD_REPLAY_U68_290B_CLEAR_H
#define POOLRAD_REPLAY_U68_290B_CLEAR_H
#include "poolrad/replay/u68_290b_mode.h"
#include "poolrad/recovered/resident_03f1_clear_0137_cpu.h"
typedef struct { size_t clear_loopbacks,rect_loopbacks; } pr_290b_clear_limits;
typedef struct { int status;pr_r03f1_clear_cpu_diagnostic clear; } pr_290b_clear_diagnostic;
pr_290b_first_status pr_290b_clear(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_290b_clear_limits *limits,pr_290b_clear_diagnostic *diagnostic);
typedef enum { PR_U68_290B_CLEAR_EMPTY=0,PR_U68_290B_CLEAR_WAITING,PR_U68_290B_CLEAR_FINISHED } pr_u68_290b_clear_phase;
typedef enum { PR_U68_290B_CLEAR_YIELD=0,PR_U68_290B_CLEAR_REACHED,PR_U68_290B_CLEAR_ARGUMENT,PR_U68_290B_CLEAR_STOP } pr_u68_290b_clear_status;
typedef struct { pr_u68_290b_clear_phase phase;pr_u68_290b_mode mode;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_clear_limits limits; } pr_u68_290b_clear;
typedef struct { pr_u68_290b_mode_status mode_status;pr_u68_290b_mode_diagnostic mode;pr_290b_first_status clear_status;pr_290b_clear_diagnostic clear; } pr_u68_290b_clear_diagnostic;
void pr_u68_290b_clear_init(pr_u68_290b_clear *state);
pr_u68_290b_clear_status pr_u68_290b_clear_start(pr_u68_290b_clear *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *limits,pr_u68_290b_clear_diagnostic *diagnostic);
pr_u68_290b_clear_status pr_u68_290b_clear_resume(pr_u68_290b_clear *state,pr_u68_290b_clear_diagnostic *diagnostic);
#endif
