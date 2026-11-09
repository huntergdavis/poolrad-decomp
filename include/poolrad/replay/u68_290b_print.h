#ifndef POOLRAD_REPLAY_U68_290B_PRINT_H
#define POOLRAD_REPLAY_U68_290B_PRINT_H
#include "poolrad/replay/u68_290b_clear.h"
#include "poolrad/recovered/resident_0709_printer_0626_cpu.h"
typedef struct { size_t printer_loopbacks,glyph_loopbacks; } pr_290b_print_limits;
typedef struct { uint16_t cs,ip,sp,bp,ss; } pr_290b_return_target;
typedef struct { int status;pr_r0709_printer_cpu_diagnostic printer; } pr_290b_print_diagnostic;
pr_290b_first_status pr_290b_print_return(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_290b_print_limits *limits,const pr_290b_return_target *target,pr_290b_print_diagnostic *diagnostic);
typedef enum { PR_U68_290B_PRINT_EMPTY=0,PR_U68_290B_PRINT_WAITING,PR_U68_290B_PRINT_FINISHED } pr_u68_290b_print_phase;
typedef enum { PR_U68_290B_PRINT_YIELD=0,PR_U68_290B_PRINT_RETURNED,PR_U68_290B_PRINT_ARGUMENT,PR_U68_290B_PRINT_STOP } pr_u68_290b_print_status;
typedef struct { pr_u68_290b_print_phase phase;pr_u68_290b_clear clear;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_print_limits limits;pr_290b_return_target target; } pr_u68_290b_print;
typedef struct { pr_u68_290b_clear_status clear_status;pr_u68_290b_clear_diagnostic clear;pr_290b_first_status print_status;pr_290b_print_diagnostic print; } pr_u68_290b_print_diagnostic;
void pr_u68_290b_print_init(pr_u68_290b_print *state);
pr_u68_290b_print_status pr_u68_290b_print_start(pr_u68_290b_print *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,pr_u68_290b_print_diagnostic *diagnostic);
pr_u68_290b_print_status pr_u68_290b_print_resume(pr_u68_290b_print *state,pr_u68_290b_print_diagnostic *diagnostic);
#endif
