#ifndef POOLRAD_U19_ROW14_GENERIC_ASSIGNMENT_OPERATION_HPP
#define POOLRAD_U19_ROW14_GENERIC_ASSIGNMENT_OPERATION_HPP

#include <array>
#include <cstddef>
#include <cstdint>

#include "u19_row14_in_handler_capture.hpp"

namespace poolrad_u19_generic_assignment {

using poolrad_u19_capture::Frame;
using poolrad_u19_capture::Handler;

enum class Stop : uint8_t {
    none, disabled, missing_operation, duplicate_operation, wrong_frame,
    wrong_branch, wrong_target, wrong_width, unbound_mapping, invalid_backing
};

// final_planeaddr is the word index after the generic helper's live mask
// and odd/even remap. It is neither a guest linear address nor the index
// supplied to VGA_Generic_Write_Handler<false> at entry.
struct Input {
    Frame armed{}, observed{};
    Handler handler = Handler::unsupported;
    uint32_t guest_linear = 0, final_planeaddr = 0;
    unsigned width = 0;
    bool mapping_bound = false; // Caller links this final index to this guest attempt.
    const uint8_t* backing = nullptr;
    std::size_t backing_bytes = 0;
};

struct Report {
    bool enabled = false; // Source-only, default-off observation.
    Stop stop = Stop::none;
    uint32_t operation_calls = 0; // Saturated at 2; includes disabled calls.
    bool reached_store = false; // Supplied callback returned; not native proof.
    bool sample_complete = false;
    Frame armed{}, observed{};
    uint32_t guest_linear = 0, final_planeaddr = 0;
    std::array<uint8_t, 4> before{}, after{};
};

inline bool selected_frame(const Frame& armed, const Frame& observed) {
    const uint16_t successor = poolrad_u19_capture::successor_for(armed.ip);
    return armed.cs == 0x0F2Du && successor != 0u &&
           armed.es == 0xA000u && armed.di == 0x118Fu &&
           observed.cs == armed.cs &&
           (observed.ip == armed.ip || observed.ip == successor) &&
           observed.es == armed.es && observed.di == armed.di;
}

inline void copy_word_bytes(std::array<uint8_t, 4>& out,
                            const uint8_t* backing, uint32_t index) {
    const std::size_t offset = std::size_t(index) * 4u;
    for (std::size_t byte = 0; byte < out.size(); ++byte)
        out[byte] = backing[offset + byte];
}

// Call only at the original generic helper's final assignment, after the
// unchained parent's trigger at vga_memory.cpp:590 and after ModeOperation
// and final index remap. The adapter never invokes or repeats that trigger.
// original_store is the exact assignment at :383, not the whole helper.
// It must be nonthrowing and runs once even on disabled/rejected paths.
template <typename OriginalStore>
inline Stop observe(Report& out, const Input& in, OriginalStore original_store) {
    if (out.operation_calls < 2u) ++out.operation_calls;
    if (!out.enabled) {
        original_store();
        return Stop::disabled;
    }

    Stop error = Stop::none;
    if (out.operation_calls != 1u || out.stop != Stop::none)
        error = Stop::duplicate_operation;
    else if (!selected_frame(in.armed, in.observed))
        error = Stop::wrong_frame;
    else if (in.handler != Handler::uvga)
        error = Stop::wrong_branch;
    else if (in.guest_linear != 0xA118Fu)
        error = Stop::wrong_target;
    else if (in.width != 1u)
        error = Stop::wrong_width;
    else if (!in.mapping_bound)
        error = Stop::unbound_mapping;
    else if (in.backing == nullptr ||
             std::size_t(in.final_planeaddr) >= in.backing_bytes / 4u)
        error = Stop::invalid_backing;

    if (error == Stop::none) {
        out.armed = in.armed;
        out.observed = in.observed;
        out.guest_linear = in.guest_linear;
        out.final_planeaddr = in.final_planeaddr;
        copy_word_bytes(out.before, in.backing, in.final_planeaddr);
    }

    original_store();
    out.reached_store = true;
    if (out.operation_calls != 1u) error = Stop::duplicate_operation;
    else if (out.stop != Stop::none) error = out.stop;
    if (error != Stop::none) {
        out.stop = error;
        out.sample_complete = false;
        return error;
    }

    copy_word_bytes(out.after, in.backing, in.final_planeaddr);
    out.sample_complete = true;
    return Stop::none;
}

inline Stop finish(Report& out) {
    if (!out.enabled) return Stop::disabled;
    if (out.operation_calls == 0u) return out.stop = Stop::missing_operation;
    if (out.stop != Stop::none) return out.stop;
    if (out.operation_calls != 1u) return out.stop = Stop::duplicate_operation;
    if (!out.reached_store || !out.sample_complete)
        return out.stop = Stop::missing_operation;
    return Stop::none; // Source-model sample only; no actual_store receipt.
}

} // namespace poolrad_u19_generic_assignment

#endif
