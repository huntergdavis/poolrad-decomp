#ifndef POOLRAD_REPLAY_U25_OPERAND_PREFIX_H
#define POOLRAD_REPLAY_U25_OPERAND_PREFIX_H
#include "poolrad/replay/u1c_save_loader_entry.h"
pr_290b_first_status pr_u25_operand_prefix(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
typedef enum { PR_U25_OPERAND_EMPTY=0,PR_U25_OPERAND_WAITING,PR_U25_OPERAND_FINISHED } pr_u25_operand_phase;
typedef enum { PR_U25_OPERAND_YIELD=0,PR_U25_OPERAND_REACHED,PR_U25_OPERAND_ZERO,PR_U25_OPERAND_LOOP,PR_U25_OPERAND_OTHER,PR_U25_OPERAND_ARGUMENT,PR_U25_OPERAND_STOP } pr_u25_operand_status;
typedef struct { pr_u25_operand_phase phase;pr_u1c_save_session save;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u25_operand_session;
typedef struct { pr_u1c_save_status save_status;pr_u1c_save_diagnostic save;pr_290b_first_status prefix_status; } pr_u25_operand_diagnostic;
void pr_u25_operand_session_init(pr_u25_operand_session *state);
pr_u25_operand_status pr_u25_operand_start(pr_u25_operand_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,pr_u25_operand_diagnostic *diagnostic);
pr_u25_operand_status pr_u25_operand_resume(pr_u25_operand_session *state,pr_u25_operand_diagnostic *diagnostic);
#endif
