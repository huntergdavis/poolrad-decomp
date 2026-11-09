#ifndef POOLRAD_REPLAY_U1C_SAVE_WORD_ENTRY_H
#define POOLRAD_REPLAY_U1C_SAVE_WORD_ENTRY_H
#include "poolrad/replay/u25_operand_return.h"
pr_290b_first_status pr_u1c_save_word_entry(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_SAVE_WORD_EMPTY=0,PR_SAVE_WORD_WAITING,PR_SAVE_WORD_FINISHED } pr_save_word_phase;
typedef enum { PR_SAVE_WORD_YIELD=0,PR_SAVE_WORD_REACHED,PR_SAVE_WORD_STRING,PR_SAVE_WORD_LIMIT,PR_SAVE_WORD_PROMPT,PR_SAVE_WORD_OTHER,PR_SAVE_WORD_ARGUMENT,PR_SAVE_WORD_STOP } pr_save_word_status;
typedef struct { pr_save_word_phase phase;pr_u25_return_session loader;pr_bus *bus;pr_r0899_cpu *cpu; } pr_save_word_session;
typedef struct { pr_u25_return_status loader_status;pr_u25_return_diagnostic loader;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_save_word_diagnostic;
void pr_save_word_session_init(pr_save_word_session *state);
pr_save_word_status pr_save_word_start(pr_save_word_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_save_word_diagnostic *diagnostic);
pr_save_word_status pr_save_word_resume(pr_save_word_session *state,pr_save_word_diagnostic *diagnostic);
#endif
