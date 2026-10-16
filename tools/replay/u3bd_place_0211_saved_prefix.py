#!/usr/bin/env python3
"""Read-only saved-game U3BD +0211 prefix gate, stopping at its first tile call.

The native dump is before +02FB; the C model has prepared that far call.
Only its four newly pushed far-return bytes are excluded from the RAM diff.
This does not execute the tile callee or establish a full-return parity gate.
"""

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
CAPTURE_SHA = {
    "ram-0860-precall.bin": "6d724167613df028dc0a1d70368f18d356ea33e59efd8cd64ded9a6ffacf6639",
    "ram-02fb-first-pre.bin": "bb427b275705cf61112d6c3265f7ec89d5804557cd701fc8ee3a41d39c88fe11",
}
SOURCE_SHA = "b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03"
RAM_SIZE = 1 << 20


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


def physical(seg, off):
    return ((seg << 4) + off) & (RAM_SIZE - 1)


def put_word(ram, seg, off, value):
    address = physical(seg, off)
    ram[address] = value & 255
    ram[(address + 1) & (RAM_SIZE - 1)] = value >> 8


def read_capture(directory, name):
    data = (directory / name).read_bytes()
    assert len(data) == 0xA0000, (name, len(data))
    expected = CAPTURE_SHA[name]
    if expected is not None:
        assert sha256(data).hexdigest() == expected, name
    return data


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture-dir", required=True, type=Path)
    parser.add_argument("--original", required=True, type=Path)
    args = parser.parse_args()
    source = args.original.read_bytes()[0x2EB08:0x2EC16]
    assert len(source) == 270 and sha256(source).hexdigest() == SOURCE_SHA
    before = read_capture(args.capture_dir, "ram-0860-precall.bin")
    tile_pre = read_capture(args.capture_dir, "ram-02fb-first-pre.bin")
    loaded = before[physical(0x2D14, 0x0211):physical(0x2D14, 0x0211) + 270]
    expected_loaded = bytearray(source)
    expected_loaded[0x02FE - 0x0211:0x0300 - 0x0211] = (0x0C4C).to_bytes(2, "little")
    assert loaded == bytes(expected_loaded)

    memory = bytearray(RAM_SIZE)
    memory[:len(before)] = before
    # +0860 is a near CALL following PUSH CS. Its only entry-state write is
    # the return IP at SS:SP-2, and it leaves the arithmetic flags unchanged.
    put_word(memory, 0x1D16, 0x3E92, 0x0863)
    cpu = Cpu(0, 0, 4, 0x000D, 0x3E68, 0x004D, 0x3EB6, 0x3E92,
              0x2D14, 0x0211, 0x14A0, 0x3DA1, 0x1D16, 0x7206)

    with tempfile.TemporaryDirectory(prefix="poolrad-u3bd-0211-saved-") as temp:
        library = Path(temp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u3bd_place_0211.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library)).pr_u3bd_place_0211
        callback_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(ctypes.c_uint8),
                                         ctypes.c_size_t, ctypes.POINTER(Cpu),
                                         ctypes.c_void_p)
        model.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                          ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p)
        model.restype = ctypes.c_int

        @callback_type
        def stop_at_tile(ram, ram_size, state, context):
            del ram, ram_size, state, context
            return 1

        ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer(memory)
        assert model(ram, RAM_SIZE, ctypes.byref(cpu), stop_at_tile, None) == 3

    expected_cpu = (1, 0, 0, 0, 0x3E68, 0x0890, 0x3E90,
                    0x3E74, 0x0C4C, 0x002A, 0x14A0, 0x3D0E,
                    0x1D16, 0x7246)
    observed_cpu = tuple(getattr(cpu, name) for name, _ in Cpu._fields_)
    assert observed_cpu == expected_cpu, (observed_cpu, expected_cpu)
    far_return = {physical(0x1D16, off) for off in range(0x3E74, 0x3E78)}
    differences = [i for i, (actual, expected) in enumerate(zip(memory, tile_pre))
                   if actual != expected and i not in far_return]
    assert not differences, [hex(i) for i in differences[:16]]
    assert memory[physical(0x1D16, 0x3E74):physical(0x1D16, 0x3E74) + 4] == \
        bytes.fromhex("0003142d")
    print("U3BD +0211 saved natural +0860 -> first +02FB: 640KiB caller prefix "
          "and CPU14 exact; tile callee/whole return remain open")


if __name__ == "__main__":
    main()
