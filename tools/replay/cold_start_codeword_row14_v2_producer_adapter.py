#!/usr/bin/env python3
"""Source-only v2 producer packet built from an audited U19 v1 transcript.

The caller supplies actual v1 raw bytes, manifest, and pinned source bytes.
This module has no emulator hook, file writer, or native coverage claim.
"""

import json

from tools.replay.cold_start_codeword_row14_candidate_coverage_gate import (
    MAX_STREAM_RECORDS, ROUTE_CANDIDATES,
)
from tools.replay.cold_start_codeword_row14_selected_pc_recorder import (
    HEX_SHA, PC_WORDS, SELECTED_FIELDS, TOKEN, audit_recording,
)


VERSION = 2
MAX_PACKET_BYTES = 4096
PACKET_FIELDS = frozenset((
    "version", "status", "selected_event", "provenance",
))
PROVENANCE_FIELDS = frozenset((
    "source_domain", "source_sha256", "raw_sha256", "candidate_count",
    "frame_count", "terminal", "native_coverage_proved",
))
PROVENANCE_ORDER = (
    "source_domain", "source_sha256", "raw_sha256", "candidate_count",
    "frame_count", "terminal", "native_coverage_proved",
)
ACCEPTED = frozenset((
    "SOURCE_ONLY_FULL_RETURN_UNAUTHENTICATED",
    "SOURCE_ONLY_SELECTED_PREFIX_UNAUTHENTICATED",
))
TOKENS = ("run_token", "arm_token", "stream_id", "plane_event_id")
PCS = ("pc", "successor_pc")
SELECTED_ORDER = (*TOKENS, "raw_sha256", "candidate_ordinal", *PCS)


def _enabled(enabled: object) -> None:
    if enabled is not True:
        raise ValueError("STOP_V2_DISABLED")


def _bytes(packet: object) -> bytes:
    try:
        return json.dumps(packet, separators=(",", ":"), ensure_ascii=True,
                          allow_nan=False).encode("ascii")
    except (TypeError, ValueError, UnicodeError) as exc:
        raise ValueError("STOP_V2_PACKET_SHAPE") from exc


def _shape(packet: object) -> None:
    if not isinstance(packet, dict) or set(packet) != PACKET_FIELDS:
        raise ValueError("STOP_V2_PACKET_SHAPE")
    if type(packet["version"]) is not int or packet["version"] != VERSION:
        raise ValueError("STOP_V2_VERSION")
    if type(packet["status"]) is not str or packet["status"] not in ACCEPTED:
        raise ValueError("STOP_V2_STATUS")
    selected = packet["selected_event"]
    if not isinstance(selected, dict) or set(selected) != SELECTED_FIELDS:
        raise ValueError("STOP_V2_SELECTED_SHAPE")
    if (not all(type(selected[name]) is str and
                TOKEN.fullmatch(selected[name]) is not None for name in TOKENS) or
            type(selected["raw_sha256"]) is not str or
            HEX_SHA.fullmatch(selected["raw_sha256"]) is None or
            type(selected["candidate_ordinal"]) is not int or
            not 1 <= selected["candidate_ordinal"] <= ROUTE_CANDIDATES or
            not all(type(selected[name]) is str and
                    PC_WORDS.fullmatch(selected[name]) is not None for name in PCS)):
        raise ValueError("STOP_V2_SELECTED_SHAPE")
    provenance = packet["provenance"]
    if not isinstance(provenance, dict) or set(provenance) != PROVENANCE_FIELDS:
        raise ValueError("STOP_V2_PROVENANCE_SHAPE")
    if (type(provenance["source_domain"]) is not str or
            type(provenance["source_sha256"]) is not str or
            HEX_SHA.fullmatch(provenance["source_sha256"]) is None or
            type(provenance["raw_sha256"]) is not str or
            HEX_SHA.fullmatch(provenance["raw_sha256"]) is None or
            type(provenance["candidate_count"]) is not int or
            not 1 <= provenance["candidate_count"] <= ROUTE_CANDIDATES or
            type(provenance["frame_count"]) is not int or
            not 2 <= provenance["frame_count"] <= MAX_STREAM_RECORDS or
            provenance["terminal"] not in ("full_return", "selected_prefix") or
            type(provenance["terminal"]) is not str or
            provenance["native_coverage_proved"] is not False):
        raise ValueError("STOP_V2_PROVENANCE_SHAPE")
    if len(_bytes(packet)) > MAX_PACKET_BYTES:
        raise ValueError("STOP_V2_PACKET_SIZE")


def produce_packet(raw_jsonl: bytes, v1_manifest: object, source_bytes: bytes,
                   *, enabled: bool = False) -> dict:
    """Build one bounded, unauthenticated packet from a recomputed v1 audit."""
    _enabled(enabled)
    result = audit_recording(raw_jsonl, v1_manifest, source_bytes)
    if result.status not in ACCEPTED:
        raise ValueError("STOP_V1_SELECTION")
    # audit_recording has proved the exact manifest shape and rebuilt it from
    # the supplied raw stream, including the selected immediate successor.
    selected = v1_manifest["selected_event"]
    if (selected is None or
            selected["candidate_ordinal"] != result.selected_candidate_ordinal or
            v1_manifest["candidate_count"] != result.candidate_count):
        raise ValueError("STOP_V1_SELECTION")
    provenance = {name: v1_manifest[name] for name in PROVENANCE_ORDER}
    packet = {
        "version": VERSION,
        "status": result.status,
        "selected_event": {name: selected[name] for name in SELECTED_ORDER},
        "provenance": provenance,
    }
    _shape(packet)
    return packet


def audit_packet(raw_jsonl: bytes, v1_manifest: object, source_bytes: bytes,
                 packet: object, *, enabled: bool = False) -> str:
    """Reject any noncanonical v2 field or mismatch to the audited v1 input."""
    expected = produce_packet(raw_jsonl, v1_manifest, source_bytes,
                              enabled=enabled)
    _shape(packet)
    if packet != expected:
        raise ValueError("STOP_V2_MISMATCH")
    return expected["status"]
