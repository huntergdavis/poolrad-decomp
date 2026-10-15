#!/usr/bin/env python3
"""Classify saved-A first-flush conventional RAM deltas by address only."""

from __future__ import annotations

import argparse
import csv
from hashlib import sha256
from io import StringIO
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "resources/oracle-runs/slot-a-view4-last6-compose-p5-001"
TABLE = ROOT / "docs/re/res464_first_page_ram_delta.csv"
RAM_SIZE = 0xA0000
DS_BASE = 0x14A00  # 14A0:0000
SS_BASE = 0x1D160  # 1D16:0000
ENTRY_SHA = "48becc9f9848794f232e6887a08edfd8b59d901833ccffbbbea4c336245080d7"
RETURN_SHA = "db0fc49dad03cc5a45ececba4017f2eddf51d608f1c5993608334d63f777085f"


def pinned(name: str, expected: str) -> bytes:
    path = (RUN / name).resolve(strict=True)
    if not path.is_relative_to((ROOT / "resources").resolve()):
        raise ValueError("RAM capture must remain under ignored resources/")
    data = path.read_bytes()
    if len(data) != RAM_SIZE or sha256(data).hexdigest() != expected:
        raise ValueError(f"private RAM input changed: {name}")
    return data


def word(data: bytes, at: int) -> int:
    return int.from_bytes(data[at:at + 2], "little")


def render() -> str:
    before = pinned("ram-flush-entry.bin", ENTRY_SHA)
    after = pinned("ram-flush-return.bin", RETURN_SHA)
    changed = {at for at, (old, new) in enumerate(zip(before, after)) if old != new}

    first_row, end_row = 16, 104  # half-open; 88 dirty rows
    for row in range(168):
        flag = DS_BASE + 0x4A28 + row
        first = DS_BASE + 0x4C20 + 2 * row
        last = DS_BASE + 0x4D70 + 2 * row
        if first_row <= row < end_row:
            if (before[flag] == 0 or after[flag] != 0 or
                    word(after, first) != 0xFFFF or word(after, last) != 0):
                raise ValueError(f"dirty-row reset differs at row {row}")
        elif (before[flag] != after[flag] or
              before[first:first + 2] != after[first:first + 2] or
              before[last:last + 2] != after[last:last + 2]):
            raise ValueError(f"non-dirty row metadata changed at row {row}")

    # End-exclusive physical ranges. The two stack gaps remain distinct.
    groups = (
        ("bios_tick", 0x046C, 0x046D, "0040:006C"),
        ("dirty_flags", DS_BASE + 0x4A38, DS_BASE + 0x4A90,
         "14A0:4A38..4A8F"),
        ("first_extent_words", DS_BASE + 0x4C40, DS_BASE + 0x4CF0,
         "14A0:4C40..4CEF"),
        ("last_extent_words", DS_BASE + 0x4D90, DS_BASE + 0x4E40,
         "14A0:4D90..4E3F"),
        ("stack_residue", SS_BASE + 0x3E76, SS_BASE + 0x3E88,
         "1D16:3E76..3E87"),
        ("stack_residue", SS_BASE + 0x3E8A, SS_BASE + 0x3E94,
         "1D16:3E8A..3E93"),
    )
    expected = set()
    output = StringIO()
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow(("class", "physical_first", "physical_last",
                     "segment_offsets", "changed_bytes"))
    for label, start, stop, segment in groups:
        addresses = set(range(start, stop))
        if changed.intersection(addresses) != addresses or expected.intersection(addresses):
            raise ValueError(f"incomplete or overlapping RAM delta: {label}")
        expected.update(addresses)
        writer.writerow((label, f"{start:05X}", f"{stop - 1:05X}",
                         segment, len(addresses)))
    if changed != expected or len(changed) != 469:
        raise ValueError("unclassified or missing conventional RAM delta")
    return output.getvalue()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emit-csv", action="store_true")
    args = parser.parse_args()
    table = render()
    if args.emit_csv:
        print(table, end="")
        return
    if TABLE.read_text(encoding="ascii") != table:
        raise ValueError("checked-in address-only RAM table differs")
    print("two 640-KiB RAM stops hash-locked; all 469 changed addresses classified")
    print("dirty metadata 440; stack residue 28; BIOS timer 1; other RAM 0")
    print("address-only CSV SHA-256", sha256(table.encode("ascii")).hexdigest())


if __name__ == "__main__":
    main()
