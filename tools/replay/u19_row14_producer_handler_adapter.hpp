#ifndef POOLRAD_U19_ROW14_PRODUCER_HANDLER_ADAPTER_HPP
#define POOLRAD_U19_ROW14_PRODUCER_HANDLER_ADAPTER_HPP

#include "tools/replay/u19_row14_in_handler_capture.hpp"

#include <array>
#include <cstdint>
#include <string_view>

namespace poolrad_u19_producer_handler {

using poolrad_u19_capture::Capture;
using poolrad_u19_capture::EventEmitter;

// These fields correspond to the v2 producer's exact selected_event object.
struct SelectedIdentity {
    std::string_view run_token, arm_token, stream_id, plane_event_id;
    std::string_view raw_sha256;
    uint32_t candidate_ordinal = 0;
    std::string_view pc, successor_pc;
};

struct Provenance {
    std::string_view source_domain, source_sha256, raw_sha256;
    uint32_t candidate_count = 0, frame_count = 0;
    std::string_view terminal;
    bool native_coverage_proved = false;
};

struct ProducerV2 {
    uint32_t version = 0;
    std::string_view status;
    SelectedIdentity selected{};
    Provenance provenance{};
};

// The caller sets checked only after Python audit_packet has recomputed the
// supplied v1 raw JSONL, manifest, actual source bytes, and v2 packet. The
// digest fields are the auditor's results, not labels from the host callback.
// This C++ bridge checks the copy supplied to the handler; it cannot itself
// authenticate the caller or the native capture.
struct AuditBinding {
    bool checked = false;
    SelectedIdentity selected{};
    std::string_view status, terminal, source_domain;
    std::string_view source_sha256, raw_sha256;
    uint32_t candidate_count = 0, frame_count = 0;
};

enum class Result : uint8_t {
    disabled, unaudited, duplicate, capture_incomplete, invalid_packet,
    audit_mismatch, source_mismatch, raw_mismatch, wrong_pc, emitted
};

inline bool same_selected(const SelectedIdentity& a,
                          const SelectedIdentity& b) {
    return a.run_token == b.run_token && a.arm_token == b.arm_token &&
           a.stream_id == b.stream_id &&
           a.plane_event_id == b.plane_event_id &&
           a.raw_sha256 == b.raw_sha256 &&
           a.candidate_ordinal == b.candidate_ordinal &&
           a.pc == b.pc && a.successor_pc == b.successor_pc;
}

inline bool digest(std::string_view value) {
    std::array<char, 65> copied{};
    return poolrad_u19_capture::copy_digest(copied, value);
}

inline bool packet_shape(const ProducerV2& packet) {
    const bool full = packet.status ==
        "SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED";
    const bool prefix = packet.status ==
        "SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED";
    const Provenance& p = packet.provenance;
    if (packet.version != 2u || (!full && !prefix) ||
        p.native_coverage_proved || p.candidate_count < 1u ||
        p.candidate_count > 2304u ||
        packet.selected.candidate_ordinal < 1u ||
        packet.selected.candidate_ordinal > p.candidate_count ||
        !digest(packet.selected.raw_sha256) || !digest(p.raw_sha256) ||
        !digest(p.source_sha256))
        return false;
    if (full)
        return p.terminal == "full_return" &&
               p.candidate_count == 2304u && p.frame_count == 2307u;
    return p.terminal == "selected_prefix" &&
           p.frame_count == p.candidate_count + 2u;
}

inline Result emit(EventEmitter& out, const Capture& capture,
                   const ProducerV2& packet, const AuditBinding& audited) {
    if (!out.enabled) return Result::disabled;
    if (out.emitted) return Result::duplicate;
    if (!audited.checked) return Result::unaudited;
    if (!poolrad_u19_capture::finished_capture(capture))
        return Result::capture_incomplete;
    if (!packet_shape(packet)) return Result::invalid_packet;
    const Provenance& p = packet.provenance;
    if (!same_selected(packet.selected, audited.selected) ||
        packet.status != audited.status || p.terminal != audited.terminal ||
        p.candidate_count != audited.candidate_count ||
        p.frame_count != audited.frame_count)
        return Result::audit_mismatch;
    if (p.source_domain != "src/debug/debug.cpp raw bytes" ||
        p.source_domain != audited.source_domain ||
        p.source_sha256 != audited.source_sha256 ||
        !digest(audited.source_sha256))
        return Result::source_mismatch;
    if (packet.selected.raw_sha256 != p.raw_sha256 ||
        p.raw_sha256 != audited.raw_sha256 ||
        !digest(audited.raw_sha256))
        return Result::raw_mismatch;

    std::array<char, 10> site{}, successor{};
    poolrad_u19_capture::format_pc(site, capture.armed_frame.cs,
                                   capture.site);
    poolrad_u19_capture::format_pc(successor, capture.armed_frame.cs,
                                   capture.successor);
    if (packet.selected.pc != std::string_view(site.data()) ||
        packet.selected.successor_pc != std::string_view(successor.data()))
        return Result::wrong_pc;

    const poolrad_u19_capture::SelectedEventInput input{
        packet.selected.run_token, packet.selected.arm_token,
        packet.selected.stream_id, packet.selected.plane_event_id,
        packet.selected.raw_sha256, packet.selected.candidate_ordinal
    };
    const auto emitted = poolrad_u19_capture::emit_selected_event(
        out, capture, input);
    return emitted == poolrad_u19_capture::EmitStatus::emitted
        ? Result::emitted : Result::invalid_packet;
}

} // namespace poolrad_u19_producer_handler

#endif
