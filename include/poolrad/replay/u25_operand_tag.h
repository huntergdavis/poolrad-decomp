#ifndef POOLRAD_REPLAY_U25_OPERAND_TAG_H
#define POOLRAD_REPLAY_U25_OPERAND_TAG_H
#include "poolrad/replay/u25_operand_prefix.h"
pr_290b_first_status pr_u25_operand_tag(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
typedef enum { PR_U25_TAG_EMPTY=0,PR_U25_TAG_WAITING,PR_U25_TAG_FINISHED } pr_u25_tag_phase;
typedef enum { PR_U25_TAG_YIELD=0,PR_U25_TAG_REACHED,PR_U25_TAG_STRING,PR_U25_TAG_LOOP,PR_U25_TAG_OTHER,PR_U25_TAG_ARGUMENT,PR_U25_TAG_STOP } pr_u25_tag_status;
typedef struct { pr_u25_tag_phase phase;pr_u25_operand_session operand;pr_bus *bus;pr_r0899_cpu *cpu; } pr_u25_tag_session;
typedef struct { pr_u25_operand_status operand_status;pr_u25_operand_diagnostic operand;pr_290b_first_status tag_status; } pr_u25_tag_diagnostic;
void pr_u25_tag_session_init(pr_u25_tag_session *state);
pr_u25_tag_status pr_u25_tag_start(pr_u25_tag_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,pr_u25_tag_diagnostic *diagnostic);
pr_u25_tag_status pr_u25_tag_resume(pr_u25_tag_session *state,pr_u25_tag_diagnostic *diagnostic);
#endif
