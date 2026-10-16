#!/usr/bin/env python3
"""Validate one supplied U19 v2 four-plane packet, without native provenance.

The eight selected-event fields can be compared with an independently audited
recorder event. This module does not authenticate those fields by itself.
"""

from dataclasses import dataclass
import json
from pathlib import Path
import re

from tools.replay.cold_start_codeword_row14_internal_plane_resolver import (
    HostMap, planar_index,
)
from tools.replay.cold_start_codeword_row14_passive_snapshot import (
    _host_map, _sample, require,
)
from tools.replay.cold_start_codeword_row14_target_filter import NEXT_PC


MAX_PACKET_BYTES = 4096
MAX_CANDIDATE_ORDINAL = 2304
TOKEN = re.compile(r"[A-Za-z0-9_-]{1,64}\Z")
HEX_SHA = re.compile(r"[0-9a-f]{64}\Z")
PC_WORDS = re.compile(r"[0-9A-F]{4}:[0-9A-F]{4}\Z")
IDENTITY_FIELDS = (
    "run_token", "arm_token", "stream_id", "plane_event_id",
    "raw_sha256", "candidate_ordinal", "pc", "successor_pc",
)
PACKET_FIELDS = frozenset((
    "version", *IDENTITY_FIELDS, "probe", "extent", "witness", "host_map",
    "handler_index", "write_count", "before", "after",
))
WITNESS = {"x": 122, "y": 112, "segment": "A000", "offset": "118F",
           "bit": 0x20}


@dataclass(frozen=True)
class SnapshotResultV2:
    version: int
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
    host_map: HostMap
    backing_index: int
    before_planes: bytes
    after_planes: bytes
    changed_planes: tuple[int, ...]


def _unique_pairs(pairs: list[tuple[str, object]]) -> dict:
    value = {}
    for name, item in pairs:
        require(name not in value, "duplicate packet JSON key")
        value[name] = item
    return value


def _reject_constant(value: str) -> None:
    raise ValueError(f"invalid packet JSON constant {value}")


def parse_capture_v2(raw_packet: bytes) -> SnapshotResultV2:
    """Check the actual UTF-8 byte extent before parsing one JSON object."""
    require(type(raw_packet) is bytes and len(raw_packet) <= MAX_PACKET_BYTES,
            "v2 packet exceeds byte bound or is not bytes")
    try:
        record = json.loads(raw_packet.decode("utf-8"),
                            object_pairs_hook=_unique_pairs,
                            parse_constant=_reject_constant)
    except (UnicodeDecodeError, json.JSONDecodeError) as exc:
        raise ValueError("invalid v2 packet JSON") from exc
    return validate_capture_v2(record)


def validate_capture_v2(record: object) -> SnapshotResultV2:
    """Check the parsed packet shape and source-only plane observation."""
    require(type(record) is dict and set(record) == PACKET_FIELDS,
            "v2 packet fields incomplete or extra")
    try:
        encoded = json.dumps(record, separators=(",", ":"), ensure_ascii=False,
                             allow_nan=False).encode("utf-8")
    except (TypeError, ValueError, UnicodeEncodeError) as exc:
        raise ValueError("v2 packet is not serializable JSON") from exc
    require(len(encoded) <= MAX_PACKET_BYTES, "v2 packet exceeds byte bound")
    require(type(record["version"]) is int and record["version"] == 2,
            "unsupported v2 packet version")
    for name in ("run_token", "arm_token", "stream_id", "plane_event_id"):
        require(type(record[name]) is str and TOKEN.fullmatch(record[name]) is not None,
                f"invalid {name}")
    require(type(record["raw_sha256"]) is str and
            HEX_SHA.fullmatch(record["raw_sha256"]) is not None,
            "invalid raw SHA-256")
    ordinal = record["candidate_ordinal"]
    require(type(ordinal) is int and 1 <= ordinal <= MAX_CANDIDATE_ORDINAL,
            "invalid candidate ordinal")
    pc, successor_pc = record["pc"], record["successor_pc"]
    require(type(pc) is str and PC_WORDS.fullmatch(pc) is not None and
            type(successor_pc) is str and PC_WORDS.fullmatch(successor_pc) is not None,
            "noncanonical selected PC")
    require(pc.startswith("0F2D:") and successor_pc.startswith("0F2D:"),
            "wrong selected PC segment")
    site = int(pc[5:], 16)
    successor = NEXT_PC.get(site)
    require(successor is not None and successor_pc == f"0F2D:{successor:04X}",
            "wrong selected store successor")
    witness = record["witness"]
    require(type(witness) is dict and set(witness) == set(WITNESS) and
            all(type(witness[name]) is int for name in ("x", "y", "bit")) and
            all(type(witness[name]) is str for name in ("segment", "offset")) and
            record["probe"] == "host_backing_word" and
            record["extent"] == "one_byte_per_plane" and witness == WITNESS,
            "not the fixed one-word passive witness")
    require(type(record["write_count"]) is int and record["write_count"] == 1,
            "probe did not capture exactly one selected write")
    mapping = _host_map(record["host_map"])
    require(0 <= mapping.seq_memory_mode <= 0xFF and
            0 <= mapping.gfx_miscellaneous <= 0xFF,
            "host VGA register byte out of range")
    require(type(record["handler_index"]) is int and
            record["handler_index"] == planar_index(mapping),
            "handler index disagrees with independent resolver")
    before, before_state = _sample(record["before"], "before")
    after, after_state = _sample(record["after"], "after")
    frame = {name: before_state[name] for name in ("cs", "ip", "es", "di")}
    require(frame == {name: after_state[name] for name in frame},
            "observed write frame changed across samples")
    require(frame["cs"] == "0F2D" and
            frame["ip"] in {f"{site:04X}", f"{successor:04X}"} and
            frame["es"] == "A000" and frame["di"] == "118F",
            "observed write frame disagrees with selected store")
    changed = tuple(i for i, (old, new) in enumerate(zip(before, after))
                    if old != new)
    return SnapshotResultV2(
        version=2, run_token=record["run_token"], arm_token=record["arm_token"],
        stream_id=record["stream_id"], plane_event_id=record["plane_event_id"],
        raw_sha256=record["raw_sha256"], candidate_ordinal=ordinal,
        pc=pc, successor_pc=successor_pc, site=site, successor=successor,
        host_map=mapping, backing_index=record["handler_index"],
        before_planes=before, after_planes=after, changed_planes=changed,
    )


def main() -> None:
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("record", type=Path, help="private v2 plane packet JSON")
    args = parser.parse_args()
    result = parse_capture_v2(args.record.read_bytes())
    print(f"selected={result.pc}->{result.successor_pc}; "
          f"candidate_ordinal={result.candidate_ordinal}; "
          f"changed_plane_count={len(result.changed_planes)}")
    print("supplied source-only packet; native writer, page and RGB unproved")


if __name__ == "__main__":
    main()
