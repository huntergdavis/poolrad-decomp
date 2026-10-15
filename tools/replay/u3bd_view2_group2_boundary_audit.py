#!/usr/bin/env python3
"""Check the saved +097F continuation up to its first unmodeled tile effect.

The original executable, save, and RAM captures stay under ignored resources/.
This audit never copies the native group-two post-object into the C object.
"""

from __future__ import annotations

import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
from tempfile import TemporaryDirectory

import u3bd_first_view_object_chain_audit as prior


ROOT = Path(__file__).resolve().parents[2]
RUN = ROOT / "resources/oracle-runs/slot-a-art-next-p5-005"
OBJECT_SIZE = prior.OBJECT_SIZE
OBJECT_AT = 0x3DE48
RAM_HASHES = {
    "ram-097f.bin": "f929cd194f8f8a25630300912bbf80f41c061b6ec1053f71e7148b17dcd00036",
    "ram-0a16.bin": "6f2283b0f580cf2125d016171819c02d36426647af734a9a3ea0d8c6c883fa21",
    "ram-next-tile.bin": "6f580d8efda4efac89e11dcf1279117a21858bbb88993c48cb9236bea97355e3",
    "ram-view3-post-1.bin": "8ee6479222d4eac1e2b3d365c319a3f023ab07b61535a78f41dbe0e2a693c5b4",
    "ram-view3-call-2.bin": "0acd6237ac19684c78de680464c07e4e8b67cd88c0f48004fe97ed57910cb643",
    "ram-view3-call-3.bin": "8605782d3fb90c0c6f33ff14e83c5abe67203aa58e0aea6fcfb0f47205373f8b",
}
SPANS = (
    (0x097F, 0x0A16, "cffa2359b10e558baf9e85c444ca0524bbf21d5f7430c0c6ea719e6a8d7ca522"),
    (0x0A19, 0x0A7A, "273127afaa4fec0c268aaddf137e9a6d66b74817a1375713bb3884bbd6ab204b"),
    (0x0A7A, 0x0AA9, "61f65a27f0a5bbfae08e76491d8c44c3d7d6a9f77598d37bca99c83b19054c1b"),
    (0x0211, 0x031F, "b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03"),
)


def object_at(ram: bytes) -> bytes:
    return ram[OBJECT_AT:OBJECT_AT + OBJECT_SIZE]


def capture(name: str) -> bytes:
    data = (RUN / name).read_bytes()
    assert len(data) == 0xA0000
    assert sha256(data).hexdigest() == RAM_HASHES[name]
    return data


def main() -> None:
    # The reviewed ten-call model supplies a C object with the same +097F
    # hash. Its run and the continuation run are separate native executions.
    prior.main()
    original = (ROOT / "resources/DOS/POOLRAD/GAME.OVR").read_bytes()
    assert sha256(original).hexdigest() == prior.OVR_SHA
    for lo, hi, expected in SPANS:
        assert sha256(original[0x2E8F7 + lo:0x2E8F7 + hi]).hexdigest() == expected
    assert sha256((RUN / "POOLRAD/GAME.OVR").read_bytes()).hexdigest() == prior.OVR_SHA
    assert sha256((RUN / "POOLRAD/START.EXE").read_bytes()).hexdigest() == prior.EXE_SHA

    ram = {name: capture(name) for name in RAM_HASHES}
    start = object_at(ram["ram-097f.bin"])
    assert sha256(start).hexdigest() == prior.OBJECT_SHA_END
    assert object_at(ram["ram-0a16.bin"]) == start
    first_pre = ram["ram-next-tile.bin"]
    first_post = ram["ram-view3-post-1.bin"]
    second_pre = ram["ram-view3-call-2.bin"]
    third_pre = ram["ram-view3-call-3.bin"]
    assert object_at(first_pre) == start
    assert prior.far_pointer(first_pre, 0x14A0, 0x4A20) == OBJECT_AT
    assert first_pre[prior.physical(0x14A0, 0x4A1E)] == 1

    # The first view-three tile is the supported group-zero item 1. The
    # second tile is deliberately outside this model and is never replayed.
    args = [prior.word(first_pre, 0x1D16, 0x3E78 + 2 * i)
            for i in range(5)]
    assert args == [1, 1, 1, 5, 2]
    assert prior.word(first_pre, 0x1D16, 0x3E90 + 2) == 0x0AA9
    assert [prior.word(first_pre, 0x14A0, 0x2722 + 2 * i)
            for i in range(5)] == [1, 46, 116, 186, 256]
    source_at = prior.far_pointer(first_pre, 0x14A0, 0x5E3A)
    mask_at = prior.far_pointer(
        first_pre,
        prior.word(first_pre, 0x14A0, 0x5E3C),
        prior.word(first_pre, 0x14A0, 0x5E3A) + 0x13)
    source = (ctypes.c_uint8 * (0x17 + 45 * 32)).from_buffer_copy(
        first_pre[source_at:source_at + 0x17 + 45 * 32])
    mask = (ctypes.c_uint8 * (45 * 8)).from_buffer_copy(
        first_pre[mask_at:mask_at + 45 * 8])
    destination = (ctypes.c_uint8 * OBJECT_SIZE).from_buffer_copy(start)
    state = prior.TileState(
        item_id=1, group0_remap=1, x=2, y=5, flag=1, video_state=1,
        source=source, source_size=len(source), aux_mask=mask,
        aux_mask_size=len(mask), destination=destination,
        destination_size=OBJECT_SIZE)
    with TemporaryDirectory(prefix="poolrad-u3bd-view2-boundary-") as tmp:
        library = Path(tmp) / "tile.so"
        subprocess.run([
            "cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            str(ROOT / "src/recovered/overlay_u40c_tile_plane.c"),
            "-o", str(library),
        ], check=True)
        model = ctypes.CDLL(str(library)).pr_u40c_entry2_mode1_tile_plane
        model.argtypes = [ctypes.POINTER(prior.TileState),
                          ctypes.POINTER(prior.TileReport)]
        model.restype = ctypes.c_int
        report = prior.TileReport()
        assert model(ctypes.byref(state), ctypes.byref(report)) == 0
        assert report.bytes_written == 32 and report.bytes_changed == 0
    assert bytes(destination) == object_at(first_post)
    assert bytes(destination) == object_at(second_pre)

    second_args = [prior.word(second_pre, 0x1D16, 0x3E78 + 2 * i)
                   for i in range(5)]
    assert second_args == [1, 1, 143, 6, 2]
    assert prior.word(second_pre, 0x1D16, 0x3E90 + 2) == 0x0AA9
    assert 116 <= second_args[2] < 186  # group two, index 27
    assert sum(a != b for a, b in zip(object_at(second_pre),
                                     object_at(third_pre))) == 16
    print("U3BD +097F to view-three second tile: six same-run RAM stops hash locked")
    print("view-two unchanged; first group-zero C/native object 14,135/14,135")
    print("first unsupported effect: group-two item 143, 16 net object bytes")
    print("stopped before group-two model, later 615-byte writer, or visible page")


if __name__ == "__main__":
    main()
