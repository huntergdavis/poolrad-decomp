#ifndef POOLRAD_RECOVERED_OVERLAY_LOADED_STUBS_H
#define POOLRAD_RECOVERED_OVERLAY_LOADED_STUBS_H
#include <stddef.h>
#include <stdint.h>

/* Read-only preflight for immutable loaded bindings in 1 MiB A20-off RAM.
 * The caller validates RAM first. No CPU or RAM writes occur here. */
static inline size_t pr_stub_physical(uint16_t segment, uint16_t offset)
{
    return (((size_t)segment << 4u) + offset) & 0xFFFFFu;
}
static inline uint16_t pr_stub_word(const uint8_t *ram, uint16_t segment, uint16_t offset)
{
    size_t at = pr_stub_physical(segment, offset);
    return (uint16_t)(ram[at] | ((uint16_t)ram[(at + 1u) & 0xFFFFFu] << 8u));
}
static inline int pr_overlay_stub_target(const uint8_t *ram, uint16_t caller_cs,
    uint16_t call_ip, uint16_t stub_ip, uint16_t entry_ip, uint16_t *target_cs)
{
    uint16_t stub_cs;
    if (ram[pr_stub_physical(caller_cs, call_ip)] != 0x9Au ||
        pr_stub_word(ram, caller_cs, (uint16_t)(call_ip + 1u)) != stub_ip)
        return 0;
    stub_cs = pr_stub_word(ram, caller_cs, (uint16_t)(call_ip + 3u));
    if (ram[pr_stub_physical(stub_cs, stub_ip)] != 0xEAu ||
        pr_stub_word(ram, stub_cs, (uint16_t)(stub_ip + 1u)) != entry_ip)
        return 0;
    if (target_cs != NULL)
        *target_cs = pr_stub_word(ram, stub_cs, (uint16_t)(stub_ip + 3u));
    return 1;
}
static inline int pr_overlay_stub_matches(const uint8_t *ram, uint16_t caller_cs,
    uint16_t call_ip, uint16_t stub_ip, uint16_t entry_ip, uint16_t target_cs)
{
    uint16_t loaded_cs;
    return pr_overlay_stub_target(ram, caller_cs, call_ip, stub_ip, entry_ip, &loaded_cs) &&
           loaded_cs == target_cs;
}
#endif
