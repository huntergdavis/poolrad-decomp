#include "tools/replay/u19_row14_producer_handler_adapter.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <string_view>

using namespace poolrad_u19_producer_handler;
namespace capture = poolrad_u19_capture;

static constexpr std::string_view RAW =
    "0123456789abcdef0123456789abcdef"
    "0123456789abcdef0123456789abcdef";
static constexpr std::string_view SOURCE =
    "abcdef0123456789abcdef0123456789"
    "abcdef0123456789abcdef0123456789";

static ProducerV2 packet() {
    ProducerV2 result{};
    result.version = 2;
    result.status = "SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED";
    result.selected = SelectedIdentity{
        "run_1", "arm_1", "stream_1", "plane_1", RAW,
        897u, "0F2D:0445", "0F2D:0449"
    };
    result.provenance = Provenance{
        "src/debug/debug.cpp raw bytes", SOURCE, RAW,
        2304u, 2307u, "full_return", false
    };
    return result;
}

static AuditBinding binding(const ProducerV2& value) {
    return AuditBinding{
        true, value.selected, value.status, value.provenance.terminal,
        value.provenance.source_domain, SOURCE, RAW,
        value.provenance.candidate_count, value.provenance.frame_count
    };
}

static capture::Capture finished(uint16_t site, int& reads, int& writes) {
    capture::Capture result{};
    const capture::Frame frame{0x0F2D, site, 0xA000, 0x118F};
    assert(capture::arm(result, frame) == capture::Stop::none);
    std::array<uint8_t, 8> backing{};
    capture::observe_write(
        result, capture::Handler::uvga, 0xA118F, 1u, 0u,
        backing.data(), backing.size(),
        [&] { ++writes; backing[0] = 0x20; },
        [&] {
            ++reads;
            capture::GuestState state{};
            state.frame = frame;
            return state;
        });
    assert(capture::finish(result, capture::Frame{
        frame.cs, capture::successor_for(site), frame.es, frame.di
    }) == capture::Stop::none);
    return result;
}

static void default_off_and_no_resampling() {
    int reads = 0, writes = 0;
    const capture::Capture complete = finished(0x0445, reads, writes);
    assert(reads == 4 && writes == 1);
    ProducerV2 input = packet();
    const AuditBinding audited = binding(input);
    EventEmitter emitter{};
    assert(emit(emitter, complete, input, audited) == Result::disabled);
    emitter.enabled = true;
    AuditBinding unverified = audited;
    unverified.checked = false;
    assert(emit(emitter, complete, input, unverified) == Result::unaudited);
    assert(emit(emitter, complete, input, audited) == Result::emitted);
    assert(reads == 4 && writes == 1 && emitter.emitted);
    assert(std::string_view(emitter.selected_event.run_token.data()) ==
           input.selected.run_token);
    assert(std::string_view(emitter.selected_event.arm_token.data()) ==
           input.selected.arm_token);
    assert(std::string_view(emitter.selected_event.stream_id.data()) ==
           input.selected.stream_id);
    assert(std::string_view(emitter.selected_event.plane_event_id.data()) ==
           input.selected.plane_event_id);
    assert(std::string_view(emitter.selected_event.raw_sha256.data()) == RAW);
    assert(emitter.selected_event.candidate_ordinal == 897u);
    assert(std::string_view(emitter.selected_event.pc.data()) == "0F2D:0445");
    assert(std::string_view(emitter.selected_event.successor_pc.data()) ==
           "0F2D:0449");
    assert(emit(emitter, complete, input, audited) == Result::duplicate);
    assert(reads == 4 && writes == 1);
}

static void all_eight_fields_must_match_audit() {
    int reads = 0, writes = 0;
    const capture::Capture complete = finished(0x0445, reads, writes);
    const AuditBinding audited = binding(packet());
    for (unsigned field = 0; field < 8u; ++field) {
        ProducerV2 changed = packet();
        switch (field) {
            case 0: changed.selected.run_token = "other_run"; break;
            case 1: changed.selected.arm_token = "other_arm"; break;
            case 2: changed.selected.stream_id = "other_stream"; break;
            case 3: changed.selected.plane_event_id = "other_plane"; break;
            case 4: changed.selected.raw_sha256 = SOURCE; break;
            case 5: changed.selected.candidate_ordinal = 898u; break;
            case 6: changed.selected.pc = "0F2D:0412"; break;
            default: changed.selected.successor_pc = "0F2D:0415"; break;
        }
        EventEmitter emitter{};
        emitter.enabled = true;
        assert(emit(emitter, complete, changed, audited) ==
               Result::audit_mismatch);
        assert(!emitter.emitted);
    }
    assert(reads == 4 && writes == 1);
}

static void source_raw_terminal_and_pc_are_bound() {
    int reads = 0, writes = 0;
    const capture::Capture complete = finished(0x0445, reads, writes);
    ProducerV2 input = packet();
    const AuditBinding audited = binding(input);
    EventEmitter emitter{};
    emitter.enabled = true;

    input.provenance.source_sha256 = RAW;
    assert(emit(emitter, complete, input, audited) == Result::source_mismatch);
    input = packet();
    input.provenance.source_domain = "other source";
    assert(emit(emitter, complete, input, audited) == Result::source_mismatch);
    input = packet();
    input.provenance.raw_sha256 = SOURCE;
    assert(emit(emitter, complete, input, audited) == Result::raw_mismatch);
    input = packet();
    input.provenance.native_coverage_proved = true;
    assert(emit(emitter, complete, input, audited) == Result::invalid_packet);
    input = packet();
    input.provenance.frame_count = 2306u;
    assert(emit(emitter, complete, input, audited) == Result::invalid_packet);
    input = packet();
    input.status = "SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED";
    assert(emit(emitter, complete, input, audited) == Result::invalid_packet);
    input = packet();
    assert(emit(emitter, finished(0x0412, reads, writes), input, audited) ==
           Result::wrong_pc);
    assert(!emitter.emitted && reads == 8 && writes == 2);
}

static void prefix_and_incomplete_capture() {
    ProducerV2 input = packet();
    input.status = "SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED";
    input.provenance.terminal = "selected_prefix";
    input.provenance.candidate_count = 897u;
    input.provenance.frame_count = 899u;
    const AuditBinding audited = binding(input);
    capture::Capture incomplete{};
    assert(capture::arm(incomplete, capture::Frame{
        0x0F2D, 0x0445, 0xA000, 0x118F
    }) == capture::Stop::none);
    EventEmitter emitter{};
    emitter.enabled = true;
    assert(emit(emitter, incomplete, input, audited) ==
           Result::capture_incomplete);
    int reads = 0, writes = 0;
    const capture::Capture complete = finished(0x0445, reads, writes);
    assert(emit(emitter, complete, input, audited) == Result::emitted);
    assert(reads == 4 && writes == 1);
}

int main() {
    default_off_and_no_resampling();
    all_eight_fields_must_match_audit();
    source_raw_terminal_and_pc_are_bound();
    prefix_and_incomplete_capture();
}
