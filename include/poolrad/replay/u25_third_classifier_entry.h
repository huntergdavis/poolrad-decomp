#ifndef POOLRAD_REPLAY_U25_THIRD_CLASSIFIER_ENTRY_H
#define POOLRAD_REPLAY_U25_THIRD_CLASSIFIER_ENTRY_H
#include "poolrad/replay/u1c_third_store_entry.h"
#include "poolrad/replay/u25_store_classifier_entry.h"
typedef enum { PR_THIRD_CLASS_EMPTY=0,PR_THIRD_CLASS_WAITING,PR_THIRD_CLASS_FINISHED } pr_third_class_phase;
typedef struct { pr_third_class_phase phase;pr_third_store_session store;pr_bus *bus;pr_r0899_cpu *cpu; } pr_third_class_session;
typedef struct { pr_second_dispatch_status store_status;pr_third_store_diagnostic store;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_third_class_diagnostic;
void pr_third_class_session_init(pr_third_class_session *state);
pr_second_dispatch_status pr_third_class_start(pr_third_class_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_third_class_diagnostic *diagnostic);
pr_second_dispatch_status pr_third_class_resume(pr_third_class_session *state,pr_third_class_diagnostic *diagnostic);
#endif
