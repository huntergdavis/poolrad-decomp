#!/usr/bin/env python3
"""Bounded saved-A +0AD8 to +0AD5 C object comparison.

Original files and full RAM captures are read only from ignored resources/.
This script prints addresses, counts, and hashes; it never writes game data.
"""

from __future__ import annotations

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory

import u3bd_first_view_object_chain_audit as prior
from u3bd_group2_tile_audit import Group2Report, Group2State


ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "resources/oracle-runs/slot-a-art-view4-p5-006"
FIRST_SHA = "a5080458fedcb618628e6387c692141a24b1b1073b9cac7b86fdc86a76040fa7"
STOP_SHA = "50f0dfe340697048d8fefeb0b89bab79b68e343fabe79acb20260ac15ab576e3"
FIRST_OBJECT_SHA = "e2a8d70f54501d664f3d47e50e0b614d563f36dbc3cde6eb15de6210d1c0ce94"
STOP_OBJECT_SHA = "f54d5c29e66e399b7bf9ceae7685c1b83e2a00fd15cb55a7dd771a48a9c48e3d"
OBJECT_AT = 0x3DE48
OBJECT_SIZE = prior.OBJECT_SIZE
DS = 0x14A0
SS = 0x1D16
BP = 0x3EB6


class PrefixState(ctypes.Structure):
    _fields_ = [
        ("sample_x", ctypes.c_uint16),
        ("sample_y", ctypes.c_uint16),
        ("lateral_offset", ctypes.c_uint16),
        ("scan_index", ctypes.c_uint8),
        ("direction_index", ctypes.c_uint8),
        ("outer_selector", ctypes.c_uint8),
        ("facing", ctypes.c_uint8),
        ("map_id", ctypes.c_uint8),
        ("geo", ctypes.POINTER(ctypes.c_uint8)),
        ("geo_size", ctypes.c_size_t),
        ("pieces", ctypes.POINTER(ctypes.c_uint8)),
        ("pieces_size", ctypes.c_size_t),
        ("x_shift", ctypes.POINTER(ctypes.c_uint8)),
        ("y_shift", ctypes.POINTER(ctypes.c_uint8)),
        ("shift_count", ctypes.c_size_t),
        ("view3_offset", ctypes.c_uint8),
        ("view3_columns", ctypes.c_uint8),
        ("view3_rows", ctypes.c_uint8),
        ("view3_base_x", ctypes.c_uint16),
        ("view3_y", ctypes.c_uint8),
        ("lookup_frame_residue", ctypes.c_uint8),
    ]


class PrefixReport(ctypes.Structure):
    _fields_ = [
        ("sample_x", ctypes.c_uint16),
        ("sample_y", ctypes.c_uint16),
        ("lateral_offset", ctypes.c_uint16),
        ("scan_index", ctypes.c_uint8),
        ("view3_wall", ctypes.c_uint8 * 2),
        ("view4_wall", ctypes.c_uint8 * 2),
        ("calls_done", ctypes.c_uint),
    ]


DRAW = ctypes.CFUNCTYPE(
    ctypes.c_int, ctypes.c_void_p, ctypes.c_uint16, ctypes.c_uint16,
    ctypes.c_uint8, ctypes.c_uint16, ctypes.c_uint16,
)


def checked(name: str, digest: str) -> bytes:
    data = (RUN / name).read_bytes()
    assert len(data) == 0xA0000 and sha256(data).hexdigest() == digest
    return data


def word(ram: bytes, seg: int, off: int) -> int:
    at = seg * 16 + off
    return int.from_bytes(ram[at:at + 2], "little")


def far(ram: bytes, seg: int, off: int) -> int:
    return 16 * word(ram, seg, off + 2) + word(ram, seg, off)


def array(ram: bytes, at: int, size: int):
    assert 0 <= at <= len(ram) and 0 <= size <= len(ram) - at
    return (ctypes.c_uint8 * size).from_buffer_copy(ram[at:at + size])


