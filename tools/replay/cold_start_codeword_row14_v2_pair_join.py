"""Source-only U19 v2 pair join for separately audited recorder and plane packets.

The caller supplies actual recorder bytes, its manifest, source bytes, the
derived producer envelope, and actual passive-packet bytes. No native origin,
first writer, page, clock, RGB, or full-frame property follows from this join.
"""

from dataclasses import dataclass


IDENTITY_FIELDS = (
    "run_token", "arm_token", "stream_id", "plane_event_id",
    "raw_sha256", "candidate_ordinal", "pc", "successor_pc",
)
JOIN_STATUS = "SOURCE_ONLY_V2_PAIR_NO_NATIVE_PLANE_OR_RGB_ASSERTION"


@dataclass(frozen=True)
class PairJoinV2:
    version: int
    status: str
    producer_status: str
    run_token: str
    arm_token: str
    stream_id: str
    plane_event_id: str
    raw_sha256: str
    candidate_ordinal: int
    pc: str
    successor_pc: str
    site: int
    successor: int
    source_domain: str
    source_sha256: str
    candidate_count: int
    frame_count: int
    terminal: str
    backing_index: int
    before_planes: bytes
    after_planes: bytes
    changed_planes: tuple[int, ...]
    native_coverage_proved: bool = False


def join_pair_v2(raw_jsonl: bytes, v1_manifest: object, source_bytes: bytes,
                 producer_packet: object, plane_packet_raw: bytes, *,
                 enabled: bool = False) -> PairJoinV2:
    """Recompute p5's selection, parse pR's bytes, then bind all eight fields.

    These versioned producer modules are separate dependencies. The v1
    cap-128 route and passive packet are never widened or relabeled here.
    """
    if enabled is not True:
        raise ValueError("U19 v2 pair join requires exact opt-in")

    from tools.replay.cold_start_codeword_row14_v2_producer_adapter import (
        audit_packet,
    )
    from tools.replay.cold_start_codeword_row14_passive_snapshot_v2 import (
        parse_capture_v2,
    )

    producer_status = audit_packet(raw_jsonl, v1_manifest, source_bytes,
                                   producer_packet, enabled=True)
    plane = parse_capture_v2(plane_packet_raw)
    selected = producer_packet["selected_event"]
    for name in IDENTITY_FIELDS:
        value = getattr(plane, name)
        expected = selected[name]
        if type(value) is not type(expected) or value != expected:
            raise ValueError(f"U19 v2 selected-event {name} differs")

    provenance = producer_packet["provenance"]
    if (type(plane.version) is not int or plane.version != 2 or
            type(producer_packet["version"]) is not int or
            producer_packet["version"] != 2 or
            provenance["raw_sha256"] != selected["raw_sha256"] or
            provenance["native_coverage_proved"] is not False):
        raise ValueError("U19 v2 packet provenance differs")

    return PairJoinV2(
        version=2, status=JOIN_STATUS, producer_status=producer_status,
        run_token=selected["run_token"], arm_token=selected["arm_token"],
        stream_id=selected["stream_id"],
        plane_event_id=selected["plane_event_id"],
        raw_sha256=selected["raw_sha256"],
        candidate_ordinal=selected["candidate_ordinal"], pc=selected["pc"],
        successor_pc=selected["successor_pc"], site=plane.site,
        successor=plane.successor,
        source_domain=provenance["source_domain"],
        source_sha256=provenance["source_sha256"],
        candidate_count=provenance["candidate_count"],
        frame_count=provenance["frame_count"], terminal=provenance["terminal"],
        backing_index=plane.backing_index,
        before_planes=plane.before_planes, after_planes=plane.after_planes,
        changed_planes=plane.changed_planes,
    )
