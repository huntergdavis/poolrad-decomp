#!/usr/bin/env python3
"""Hash-locked saved-A first dirty-row A200 staging audit.

Private RAM and plane data are read only from ignored resources/. Public
output contains addresses, counts and hashes only.
"""

from __future__ import annotations

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "resources/oracle-runs/slot-a-view4-last6-compose-p5-001"
GC_RUN = ROOT / "resources/oracle-runs/slot-a-u40c-all-dirty-p5-001"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
PREFIX_SHA = "4a9724b105a51c627c182d056235edba06e19830cb4c7ee7ae7bdeb2d0bcacae"
BODY_SHA = "7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c"
RAM_SHA = {
    "ram-flush-entry.bin": "48becc9f9848794f232e6887a08edfd8b59d901833ccffbbbea4c336245080d7",
    "ram-flush-return.bin": "db0fc49dad03cc5a45ececba4017f2eddf51d608f1c5993608334d63f777085f",
}
PRE_PLANE_SHA = "9f1dcbc35c350d6027f98be0f5c8b43b42ca52b7604459c0c42be3aa88913d47"
POST_PLANE_SHA = (
    "82250cc0d633b5433774d5bc0f599862f7e001ca1678c1c6160a8ce4bf349103",
    "b02181363785115b0b6f83d1abf1fa5f31978d10f683f24c4732d3cd383c109f",
    "1e1c9a9fcfeccbe9b79a33de8c00192a20c1868ee7dfd2b953d672b2b324e355",
    "41c647bc7f2fb8b76278413779fa97cb516e378ec9a00a0cecb1381684a01a70",
)
GC_SHA = "4a474c34200525498a5a223630e9a858db3902bb713d4191c70b7214d5bd6ce5"
DS = 0x14A00
SS = 0x1D160
OBJECT_AT = 0x3DE48
OBJECT_SIZE = 14135
OBJECT_SHA = "489572698be6487b4d9c88b470f62b530e0bd98539c99b75444a76b3639b14e1"
PLANE_SIZE = 8192


class Row(ctypes.Structure):
    _fields_ = [("dirty", ctypes.c_uint8), ("first", ctypes.c_uint16),
                ("last", ctypes.c_uint16), ("low", ctypes.c_uint16),
                ("stage_base", ctypes.c_uint16)]


class State(ctypes.Structure):
    _fields_ = [("row", Row * 168), ("page_base", ctypes.c_uint16),
                ("map_mask", ctypes.c_uint8 * 4),
                ("gc_set_reset", ctypes.c_uint8),
                ("gc_enable_set_reset", ctypes.c_uint8),
                ("gc_rotate", ctypes.c_uint8),
                ("gc_mode", ctypes.c_uint8),
                ("gc_bit_mask", ctypes.c_uint8),
                ("vram_origin", ctypes.c_uint16),
                ("vram_size", ctypes.c_size_t),
                ("staging", ctypes.POINTER(ctypes.c_uint8) * 4),
                ("page", ctypes.POINTER(ctypes.c_uint8) * 4)]


class Report(ctypes.Structure):
    _fields_ = [("row", ctypes.c_uint), ("first", ctypes.c_uint16),
                ("last", ctypes.c_uint16), ("stage_offset", ctypes.c_size_t),
                ("bytes_per_plane", ctypes.c_size_t),
                ("plane_writes", ctypes.c_size_t)]


def pinned(path: Path, expected: str) -> bytes:
    data = path.read_bytes()
    assert sha256(data).hexdigest() == expected, path.name
    return data


def word(data: bytes, at: int) -> int:
    return int.from_bytes(data[at:at + 2], "little")


