#!/usr/bin/env python3
"""Verify ignored native +196A call snapshots without exposing game content."""

import hashlib
import sys
from pathlib import Path


START_HASH = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_HASH = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_HASH = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
CALL_ORIGINAL_HASH = "965606c688f124e8166f41f356e580d9d2d0897521f1a7db054b55733834822d"
CALL_LOADED_HASH = "f2fd40cc31e4aa01a804aac27d0a12ab79d65949e939e63564877d0448cc6dee"
BODY_ORIGINAL_HASH = "ab5aee9717d1a2e5f472234bc2198528c02792bec1098de0dbb7be0fef4ea889"
BODY_LOADED_HASH = "2d2a3260b63ef4f99decb51f44ec543a83d5c1ec4f8c854da949a376e56b722e"
FINAL_CALL_ORIGINAL_HASH = "6d7fadfa225a8fcbe50f8aa9bd23a82b01605ad36537649b8c460d85459776a1"
FINAL_CALL_LOADED_HASH = "74e04ab27a444791cf75ab6dda2cb5081e6b6378829ce6fe21620a6baa3156f1"
COPY_HASH = "009955722e8f73d0a0f7076aa418d141f642c5a729b2a58dfa7a7a3922989118"
STOSB_HASH = "bceef655b5a034911f1c3718ce056531b45ef03b4c7b1f15629e867294011a7d"
REP_MOVSB_HASH = "00b4f1541a5abd60fc5e6d7eaa5a807d3c34cdda45627839b36c91f79340c935"
LIVE_EV_HASH = "1ec062b5e5bddd5e7811133516a723d1d2bec9a0c68fe9bd02933185ce768309"
CONTROL_EV_HASH = "2e12550d09d8d9048a810a4ea7bbb940965eb9dc2f2ffecef58a0337cb7baeaf"

LIVE_HASHES = {
    "196a-first": "77625a39d03a9630d11103df2a8cbf5b7da65867335ca031c5a51280b7f5af7c",
    "196a": "4e25d55dfe20735950fb26c9115b5ec2d8d59990e1b0c62dc4b4173207c17f17",
    "0356": "67f714ede36946adcbdc3c9bef3ec206ab2d0b91cad2f2d666b7e6cc52389a77",
    "0357": "8a7317171de1fff016cd062f22a6bec77747bc3f449a98b19347dd60f78d3a05",
    "036c": "2656af61f6882e6f45d3c62c29442efc279b8a7acd256d2c7f480a381898f5d9",
    "0b3d": "b479b9355db3e237e9149385be01bd901dac6b2a44f0cb497dd34d5e5f00c59a",
    "0b3e": "b479b9355db3e237e9149385be01bd901dac6b2a44f0cb497dd34d5e5f00c59a",
    "0b44": "4191e7da295ad18c4d51a628baff25ba0abe040a690eba1c6612101f951c9fd3",
    "0371": "4191e7da295ad18c4d51a628baff25ba0abe040a690eba1c6612101f951c9fd3",
    "196f": "1baad8fc261b2bbf80d5434e34ea1637e2c2ced7985aff77a706deb113cbe816",
}
CONTROL_HASHES = {
    "041a": "d05a25d10f718600cbbfbc74baa7b45ae2f8f4d30b84a92804dac4ea4f59e98e",
    "0b3d": "0f6ccc9f2def1e647eb7c319585c8805d6492f1ab44f144f37bee2cdcf2c8a67",
    "0b3e": "0f6ccc9f2def1e647eb7c319585c8805d6492f1ab44f144f37bee2cdcf2c8a67",
    "0b44": LIVE_HASHES["196f"],
    "041f": LIVE_HASHES["196f"],
}
RELOCATIONS = (0x19, 0x2D, 0x83, 0x8E, 0xA9, 0xC7)


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


def snapshots(root: Path, hashes: dict[str, str], prefix: str = "") -> dict[str, bytes]:
    return {label: checked(root / f"ram-{prefix}{label}.bin", 655360, expected)
            for label, expected in hashes.items()}


