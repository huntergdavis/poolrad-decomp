#ifndef POOLRAD_REPLAY_U1C_SIXTH_COMPARE_TAG_BRANCH_H
#define POOLRAD_REPLAY_U1C_SIXTH_COMPARE_TAG_BRANCH_H
#include "poolrad/replay/u25_sixth_compare_loader_return.h"
#include "poolrad/replay/u25_operand_return.h"
pr_290b_first_status pr_u1c_compare_tag_branch(pr_bus *bus,size_t size,pr_r0899_cpu *cpu);
enum { PR_SIXTH_COMPARE_TAG_NUMERIC=PR_SIXTH_COMPARE_LOADER_LIMIT+1,PR_SIXTH_COMPARE_TAG_STRING };
typedef enum { PR_SIXTH_COMPARE_TAG_EMPTY=0,PR_SIXTH_COMPARE_TAG_WAITING,PR_SIXTH_COMPARE_TAG_FINISHED } pr_sixth_compare_tag_phase;
typedef struct { pr_sixth_compare_tag_phase phase;pr_sixth_compare_loader_session loader;pr_bus *bus;pr_r0899_cpu *cpu; } pr_sixth_compare_tag_session;
typedef struct { pr_second_dispatch_status loader_status;pr_sixth_compare_loader_diagnostic loader;pr_290b_first_status branch_status; } pr_sixth_compare_tag_diagnostic;
void pr_sixth_compare_tag_session_init(pr_sixth_compare_tag_session *state);
pr_second_dispatch_status pr_sixth_compare_tag_start(pr_sixth_compare_tag_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_sixth_compare_tag_diagnostic *diagnostic);
pr_second_dispatch_status pr_sixth_compare_tag_resume(pr_sixth_compare_tag_session *state,pr_sixth_compare_tag_diagnostic *diagnostic);
#endif