def main() -> None:
    exe = pinned(ROOT / "resources/DOS/POOLRAD/START.EXE", EXE_SHA)
    assert sha256(exe[0x61CB:0x63A0]).hexdigest() == PREFIX_SHA
    assert sha256(exe[0x61CB:0x64D5]).hexdigest() == BODY_SHA
    # This instruction-bounded prefix includes other-mode code but follows
    # only the observed video-state-1 branch at +1B5D and +1BCA.
    stores = [exe[0x61CB + ip - 0x1B52:0x61CB + ip - 0x1B52 + 3]
              for ip in (0x1C37, 0x1C7E, 0x1CC6, 0x1D0F)]
    assert all(len(store) == 3 and store == stores[0] for store in stores)

    first = pinned(RUN / "ram-flush-entry.bin", RAM_SHA["ram-flush-entry.bin"])
    native_return = pinned(RUN / "ram-flush-return.bin",
                           RAM_SHA["ram-flush-return.bin"])
    assert len(first) == len(native_return) == 0xA0000
    assert (word(first, SS + 0x3E94), word(first, SS + 0x3E96),
            word(first, SS + 0x3E9A), word(first, SS + 0x3E9C)) == (
                0x05FB, 0x2D14, 0x0DAE, 0x2D14)
    assert first[DS + 0x4A1E] == 1
    object_at = word(first, DS + 0x4A20) + 16 * word(first, DS + 0x4A22)
    assert object_at == OBJECT_AT
    object_bytes = first[object_at:object_at + OBJECT_SIZE]
    assert len(object_bytes) == OBJECT_SIZE
    assert sha256(object_bytes).hexdigest() == OBJECT_SHA
    assert [r for r in range(168) if first[DS + 0x4A28 + r]] == list(range(16, 104))
    assert tuple(first[DS + 0x25F6:DS + 0x25FA]) == (1, 2, 4, 8)

    # GC registers were directly read at +1B52 in a separate saved-route
    # capture, not in this same-run plane capture.
    gc_lines = pinned(GC_RUN / "gc-flush-entry.txt", GC_SHA).decode("ascii").splitlines()
    assert gc_lines[:4] == [
        "source=heavy debugger output captured in this run",
        "stop=0C88:1B52", "return=2D14:05FB", "ds=14A0",
    ] and len(gc_lines) == 17 and gc_lines[-1] == "OUTP 3CE 05"
    gc_values = []
    for index, register in enumerate((0, 1, 3, 4, 5, 8)):
        assert gc_lines[4 + 2 * index] == f"OUTP 3CE {register:02X}"
        assert gc_lines[5 + 2 * index].startswith("INP 3CF -> ")
        gc_values.append(int(gc_lines[5 + 2 * index].split()[-1], 16))
    assert tuple(gc_values) == (0, 0, 0, 0, 0, 0xFF)

    stage_before = []
    stage_after = []
    for plane in range(4):
        before = pinned(RUN / f"a200-plane{plane}-pre.bin", PRE_PLANE_SHA)
        after = pinned(RUN / f"a200-plane{plane}-post.bin",
                       POST_PLANE_SHA[plane])
        assert len(before) == len(after) == PLANE_SIZE
        stage_before.append(before)
        stage_after.append(after)

    state = State()
    (state.gc_set_reset, state.gc_enable_set_reset, state.gc_rotate,
     _read_map, state.gc_mode, state.gc_bit_mask) = gc_values
    state.vram_size = PLANE_SIZE
    for row in range(168):
        entry = state.row[row]
        entry.dirty = first[DS + 0x4A28 + row]
        entry.first = word(first, DS + 0x4C20 + 2 * row)
        entry.last = word(first, DS + 0x4D70 + 2 * row)
        entry.low = word(first, DS + 0x4AD0 + 2 * row)
        entry.stage_base = word(first, DS + 0x5050 + 2 * (row + 8))
    plane_buffers = [(ctypes.c_uint8 * PLANE_SIZE).from_buffer_copy(data)
                     for data in stage_before]
    for plane in range(4):
        state.map_mask[plane] = first[DS + 0x25F6 + plane]
        state.staging[plane] = plane_buffers[plane]
    native_object = (ctypes.c_uint8 * OBJECT_SIZE).from_buffer_copy(object_bytes)

    with TemporaryDirectory(prefix="poolrad-first-stage-row-") as temp:
        library = Path(temp) / "first_stage_row.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-fPIC", "-shared", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/resident_464_first_stage_row.c"),
                        "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).pr_r464_first_stage_row
        function.argtypes = [ctypes.POINTER(State), ctypes.c_uint8,
                             ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                             ctypes.POINTER(Report)]
        function.restype = ctypes.c_int
        report = Report()
        assert function(ctypes.byref(state), 1, native_object, OBJECT_SIZE,
                        ctypes.byref(report)) == 0
    assert (report.row, report.first, report.last, report.stage_offset,
            report.bytes_per_plane, report.plane_writes) == (
                16, 1352, 1395, 0x03C3, 11, 44)

    changed = []
    for plane in range(4):
        actual = bytes(plane_buffers[plane])
        before = stage_before[plane]
        after = stage_after[plane]
        start, end = report.stage_offset, report.stage_offset + report.bytes_per_plane
        assert actual[start:end] == after[start:end]
        assert actual[:start] == before[:start] and actual[end:] == before[end:]
        assert all(object_bytes[0x17 + report.first + plane + 4 * column] ==
                   after[start + column]
                   for column in range(report.bytes_per_plane))
        changed.append(sum(a != b for a, b in zip(before[start:end],
                                                   after[start:end])))
    assert changed == [11, 11, 0, 11]
    print("RES_0464:1B52..1D27 first dirty-row staging prefix: "
          f"469 bytes sha256 {PREFIX_SHA}")
    print("same-run flush entry/return RAM sha256 "
          f"{RAM_SHA['ram-flush-entry.bin']} / "
          f"{RAM_SHA['ram-flush-return.bin']}")
    print("row 16 source [1352,1395], A200 offset 0x03C3, "
          "four planes 44/44 bytes C/native post-return match; "
          "native changes by plane 11/11/0/11")
    print("comparison is to immediate native return, not an intermediate "
          "+1D27 stop; no page/RGB parity claim")


if __name__ == "__main__":
    main()
