#include "poolrad/replay/slot_a_compare_runner_next.h"
#include "poolrad/replay/slot_a_save_runner_jump.h"
#include "poolrad/replay/slot_a_save_runner_loop.h"
#include "poolrad/replay/slot_a_runner_opcode_prefix.h"
#include "poolrad/replay/slot_a_runner_opcode_fetch.h"
pr_compare_runner_status pr_slot_a_compare_runner_next(uint8_t *ram,size_t size,const uint8_t *loop,const uint8_t *fetch,pr_u1c_3171_cpu *cpu)
{
    if(!ram||size!=0x100000u||!loop||!fetch||!cpu||cpu->ip!=0x31a8u)return PR_COMPARE_RUNNER_ARGUMENT;
    if(!pr_slot_a_save_runner_jump(cpu,ram,size))return PR_COMPARE_RUNNER_JUMP_REFUSED;
    if(pr_slot_a_save_runner_loop(ram,size,loop,14,cpu))return PR_COMPARE_RUNNER_LOOP_REFUSED;
    if(cpu->ip!=0x318du)return PR_COMPARE_RUNNER_EXIT;
    if(pr_slot_a_runner_opcode_prefix(ram,size,cpu)!=0x319au)return PR_COMPARE_RUNNER_PREFIX_STOP;
    if(pr_slot_a_runner_opcode_fetch(ram,size,fetch,10,cpu))return PR_COMPARE_RUNNER_FETCH_REFUSED;
    return PR_COMPARE_RUNNER_FETCHED;
}
