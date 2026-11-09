#ifndef POOLRAD_REPLAY_U1C_SAVE_TAG_BRANCH_H
#define POOLRAD_REPLAY_U1C_SAVE_TAG_BRANCH_H
#include "poolrad/replay/u25_word_return.h"
pr_290b_first_status pr_u1c_save_tag_branch(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
typedef enum { PR_SAVE_TAG_EMPTY=0,PR_SAVE_TAG_WAITING,PR_SAVE_TAG_FINISHED } pr_save_tag_phase;
typedef enum { PR_SAVE_TAG_YIELD=0,PR_SAVE_TAG_NUMERIC,PR_SAVE_TAG_STRING,PR_SAVE_TAG_LOADER_STRING,PR_SAVE_TAG_LIMIT,PR_SAVE_TAG_PROMPT,PR_SAVE_TAG_OTHER,PR_SAVE_TAG_ARGUMENT,PR_SAVE_TAG_STOP } pr_save_tag_status;
typedef struct { pr_save_tag_phase phase;pr_word_return_session word;pr_bus *bus;pr_r0899_cpu *cpu; } pr_save_tag_session;
typedef struct { pr_word_return_status word_status;pr_word_return_diagnostic word;pr_290b_first_status branch_status; } pr_save_tag_diagnostic;
void pr_save_tag_session_init(pr_save_tag_session *state);
pr_save_tag_status pr_save_tag_start(pr_save_tag_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_save_tag_diagnostic *diagnostic);
pr_save_tag_status pr_save_tag_resume(pr_save_tag_session *state,pr_save_tag_diagnostic *diagnostic);
#endif
