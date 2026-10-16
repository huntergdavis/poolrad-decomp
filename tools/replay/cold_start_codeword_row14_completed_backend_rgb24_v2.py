"""Validate supplied U19 RGB24 frame bytes and a source-only v2 manifest.

The manifest's selected CPU labels and backend-completion kind are declarations.
This module does not authenticate a native backend, frame, page, or event clock.
"""

from dataclasses import dataclass
from hashlib import sha256
import json

from tools.replay.cold_start_codeword_row14_passive_snapshot_v2 import (
    HEX_SHA, IDENTITY_FIELDS, MAX_CANDIDATE_ORDINAL, PC_WORDS, TOKEN,
)
from tools.replay.cold_start_codeword_row14_target_filter import NEXT_PC


MAX_MANIFEST_BYTES = 4096
MAX_RGB_BYTES = 4 * 1024 * 1024
WIDTH = 640
HEIGHT = 400
PIXEL_FORMAT = "RGB24"
CHANNEL_ORDER = "RGB"
STATUS = "SUPPLIED_FRAME_NO_NATIVE_BACKEND_ASSERTION"
PACKET_FIELDS = frozenset((
    "version", "selected_event", "backend_event_id", "backend",
    "completion_kind", "width", "height", "row_stride", "pixel_format",
    "channel_order", "rgb_byte_length", "rgb_sha256",
))


@dataclass(frozen=True)
class CompletedFrameV2:
    version: int
    status: str
    run_token: str
    arm_token: str
    stream_id: str
    plane_event_id: str
    raw_sha256: str
    candidate_ordinal: int
    pc: str
    successor_pc: str
    backend_event_id: str
    backend: str
    width: int
    height: int
    row_stride: int
    pixel_format: str
    channel_order: str
    rgb_byte_length: int
    rgb_sha256: str
    rgb24: bytes


def _require(condition: bool, message: str) -> None:
    if not condition:
        raise ValueError(message)


def _unique_pairs(pairs: list[tuple[str, object]]) -> dict:
    result = {}
    for key, value in pairs:
        _require(key not in result, "duplicate RGB24 manifest JSON key")
        result[key] = value
    return result


def _reject_constant(value: str) -> None:
    raise ValueError(f"invalid RGB24 manifest JSON constant {value}")


def parse_completed_backend_frame_v2(raw_manifest: bytes,
                                     rgb24: bytes) -> CompletedFrameV2:
    """Check the original manifest byte extent before parsing one JSON object."""
    _require(type(raw_manifest) is bytes and
             len(raw_manifest) <= MAX_MANIFEST_BYTES,
             "RGB24 manifest exceeds byte bound or is not bytes")
    try:
        record = json.loads(raw_manifest.decode("utf-8"),
                            object_pairs_hook=_unique_pairs,
                            parse_constant=_reject_constant)
    except (UnicodeDecodeError, json.JSONDecodeError, RecursionError) as exc:
        raise ValueError("invalid RGB24 manifest JSON") from exc
    return validate_completed_backend_frame_v2(record, rgb24)


def validate_completed_backend_frame_v2(record: object,
                                        rgb24: bytes) -> CompletedFrameV2:
    """Validate supplied bytes and declarations without native attribution."""
    _require(type(record) is dict and set(record) == PACKET_FIELDS,
             "RGB24 manifest fields incomplete or extra")
    try:
        encoded = json.dumps(record, separators=(",", ":"), ensure_ascii=False,
                             allow_nan=False).encode("utf-8")
    except (TypeError, ValueError, UnicodeEncodeError, RecursionError) as exc:
        raise ValueError("RGB24 manifest is not serializable JSON") from exc
    _require(len(encoded) <= MAX_MANIFEST_BYTES,
             "RGB24 manifest exceeds byte bound")
    _require(type(rgb24) is bytes and len(rgb24) <= MAX_RGB_BYTES,
             "RGB24 frame exceeds byte bound or is not bytes")
    _require(type(record["version"]) is int and record["version"] == 2,
             "unsupported RGB24 manifest version")

    selected = record["selected_event"]
    _require(type(selected) is dict and set(selected) == set(IDENTITY_FIELDS),
             "selected CPU labels incomplete or extra")
    for key in ("run_token", "arm_token", "stream_id", "plane_event_id"):
        _require(type(selected[key]) is str and
                 TOKEN.fullmatch(selected[key]) is not None,
                 f"invalid selected {key}")
    _require(type(selected["raw_sha256"]) is str and
             HEX_SHA.fullmatch(selected["raw_sha256"]) is not None,
             "invalid selected raw SHA-256")
    ordinal = selected["candidate_ordinal"]
    _require(type(ordinal) is int and 1 <= ordinal <= MAX_CANDIDATE_ORDINAL,
             "invalid selected candidate ordinal")
    pc, successor_pc = selected["pc"], selected["successor_pc"]
    _require(type(pc) is str and PC_WORDS.fullmatch(pc) is not None and
             type(successor_pc) is str and
             PC_WORDS.fullmatch(successor_pc) is not None and
             pc.startswith("0F2D:") and successor_pc.startswith("0F2D:"),
             "noncanonical selected PC")
    site = int(pc[5:], 16)
    successor = NEXT_PC.get(site)
    _require(successor is not None and
             successor_pc == f"0F2D:{successor:04X}",
             "wrong selected store successor")

    for key in ("backend_event_id", "backend"):
        _require(type(record[key]) is str and
                 TOKEN.fullmatch(record[key]) is not None,
                 f"invalid {key}")
    _require(record["completion_kind"] == "backend_complete" and
             type(record["completion_kind"]) is str,
             "backend completion declaration missing")
    _require(all(type(record[key]) is int for key in
                 ("width", "height", "row_stride", "rgb_byte_length")) and
             record["width"] == WIDTH and record["height"] == HEIGHT and
             WIDTH * 3 <= record["row_stride"] and
             record["row_stride"] * HEIGHT <= MAX_RGB_BYTES and
             record["rgb_byte_length"] == len(rgb24) ==
             record["row_stride"] * HEIGHT,
             "RGB24 dimensions, stride, or byte extent differ")
    _require(record["pixel_format"] == PIXEL_FORMAT and
             type(record["pixel_format"]) is str and
             record["channel_order"] == CHANNEL_ORDER and
             type(record["channel_order"]) is str,
             "noncanonical RGB24 pixel format or channel order")
    digest = sha256(rgb24).hexdigest()
    _require(type(record["rgb_sha256"]) is str and
             HEX_SHA.fullmatch(record["rgb_sha256"]) is not None and
             record["rgb_sha256"] == digest,
             "RGB24 bytes disagree with supplied SHA-256")

    return CompletedFrameV2(
        version=2, status=STATUS,
        run_token=selected["run_token"], arm_token=selected["arm_token"],
        stream_id=selected["stream_id"],
        plane_event_id=selected["plane_event_id"],
        raw_sha256=selected["raw_sha256"], candidate_ordinal=ordinal,
        pc=pc, successor_pc=successor_pc,
        backend_event_id=record["backend_event_id"],
        backend=record["backend"], width=WIDTH, height=HEIGHT,
        row_stride=record["row_stride"], pixel_format=PIXEL_FORMAT,
        channel_order=CHANNEL_ORDER,
        rgb_byte_length=len(rgb24), rgb_sha256=digest, rgb24=rgb24,
    )
