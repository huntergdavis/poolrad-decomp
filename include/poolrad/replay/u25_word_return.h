#ifndef POOLRAD_REPLAY_U25_WORD_RETURN_H
#define POOLRAD_REPLAY_U25_WORD_RETURN_H
#include "poolrad/replay/u1c_save_word_entry.h"
pr_290b_first_status pr_u25_word_return(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_290b_return_target *target);
typedef enum { PR_WORD_RETURN_EMPTY=0,PR_WORD_RETURN_WAITING,PR_WORD_RETURN_FINISHED } pr_word_return_phase;
typedef enum { PR_WORD_RETURN_YIELD=0,PR_WORD_RETURN_REACHED,PR_WORD_RETURN_STRING,PR_WORD_RETURN_LIMIT,PR_WORD_RETURN_PROMPT,PR_WORD_RETURN_OTHER,PR_WORD_RETURN_ARGUMENT,PR_WORD_RETURN_STOP } pr_word_return_status;
typedef struct { pr_word_return_phase phase;pr_save_word_session word;pr_bus *bus;pr_r0899_cpu *cpu;pr_290b_return_target target; } pr_word_return_session;
typedef struct { pr_save_word_status word_status;pr_save_word_diagnostic word;pr_290b_first_status return_status; } pr_word_return_diagnostic;
void pr_word_return_session_init(pr_word_return_session *state);
pr_word_return_status pr_word_return_start(pr_word_return_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_word_return_diagnostic *diagnostic);
pr_word_return_status pr_word_return_resume(pr_word_return_session *state,pr_word_return_diagnostic *diagnostic);
#endif
