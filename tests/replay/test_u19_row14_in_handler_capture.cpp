#include "tools/replay/u19_row14_in_handler_capture.hpp"

#include <array>
#include <cassert>
#include <cstdint>

using namespace poolrad_u19_capture;

static GuestState state(Frame frame = Frame{0x0F2D, 0x0445, 0xA000, 0x118F}) {
    GuestState result{};
    result.frame = frame;
    result.gfx[0] = 4;
    result.seq[0] = 2;
    result.latch = 0x12345678;
    return result;
}

static GuestState default_state() {
    return state();
}

static void accepted_one_write() {
    Capture capture{};
    assert(arm(capture, Frame{0x0F2D, 0x0445, 0xA000, 0x118F}) == Stop::none);
    std::array<uint8_t, 32> backing{};
    int writes = 0;
    observe_write(capture, Handler::uvga, 0xA118F, 1, 3,
                  backing.data(), backing.size(),
                  [&] { ++writes; backing[13] = 0x20; }, default_state);
    assert(writes == 1 && backing[13] == 0x20);
    assert(capture.before.planes[1] == 0 && capture.after.planes[1] == 0x20);
    assert(capture.actual_index == 3 && capture.write_count == 1);
    assert(finish(capture, Frame{0x0F2D, 0x0449, 0xA000, 0x118F}) == Stop::none);
}

static void rejection_never_suppresses_write() {
    for (unsigned bad = 0; bad < 4; ++bad) {
        Capture capture{};
        assert(arm(capture, Frame{0x0F2D, 0x0412, 0xA000, 0x118F}) == Stop::none);
        std::array<uint8_t, 8> backing{};
        int writes = 0;
        const uint32_t target = bad == 0 ? 0xA1190 : 0xA118F;
        const unsigned width = bad == 1 ? 2u : 1u;
        const Handler handler = bad == 2 ? Handler::unsupported : Handler::uvga_fast;
        const uint32_t index = bad == 3 ? 2u : 0u;
        observe_write(capture, handler, target, width, index,
                      backing.data(), backing.size(), [&] { ++writes; }, default_state);
        assert(writes == 1 && capture.stage == Stage::failed);
        assert(capture.stop == (bad == 0 ? Stop::wrong_target :
                                bad == 1 ? Stop::wrong_width :
                                bad == 2 ? Stop::unsupported_handler :
                                           Stop::invalid_backing));
    }
}

static void duplicate_and_missing_write_fail() {
    Capture missing{};
    assert(arm(missing, Frame{0x0F2D, 0x0438, 0xA000, 0x118F}) == Stop::none);
    assert(finish(missing, Frame{0x0F2D, 0x043B, 0xA000, 0x118F}) == Stop::missing_write);

    Capture duplicate{};
    assert(arm(duplicate, Frame{0x0F2D, 0x0451, 0xA000, 0x118F}) == Stop::none);
    std::array<uint8_t, 8> backing{};
    int writes = 0;
    auto active_state = [] { return state(Frame{0x0F2D, 0x0451, 0xA000, 0x118F}); };
    auto run = [&] {
        observe_write(duplicate, Handler::uvga_fast, 0xA118F, 1, 0,
                      backing.data(), backing.size(), [&] { ++writes; }, active_state);
    };
    run();
    run();
    assert(writes == 2 && duplicate.write_count == 2);
    assert(finish(duplicate, Frame{0x0F2D, 0x0455, 0xA000, 0x118F}) ==
           Stop::duplicate_write);
}

static void observation_mutation_fails_after_original_write() {
    Capture capture{};
    assert(arm(capture, Frame{0x0F2D, 0x0445, 0xA000, 0x118F}) == Stop::none);
    std::array<uint8_t, 8> backing{};
    int reads = 0, writes = 0;
    auto changing_state = [&] {
        GuestState current = state();
        current.latch += static_cast<uint32_t>(reads++);
        return current;
    };
    observe_write(capture, Handler::uvga, 0xA118F, 1, 0,
                  backing.data(), backing.size(), [&] { ++writes; }, changing_state);
    assert(writes == 1 && capture.stop == Stop::changed_observation_state);
}

static void wrong_observed_frame_rejects_every_field() {
    const Frame wrong[] = {
        Frame{0x0F2E, 0x0451, 0xA000, 0x118F},
        Frame{0x0F2D, 0x0445, 0xA000, 0x118F},
        Frame{0x0F2D, 0x0451, 0xA001, 0x118F},
        Frame{0x0F2D, 0x0451, 0xA000, 0x1190},
    };
    for (const Frame& observed : wrong) {
        Capture capture{};
        assert(arm(capture, Frame{0x0F2D, 0x0451, 0xA000, 0x118F}) == Stop::none);
        std::array<uint8_t, 8> backing{};
        int writes = 0;
        observe_write(capture, Handler::uvga, 0xA118F, 1, 0,
                      backing.data(), backing.size(), [&] { ++writes; },
                      [&] { return state(observed); });
        assert(writes == 1 && capture.stop == Stop::wrong_observed_frame);
    }
}

static void wrong_successor_target_fails() {
    for (unsigned changed = 0; changed < 2; ++changed) {
        Capture capture{};
        assert(arm(capture, Frame{0x0F2D, 0x0412, 0xA000, 0x118F}) == Stop::none);
        std::array<uint8_t, 8> backing{};
        int writes = 0;
        observe_write(capture, Handler::uvga_fast, 0xA118F, 1, 0,
                      backing.data(), backing.size(), [&] { ++writes; },
                      [] { return state(Frame{0x0F2D, 0x0415, 0xA000, 0x118F}); });
        Frame successor{0x0F2D, 0x0415, 0xA000, 0x118F};
        if (changed == 0) successor.es = 0xA001;
        else successor.di = 0x1190;
        assert(writes == 1 && finish(capture, successor) == Stop::wrong_successor);
    }
}

int main() {
    accepted_one_write();
    rejection_never_suppresses_write();
    duplicate_and_missing_write_fail();
    observation_mutation_fails_after_original_write();
    wrong_observed_frame_rejects_every_field();
    wrong_successor_target_fails();
}
