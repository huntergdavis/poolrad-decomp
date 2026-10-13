#!/usr/bin/env python3
"""Check ignored native slot-A SYSTEM-copy snapshots without printing game data."""

import hashlib
import sys
from pathlib import Path


START_HASH = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_HASH = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_HASH = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
HELPER_HASH = "4681f1935f55d71f05afab0b13fff0505aa17e9a04fb372ff559ef28ed88058f"
CALL_ORIGINAL_HASH = "7780859e922a704a7253291b792982fced470c34e06c47f1daf92468f6774bae"
CALL_LOADED_HASH = "1b227ae15a60114aee8a68adef67dd8cad844a7d284497c578ca3160566b5a9b"

FIRST_HASHES = {
    "1965": "d019b40c52b233a773b23ef255e7927ed10f4f4da41c8e94f30cb4755a6d0121",
    "0b19": "76f78b872811b48514083e031d24b87b791ee4afa9a0967c3a42dbcf20e5c9d6",
    "0b1a": "9b856250978bac20226acdaeca3bf3ae5eee0efe444397bb415f13e6f9053baf",
    "0b20": "67d1ff270888916e4f05ca07946d16e2c335775a5d5709fe44eff1a44da1c2a8",
    "196a": "67d1ff270888916e4f05ca07946d16e2c335775a5d5709fe44eff1a44da1c2a8",
}
SECOND_AFTER_CALL_HASH = "e878e2d519ce6a722aa6ed2236262bbc02ac29005dbec04816af2e6a664ad77d"
SECOND_HASHES = {
    "1965": "a2f88f2c6b6c326fd4bd556a1f549ec87c3886276ced73e2a1ca30077ba883b2",
    **{label: SECOND_AFTER_CALL_HASH for label in
       ("0b0b", "0b19", "0b1a", "0b1e", "0b20", "196a")},
}
FIRST_EV_HASH = "4b24e13e3470057d2a93f4539b6570e35b09217ed11b9aada9f1abdb1e8f6269"
SECOND_EV_HASH = "df2651e012b3363b905ee2afc7601d7a1b615d0dceefa20becef804c110ee472"


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
    return {i for i, pair in enumerate(zip(before, after)) if pair[0] != pair[1]}


def snapshots(root: Path, hashes: dict[str, str]) -> dict[str, bytes]:
    return {label: checked(root / f"ram-{label}.bin", 655360, expected)
            for label, expected in hashes.items()}


def transcript(root: Path, name: str, expected_hash: str,
               labels: list[str]) -> dict[str, list[int]]:
    data = (root / name).read_bytes()
    assert digest(data) == expected_hash, "CPU transcript hash mismatch"
    rows = [line.split() for line in data.decode("ascii").splitlines()]
    assert [row[0] for row in rows] == labels
    assert all(len(row) == 17 for row in rows)
    values = {row[0]: [int(field, 16) for field in row[1:]] for row in rows}
    assert all(cpu[0] == (0x2A8E if label.startswith("19") else 0x131C)
               and cpu[1] == int(label, 16) for label, cpu in values.items())
    return values


def check_code(root: Path, ram: bytes) -> None:
    start = checked(root / "POOLRAD/START.EXE", 65231, START_HASH)
    game = checked(root / "POOLRAD/GAME.OVR", 206105, GAME_HASH)
    checked(root / "POOLRAD/SAVGAMA.DAT", 13137, SAVE_HASH)
    helper = start[0x0B781:0x0B79B]
    loaded_helper = ram[physical(0x131C, 0x0B0B):physical(0x131C, 0x0B25)]
    assert len(helper) == 26 and digest(helper) == HELPER_HASH
    assert loaded_helper == helper
    call = game[0x1DFE6 + 0x1965:0x1DFE6 + 0x196A]
    loaded_call = ram[physical(0x2A8E, 0x1965):physical(0x2A8E, 0x196A)]
    assert digest(call) == CALL_ORIGINAL_HASH
    assert digest(loaded_call) == CALL_LOADED_HASH
    assert call[0] == loaded_call[0] == 0x9A
    assert word(call, 1) == word(loaded_call, 1) == 0x0B0B
    assert word(call, 3) == 0x0AF8 and word(loaded_call, 3) == 0x131C


def check(first: Path, second: Path) -> None:
    a = snapshots(first, FIRST_HASHES)
    b = snapshots(second, SECOND_HASHES)
    check_code(first, a["1965"])
    check_code(second, b["1965"])
    first_cpu = transcript(first, "ev-transcript-1965-first.txt", FIRST_EV_HASH,
                           list(FIRST_HASHES))
    second_cpu = transcript(second, "ev-transcript-1965.txt", SECOND_EV_HASH,
                            list(SECOND_HASHES))
    assert first_cpu["1965"][9] == 0x29
    assert second_cpu["1965"][9] == 0x52
    assert all(cpu[7] == 0x37A0 for label, cpu in first_cpu.items()
               if label.startswith("0b"))
    assert all(cpu[7] == 0x37A0 for label, cpu in second_cpu.items()
               if label.startswith("0b"))
    assert first_cpu["196a"][7] == second_cpu["196a"][7] == 0x37A8
    assert first_cpu["196a"][2] == second_cpu["196a"][2] == 0x14A0

    call_frame = set(range(0x20900, 0x20904))
    destination = physical(0x1D16, 0x3BB6)
    source = physical(0x2A8E, 0x15E7)
    for ram in (a["1965"], b["1965"]):
        stack = physical(0x1D16, 0x37A4)
        assert [word(ram, stack + i) for i in (0, 2, 4, 6)] == [
            0x15E7, 0x2A8E, 0x3BB6, 0x1D16
        ]
    assert changed(a["1965"], a["0b19"]) == call_frame
    assert changed(a["0b19"], a["0b1a"]) == {destination}
    assert (a["0b19"][destination], a["0b1a"][destination]) == (97, 1)
    assert changed(a["0b1a"], a["0b20"]) == {destination + 1}
    assert changed(a["0b20"], a["196a"]) == set()
    assert changed(a["1965"], a["196a"]) == call_frame | {destination, destination + 1}
    assert a["1965"][source] == 1
    assert a["196a"][source:source + 2] == a["196a"][destination:destination + 2]

    assert changed(b["1965"], b["0b0b"]) == call_frame
    assert all(changed(b["0b0b"], b[label]) == set() for label in
               ("0b19", "0b1a", "0b1e", "0b20", "196a"))
    assert b["1965"][source] == b["1965"][destination] == 1
    assert b["1965"][source:source + 2] == b["1965"][destination:destination + 2]
    assert word(b["0b0b"], physical(0x1D16, 0x37A0)) == 0x196A
    assert word(b["0b0b"], physical(0x1D16, 0x37A2)) == 0x2A8E
    assert word(b["196a"], physical(0x1D16, 0x37A8)) == 0x3BB6
    assert word(b["196a"], physical(0x1D16, 0x37AA)) == 0x1D16


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: check_slot_a_u6f_1965_native.py FIRST_RUN SECOND_RUN")
    check(Path(sys.argv[1]), Path(sys.argv[2]))
    print("first and second saved-slot SYSTEM-copy snapshots agree")
