#ifndef POOLRAD_REPLAY_U25_STORE_CLASSIFIER_ENTRY_H
#define POOLRAD_REPLAY_U25_STORE_CLASSIFIER_ENTRY_H
#include "poolrad/replay/u1c_save_store_entry.h"
pr_290b_first_status pr_u25_store_classifier_entry(pr_bus *bus,size_t size,pr_r0899_cpu *cpu,pr_290b_first_diagnostic *diagnostic);
typedef enum { PR_STORE_CLASS_EMPTY=0,PR_STORE_CLASS_WAITING,PR_STORE_CLASS_FINISHED } pr_store_class_phase;
typedef enum { PR_STORE_CLASS_YIELD=0,PR_STORE_CLASS_REACHED,PR_STORE_CLASS_ZERO,PR_STORE_CLASS_WORD,PR_STORE_CLASS_OTHER_TYPE,PR_STORE_CLASS_STRING,PR_STORE_CLASS_LOADER_STRING,PR_STORE_CLASS_LIMIT,PR_STORE_CLASS_PROMPT,PR_STORE_CLASS_OTHER,PR_STORE_CLASS_ARGUMENT,PR_STORE_CLASS_STOP } pr_store_class_status;
typedef struct { pr_store_class_phase phase;pr_save_store_session store;pr_bus *bus;pr_r0899_cpu *cpu; } pr_store_class_session;
typedef struct { pr_save_store_status store_status;pr_save_store_diagnostic store;pr_290b_first_status entry_status;pr_290b_first_diagnostic entry; } pr_store_class_diagnostic;
void pr_store_class_session_init(pr_store_class_session *state);
pr_store_class_status pr_store_class_start(pr_store_class_session *state,pr_bus *bus,size_t size,pr_r0899_cpu *cpu,const pr_u3b0_clock_draw_env *env,const pr_290b_clear_limits *clear_limits,const pr_290b_print_limits *print_limits,const pr_290b_return_target *target,size_t max_iterations,pr_store_class_diagnostic *diagnostic);
pr_store_class_status pr_store_class_resume(pr_store_class_session *state,pr_store_class_diagnostic *diagnostic);
#endif
