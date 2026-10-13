#!/usr/bin/env python3
"""Check ignored slot-A output-to-first-view native evidence without printing media."""

import hashlib
import sys
from pathlib import Path

from PIL import Image


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "overlay"))
from overlay_inventory import find_units, parse_mz  # noqa: E402


START_HASH = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_HASH = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_HASH = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
TABLE_CALL_ORIGINAL_HASH = "fe81385603a3fc55df47de14941cf1bdbf72d0bd7c936edaecb4ceda0c23d52e"
TABLE_CALL_LOADED_HASH = "78cec5fcdde872ab5e4c4add6d5debd1f4cf3d3430f3e12b1a051c4ce061d147"
COPY_CALL_ORIGINAL_HASH = "40340a7f4f937729d3786c40ce28c8cc26d796f9abd977727eaafe0a254b1253"
COPY_CALL_LOADED_HASH = "85ccb6667d3a08f5fe27f10b74ee586c9487648ced403928853c8d8c06d4aa6c"
CALLER_ORIGINAL_HASH = "6f90c7c01bc467148af435b6348d20ae6e97d7bb2ad8e08d3a4b0f26a3c1cf48"
CALLER_LOADED_HASH = "cfc00b5e4f51e5754d3bfa8aed995187e14e1ad59f76afcddf8fe43743fdda86"
STUB_HASH = "edeabeacb40dbdec68484c4dea4f72e0c4a64f80d020f2d1e068e71f7e03d025"
ENTRY_HASH = "1cf21bcb074db68d9bf2e7461b2e3db63fe9e9f5e9223de262b12a7762e02144"
STRING_HASH = "6c110ad7a5f343c8961fdb3e3762d2d8c7ebfba40902e1fd85015fcf640c3cdd"
CPU_HASH = "4c7f2193e77befaf44464fd3f427bc96bf60d49d67f41b195af26a5112a899a8"

RAM_HASHES = {
    "196f": "3e46c01d37b393bd88b558584f72f8f93b8ab545a6a607fdddbb738eb0a45844",
    "1974": "d0ebc24e57fec9d352628a1d3198d2690a34ffdb58d322e83d7faf5dd1eb6b4e",
    "1983": "61d3de1e4897f362e66cd92f21457c5a4bcdbbda08b923caf0c7590e112d92b5",
    "336a": "31424ea33ec4abce1967fff7032be17b95d4b70b4cfab0d9624c29a54e3072c3",
    "3382": "31424ea33ec4abce1967fff7032be17b95d4b70b4cfab0d9624c29a54e3072c3",
    "stub": "e5e8d23f49f5c2348c79e3b4009067b056197a5236dff1d61c84686a6530259f",
    "3387": "d5cc4a1f61561bbb19766616f3addfa100fb63284bc248854ce13079d678f7a0",
}
SCREEN_HASHES = {
    "3382": ("f4eadb2cdcc25b159d63f917793eb344b9e7e3d449a01477a4ada9f3900a3b9a",
             "1907d3488d948f0df53ae2b1e0ef686527d61b6176a3e07220e4159ef02f9399"),
    "3387": ("6f6b65475848ab63e35bc4fb55549c04a5957e8007cd6633fe1d4be377e61b7e",
             "9f5f210a0483d6d7aa86ab80cd198007e6e60dc21e7322c84be760ee827c22ff"),
}


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def checked(path: Path, size: int, expected_hash: str) -> bytes:
    data = path.read_bytes()
    assert len(data) == size, f"wrong size: {path}"
    assert digest(data) == expected_hash, f"wrong hash: {path}"
    return data


def physical(segment: int, offset: int) -> int:
    return ((segment << 4) + offset) & 0xFFFFF


def word(data: bytes, position: int) -> int:
    return int.from_bytes(data[position:position + 2], "little")


def changed(before: bytes, after: bytes) -> set[int]:
    return {i for i, (a, b) in enumerate(zip(before, after)) if a != b}


