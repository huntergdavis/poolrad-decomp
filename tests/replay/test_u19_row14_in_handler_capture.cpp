#include "tools/replay/u19_row14_in_handler_capture.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <string>
#include <string_view>

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

static SelectedEventInput selected_identity() {
    return SelectedEventInput{
        "run_1", "arm_1", "stream_1", "plane_1",
        std::string_view(
            "0123456789abcdef0123456789abcdef"
            "0123456789abcdef0123456789abcdef"),
        897u
    };
}

static void selected_event_is_default_off_and_post_successor_only() {
    Capture capture{};
    assert(arm(capture, Frame{0x0F2D, 0x0445, 0xA000, 0x118F}) == Stop::none);
    EventEmitter emitter{};
    const SelectedEventInput input = selected_identity();
    assert(emit_selected_event(emitter, capture, input) == EmitStatus::disabled);
    assert(!emitter.emitted);
    emitter.enabled = true;
    assert(emit_selected_event(emitter, capture, input) ==
           EmitStatus::capture_incomplete);
    std::array<uint8_t, 8> backing{};
    int writes = 0, reads = 0;
    observe_write(capture, Handler::uvga, 0xA118F, 1, 0,
                  backing.data(), backing.size(),
                  [&] { ++writes; backing[0] = 0x20; },
                  [&] { ++reads; return state(); });
    assert(reads == 4 && writes == 1);
    assert(emit_selected_event(emitter, capture, input) ==
           EmitStatus::capture_incomplete);
    assert(finish(capture, Frame{0x0F2D, 0x0449, 0xA000, 0x118F}) == Stop::none);
    const auto before = capture.before.planes;
    const auto after = capture.after.planes;
    assert(emit_selected_event(emitter, capture, input) == EmitStatus::emitted);
    assert(emitter.emitted && reads == 4 && writes == 1);
    assert(capture.before.planes == before && capture.after.planes == after);
    const SelectedEvent& event = emitter.selected_event;
    assert(std::string_view(event.run_token.data()) == "run_1");
    assert(std::string_view(event.arm_token.data()) == "arm_1");
    assert(std::string_view(event.stream_id.data()) == "stream_1");
    assert(std::string_view(event.plane_event_id.data()) == "plane_1");
    assert(std::string_view(event.raw_sha256.data()) == input.raw_sha256);
    assert(event.candidate_ordinal == 897u);
    assert(std::string_view(event.pc.data()) == "0F2D:0445");
    assert(std::string_view(event.successor_pc.data()) == "0F2D:0449");
    assert(emit_selected_event(emitter, capture, input) == EmitStatus::duplicate);
}

static void selected_event_rejects_bad_identity_and_changed_sample() {
    Capture capture{};
    assert(arm(capture, Frame{0x0F2D, 0x0412, 0xA000, 0x118F}) == Stop::none);
    std::array<uint8_t, 8> backing{};
    observe_write(capture, Handler::uvga_fast, 0xA118F, 1, 0,
                  backing.data(), backing.size(), [] {},
                  [] { return state(Frame{0x0F2D, 0x0412, 0xA000, 0x118F}); });
    assert(finish(capture, Frame{0x0F2D, 0x0415, 0xA000, 0x118F}) == Stop::none);
    EventEmitter emitter{};
    emitter.enabled = true;
    SelectedEventInput input = selected_identity();
    for (uint32_t ordinal : {0u, 2305u}) {
        input.candidate_ordinal = ordinal;
        assert(emit_selected_event(emitter, capture, input) ==
               EmitStatus::invalid_identity);
    }
    input = selected_identity();
    const std::string long_token(65, 'a');
    input.run_token = long_token;
    assert(emit_selected_event(emitter, capture, input) ==
           EmitStatus::invalid_identity);
    input = selected_identity();
    input.arm_token = "bad token";
    assert(emit_selected_event(emitter, capture, input) ==
           EmitStatus::invalid_identity);
    input = selected_identity();
    input.raw_sha256 = "ABCDEF";
    assert(emit_selected_event(emitter, capture, input) ==
           EmitStatus::invalid_identity);
    assert(!emitter.emitted);
    capture.before.state_after_copy.latch ^= 1u;
    assert(emit_selected_event(emitter, capture, selected_identity()) ==
           EmitStatus::capture_incomplete);
}

static void selected_event_maps_each_site_and_keeps_write_boundary() {
    struct Case {
        uint16_t site, successor;
        const char* pc;
        const char* next_pc;
    };
    const Case cases[] = {
        {0x0412, 0x0415, "0F2D:0412", "0F2D:0415"},
        {0x0438, 0x043B, "0F2D:0438", "0F2D:043B"},
        {0x0445, 0x0449, "0F2D:0445", "0F2D:0449"},
        {0x0451, 0x0455, "0F2D:0451", "0F2D:0455"},
    };
    for (const Case& item : cases) {
        const Frame armed{0x0F2D, item.site, 0xA000, 0x118F};
        Capture capture{};
        assert(arm(capture, armed) == Stop::none);
        std::array<uint8_t, 8> backing{};
        GuestState current = state(armed);
        int reads = 0, writes = 0;
        observe_write(capture, Handler::uvga, 0xA118F, 1, 0,
                      backing.data(), backing.size(),
                      [&] {
                          ++writes;
                          backing[0] = 0x20;
                          current.latch ^= 1u;
                      },
                      [&] {
                          ++reads;
                          return current;
                      });
        assert(finish(capture, Frame{0x0F2D, item.successor, 0xA000, 0x118F}) ==
               Stop::none);
        assert(capture.before.state_before_copy.latch !=
               capture.after.state_before_copy.latch);
        EventEmitter emitter{};
        emitter.enabled = true;
        SelectedEventInput input = selected_identity();
        input.candidate_ordinal = 2304u;
        assert(emit_selected_event(emitter, capture, input) == EmitStatus::emitted);
        assert(std::string_view(emitter.selected_event.pc.data()) == item.pc);
        assert(std::string_view(emitter.selected_event.successor_pc.data()) ==
               item.next_pc);
        assert(emitter.selected_event.candidate_ordinal == 2304u);
        assert(reads == 4 && writes == 1);
        if (item.site == 0x0412) {
            Capture forged = capture;
            forged.site = forged.successor = forged.armed_frame.ip = 0;
            forged.before.state_before_copy.frame.ip = 0;
            forged.before.state_after_copy.frame.ip = 0;
            forged.after.state_before_copy.frame.ip = 0;
            forged.after.state_after_copy.frame.ip = 0;
            EventEmitter invalid{};
            invalid.enabled = true;
            assert(emit_selected_event(invalid, forged, input) ==
                   EmitStatus::capture_incomplete);
        }
    }
}

int main() {
    accepted_one_write();
    rejection_never_suppresses_write();
    duplicate_and_missing_write_fail();
    observation_mutation_fails_after_original_write();
    wrong_observed_frame_rejects_every_field();
    wrong_successor_target_fails();
    selected_event_is_default_off_and_post_successor_only();
    selected_event_rejects_bad_identity_and_changed_sample();
    selected_event_maps_each_site_and_keeps_write_boundary();
}
