#!/usr/bin/env python3
"""Check the saved U3BD +0211 placement and two tile-data effects."""

from __future__ import annotations

import ctypes
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16


ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
RUN = ROOT / "resources/oracle-runs/slot-a-u3bd-art-effect-p5-032"
ORIGINAL_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03"
LOADED_SHA = "3514808da1ae78c2476bc9eabafdef19a30cb66ee9f6cc31ee8ae05aea9e1f6a"
TRANSCRIPT_SHA = "5ee5e4eea81e528867a5539e8c2edef2476efe0b6807e22a33f479292b8a6d44"
NAMES = (
    "0860-precall", "02fb-first-pre", "0300-first-post",
    "02fb-second-pre", "0300-second-post", "0863-placement-return",
)
RAM_SHA = (
    "6d724167613df028dc0a1d70368f18d356ea33e59efd8cd64ded9a6ffacf6639",
    "bb427b275705cf61112d6c3265f7ec89d5804557cd701fc8ee3a41d39c88fe11",
    "86f0057f914682f0f3e0fa73f402ef045b28531bf3bfbb4448b301dae426a438",
    "14e0c78b39eebcaf5ed41d6a5193530ef7b842ba20a9a77aeee4475154490649",
    "dbbed3185775c591e3d67c60319846c74396cc545bdf8a2c4707676f7cd38c10",
    "d2601750ebcb98252bc38f31fc5a0e1fffd4c551151fd74de7f0de0fad2236c3",
)
FIELDS = ("CS", "IP", "AX", "BX", "CX", "DX", "SI", "DI",
          "BP", "SP", "DS", "ES", "SS", "FLAGS")
EV = re.compile(rb"LOG: EV of '([A-Z]+)' is:\r?\nLOG: ([0-9a-f]+)\r?\n")


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


class State(ctypes.Structure):
    _fields_ = [
        ("wall", ctypes.c_uint8), ("origin_x", ctypes.c_uint16),
        ("origin_y", ctypes.c_uint8), ("view0_offset", ctypes.c_uint8),
        ("view0_columns", ctypes.c_uint8), ("view0_rows", ctypes.c_uint8),
        ("pieces", ctypes.POINTER(ctypes.c_uint8)),
        ("pieces_size", ctypes.c_size_t), ("tile", TileState),
    ]


