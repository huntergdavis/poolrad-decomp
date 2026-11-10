#ifndef POOLRAD_REPLAY_U1C_FIFTH_SAVE_BRANCH_H
#define POOLRAD_REPLAY_U1C_FIFTH_SAVE_BRANCH_H
#include "poolrad/replay/u25_fifth_loader_return.h"
#include "poolrad/replay/u1c_save_word_entry.h"
#include "poolrad/replay/u25_word_return.h"
#include "poolrad/replay/u1c_save_tag_branch.h"
enum { PR_FIFTH_BRANCH_NUMERIC=PR_FIFTH_LOADER_LIMIT+1,PR_FIFTH_BRANCH_SAVE_STRING };
typedef enum { PR_FIFTH_BRANCH_EMPTY=0,PR_FIFTH_BRANCH_WAITING,PR_FIFTH_BRANCH_FINISHED } pr_fifth_branch_phase;
typedef struct { pr_fifth_branch_phase phase;pr_fifth_loader_session loader;pr_bus *bus;pr_r0899_cpu *cpu; } pr_fifth_branch_session;
typedef struct { pr_second_dispatch_status loader_status;pr_fifth_loader_diagnostic loader;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry;pr_290b_first_status word_status;pr_290b_first_status branch_status; } pr_fifth_branch_diagnostic;
void pr_fifth_branch_session_init(pr_fifth_branch_session *state);
pr_second_dispatch_status pr_fifth_branch_start(pr_fifth_branch_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_fifth_branch_diagnostic *diagnostic);
pr_second_dispatch_status pr_fifth_branch_resume(pr_fifth_branch_session *state,pr_fifth_branch_diagnostic *diagnostic);
#endif
