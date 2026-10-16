#!/usr/bin/env python3
"""Check a supplied U19 host-backed single-write plane observation.

This validates a private probe record; it cannot authenticate a native stop,
prove the first writer, or establish page/RGB timing.
"""

from dataclasses import dataclass, fields
import json
from pathlib import Path
import re

from tools.replay.cold_start_codeword_row14_internal_plane_resolver import (
    HostMap, planar_index,
)
from tools.replay.cold_start_codeword_row14_target_filter import NEXT_PC


TOKEN = re.compile(r"[A-Za-z0-9_-]{1,64}\Z")
HEX8 = re.compile(r"[0-9A-F]{8}\Z")
HEX20 = re.compile(r"[0-9A-F]{20}\Z")
HEX12 = re.compile(r"[0-9A-F]{12}\Z")
HEX4 = re.compile(r"[0-9A-F]{4}\Z")


@dataclass(frozen=True)
class SnapshotResult:
    run_token: str
    plane_event_id: str
    arm_token: str
    store_ordinal: int
    site: int
    successor: int
    host_map: HostMap
    backing_index: int
    before_planes: bytes
    after_planes: bytes
    changed_planes: tuple[int, ...]


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValueError(reason)


def _state(state: object) -> None:
    require(isinstance(state, dict) and set(state) ==
            {"cs", "ip", "es", "di", "gfx", "seq", "latch"},
            "observation state fields incomplete")
    for name in ("cs", "ip", "es", "di"):
        require(isinstance(state[name], str) and HEX4.fullmatch(state[name]) is not None,
                f"invalid {name} field")
    for name, pattern in (("gfx", HEX20), ("seq", HEX12), ("latch", HEX8)):
        require(isinstance(state[name], str) and pattern.fullmatch(state[name]) is not None,
                f"invalid {name} field")


def _sample(sample: object, phase: str) -> tuple[bytes, dict]:
    require(isinstance(sample, dict) and set(sample) ==
            {"planes", "state_before_copy", "state_after_copy"},
            f"{phase} sample fields incomplete")
    before, after = sample["state_before_copy"], sample["state_after_copy"]
    _state(before)
    _state(after)
    require(before == after, f"{phase} observation changed guest state")
    value = sample["planes"]
    require(isinstance(value, str) and HEX8.fullmatch(value) is not None,
            f"{phase} four-plane word invalid")
    return bytes.fromhex(value), before


def _host_map(value: object) -> HostMap:
    names = {field.name for field in fields(HostMap)}
    require(isinstance(value, dict) and set(value) == names,
            "host map fields incomplete")
    bool_names = {"compatible_chain4", "ignore_odd_even",
                  "ignore_extended_memory_bit", "is_vga_arch"}
    for name in names - {"handler"}:
        expected = bool if name in bool_names else int
        require(type(value[name]) is expected, f"invalid host map field {name}")
    require(isinstance(value["handler"], str), "invalid host handler")
    return HostMap(**value)


def validate_capture(record: object) -> SnapshotResult:
    """Reject route, resolver, or observation-state discrepancies in one record."""
    require(isinstance(record, dict) and set(record) ==
            {"version", "run_token", "plane_event_id", "arm_token",
             "store_ordinal", "probe", "extent", "site", "successor",
             "witness", "host_map", "handler_index", "write_count",
             "before", "after"}, "capture fields incomplete")
    require(type(record["version"]) is int and record["version"] == 1,
            "unsupported capture version")
    require(isinstance(record["run_token"], str) and
            TOKEN.fullmatch(record["run_token"]) is not None,
            "invalid run token")
    require(isinstance(record["plane_event_id"], str) and
            TOKEN.fullmatch(record["plane_event_id"]) is not None and
            isinstance(record["arm_token"], str) and
            TOKEN.fullmatch(record["arm_token"]) is not None and
            type(record["store_ordinal"]) is int and
            1 <= record["store_ordinal"] <= 128,
            "invalid selected-store join key")
    require(record["probe"] == "host_backing_word" and
            record["extent"] == "one_byte_per_plane" and
            record["witness"] == {"x": 122, "y": 112, "segment": "A000",
                                  "offset": "118F", "bit": 0x20},
            "not the fixed one-word passive witness")
    site, successor = record["site"], record["successor"]
    require(type(site) is int and type(successor) is int and
            NEXT_PC.get(site) == successor, "wrong selected store successor")
    require(type(record["write_count"]) is int and record["write_count"] == 1,
            "probe did not capture exactly one selected write")
    mapping = _host_map(record["host_map"])
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
    changed = tuple(i for i, (old, new) in enumerate(zip(before, after)) if old != new)
    return SnapshotResult(record["run_token"], record["plane_event_id"],
                          record["arm_token"], record["store_ordinal"],
                          site, successor, mapping,
                          record["handler_index"], before, after, changed)


def main() -> None:
    import argparse

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("record", type=Path, help="private same-run probe JSON")
    args = parser.parse_args()
    result = validate_capture(json.loads(args.record.read_text()))
    print(f"selected=0F2D:{result.site:04X}->0F2D:{result.successor:04X}; "
          f"backing_index={result.backing_index:08X}; "
          f"changed_plane_count={len(result.changed_planes)}")
    print("supplied probe only; native route, first writer, page and RGB unproved")


if __name__ == "__main__":
    main()
