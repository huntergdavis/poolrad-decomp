#!/usr/bin/env python3
"""Check ten saved-slot-A U3BD tile effects on one persistent C object.

Owner DOS inputs and all RAM captures live only under ignored resources/.
The public script contains addresses, lengths, hashes, and callback tuples.
"""

from __future__ import annotations

import ctypes
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "resources/oracle-runs/slot-a-u3bd-view1-object-join-p5-033"
ORIGINAL = ROOT / "resources/DOS/POOLRAD"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
TRANSCRIPT_SHA = "0e52d38d532a5a0eec6b58304497313ddc299db84279a34242902676a19fc109"
MANIFEST_SHA = "2d58f9e1a960dfeeffd969f939759fa8e7b1bbb60dfe695f0f4d40fa649dc219"
LAUNCH_SHA = "570038924034eba9ab00f2cad6929b268735e15bc8395b60ebe7f1bc6211274c"
EMULATOR_SHA = "c05ce28a60258c320c5a209a477603d91fb2b7558c0cc03919e518ddd99eb457"
OBJECT_SHA_START = "39962babb659d2eb872c4548dcf7e592d4ee5c2c51672cba8964a16ed79a00e9"
OBJECT_SHA_END = "15ef1f9670025967b98c18778f0e9cb0d5e6ca96b77b48e12e92a5ff57d9ffbc"
OBJECT_SIZE = 0x17 + 21 * 168 * 4
FIELDS = ("CS", "IP", "AX", "BX", "CX", "DX", "SI", "DI",
          "BP", "SP", "DS", "ES", "SS", "FLAGS")
EV = re.compile(rb"LOG: EV of '([A-Z]+)' is:\r?\nLOG: ([0-9a-f]+)\r?\n")
MANIFEST_LINE = re.compile(r"([0-9a-f]{64})  (ram-[a-z0-9-]+\.bin)")
LABELS = ("first", "second", "third", "fourth", "fifth", "sixth",
          "v1-1", "v1-2", "v1-3", "v1-4")
TUPLES = ((7, 6, 1), (7, 7, 13), (8, 6, 1), (8, 7, 13),
          (9, 6, 1), (9, 7, 13), (6, 5, 1), (6, 6, 18),
          (6, 7, 26), (6, 8, 28))
PARENTS = (0x0863, 0x0863, 0x0847, 0x0847, 0x0863, 0x0863,
           0x0938, 0x0938, 0x0938, 0x0938)
CHANGED = (0, 0, 0, 9, 0, 9, 0, 14, 15, 22)
NAMES = (
    "0860-pre", "02fb-first-pre", "0300-first-post",
    "02fb-second-pre", "0300-second-post", "0863-return",
    "02fb-third-pre", "0300-third-post",
    "02fb-fourth-pre", "0300-fourth-post", "0860-second-pre",
    "02fb-fifth-pre", "0300-fifth-post",
    "02fb-sixth-pre", "0300-sixth-post", "0863-final-return",
    "08e8-scan-entry",
    "02fb-v1-1-pre", "0300-v1-1-post",
    "02fb-v1-2-pre", "0300-v1-2-post",
    "02fb-v1-3-pre", "0300-v1-3-post",
    "02fb-v1-4-pre", "0300-v1-4-post", "097f-scan-exit",
)
IPS = (
    0x0860, 0x02FB, 0x0300, 0x02FB, 0x0300, 0x0863,
    0x02FB, 0x0300, 0x02FB, 0x0300, 0x0860,
    0x02FB, 0x0300, 0x02FB, 0x0300, 0x0863, 0x08E8,
    0x02FB, 0x0300, 0x02FB, 0x0300,
    0x02FB, 0x0300, 0x02FB, 0x0300, 0x097F,
)
SPANS = (
    ("GAME.OVR", 0x2EB08, 0x2EC16,
     "b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03"),
    ("GAME.OVR", 0x2F11B, 0x2F15C,
     "23a4689e3551c41ef9e029f4870be70705cc261c672d55599262ceffb15b9fab"),
    ("GAME.OVR", 0x2F15C, 0x2F1B1,
     "3af5d1fd4ecf9689eab13e72fc118b7e4517fd55e692632698b138554b7b1bf8"),
    ("GAME.OVR", 0x2F1DF, 0x2F276,
     "ecab0ea500549a1daa4b74f6e852c4f28c3340c19a43c690dc9cef7dd898eb0a"),
    ("GAME.OVR", 0x32392, 0x324E2,
     "2a8a7826dcf35576b32a0aa02862f4d7bb43c985918f9b2cb170acf0cbe9df00"),
    ("START.EXE", 0x4F4D, 0x53EA,
     "9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe"),
)