def main() -> None:
    over = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    exe = (ROOT / "resources/DOS/POOLRAD/START.EXE").read_bytes()
    assert sha256(over).hexdigest() == prior.OVR_SHA
    assert sha256(exe).hexdigest() == prior.EXE_SHA
    for name, data, lo, hi, digest in (
        ("loop", over, 0x2E8F7 + 0x0A71, 0x2E8F7 + 0x0AD8,
         "f9b268663cd179e399a25f72b62ec4fdd98b1a1f75c5941e3e3c50ff23fc9219"),
        ("loop update", over, 0x2E8F7 + 0x0AD8, 0x2E8F7 + 0x0B06,
         "575f649bcdaff7feb5a4aae000a2363f7ab2a5023cf63abd7e8eff1d342b8f3b"),
        ("walker", over, 0x2EB08, 0x2EC16,
         "b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03"),
        ("tile", exe, 0x4F4D, 0x53EA,
         "9a486dce6109d9a6c2241339b9ae1f088829d4bd5b1a8128a21b1a32de1ec9fe"),
        ("store", exe, 0x52E3, 0x52E7,
         "f1798d82c73cbb1d10f18e8d3d5f89265a778c5e2ea9cdfaca48e0dd4d68ec90"),
    ):
        assert sha256(data[lo:hi]).hexdigest() == digest, name

    first = checked("ram-view4-zero-1.bin", FIRST_SHA)
    native_stop = checked("ram-view4-pre.bin", STOP_SHA)
    assert far(first, DS, 0x4A20) == far(native_stop, DS, 0x4A20) == OBJECT_AT
    assert sha256(first[OBJECT_AT:OBJECT_AT + OBJECT_SIZE]).hexdigest() == FIRST_OBJECT_SHA
    assert sha256(native_stop[OBJECT_AT:OBJECT_AT + OBJECT_SIZE]).hexdigest() == STOP_OBJECT_SHA
    assert first[DS * 16 + 0x84DC] == 20
    assert first[DS * 16 + 0x4A1E] == 1

    geo = array(first, far(first, DS, 0x6A5C), 1024)
    pieces = array(first, far(first, DS, 0x6A58), 2340)
    x_shift = array(first, DS * 16 + 0x2736, 9)
    y_shift = array(first, DS * 16 + 0x273F, 9)
    dest = array(first, OBJECT_AT, OBJECT_SIZE)
    state = PrefixState(
        sample_x=word(first, SS, BP - 0x0D),
        sample_y=word(first, SS, BP - 0x0F),
        lateral_offset=word(first, SS, BP - 0x12),
        scan_index=first[SS * 16 + BP - 0x10],
        direction_index=first[SS * 16 + BP - 2],
        outer_selector=first[SS * 16 + BP + 6],
        facing=first[SS * 16 + BP - 1],
        map_id=first[DS * 16 + 0x84DC],
        geo=geo, geo_size=len(geo),
        pieces=pieces, pieces_size=len(pieces),
        x_shift=x_shift, y_shift=y_shift, shift_count=len(x_shift),
        view3_offset=first[DS * 16 + 0x0C8F],
        view3_columns=first[DS * 16 + 0x0C99],
        view3_rows=first[DS * 16 + 0x0CA3],
        view3_base_x=word(first, DS, 0x0CB0),
        view3_y=first[DS * 16 + 0x0CC4],
        lookup_frame_residue=0,
    )
    assert (state.sample_x, state.sample_y, state.lateral_offset,
            state.scan_index, state.direction_index,
            state.outer_selector, state.facing) == (14, 6, 0xFFFA, 0, 0, 6, 4)

    with TemporaryDirectory(prefix="poolrad-u3bd-next-writer-") as temp:
        library = Path(temp) / "next_writer.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            *[str(ROOT / path) for path in (
                "src/recovered/overlay_u3bd_view3_next_writer.c",
                "src/recovered/overlay_u3bd_view3_wall.c",
                "src/recovered/overlay_u3bd_nibble.c",
                "src/recovered/overlay_u40c_tile_plane.c",
                "src/recovered/overlay_u40c_group2_tile_plane.c",
            )],
            "-o", str(library),
        ], check=True)
        dll = ctypes.CDLL(str(library))
        prefix = dll.pr_u3bd_entry5_view3_next_writer
        prefix.argtypes = [ctypes.POINTER(PrefixState), DRAW,
                           ctypes.c_void_p, ctypes.POINTER(PrefixReport)]
        prefix.restype = ctypes.c_int
        group0 = dll.pr_u40c_entry2_mode1_tile_plane
        group0.argtypes = [ctypes.POINTER(prior.TileState),
                           ctypes.POINTER(prior.TileReport)]
        group0.restype = ctypes.c_int
        group2 = dll.pr_u40c_entry2_group2_tile_plane
        group2.argtypes = [ctypes.POINTER(Group2State),
                           ctypes.POINTER(Group2Report)]
        group2.restype = ctypes.c_int

        calls: list[tuple[int, int, int, int]] = []
        errors: list[str] = []
        stores = 0
        first_change: tuple[int, int, int, int, int] | None = None

        def draw(_ctx: int, x: int, y: int, item: int,
                 one_a: int, one_b: int) -> int:
            nonlocal stores, first_change
            try:
                assert (one_a, one_b) == (1, 1)
                if 1 <= item <= 45:
                    index = item - word(first, DS, 0x2722)
                    source_at = far(first, DS, 0x5E3A)
                    source, mask = source_mask(first, source_at, index)
                    tile = prior.TileState(
                        item_id=item, group0_remap=1, x=x, y=y,
                        flag=1, video_state=1, source=source,
                        source_size=len(source), aux_mask=mask,
                        aux_mask_size=len(mask), destination=dest,
                        destination_size=OBJECT_SIZE)
                    tile_report = prior.TileReport()
                    assert group0(ctypes.byref(tile), ctypes.byref(tile_report)) == 0
                elif 116 <= item <= 185:
                    index = item - word(first, DS, 0x2726)
                    source_at = far(first, DS, 0x5E42)
                    source, mask = source_mask(first, source_at, index)
                    tile = Group2State(
                        item_id=item, group2_remap=116, x=x, y=y,
                        flag=1, video_state=1, source=source,
                        source_size=len(source), aux_mask=mask,
                        aux_mask_size=len(mask), destination=dest,
                        destination_size=OBJECT_SIZE)
                    tile_report = Group2Report()
                    assert group2(ctypes.byref(tile), ctypes.byref(tile_report)) == 0
                else:
                    raise AssertionError(("unsupported item", item))
                assert tile_report.bytes_written == 32
                calls.append((x, y, item, tile_report.bytes_changed))
                stores += tile_report.bytes_written
                if first_change is None and tile_report.bytes_changed:
                    first_change = (len(calls), x, y, item,
                                    tile_report.bytes_changed)
                return 0
            except Exception as exc:
                errors.append(repr(exc))
                return 1

        report = PrefixReport()
        callback = DRAW(draw)
        result = prefix(ctypes.byref(state), callback, None,
                        ctypes.byref(report))
        assert not errors, errors
        assert result == 0 and report.calls_done == 24
        assert (report.sample_x, report.sample_y, report.lateral_offset,
                report.scan_index) == (14, 4, 0, 2)
        assert list(report.view3_wall) == [9, 14]
        assert list(report.view4_wall) == [0, 15]
        assert len(calls) == 24 and stores == 768
        assert first_change == (4, 3, 6, 141, 16)

    actual_object = bytes(dest)
    expected_object = native_stop[OBJECT_AT:OBJECT_AT + OBJECT_SIZE]
    assert actual_object == expected_object
    assert sha256(actual_object).hexdigest() == STOP_OBJECT_SHA
    c_ram = bytearray(first)
    c_ram[OBJECT_AT:OBJECT_AT + OBJECT_SIZE] = actual_object
    residual = [at for at, (a, b) in enumerate(zip(c_ram, native_stop))
                if a != b]
    assert len(residual) == 47
    assert residual.count(0x46C) == 1
    assert all(at == 0x46C or SS * 16 + 0x3E2A <= at <= SS * 16 + 0x3EA7
               for at in residual)
    assert sum(a != b for a, b in zip(first, native_stop)) == 330
    print("U3BD +0AD8 to measured +0AD5: two view-three placements, 24 C calls")
    print("768 modeled plane-byte stores; first modeled change at call 4")
    print("native/C offscreen object 14,135/14,135 exact")
    print("full 640-KiB RAM comparison: 47 residual outside object "
          "(46 stack, 1 timer); no full-RAM or page parity claim")


def source_mask(ram: bytes, source_at: int, index: int):
    assert index >= 0
    source_size = 0x17 + (index + 1) * 32
    mask_size = (index + 1) * 8
    mask_at = (int.from_bytes(ram[source_at + 0x15:source_at + 0x17],
                              "little") * 16 +
               int.from_bytes(ram[source_at + 0x13:source_at + 0x15],
                              "little"))
    return (array(ram, source_at, source_size),
            array(ram, mask_at, mask_size))


if __name__ == "__main__":
    main()