def transcript(root: Path, name: str, expected_hash: str,
               labels: list[str]) -> dict[str, list[int]]:
    data = (root / name).read_bytes()
    assert digest(data) == expected_hash, f"wrong transcript hash: {name}"
    rows = [line.split() for line in data.decode("ascii").splitlines()]
    assert [row[0] for row in rows] == labels
    assert all(len(row) == 17 for row in rows)
    return {row[0]: [int(field, 16) for field in row[1:]] for row in rows}


def check_code(root: Path, ram: bytes) -> None:
    start = checked(root / "POOLRAD/START.EXE", 65231, START_HASH)
    game = checked(root / "POOLRAD/GAME.OVR", 206105, GAME_HASH)
    checked(root / "POOLRAD/SAVGAMA.DAT", 13137, SAVE_HASH)

    original_call = game[0x1DFE6 + 0x196A:0x1DFE6 + 0x196F]
    loaded_call = ram[physical(0x2A8E, 0x196A):physical(0x2A8E, 0x196F)]
    assert digest(original_call) == CALL_ORIGINAL_HASH
    assert digest(loaded_call) == CALL_LOADED_HASH
    assert original_call[0] == loaded_call[0] == 0x9A
    assert word(original_call, 1) == word(loaded_call, 1) == 0x0356
    assert word(original_call, 3) == 0x0802
    assert word(loaded_call, 3) == 0x1026

    original_body = start[0x083AD:0x0847C]
    loaded_body = ram[physical(0x1026, 0x0356):physical(0x1026, 0x0425)]
    assert len(original_body) == len(loaded_body) == 207
    assert digest(original_body) == BODY_ORIGINAL_HASH
    assert digest(loaded_body) == BODY_LOADED_HASH
    assert {i for i, (a, b) in enumerate(zip(original_body, loaded_body)) if a != b} == {
        i + delta for i in RELOCATIONS for delta in (0, 1)
    }
    for index in RELOCATIONS:
        assert word(original_body, index) == 0x0AF8
        assert word(loaded_body, index) == 0x131C

    final_offset = 0x041A - 0x0356
    original_final = original_body[final_offset:final_offset + 5]
    loaded_final = loaded_body[final_offset:final_offset + 5]
    assert digest(original_final) == FINAL_CALL_ORIGINAL_HASH
    assert digest(loaded_final) == FINAL_CALL_LOADED_HASH
    assert original_final[0] == loaded_final[0] == 0x9A
    assert word(original_final, 1) == word(loaded_final, 1) == 0x0B25
    assert word(original_final, 3) == 0x0AF8
    assert word(loaded_final, 3) == 0x131C

    original_copy = start[0x0B79B:0x0B7BF]
    loaded_copy = ram[physical(0x131C, 0x0B25):physical(0x131C, 0x0B49)]
    assert digest(original_copy) == COPY_HASH and loaded_copy == original_copy
    assert digest(original_copy[0x0B3D - 0x0B25:0x0B3E - 0x0B25]) == STOSB_HASH
    assert digest(original_copy[0x0B42 - 0x0B25:0x0B44 - 0x0B25]) == REP_MOVSB_HASH