class Report(ctypes.Structure):
    _fields_ = [
        ("cells_seen", ctypes.c_uint), ("tile_calls", ctypes.c_uint),
        ("plane_bytes_written", ctypes.c_uint),
        ("plane_bytes_changed", ctypes.c_uint),
        ("first_item", ctypes.c_uint8), ("last_item", ctypes.c_uint8),
        ("first_x", ctypes.c_uint16), ("first_y", ctypes.c_uint16),
        ("last_x", ctypes.c_uint16), ("last_y", ctypes.c_uint16),
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


def cpu_rows(transcript: bytes) -> list[dict[str, int]]:
    pairs = [(match.group(1).decode(), int(match.group(2), 16))
             for match in EV.finditer(transcript)]
    assert len(pairs) == len(NAMES) * len(FIELDS)
    rows = []
    for start in range(0, len(pairs), len(FIELDS)):
        row = pairs[start:start + len(FIELDS)]
        assert [name for name, _ in row] == list(FIELDS)
        rows.append(dict(row))
    assert [(row["CS"], row["IP"]) for row in rows] == [
        (0x2D14, ip) for ip in (0x0860, 0x02FB, 0x0300,
                               0x02FB, 0x0300, 0x0863)
    ]
    assert transcript.count(b"LOG: DEBUG: Memory dump binary success.") == 6
    return rows


def main() -> None:
    original = ORIGINAL.read_bytes()
    transcript = (RUN / "debugger.typescript").read_bytes()
    ram = [(RUN / f"ram-{name}.bin").read_bytes() for name in NAMES]
    assert digest(original) == ORIGINAL_SHA
    assert digest(transcript) == TRANSCRIPT_SHA
    assert all(len(image) == 0xA0000 for image in ram)
    assert tuple(map(digest, ram)) == RAM_SHA
    cpu = cpu_rows(transcript)

    start, end = 0x0211, 0x031F
    span = original[0x2E8F7 + start:0x2E8F7 + end]
    assert len(span) == 270 and digest(span) == SPAN_SHA
    loaded = physical(0x2D14, start)
    loaded_span = ram[0][loaded:loaded + len(span)]
    assert digest(loaded_span) == LOADED_SHA
    assert [start + at for at, (file_byte, live_byte)
            in enumerate(zip(span, loaded_span))
            if file_byte != live_byte] == [0x02FE, 0x02FF]
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, start))
    assert len(ins) == 108 and sum(item.size for item in ins) == len(span)
    assert ins[0].address == start and ins[-1].address + ins[-1].size == end

    changes = [
        [at for at, (a, b) in enumerate(zip(left, right)) if a != b]
        for left, right in zip(ram, ram[1:])
    ]
    assert [len(change) for change in changes] == [16, 13, 4, 9, 1]
    assert all(0x20F00 <= at < 0x21000 for change in changes for at in change)

    ss, ds = cpu[0]["SS"], cpu[0]["DS"]
    assert all(row["SS"] == ss and row["DS"] == ds for row in cpu)
    assert [word(ram[0], ss, cpu[0]["SP"] + 2 * i)
            for i in range(5)] == [0x2D14, 0, 14, 4, 5]
    assert [[word(ram[i], ss, cpu[i]["SP"] + 2 * j)
             for j in range(5)] for i in (1, 3)] == [
        [1, 1, 1, 6, 7], [1, 1, 13, 7, 7]
    ]

    pieces_at = far_pointer(ram[0], ds, 0x6A58)
    source_at = far_pointer(ram[1], ds, 0x5E3A)
    mask_at = far_pointer(ram[1],
                          word(ram[1], ds, 0x5E3C),
                          word(ram[1], ds, 0x5E3A) + 0x13)
    dest_at = far_pointer(ram[1], ds, 0x4A20)
    piece_count = 14 * 156
    source_count = 0x17 + 13 * 32
    mask_count = 13 * 8
    dest_count = 0x17 + 21 * 168 * 4
    pieces = (ctypes.c_uint8 * piece_count).from_buffer_copy(
        ram[0][pieces_at:pieces_at + piece_count])
    source = (ctypes.c_uint8 * source_count).from_buffer_copy(
        ram[1][source_at:source_at + source_count])
    mask = (ctypes.c_uint8 * mask_count).from_buffer_copy(
        ram[1][mask_at:mask_at + mask_count])
    dest = (ctypes.c_uint8 * dest_count).from_buffer_copy(
        ram[1][dest_at:dest_at + dest_count])
    state = State(
        wall=word(ram[0], ss, cpu[0]["SP"] + 4),
        origin_x=word(ram[0], ss, cpu[0]["SP"] + 8),
        origin_y=word(ram[0], ss, cpu[0]["SP"] + 6),
        view0_offset=ram[0][physical(ds, 0x0C8C)],
        view0_columns=ram[0][physical(ds, 0x0C96)],
        view0_rows=ram[0][physical(ds, 0x0CA0)],
        pieces=pieces, pieces_size=piece_count,
        tile=TileState(
            group0_remap=word(ram[1], ds, 0x2722), flag=1,
            video_state=ram[1][physical(ds, 0x4A1E)],
            source=source, source_size=source_count,
            aux_mask=mask, aux_mask_size=mask_count,
            destination=dest, destination_size=dest_count,
        ),
    )
    assert (state.wall, state.origin_x, state.origin_y,
            state.view0_offset, state.view0_columns,
            state.view0_rows) == (14, 5, 4, 0, 1, 2)

    with tempfile.TemporaryDirectory(prefix="poolrad-u3bd-art-join-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run([
            "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u3bd_0211_art_join.c"),
            str(ROOT / "src/recovered/overlay_u40c_tile_plane.c"),
            "-o", str(library),
        ], check=True)
        model = ctypes.CDLL(str(library)).pr_u3bd_0211_art_join
        model.argtypes = [ctypes.POINTER(State), ctypes.POINTER(Report)]
        model.restype = ctypes.c_int
        report = Report()
        assert model(ctypes.byref(state), ctypes.byref(report)) == 0

    assert (report.cells_seen, report.tile_calls,
            report.plane_bytes_written, report.plane_bytes_changed) == (2, 2, 64, 0)
    assert (report.first_item, report.first_x, report.first_y) == (1, 7, 6)
    assert (report.last_item, report.last_x, report.last_y) == (13, 7, 7)
    assert bytes(dest) == ram[-1][dest_at:dest_at + dest_count]
    assert all(before[dest_at:dest_at + dest_count] ==
               after[dest_at:dest_at + dest_count]
               for before, after in zip(ram, ram[1:]))

    print("U3BD +0211..+031F: 270 bytes/108 aligned instructions hash locked")
    print("same-run +0860 -> two +02FB/+0300 pairs -> +0863 CPU/RAM bound")
    print("native callbacks: 2, non-stack changes: 0, C object parity: exact")
    print("C tile writes: 64, changed plane bytes: 0; no visible-frame claim")


if __name__ == "__main__":
    main()
