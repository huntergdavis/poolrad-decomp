#ifndef POOLRAD_U19_ROW14_IN_HANDLER_CAPTURE_HPP
#define POOLRAD_U19_ROW14_IN_HANDLER_CAPTURE_HPP

#include <array>
#include <cstddef>
#include <cstdint>

namespace poolrad_u19_capture {

enum class Handler : uint8_t { uvga, uvga_fast, unsupported };
enum class Stage : uint8_t { idle, armed, captured, failed, finished };
enum class Stop : uint8_t {
    none, wrong_arm_frame, wrong_target, wrong_width, unsupported_handler,
    invalid_backing, changed_observation_state, wrong_observed_frame,
    duplicate_write,
    missing_write, wrong_successor
};

struct Frame {
    uint16_t cs = 0, ip = 0, es = 0, di = 0;
};

inline bool same_frame(const Frame& a, const Frame& b) {
    return a.cs == b.cs && a.ip == b.ip && a.es == b.es && a.di == b.di;
}

struct GuestState {
    Frame frame{};
    std::array<uint8_t, 10> gfx{};
    std::array<uint8_t, 6> seq{};
    uint32_t latch = 0;
};

inline bool same_state(const GuestState& a, const GuestState& b) {
    return same_frame(a.frame, b.frame) && a.gfx == b.gfx &&
           a.seq == b.seq && a.latch == b.latch;
}

struct Sample {
    GuestState state_before_copy{};
    std::array<uint8_t, 4> planes{};
    GuestState state_after_copy{};
};

struct Capture {
    Stage stage = Stage::idle;
    Stop stop = Stop::none;
    uint16_t site = 0, successor = 0;
    Frame armed_frame{};
    uint32_t write_count = 0, actual_index = 0;
    Handler handler = Handler::unsupported;
    Sample before{}, after{};
};

inline uint16_t successor_for(uint16_t site) {
    switch (site) {
        case 0x0412: return 0x0415;
        case 0x0438: return 0x043B;
        case 0x0445: return 0x0449;
        case 0x0451: return 0x0455;
        default: return 0;
    }
}

inline Stop arm(Capture& out, const Frame& stopped) {
    out = Capture{};
    const uint16_t successor = successor_for(stopped.ip);
    if (stopped.cs != 0x0F2D || successor == 0 ||
        stopped.es != 0xA000 || stopped.di != 0x118F) {
        out.stage = Stage::failed;
        return out.stop = Stop::wrong_arm_frame;
    }
    out.site = stopped.ip;
    out.successor = successor;
    out.armed_frame = stopped;
    out.stage = Stage::armed;
    return Stop::none;
}

template <typename ReadState>
inline bool sample_word(Sample& out, const uint8_t* backing,
                        uint32_t index, ReadState read_state) {
    out.state_before_copy = read_state();
    const std::size_t offset = std::size_t(index) * 4u;
    for (std::size_t plane = 0; plane < 4u; ++plane)
        out.planes[plane] = backing[offset + plane];
    out.state_after_copy = read_state();
    return same_state(out.state_before_copy, out.state_after_copy);
}

inline bool frame_matches_armed_write(const Capture& capture, const Frame& frame) {
    return frame.cs == capture.armed_frame.cs &&
           (frame.ip == capture.site || frame.ip == capture.successor) &&
           frame.es == capture.armed_frame.es &&
           frame.di == capture.armed_frame.di;
}

// Called at the actual resolved backing-word write. apply_write is the original
// handler operation and runs once even when the observation fails a guard.
template <typename ApplyWrite, typename ReadState>
inline void observe_write(Capture& out, Handler handler, uint32_t guest_linear,
                          unsigned width, uint32_t index,
                          const uint8_t* backing, std::size_t backing_bytes,
                          ApplyWrite apply_write, ReadState read_state) {
    if (out.stage == Stage::idle || out.stage == Stage::finished ||
        out.stage == Stage::failed) {
        apply_write();
        return;
    }
    ++out.write_count;
    if (out.stage == Stage::captured) {
        out.stage = Stage::failed;
        out.stop = Stop::duplicate_write;
        apply_write();
        return;
    }
    if (guest_linear != 0xA118F) out.stop = Stop::wrong_target;
    else if (width != 1u) out.stop = Stop::wrong_width;
    else if (handler != Handler::uvga && handler != Handler::uvga_fast)
        out.stop = Stop::unsupported_handler;
    else if (backing == nullptr || index >= backing_bytes / 4u)
        out.stop = Stop::invalid_backing;
    if (out.stop != Stop::none) {
        out.stage = Stage::failed;
        apply_write();
        return;
    }
    out.handler = handler;
    out.actual_index = index;
    if (!sample_word(out.before, backing, index, read_state)) {
        out.stage = Stage::failed;
        out.stop = Stop::changed_observation_state;
    } else if (!frame_matches_armed_write(out, out.before.state_before_copy.frame)) {
        out.stage = Stage::failed;
        out.stop = Stop::wrong_observed_frame;
    }
    apply_write();
    if (out.stage == Stage::failed) return;
    if (!sample_word(out.after, backing, index, read_state)) {
        out.stage = Stage::failed;
        out.stop = Stop::changed_observation_state;
        return;
    }
    if (!same_frame(out.before.state_before_copy.frame,
                    out.after.state_before_copy.frame) ||
        !frame_matches_armed_write(out, out.after.state_before_copy.frame)) {
        out.stage = Stage::failed;
        out.stop = Stop::wrong_observed_frame;
        return;
    }
    out.stage = Stage::captured;
}

inline Stop finish(Capture& out, const Frame& stopped) {
    if (out.stage == Stage::failed) return out.stop;
    if (stopped.cs != out.armed_frame.cs || stopped.ip != out.successor ||
        stopped.es != out.armed_frame.es || stopped.di != out.armed_frame.di) {
        out.stage = Stage::failed;
        return out.stop = Stop::wrong_successor;
    }
    if (out.stage != Stage::captured || out.write_count != 1u) {
        out.stage = Stage::failed;
        return out.stop = Stop::missing_write;
    }
    out.stage = Stage::finished;
    return Stop::none;
}

} // namespace poolrad_u19_capture

#endif