def check_code(start: bytes, game: bytes, first_ram: bytes, view_ram: bytes) -> None:
    header = parse_mz(start)["header_bytes"]
    units = find_units(start[header:], game)
    caller_unit = next(u for u in units if u["stub_para"] == 0x1C)
    view_unit = next(u for u in units if u["stub_para"] == 0x3B4)
    assert caller_unit["ovr_offset"] == 0x0D46
    assert view_unit["ovr_offset"] == 0x2DB87
    assert view_unit["memory_segment"] == 0x03CA
    assert view_unit["entries"][1] == 0x016D

    first_base = next(u for u in units if u["stub_para"] == 0x6F)["ovr_offset"]
    assert first_base == 0x1DFE6
    original_table = game[first_base + 0x196F:first_base + 0x1974]
    loaded_table = first_ram[physical(0x2A8E, 0x196F):physical(0x2A8E, 0x1974)]
    assert digest(original_table) == TABLE_CALL_ORIGINAL_HASH
    assert digest(loaded_table) == TABLE_CALL_LOADED_HASH
    assert {i for i, (a, b) in enumerate(zip(original_table, loaded_table))
            if a != b} == {3, 4}
    assert original_table[0] == loaded_table[0] == 0x9A
    assert word(original_table, 1) == word(loaded_table, 1) == 0x0425
    assert word(original_table, 3) == 0x0802 and word(loaded_table, 3) == 0x1026

    original_copy = game[first_base + 0x1974:first_base + 0x1983]
    loaded_copy = first_ram[physical(0x2A8E, 0x1974):physical(0x2A8E, 0x1983)]
    assert digest(original_copy) == COPY_CALL_ORIGINAL_HASH
    assert digest(loaded_copy) == COPY_CALL_LOADED_HASH
    assert {i for i, (a, b) in enumerate(zip(original_copy, loaded_copy))
            if a != b} == {13, 14}
    assert original_copy[10] == loaded_copy[10] == 0x9A
    assert word(original_copy, 11) == word(loaded_copy, 11) == 0x0B25
    assert word(original_copy, 13) == 0x0AF8 and word(loaded_copy, 13) == 0x131C

    start_offset = caller_unit["ovr_offset"] + 0x336A
    original = game[start_offset:start_offset + 29]
    loaded = view_ram[physical(0x3582, 0x336A):physical(0x3582, 0x3387)]
    assert start_offset == 0x40B0 and len(original) == len(loaded) == 29
    assert digest(original) == CALLER_ORIGINAL_HASH
    assert digest(loaded) == CALLER_LOADED_HASH
    assert {i for i, (a, b) in enumerate(zip(original, loaded)) if a != b} == {
        17, 18, 27, 28
    }

    # Seven complete instructions, with the two conditional branch destinations.
    assert original[0:2] == loaded[0:2] == bytes((0x80, 0x3E))
    assert word(original, 2) == 0x49F3 and original[4] == 3
    assert original[5] == 0x74 and 0x336F + 2 + original[6] == 0x3387
    assert original[7:9] == bytes((0x80, 0x3E))
    assert word(original, 9) == 0x4432 and original[11] == 1
    assert original[12] == 0x75 and 0x3376 + 2 + original[13] == 0x3382
    assert original[14] == loaded[14] == 0x9A
    assert word(original, 15) == word(loaded, 15) == 0x27D9
    assert word(original, 17) == 0x00BA and word(loaded, 17) == 0x08DE
    assert original[19:21] == bytes((0xC6, 0x06))
    assert word(original, 21) == 0x4432 and original[23] == 0
    assert original[24] == loaded[24] == 0x9A
    assert word(original, 25) == word(loaded, 25) == 0x0025
    assert word(original, 27) == 0x03CA and word(loaded, 27) == 0x0BEE

    stub = view_ram[physical(0x0BEE, 0x0025):physical(0x0BEE, 0x0025) + 5]
    assert digest(stub) == STUB_HASH and stub[0] == 0xEA
    assert (word(stub, 3), word(stub, 1)) == (0x2E35, 0x016D)
    source_entry = game[view_unit["ovr_offset"] + 0x016D:
                        view_unit["ovr_offset"] + 0x016D + 16]
    loaded_entry = view_ram[physical(0x2E35, 0x016D):physical(0x2E35, 0x016D) + 16]
    assert digest(source_entry) == digest(loaded_entry) == ENTRY_HASH


