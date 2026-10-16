#!/usr/bin/env python3
"""Validate private, paired U19 row-14 four-plane instruction-stop captures.

An ordered list of supplied pairs can identify its first *observed* byte or
pixel-bit change. Completeness of the native breakpoint/watch trace and the
guest instruction that caused a change require separate evidence.
"""

import argparse
from dataclasses import dataclass
import json
from pathlib import Path
import struct


WINDOW_START = 0x1180
WINDOW_BYTES = 0x140
WITNESS_AT = 0x118F - WINDOW_START
WITNESS_MASK = 0x20
RAM_BYTES = 655360
DS_BASE = 0x14A0 << 4
NEXT_PC = {
    "0F2D:0412": "0F2D:0415",
    "0F2D:0438": "0F2D:043B",
    "0F2D:0445": "0F2D:0449",
    "0F2D:0451": "0F2D:0455",
}


@dataclass(frozen=True)
class Change:
    sequence: int
    pc: str
    changed_planes: tuple[int, ...]
    changed_witness_bits: tuple[int, ...]


def require(condition, message):
    if not condition:
        raise ValueError(message)


def _private_file(root, name):
    require(isinstance(name, str) and name and not Path(name).is_absolute(),
            "capture path must be relative")
    root = root.resolve()
    path = (root / name).resolve()
    require(path.is_relative_to(root), "capture path escapes private run")
    return path


def read_sample(root, record):
    """Read one paused sample; return four 320-byte planes and page words."""
    require(isinstance(record, dict), "sample must be an object")
    names = record.get("planes")
    require(isinstance(names, list) and len(names) == 4 and
            len(set(names)) == 4, "sample requires four distinct plane files")
    planes = tuple(_private_file(root, name).read_bytes() for name in names)
    require(all(len(plane) == WINDOW_BYTES for plane in planes),
            "each plane window must be exactly 320 bytes")
    ram = _private_file(root, record.get("ram")).read_bytes()
    require(len(ram) == RAM_BYTES, "RAM sample must be exactly 640 KiB")
    active, draw = struct.unpack_from("<HH", ram, DS_BASE + 0x4A16)
    require((active, draw) == (0xA000, 0xA000),
            "captured page segments differ from saved EGA route")
    require(ram[DS_BASE + 0x4A1E] == 1,
            "captured video adapter differs from saved EGA route")
    before = record.get("gc_before")
    after = record.get("gc_after")
    require(isinstance(before, list) and len(before) == 2 and
            all(isinstance(value, int) for value in before) and
            before == after and 0 <= before[0] <= 8 and
            0 <= before[1] <= 3,
            "Graphics Controller selector/read map not restored")
    return planes


def compare_events(events):
    """Classify supplied pairs without asserting native trace completeness."""
    require(isinstance(events, list) and events,
            "at least one paired instruction event is required")
    changes = []
    prior_witness = None
    for index, event in enumerate(events, 1):
        require(isinstance(event, dict) and event.get("sequence") == index,
                "event sequence has a gap or duplicate")
        pc = event.get("pc")
        require(pc in NEXT_PC and event.get("next_pc") == NEXT_PC[pc],
                "event is not a pinned complete candidate instruction step")
        before, after = event.get("before"), event.get("after")
        require(isinstance(before, tuple) and isinstance(after, tuple) and
                len(before) == len(after) == 4 and
                all(len(plane) == WINDOW_BYTES for plane in before + after),
                "event needs four complete before/after plane windows")
        witness_before = tuple(plane[WITNESS_AT] for plane in before)
        witness_after = tuple(plane[WITNESS_AT] for plane in after)
        require(prior_witness is None or witness_before == prior_witness,
                "unaccounted witness-byte change between supplied pairs")
        changed_planes = tuple(plane for plane in range(4)
                               if witness_before[plane] != witness_after[plane])
        changed_bits = tuple(plane for plane in range(4)
                             if ((witness_before[plane] ^
                                  witness_after[plane]) & WITNESS_MASK) != 0)
        changes.append(Change(index, pc, changed_planes, changed_bits))
        prior_witness = witness_after
    return changes


def audit(root, manifest):
    require(isinstance(manifest, dict) and manifest.get("version") == 1,
            "unsupported private capture manifest")
    require(manifest.get("window_start") == WINDOW_START and
            manifest.get("window_bytes") == WINDOW_BYTES and
            manifest.get("witness_offset") == 0x118F,
            "capture window does not cover the pinned witness")
    pairs = manifest.get("events")
    require(isinstance(pairs, list), "capture events must be a list")
    loaded = []
    for pair in pairs:
        require(isinstance(pair, dict), "event must be an object")
        loaded.append({
            "sequence": pair.get("sequence"),
            "pc": pair.get("pc"),
            "next_pc": pair.get("next_pc"),
            "before": read_sample(root, pair.get("before")),
            "after": read_sample(root, pair.get("after")),
        })
    return compare_events(loaded)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", type=Path, required=True,
                        help="ignored private capture directory")
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    require(args.manifest.resolve().is_relative_to(args.run.resolve()),
            "manifest must remain under ignored run directory")
    changes = audit(args.run, json.loads(args.manifest.read_text()))
    byte_changes = [change for change in changes if change.changed_planes]
    bit_changes = [change for change in changes if change.changed_witness_bits]
    print(f"paired instruction events={len(changes)}")
    print(f"witness-byte changes={len(byte_changes)}; "
          f"witness-bit changes={len(bit_changes)}")
    if byte_changes:
        print(f"first observed byte change: pair {byte_changes[0].sequence} "
              f"at {byte_changes[0].pc}")
    if bit_changes:
        print(f"first observed pixel-bit change: pair {bit_changes[0].sequence} "
              f"at {bit_changes[0].pc}")
    print("native breakpoint coverage and first actual guest writer unproved")


if __name__ == "__main__":
    main()
