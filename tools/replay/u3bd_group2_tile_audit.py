#!/usr/bin/env python3
"""Replay saved view-three group-two tiles on one portable C object.

All original media, RAM images, and art bytes remain under ignored resources/.
Only addresses, counts, and hashes are printed.
"""

from __future__ import annotations

import ctypes
from contextlib import redirect_stdout
from hashlib import sha256
from io import StringIO
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory

import u3bd_view2_group2_boundary_audit as bridge


ROOT = Path(__file__).resolve().parents[2]
RUN = bridge.RUN
OBJECT_SIZE = bridge.OBJECT_SIZE
OBJECT_AT = bridge.OBJECT_AT
EXTRA_HASHES = {
    "ram-view3-call-4.bin": "a74235dbf8599f4222d4ca6731a21e83e331827c71083d7226a1b83eb7a433ae",
    "ram-view3-exit.bin": "415a784085e65b78b89e08987ea04ad0534add878e8c7d88e39e434ea078d27b",
}
CALLS = (
    ("ram-view3-call-2.bin", "ram-view3-call-3.bin", 143, 6, 16),
    ("ram-view3-call-3.bin", "ram-view3-call-4.bin", 144, 7, 18),
    ("ram-view3-call-4.bin", "ram-view3-exit.bin", 143, 8, 24),
)


class Group2State(ctypes.Structure):
    _fields_ = [
        ("item_id", ctypes.c_uint16), ("group2_remap", ctypes.c_uint16),
        ("x", ctypes.c_uint8), ("y", ctypes.c_uint8),
        ("flag", ctypes.c_uint8), ("video_state", ctypes.c_uint8),
        ("source", ctypes.POINTER(ctypes.c_uint8)),
        ("source_size", ctypes.c_size_t),
        ("aux_mask", ctypes.POINTER(ctypes.c_uint8)),
        ("aux_mask_size", ctypes.c_size_t),
        ("destination", ctypes.POINTER(ctypes.c_uint8)),
        ("destination_size", ctypes.c_size_t),
    ]


class Group2Report(ctypes.Structure):
    _fields_ = [
        ("item_index", ctypes.c_uint16),
        ("first_data_offset", ctypes.c_size_t),
        ("first_changed_offset", ctypes.c_size_t),
        ("bytes_written", ctypes.c_uint),
        ("bytes_changed", ctypes.c_uint),
    ]


def capture(name: str) -> bytes:
    if name in bridge.RAM_HASHES:
        return bridge.capture(name)
    data = (RUN / name).read_bytes()
    assert len(data) == 0xA0000
    assert sha256(data).hexdigest() == EXTRA_HASHES[name]
    return data


def main() -> None:
    with redirect_stdout(StringIO()):
        bridge.main()
    original = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert sha256(original).hexdigest() == bridge.prior.EXE_SHA
    assert sha256(original[0x4F4D:0x53EA]).hexdigest() == (
        "9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe")
    assert sha256(original[0x52E3:0x52E7]).hexdigest() == (
        "f1798d82c73cbb1d10f18e8d3d5f89265a778c5e2ea9cdfaca48e0dd4d68ec90")
    ram = {name: capture(name) for pair in CALLS for name in pair[:2]}
    destination = (ctypes.c_uint8 * OBJECT_SIZE).from_buffer_copy(
        bridge.object_at(ram[CALLS[0][0]]))

    with TemporaryDirectory(prefix="poolrad-u40c-group2-") as tmp:
        library = Path(tmp) / "group2.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u40c_group2_tile_plane.c"),
            "-o", str(library),
        ], check=True)
        model = ctypes.CDLL(str(library)).pr_u40c_entry2_group2_tile_plane
        model.argtypes = [ctypes.POINTER(Group2State),
                          ctypes.POINTER(Group2Report)]
        model.restype = ctypes.c_int

        for pre_name, next_name, item, y, changed in CALLS:
            pre = ram[pre_name]
            native_next = bridge.object_at(ram[next_name])
            assert bytes(destination) == bridge.object_at(pre)
            assert bridge.prior.far_pointer(pre, 0x14A0, 0x4A20) == OBJECT_AT
            args = [bridge.prior.word(pre, 0x1D16, 0x3E78 + 2 * i)
                    for i in range(5)]
            assert args == [1, 1, item, y, 2]
            assert bridge.prior.word(pre, 0x1D16, 0x3E90 + 2) == 0x0AA9
            assert bridge.prior.word(pre, 0x14A0, 0x2726) == 116
            source_at = bridge.prior.far_pointer(pre, 0x14A0, 0x5E42)
            mask_at = bridge.prior.far_pointer(
                pre,
                bridge.prior.word(pre, 0x14A0, 0x5E44),
                bridge.prior.word(pre, 0x14A0, 0x5E42) + 0x13)
            index = item - 116
            source_len = 0x17 + (index + 1) * 32
            mask_len = (index + 1) * 8
            source = (ctypes.c_uint8 * source_len).from_buffer_copy(
                pre[source_at:source_at + source_len])
            mask = (ctypes.c_uint8 * mask_len).from_buffer_copy(
                pre[mask_at:mask_at + mask_len])
            state = Group2State(
                item_id=item, group2_remap=116, x=2, y=y,
                flag=1, video_state=pre[bridge.prior.physical(0x14A0, 0x4A1E)],
                source=source, source_size=source_len,
                aux_mask=mask, aux_mask_size=mask_len,
                destination=destination, destination_size=OBJECT_SIZE)
            report = Group2Report()
            assert model(ctypes.byref(state), ctypes.byref(report)) == 0
            assert report.item_index == index
            assert report.bytes_written == 32
            assert report.bytes_changed == changed
            assert bytes(destination) == native_next

    assert sha256(bytes(destination)).hexdigest() == (
        "e2a8d70f54501d664f3d47e50e0b614d563f36dbc3cde6eb15de6210d1c0ce94")
    print("prior 26-stop first-view and six-stop view-two audits passed")
    print("U40C group-two view-three: 3 C calls / 96 plane-byte stores")
    print("next-stop net changes 16/18/24; 14,135/14,135 full native parity")
    print("one C destination retained through +0AA9; no later/page claim")


if __name__ == "__main__":
    main()