def check(root: Path) -> None:
    live = snapshots(root, LIVE_HASHES)
    control = snapshots(root, CONTROL_HASHES, "controlled-")
    restored = checked(root / "ram-controlled-restored-196a.bin", 655360,
                       LIVE_HASHES["196a"])
    assert restored == live["196a"]
    live_cpu = transcript(root, "ev-resident-live.txt", LIVE_EV_HASH, list(LIVE_HASHES))
    control_cpu = transcript(root, "ev-resident-controlled.txt", CONTROL_EV_HASH,
                             [*CONTROL_HASHES, "196f"])
    check_code(root, live["0356"])

    assert live_cpu["196a-first"] == live_cpu["196a"]
    assert live["196a-first"][physical(0x1D16, 0x3C01)] == 1
    assert live["196a"][physical(0x1D16, 0x3C01)] == 2
    assert live_cpu["196a"][:2] == [0x2A8E, 0x196A]
    assert live_cpu["0356"][:2] == [0x1026, 0x0356]
    assert live_cpu["0356"][7] == 0x37A4
    assert live_cpu["0357"][:2] == [0x1026, 0x0357]
    assert live_cpu["196f"][:2] == [0x2A8E, 0x196F]
    assert live_cpu["196f"][7] == 0x37B0
    assert live_cpu["196f"] == control_cpu["196f"]

    stack = physical(0x1D16, 0)
    assert [word(live["196a"], stack + i) for i in (0x37A8, 0x37AA, 0x37AC,
                                                     0x37AE, 0x37B0, 0x37B2)] == [
        0x3BB6, 0x1D16, 0x3CAB, 0x1D16, 0x3AB6, 0x1D16
    ]
    assert [word(live["0356"], stack + i) for i in (0x37A4, 0x37A6)] == [
        0x196F, 0x2A8E
    ]
    assert changed(live["196a"], live["0356"]) == {0x20904, 0x20905}
    assert changed(live["0356"], live["0357"]) == {0x20902, 0x20903}
    assert changed(live["0b3d"], live["0b3e"]) == set()
    assert changed(live["0b3e"], live["0b44"]) == set(range(0x20806, 0x2080B))
    assert changed(live["0b44"], live["0371"]) == set()
    assert changed(live["196a"], live["196f"]) == (
        set(range(0x205F6, 0x20600)) | set(range(0x20700, 0x20704)) |
        set(range(0x20806, 0x2080B)) | set(range(0x20902, 0x20906)) |
        set(range(0x20C18, 0x20C1F))
    )

    assert control_cpu["041a"][:2] == [0x1026, 0x041A]
    assert control_cpu["041a"][7] == 0x3494
    assert [word(control["041a"], stack + i) for i in
            (0x3494, 0x3496, 0x3498, 0x349A, 0x349C)] == [
        0x00FF, 0x3AB6, 0x1D16, 0x36A2, 0x1D16
    ]
    assert [word(control["0b3d"], stack + i) for i in (0x3490, 0x3492)] == [
        0x041F, 0x1026
    ]
    assert control_cpu["0b3d"][:2] == [0x131C, 0x0B3D]
    assert control_cpu["0b3d"][3] == 0x1D16  # ES
    assert control_cpu["0b3d"][14] == 0x3AB6  # DI before STOSB
    assert control_cpu["0b3e"][14] == 0x3AB7  # DI after STOSB
    assert control_cpu["0b44"][11] == 0  # CX after REP MOVSB
    assert control_cpu["0b44"][14] == 0x3ABF
    assert control_cpu["041f"][:2] == [0x1026, 0x041F]
    assert control_cpu["041f"][7] == 0x349E
    assert control_cpu["196f"][:2] == [0x2A8E, 0x196F]
    assert control_cpu["196f"][7] == 0x37B0
    assert changed(control["041a"], control["0b3d"]) == set(range(0x205F0, 0x205F4))
    assert changed(control["0b3d"], control["0b3e"]) == set()
    assert changed(control["0b3e"], control["0b44"]) == set(range(0x20C18, 0x20C1F))
    assert control["0b44"] == control["041f"] == live["196f"]

    source = physical(0x1D16, 0x36A2)
    destination = physical(0x1D16, 0x3AB6)
    selected_name = physical(0x1D16, 0x3CAB)
    assert all(ram[source] == ram[destination] == 8 for ram in
               (control["0b3d"], control["0b3e"], control["0b44"]))
    assert control["0b3e"][source:source + 9] != control["0b3e"][destination:destination + 9]
    assert control["0b44"][source:source + 9] == control["0b44"][destination:destination + 9]
    assert live["196f"][destination:destination + 9] == live["196f"][selected_name:selected_name + 9]
    assert changed(live["196a"][0x39960:0x3A960], live["196f"][0x39960:0x3A960]) == set()


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: check_slot_a_u6f_196a_resident_native.py IGNORED_RUN_DIR")
    check(Path(sys.argv[1]))
    print("saved-slot +196A resident native boundary and controlled writer agree")
