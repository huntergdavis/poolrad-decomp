#include "tools/replay/u19_row14_generic_assignment_operation.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#define CHECK(expression) do { \
    if (!(expression)) { \
        std::fprintf(stderr, "CHECK failed at %s:%d: %s\n", \
                     __FILE__, __LINE__, #expression); \
        std::abort(); \
    } \
} while (false)

using namespace poolrad_u19_generic_assignment;

static Input selected_input(const uint8_t* backing, std::size_t size) {
    Input in{};
    in.armed = Frame{0x0F2D, 0x0445, 0xA000, 0x118F};
    in.observed = Frame{0x0F2D, 0x0449, 0xA000, 0x118F};
    in.handler = Handler::uvga;
    in.guest_linear = 0xA118F;
    in.final_planeaddr = 1;
    in.width = 1;
    in.mapping_bound = true;
    in.backing = backing;
    in.backing_bytes = size;
    return in;
}

static void changing_and_equal_source_samples() {
    std::array<uint8_t, 12> backing{{0, 0, 0, 0, 1, 2, 3, 4}};
    Input in = selected_input(backing.data(), backing.size());
    Report changed{};
    changed.enabled = true;
    unsigned writes = 0;
    CHECK(observe(changed, in, [&] { ++writes; backing[6] = 9; }) == Stop::none);
    CHECK(writes == 1 && changed.reached_store && changed.sample_complete);
    CHECK((changed.before == std::array<uint8_t, 4>{{1, 2, 3, 4}}));
    CHECK((changed.after == std::array<uint8_t, 4>{{1, 2, 9, 4}}));
    CHECK(changed.final_planeaddr == 1 && changed.operation_calls == 1);
    CHECK(finish(changed) == Stop::none);

    Report equal{};
    equal.enabled = true;
    CHECK(observe(equal, in, [&] { ++writes; }) == Stop::none);
    CHECK(writes == 2 && equal.before == equal.after);
    CHECK(finish(equal) == Stop::none); // Source-model no-op, not a native receipt.
}

static void rejected_input_preserves_original_store(Input in, Stop expected) {
    Report report{};
    report.enabled = true;
    unsigned writes = 0;
    CHECK(observe(report, in, [&] { ++writes; }) == expected);
    CHECK(writes == 1 && report.reached_store && !report.sample_complete);
    CHECK(report.operation_calls == 1 && finish(report) == expected);
}

static void frame_branch_target_width_and_bounds() {
    std::array<uint8_t, 8> backing{};
    Input in = selected_input(backing.data(), backing.size());

    Input bad = in;
    bad.armed.cs = 0x0F2E;
    rejected_input_preserves_original_store(bad, Stop::wrong_frame);
    bad = in;
    bad.observed.ip = 0x044A;
    rejected_input_preserves_original_store(bad, Stop::wrong_frame);
    bad = in;
    bad.handler = Handler::uvga_fast;
    rejected_input_preserves_original_store(bad, Stop::wrong_branch);
    bad = in;
    bad.guest_linear = 0xA318F; // A different guest page may alias the index.
    rejected_input_preserves_original_store(bad, Stop::wrong_target);
    bad = in;
    bad.width = 2;
    rejected_input_preserves_original_store(bad, Stop::wrong_width);
    bad = in;
    bad.mapping_bound = false;
    rejected_input_preserves_original_store(bad, Stop::unbound_mapping);
    bad = in;
    bad.final_planeaddr = 2;
    rejected_input_preserves_original_store(bad, Stop::invalid_backing);
    bad = in;
    bad.backing = nullptr;
    rejected_input_preserves_original_store(bad, Stop::invalid_backing);
    bad = in;
    bad.backing_bytes = 7; // Index 1 needs four complete bytes [4,8).
    rejected_input_preserves_original_store(bad, Stop::invalid_backing);
}

static void missing_duplicate_and_mode_toggle() {
    std::array<uint8_t, 8> backing{};
    Input in = selected_input(backing.data(), backing.size());

    Report missing{};
    missing.enabled = true;
    CHECK(finish(missing) == Stop::missing_operation);

    Report twice{};
    twice.enabled = true;
    unsigned writes = 0;
    CHECK(observe(twice, in, [&] { ++writes; }) == Stop::none);
    CHECK(observe(twice, in, [&] { ++writes; }) == Stop::duplicate_operation);
    CHECK(writes == 2 && twice.operation_calls == 2);
    CHECK(finish(twice) == Stop::duplicate_operation);

    Report disabled_first{};
    CHECK(observe(disabled_first, in, [&] { ++writes; }) == Stop::disabled);
    CHECK(disabled_first.operation_calls == 1 && !disabled_first.reached_store);
    disabled_first.enabled = true;
    CHECK(observe(disabled_first, in, [&] { ++writes; }) ==
           Stop::duplicate_operation);
    CHECK(writes == 4 && finish(disabled_first) == Stop::duplicate_operation);

    Report enabled_disabled_enabled{};
    enabled_disabled_enabled.enabled = true;
    CHECK(observe(enabled_disabled_enabled, in, [&] { ++writes; }) == Stop::none);
    enabled_disabled_enabled.enabled = false;
    CHECK(observe(enabled_disabled_enabled, in, [&] { ++writes; }) ==
           Stop::disabled);
    enabled_disabled_enabled.enabled = true;
    CHECK(observe(enabled_disabled_enabled, in, [&] { ++writes; }) ==
           Stop::duplicate_operation);
    CHECK(writes == 7 && enabled_disabled_enabled.operation_calls == 2);
    CHECK(finish(enabled_disabled_enabled) == Stop::duplicate_operation);

    Report disabled_only{};
    CHECK(observe(disabled_only, in, [&] { ++writes; }) == Stop::disabled);
    disabled_only.enabled = true;
    CHECK(finish(disabled_only) == Stop::missing_operation);
    CHECK(writes == 8);
}

static void reentrant_store_is_not_a_single_operation() {
    std::array<uint8_t, 8> backing{};
    Input in = selected_input(backing.data(), backing.size());
    Report report{};
    report.enabled = true;
    unsigned writes = 0;
    const Stop outer = observe(report, in, [&] {
        ++writes;
        const Stop inner = observe(report, in, [&] { ++writes; });
        CHECK(inner == Stop::duplicate_operation);
    });
    CHECK(outer == Stop::duplicate_operation);
    CHECK(writes == 2 && report.operation_calls == 2);
    CHECK(finish(report) == Stop::duplicate_operation);
}

int main() {
    changing_and_equal_source_samples();
    frame_branch_target_width_and_bounds();
    missing_duplicate_and_mode_toggle();
    reentrant_store_is_not_a_single_operation();
    return 0;
}
