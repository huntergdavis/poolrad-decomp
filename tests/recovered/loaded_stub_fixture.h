#ifndef POOLRAD_TEST_LOADED_STUB_FIXTURE_H
#define POOLRAD_TEST_LOADED_STUB_FIXTURE_H
#include <stddef.h>
#include <stdint.h>
/* Synthetic instructions only; caller and stub can be relocated independently. */
static inline void test_loaded_stub(uint8_t *ram, uint16_t cs, uint16_t site,
    uint16_t stub_cs, uint16_t stub_ip, uint16_t target_cs, uint16_t target_ip)
{
    const uint8_t call[5] = {0x9A, (uint8_t)stub_ip, (uint8_t)(stub_ip >> 8),
                           (uint8_t)stub_cs, (uint8_t)(stub_cs >> 8)};
    const uint8_t jump[5] = {0xEA, (uint8_t)target_ip, (uint8_t)(target_ip >> 8),
                           (uint8_t)target_cs, (uint8_t)(target_cs >> 8)};
    unsigned i;
    for (i = 0; i < 5u; ++i) {
        ram[(((size_t)cs << 4) + (uint16_t)(site + i)) & 0xFFFFFu] = call[i];
        ram[(((size_t)stub_cs << 4) + (uint16_t)(stub_ip + i)) & 0xFFFFFu] = jump[i];
    }
}
#endif