class TileState(ctypes.Structure):
    _fields_ = [
        ("item_id", ctypes.c_uint16), ("group0_remap", ctypes.c_uint16),
        ("x", ctypes.c_uint8), ("y", ctypes.c_uint8),
        ("flag", ctypes.c_uint8), ("video_state", ctypes.c_uint8),
        ("source", ctypes.POINTER(ctypes.c_uint8)),
        ("source_size", ctypes.c_size_t),
        ("aux_mask", ctypes.POINTER(ctypes.c_uint8)),
        ("aux_mask_size", ctypes.c_size_t),
        ("destination", ctypes.POINTER(ctypes.c_uint8)),
        ("destination_size", ctypes.c_size_t),
    ]


class TileReport(ctypes.Structure):
    _fields_ = [
        ("item_index", ctypes.c_uint16),
        ("first_data_offset", ctypes.c_size_t),
        ("first_changed_offset", ctypes.c_size_t),
        ("bytes_written", ctypes.c_uint),
        ("bytes_changed", ctypes.c_uint),
    ]


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def physical(segment: int, offset: int) -> int:
    return ((segment << 4) + (offset & 0xFFFF)) & 0xFFFFF


def word(ram: bytes, segment: int, offset: int) -> int:
    at = physical(segment, offset)
    return ram[at] | (ram[at + 1] << 8)


def far_pointer(ram: bytes, segment: int, offset: int) -> int:
    return physical(word(ram, segment, offset + 2),
                    word(ram, segment, offset))


def load_captures() -> tuple[list[dict[str, int]], dict[str, bytes]]:
    manifest = (RUN / "capture.sha256").read_bytes()
    transcript = (RUN / "debugger.typescript").read_bytes()
    assert digest(manifest) == MANIFEST_SHA
    assert digest(transcript) == TRANSCRIPT_SHA
    rows = [MANIFEST_LINE.fullmatch(line) for line in
            manifest.decode("ascii").splitlines()]
    assert all(rows)
    hashes = {match.group(2): match.group(1) for match in rows}
    assert len(hashes) == len(NAMES)
    assert set(hashes) == {f"ram-{name}.bin" for name in NAMES}
    ram = {}
    for name in NAMES:
        data = (RUN / f"ram-{name}.bin").read_bytes()
        assert len(data) == 0xA0000 and digest(data) == hashes[f"ram-{name}.bin"]
        ram[name] = data

    parts = transcript.split(b"LOG: DEBUG: Memory dump binary success.")
    assert len(parts) - 1 == len(NAMES)
    cpu = []
    for part, ip in zip(parts[:-1], IPS):
        values = EV.findall(part)
        assert len(values) >= len(FIELDS)
        values = values[-len(FIELDS):]
        assert tuple(key.decode() for key, _ in values) == FIELDS
        frame = {key.decode(): int(value, 16) for key, value in values}
        assert (frame["CS"], frame["IP"], frame["DS"], frame["SS"]) == (
            0x2D14, ip, 0x14A0, 0x1D16)
        cpu.append(frame)
    return cpu, ram


