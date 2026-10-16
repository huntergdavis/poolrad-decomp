#!/usr/bin/env python3
"""Replay U3BD +0211 through two supplied, saved tile-callee returns.

This checks the caller against one same-run 640 KiB native trace. The real
tile callee is supplied as captured post-call RAM/CPU, not translated here.
"""

import argparse
import ctypes
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
RAM_SIZE = 1 << 20
CAPTURE_SIZE = 0xA0000
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SOURCE_SHA = "b4b09f211e63b7db646d1ba6c81681fd7111fec7955ab99c01db3d3a91397e03"
CAPTURE_SHA = {
    "ram-0860-precall.bin": "6d724167613df028dc0a1d70368f18d356ea33e59efd8cd64ded9a6ffacf6639",
    "ram-02fb-first-pre.bin": "bb427b275705cf61112d6c3265f7ec89d5804557cd701fc8ee3a41d39c88fe11",
    "ram-0300-first-post.bin": "86f0057f914682f0f3e0fa73f402ef045b28531bf3bfbb4448b301dae426a438",
    "ram-02fb-second-pre.bin": "14e0c78b39eebcaf5ed41d6a5193530ef7b842ba20a9a77aeee4475154490649",
    "ram-0300-second-post.bin": "dbbed3185775c591e3d67c60319846c74396cc545bdf8a2c4707676f7cd38c10",
    "ram-0863-placement-return.bin": "d2601750ebcb98252bc38f31fc5a0e1fffd4c551151fd74de7f0de0fad2236c3",
}


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in
                "ax bx cx dx si di bp sp cs ip ds es ss flags".split()]


ENTRY = (0, 0, 4, 0x000D, 0x3E68, 0x004D, 0x3EB6, 0x3E94,
         0x2D14, 0x0860, 0x14A0, 0x3DA1, 0x1D16, 0x7206)
TILE_PRE = (
    (1, 0, 0, 0, 0x3E68, 0x0890, 0x3E90, 0x3E74,
     0x0C4C, 0x002A, 0x14A0, 0x3D0E, 0x1D16, 0x7246),
    (1, 0, 1, 0, 0x3E68, 0x0891, 0x3E90, 0x3E74,
     0x0C4C, 0x002A, 0x14A0, 0x3D0E, 0x1D16, 0x7246),
)
TILE_POST = (
    (7, 0, 0, 0x00FF, 0x3E68, 0x006E, 0x3E90, 0x3E82,
     0x2D14, 0x0300, 0x14A0, 0x3DE4, 0x1D16, 0x7246),
    (7, 0, 0, 0, 0x3E68, 0x007E, 0x3E90, 0x3E82,
     0x2D14, 0x0300, 0x14A0, 0x3DE4, 0x1D16, 0x7246),
)
RETURN = (5, 0, 0, 0, 0x3E68, 0x007E, 0x3EB6, 0x3E9E,
          0x2D14, 0x0863, 0x14A0, 0x3DE4, 0x1D16, 0x7246)


def cpu_words(cpu):
    return tuple(getattr(cpu, name) for name, _ in Cpu._fields_)


def set_cpu(cpu, words):
    for (name, _), value in zip(Cpu._fields_, words):
        setattr(cpu, name, value)


def physical(segment, offset):
    return ((segment << 4) + offset) & (RAM_SIZE - 1)


def put_word(ram, segment, offset, value):
    at = physical(segment, offset)
    ram[at] = value & 0xFF
    ram[(at + 1) & (RAM_SIZE - 1)] = value >> 8


def read_capture(directory, name):
    data = (directory / name).read_bytes()
    assert len(data) == CAPTURE_SIZE, (name, len(data))
    assert sha256(data).hexdigest() == CAPTURE_SHA[name], name
    return data


def assert_ram(label, actual, expected):
    if actual != expected:
        first = next(i for i, (a, b) in enumerate(zip(actual, expected)) if a != b)
        raise AssertionError(f"{label}: first RAM mismatch at physical {first:#x}: "
                             f"C={actual[first]:02x}, capture={expected[first]:02x}")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture-dir", required=True, type=Path)
    parser.add_argument("--original", required=True, type=Path)
    args = parser.parse_args()
    game = args.original.read_bytes()
    assert len(game) == 206105 and sha256(game).hexdigest() == GAME_SHA
    source = game[0x2EB08:0x2EC16]
    assert len(source) == 270 and sha256(source).hexdigest() == SOURCE_SHA
    captures = {name: read_capture(args.capture_dir, name) for name in CAPTURE_SHA}

    memory = bytearray(RAM_SIZE)
    memory[:CAPTURE_SIZE] = captures["ram-0860-precall.bin"]
    loaded = bytearray(source)
    loaded[0x02FE - 0x0211:0x0300 - 0x0211] = (0x0C4C).to_bytes(2, "little")
    at = physical(0x2D14, 0x0211)
    assert memory[at:at + len(loaded)] == loaded
    # The native +0860 frame is before its near CALL; PUSH CS has happened.
    put_word(memory, 0x1D16, 0x3E92, 0x0863)
    cpu = Cpu(*ENTRY)
    cpu.sp, cpu.ip = 0x3E92, 0x0211
    seen = []
    failures = []

    with tempfile.TemporaryDirectory(prefix="poolrad-u3bd-0211-return-") as temp:
        library = Path(temp) / "model.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Wpedantic",
                        "-Werror", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u3bd_place_0211.c"),
                        "-o", str(library)], check=True)
        function = ctypes.CDLL(str(library)).pr_u3bd_place_0211
        callback_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.POINTER(ctypes.c_uint8),
                                         ctypes.c_size_t, ctypes.POINTER(Cpu),
                                         ctypes.c_void_p)
        function.argtypes = (ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                             ctypes.POINTER(Cpu), callback_type, ctypes.c_void_p)
        function.restype = ctypes.c_int

        @callback_type
        def saved_tile_return(ram, ram_size, state, context):
            del ram, context
            try:
                index = len(seen)
                assert index < 2 and ram_size == RAM_SIZE
                label = ("first", "second")[index]
                assert cpu_words(state.contents) == TILE_PRE[index], label
                expected = bytearray(captures[f"ram-02fb-{label}-pre.bin"])
                # Native +02FB is before LCALL. C has already pushed IP/CS.
                far_at = physical(0x1D16, 0x3E74)
                expected[far_at:far_at + 4] = bytes.fromhex("0003142d")
                assert_ram(label + " callback prestate", memory[:CAPTURE_SIZE], expected)
                memory[:CAPTURE_SIZE] = captures[f"ram-0300-{label}-post.bin"]
                set_cpu(state.contents, TILE_POST[index])
                seen.append(label)
                return 0
            except Exception as exc:
                failures.append(str(exc))
                return 1

        ram = (ctypes.c_uint8 * RAM_SIZE).from_buffer(memory)
        result = function(ram, RAM_SIZE, ctypes.byref(cpu), saved_tile_return, None)
        assert not failures, failures
        assert result == 0 and seen == ["first", "second"], (result, seen)

    assert cpu_words(cpu) == RETURN, (cpu_words(cpu), RETURN)
    assert_ram("+0863 return", memory[:CAPTURE_SIZE],
               captures["ram-0863-placement-return.bin"])
    print("U3BD +0211 supplied-callee saved return: 2/2 callback prestates "
          "and +0863 CPU14/640KiB exact; real callee and upper 384KiB open")


if __name__ == "__main__":
    main()
