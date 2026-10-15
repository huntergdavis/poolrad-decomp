#!/usr/bin/env python3
"""Map the saved first-view placement-to-flush object delta without game bytes."""

from __future__ import annotations

import argparse
import csv
from hashlib import sha256
from io import StringIO
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
PRIMARY = ROOT / "resources/oracle-runs/slot-a-view4-last6-compose-p5-001"
WRITER = ROOT / "resources/oracle-runs/slot-a-view4-writer-order-p5-002"
MAP = ROOT / "docs/re/slot_a_object_615_offsets.csv"
OBJECT_AT = 0x3DE48
HEADER = 23
ROW_STRIDE = 84
ROWS = 168
OBJECT_SIZE = HEADER + ROW_STRIDE * ROWS

# Two native runs. The first anchors the independently audited placement and
# flush stops; the second supplies same-run intermediate call boundaries.
CAPTURES = {
    "primary_placement": (
        PRIMARY, "ram-placement-return.bin",
        "e05bb3cc8974b4bef870c7fbaf46177b09b9531f16b92eb19f4c7f6c0b4e2483",
        "8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d",
    ),
    "primary_flush": (
        PRIMARY, "ram-flush-entry.bin",
        "48becc9f9848794f232e6887a08edfd8b59d901833ccffbbbea4c336245080d7",
        "489572698be6487b4d9c88b470f62b530e0bd98539c99b75444a76b3639b14e1",
    ),
    "writer_return": (
        WRITER, "ram-current-return.bin",
        "871e7f3d42cb590c4328419977749baa2c4e9dfb6059af13dc21b265b1cb7ada",
        "8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d",
    ),
    "first_pre": (
        WRITER, "ram-after16-first-tile-pre.bin",
        "f70133941a9737e887618286699ee94e8885505679392b486d0a7823dd75fc0f",
        "8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d",
    ),
    "first_post": (
        WRITER, "ram-after16-first-tile-post.bin",
        "82faf7062da4a2c3e1abdd7846d3e64cad2e7aab9533c61ff2d423396f0a19b4",
        "8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d",
    ),
    "second_pre": (
        WRITER, "ram-after16-second-tile-pre.bin",
        "391a759cf5e9083156af3e4114ff0b1913a446cdc7b89cd569c3c12846162e8d",
        "8664637ddd0016f3ef9978c4694f87ecbb7fb325ab147475f936c410ce9c6e8d",
    ),
    "second_post": (
        WRITER, "ram-after16-second-tile-post.bin",
        "224fad0a97131d157756426bc2b1912ff961498dbb391bdee194bf13cdb3dc3b",
        "8fe8460abf1e3a7ea3a7e8a1a2a80f0cddda97107133a19bffc0c96745b79fa0",
    ),
    "writer_flush": (
        WRITER, "ram-flush-entry.bin",
        "d0ce92b6a3da99664fef97111ff9c82129307c377fc789ab5e1cbaf7de37df17",
        "489572698be6487b4d9c88b470f62b530e0bd98539c99b75444a76b3639b14e1",
    ),
}


def read_object(directory: Path, name: str, ram_hash: str,
                object_hash: str) -> bytes:
    ram = (directory / name).read_bytes()
    assert len(ram) == 0xA0000, f"{name}: RAM length"
    assert sha256(ram).hexdigest() == ram_hash, f"{name}: RAM hash"
    pointer_at = 0x14A00 + 0x4A20
    offset = int.from_bytes(ram[pointer_at:pointer_at + 2], "little")
    segment = int.from_bytes(ram[pointer_at + 2:pointer_at + 4], "little")
    assert segment * 16 + offset == OBJECT_AT, f"{name}: object pointer"
    object_bytes = ram[OBJECT_AT:OBJECT_AT + OBJECT_SIZE]
    assert sha256(object_bytes).hexdigest() == object_hash, f"{name}: object hash"
    return object_bytes


def map_csv(objects: dict[str, bytes]) -> tuple[str, dict[str, int]]:
    baseline = objects["writer_return"]
    first = objects["first_post"]
    second = objects["second_post"]
    final = objects["writer_flush"]
    assert baseline == objects["primary_placement"]
    assert baseline == objects["first_pre"] == first == objects["second_pre"]
    assert final == objects["primary_flush"]

    final_offsets = [i for i in range(OBJECT_SIZE) if baseline[i] != final[i]]
    second_offsets = {i for i in range(OBJECT_SIZE) if baseline[i] != second[i]}
    assert len(final_offsets) == 615 and len(second_offsets) == 16
    assert second_offsets <= set(final_offsets)
    assert all(i >= HEADER for i in final_offsets)

    counts = {
        "second_tile_retained": 0,
        "second_tile_overwritten": 0,
        "later_unattributed": 0,
    }
    output = StringIO()
    writer = csv.writer(output, lineterminator="\n")
    writer.writerow(("object_offset", "physical_address", "row",
                     "byte_column", "plane", "observation"))
    for offset in final_offsets:
        pixel_offset = offset - HEADER
        row, within_row = divmod(pixel_offset, ROW_STRIDE)
        byte_column, plane = divmod(within_row, 4)
        assert 17 <= row <= 103 and plane in (0, 2, 3)
        if offset not in second_offsets:
            observation = "later_unattributed"
        elif second[offset] == final[offset]:
            observation = "second_tile_retained"
        else:
            observation = "second_tile_overwritten"
        counts[observation] += 1
        writer.writerow((f"0x{offset:04X}", f"0x{OBJECT_AT + offset:05X}",
                         row, byte_column, plane, observation))

    assert counts == {
        "second_tile_retained": 8,
        "second_tile_overwritten": 8,
        "later_unattributed": 599,
    }
    return output.getvalue(), counts


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--emit-csv", action="store_true",
                        help="print address-only map; never print captured bytes")
    args = parser.parse_args()
    objects = {
        key: read_object(directory, name, ram_hash, object_hash)
        for key, (directory, name, ram_hash, object_hash) in CAPTURES.items()
    }
    rendered, counts = map_csv(objects)
    if args.emit_csv:
        print(rendered, end="")
        return
    assert MAP.read_text(encoding="ascii") == rendered, "checked-in offset map"
    print("eight full-RAM captures hash-locked; two runs join by complete object")
    print("615/615 final-differing offsets mapped; no header changes")
    print("second tile retained 8, overwritten later 8; later unattributed 599")
    print(f"address-only CSV SHA-256 {sha256(rendered.encode('ascii')).hexdigest()}")


if __name__ == "__main__":
    main()