def main() -> None:
    launcher = (RUN / "launch-recorded.sh").read_bytes()
    assert digest(launcher) == LAUNCH_SHA
    player_lines = [
        line.removeprefix("player=") for line in
        launcher.decode("ascii").splitlines() if line.startswith("player=/")
    ]
    assert len(player_lines) == 1
    assert digest(Path(player_lines[0]).read_bytes()) == EMULATOR_SHA
    originals = {
        "GAME.OVR": (ORIGINAL / "GAME.OVR").read_bytes(),
        "START.EXE": (ORIGINAL / "START.EXE").read_bytes(),
    }
    assert digest(originals["GAME.OVR"]) == OVR_SHA
    assert digest(originals["START.EXE"]) == EXE_SHA
    assert digest((ORIGINAL / "SAVGAMA.DAT").read_bytes()) == SAVE_SHA
    for filename, lo, hi, expected in SPANS:
        assert digest(originals[filename][lo:hi]) == expected

    cpu, ram = load_captures()
    ds = cpu[0]["DS"]
    ss = cpu[0]["SS"]
    first = ram["0860-pre"]
    dest_at = far_pointer(first, ds, 0x4A20)
    assert dest_at == 0x3DE48
    assert digest(first[dest_at:dest_at + OBJECT_SIZE]) == OBJECT_SHA_START
    destination = (ctypes.c_uint8 * OBJECT_SIZE).from_buffer_copy(
        first[dest_at:dest_at + OBJECT_SIZE])

    with tempfile.TemporaryDirectory(prefix="poolrad-u3bd-object-chain-") as tmp:
        library = Path(tmp) / "tile.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u40c_tile_plane.c"),
            "-o", str(library),
        ], check=True)
        model = ctypes.CDLL(str(library)).pr_u40c_entry2_mode1_tile_plane
        model.argtypes = [ctypes.POINTER(TileState), ctypes.POINTER(TileReport)]
        model.restype = ctypes.c_int

        for index, label in enumerate(LABELS):
            pre = ram[f"02fb-{label}-pre"]
            post = ram[f"0300-{label}-post"]
            pre_cpu = cpu[NAMES.index(f"02fb-{label}-pre")]
            assert pre_cpu["SP"] == 0x3E78 and pre_cpu["BP"] == 0x3E90
            assert far_pointer(pre, ds, 0x4A20) == dest_at
            assert bytes(destination) == pre[dest_at:dest_at + OBJECT_SIZE]
            args = [word(pre, ss, pre_cpu["SP"] + 2 * i) for i in range(5)]
            assert args[:2] == [1, 1]
            assert (args[4], args[3], args[2]) == TUPLES[index]
            assert word(pre, ss, pre_cpu["BP"] + 2) == PARENTS[index]
            assert word(pre, ds, 0x2722) == 1

            source_at = far_pointer(pre, ds, 0x5E3A)
            mask_at = far_pointer(
                pre, word(pre, ds, 0x5E3C), word(pre, ds, 0x5E3A) + 0x13)
            source_len = 0x17 + 45 * 32
            mask_len = 45 * 8
            source = (ctypes.c_uint8 * source_len).from_buffer_copy(
                pre[source_at:source_at + source_len])
            mask = (ctypes.c_uint8 * mask_len).from_buffer_copy(
                pre[mask_at:mask_at + mask_len])
            state = TileState(
                item_id=args[2], group0_remap=1, x=args[4], y=args[3],
                flag=args[0], video_state=pre[physical(ds, 0x4A1E)],
                source=source, source_size=source_len,
                aux_mask=mask, aux_mask_size=mask_len,
                destination=destination, destination_size=OBJECT_SIZE)
            report = TileReport()
            assert model(ctypes.byref(state), ctypes.byref(report)) == 0
            assert report.bytes_written == 32
            assert report.bytes_changed == CHANGED[index]
            assert bytes(destination) == post[dest_at:dest_at + OBJECT_SIZE]

            # These are native joins in this run, never cross-run RAM pairs.
            if index == 1:
                assert bytes(destination) == ram["0863-return"][
                    dest_at:dest_at + OBJECT_SIZE]
            elif index == 3:
                assert bytes(destination) == ram["0860-second-pre"][
                    dest_at:dest_at + OBJECT_SIZE]
            elif index == 5:
                for stop in ("0863-final-return", "08e8-scan-entry"):
                    assert bytes(destination) == ram[stop][
                        dest_at:dest_at + OBJECT_SIZE]

    assert bytes(destination) == ram["097f-scan-exit"][
        dest_at:dest_at + OBJECT_SIZE]
    assert digest(bytes(destination)) == OBJECT_SHA_END
    assert sum(CHANGED) == 69
    print("U3BD +0860 to +097F: 26 native CPU/RAM stops hash locked")
    print("ten tile calls, 320 modeled plane stores, 69 immediate changed bytes")
    print("persistent C/native offscreen object: 14,135/14,135 at every stop")
    print("no visible-page, full-entry, or strict full-C claim")


if __name__ == "__main__":
    main()
