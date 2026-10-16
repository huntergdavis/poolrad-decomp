"""Source-only association of an audited U19 v2 pair with supplied RGB24.

This binds selected-event labels across two validated packets. It does not
authenticate a native backend, event clock, displayed page, or RGB parity.
"""

from dataclasses import dataclass

from tools.replay.cold_start_codeword_row14_completed_backend_rgb24_v2 import (
    CompletedFrameV2, STATUS as RGB_STATUS,
    parse_completed_backend_frame_v2,
)
from tools.replay.cold_start_codeword_row14_v2_pair_join import (
    IDENTITY_FIELDS, JOIN_STATUS as PAIR_STATUS, PairJoinV2, join_pair_v2,
)


ASSOCIATION_STATUS = "SOURCE_ONLY_V2_PAIR_WITH_SUPPLIED_RGB_NO_NATIVE_BACKEND_ASSERTION"


@dataclass(frozen=True)
class SuppliedRgbAssociationV2:
    status: str
    pair_status: str
    rgb_status: str
    producer_status: str
    plane_event_id: str
    backend_event_id: str
    rgb_sha256: str
    pair: PairJoinV2
    rgb: CompletedFrameV2
    native_coverage_proved: bool = False


def associate_pair_with_supplied_rgb_v2(
    raw_jsonl: bytes, v1_manifest: object, source_bytes: bytes,
    producer_packet: object, plane_packet_raw: bytes,
    rgb_manifest_raw: bytes, rgb24: bytes, *, enabled: bool = False,
) -> SuppliedRgbAssociationV2:
    """Recompute the CPU/plane pair, then bind its eight labels to supplied RGB.

    ``backend_event_id`` is a separate declaration. There is no sourced
    backend-event correlation or common event clock in these inputs.
    """
    if enabled is not True:
        raise ValueError("U19 v2 RGB association requires exact opt-in")

    pair = join_pair_v2(raw_jsonl, v1_manifest, source_bytes,
                        producer_packet, plane_packet_raw, enabled=True)
    rgb = parse_completed_backend_frame_v2(rgb_manifest_raw, rgb24)
    if (pair.status != PAIR_STATUS or rgb.status != RGB_STATUS or
            pair.native_coverage_proved is not False):
        raise ValueError("U19 v2 source-only packet status differs")
    for name in IDENTITY_FIELDS:
        selected = getattr(pair, name)
        declared = getattr(rgb, name)
        if type(selected) is not type(declared) or selected != declared:
            raise ValueError(f"U19 v2 RGB selected-event {name} differs")

    return SuppliedRgbAssociationV2(
        status=ASSOCIATION_STATUS, pair_status=pair.status,
        rgb_status=rgb.status, producer_status=pair.producer_status,
        plane_event_id=pair.plane_event_id,
        backend_event_id=rgb.backend_event_id, rgb_sha256=rgb.rgb_sha256,
        pair=pair, rgb=rgb,
    )