def check_cpu(root: Path) -> None:
    text = (root / "ev-numeric.tsv").read_bytes()
    assert digest(text) == CPU_HASH
    rows = [line.split() for line in text.decode("ascii").splitlines()]
    assert [row[0] for row in rows] == ["196f", "336a", "3382", "stub", "callee", "3387"]
    assert all(len(row) == 15 for row in rows)
    values = {row[0]: [int(field, 16) for field in row[1:]] for row in rows}
    assert [(values[k][0], values[k][1], values[k][5]) for k in values] == [
        (0x2A8E, 0x196F, 0x37B0), (0x3582, 0x336A, 0x3EF6),
        (0x3582, 0x3382, 0x3EF6), (0x0BEE, 0x0025, 0x3EF2),
        (0x2E35, 0x016D, 0x3EF2), (0x3582, 0x3387, 0x3EF6),
    ]
    assert all(values[k][2:5] == [0x14A0, 0x14A0, 0x1D16]
               for k in ("336a", "3382", "stub", "callee", "3387"))


def check_screen(root: Path) -> None:
    pixels = {}
    for label, (png_hash, rgb_hash) in SCREEN_HASHES.items():
        path = root / f"screen-{label}-private.png"
        image = Image.open(path).convert("RGB")
        assert image.size == (640, 400)
        assert digest(path.read_bytes()) == png_hash
        pixels[label] = image.tobytes()
        assert digest(pixels[label]) == rgb_hash
    before, after = pixels["3382"], pixels["3387"]
    changed_pixels = [i // 3 for i in range(0, len(before), 3)
                      if before[i:i + 3] != after[i:i + 3]]
    assert len(changed_pixels) == 628
    xs = [i % 640 for i in changed_pixels]
    ys = [i // 640 for i in changed_pixels]
    assert (min(xs), min(ys), max(xs) + 1, max(ys) + 1) == (418, 112, 622, 126)


def check(root: Path) -> None:
    start = checked(root / "POOLRAD/START.EXE", 65231, START_HASH)
    game = checked(root / "POOLRAD/GAME.OVR", 206105, GAME_HASH)
    checked(root / "POOLRAD/SAVGAMA.DAT", 13137, SAVE_HASH)
    ram = {label: checked(root / f"ram-{label}.bin", 655360, expected)
           for label, expected in RAM_HASHES.items()}
    check_code(start, game, ram["196f"], ram["3382"])
    check_cpu(root)
    check_screen(root)

    ss = 0x1D16
    source = physical(ss, 0x3AB6)
    destination = physical(ss, 0x3BD4)
    # +1974/+1983 have hash-locked RAM snapshots, but no preserved EV CPU rows.
    assert [word(ram["196f"], physical(ss, off)) for off in (0x37B0, 0x37B2)] == [
        0x3AB6, ss
    ]
    assert ram["1974"][source] == ram["1983"][destination] == 8
    source_value = ram["1974"][source:source + 9]
    destination_value = ram["1983"][destination:destination + 9]
    assert source_value == destination_value
    assert digest(source_value) == STRING_HASH
    assert changed(ram["1974"][destination:destination + 0x28],
                   ram["1983"][destination:destination + 0x28]) == set(range(9))

    assert ram["336a"] == ram["3382"]
    for label in ("336a", "3382", "3387"):
        state = ram[label]
        ds = 0x14A0
        assert state[physical(ds, 0x49F3)] == 4
        assert state[physical(ds, 0x4432)] == 0
        assert tuple(state[physical(ds, off)] for off in (0x6AAD, 0x6AAE, 0x6AAF)) == (
            15, 4, 6
        )
        window_offset = word(state, physical(ds, 0x49D2))
        window_segment = word(state, physical(ds, 0x49D4))
        assert (window_segment, window_offset) == (0x3996, 0)
        assert tuple(word(state, physical(window_segment, window_offset + off))
                     for off in (0x18E, 0x190, 0x192)) == (0, 5, 10)

    assert changed(ram["3382"], ram["stub"]) == {
        physical(ss, off) for off in range(0x3EF2, 0x3EF6)
    }
    assert [word(ram["stub"], physical(ss, off)) for off in (0x3EF2, 0x3EF4)] == [
        0x3387, 0x3582
    ]
    assert len(changed(ram["3382"], ram["3387"])) == 147


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: check_slot_a_first_view_native.py IGNORED_RUN_DIR")
    check(Path(sys.argv[1]))
    print("saved-slot first-view native caller, frame, RAM, and pixels agree")
