#include "poolrad/replay/slot_a_probe_return_19ad_abi.h"

pr_slot_a_probe_return_status pr_slot_a_probe_return_19ad_step(
    pr_slot_a_directory_state *s)
{
    pr_slot_a_directory_cpu *c;
    unsigned value, parity;
    if (!s) return PR_SLOT_A_PROBE_RETURN_ARGUMENT;
    if (!s->ram || s->ram_size > 0x100000u) return PR_SLOT_A_PROBE_RETURN_BLOCKED_MEMORY;
    c = &s->cpu;
    if (c->cs != 0x2A8Eu) return PR_SLOT_A_PROBE_RETURN_BLOCKED_PC;
    switch (c->ip) {
    case 0x19ADu:
        value = c->ax & 255u; parity = value;
        parity ^= parity >> 4; parity ^= parity >> 2; parity ^= parity >> 1;
        c->flags = (uint16_t)(c->flags & (uint16_t)~0x08D5u);
        if (!(parity & 1u)) c->flags |= 4u;
        if (!value) c->flags |= 0x40u;
        if (value & 0x80u) c->flags |= 0x80u;
        c->ip = 0x19AFu;
        return PR_SLOT_A_PROBE_RETURN_STEPPED;
    case 0x19AFu:
        c->ip = c->flags & 0x40u ? 0x19EAu : 0x19B1u;
        return PR_SLOT_A_PROBE_RETURN_STEPPED;
    case 0x19B1u: return PR_SLOT_A_PROBE_RETURN_BLOCKED_PRESENT_CONTINUATION;
    case 0x19EAu: return PR_SLOT_A_PROBE_RETURN_BLOCKED_LOOP_GATE;
    default: return PR_SLOT_A_PROBE_RETURN_BLOCKED_PC;
    }
}
