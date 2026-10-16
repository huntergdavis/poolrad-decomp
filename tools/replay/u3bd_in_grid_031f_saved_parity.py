#!/usr/bin/env python3
"""Compare the complete U3BD +031F C leaf with a saved reached native frame."""

from __future__ import annotations

import argparse
import ctypes
import hashlib
from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
LEAF_SHA = "ab4d7f5e3d42b8d23dba77a563b5041f669850990444583890627278764f281b"
CAPTURE_SHA = {
    "ram-07f9-before.bin": "0bf854b0b7f7324779cb6905f7ef70ce39d22e2efa39a111f0c748637bd74ad5",
    "ram-031f-entry.bin": "e76369ece75277656911157e71f8789566f5ed29f0d598c50fe3bc3efb236ef8",
    "ram-080a-before-branch.bin": "94656b315a93486dff325d530db006586b3e89b1c2fbc3b2d0903dc2c7d76e9e",
}


class Cpu(ctypes.Structure):
    _fields_ = [(name, ctypes.c_uint16) for name in (
        "ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
        "cs", "ip", "ds", "es", "ss", "flags")]


def sha(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--original", type=Path,
                        default=ROOT / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--capture-dir", type=Path,
                        default=ROOT / "resources/oracle-runs/slot-a-u3bd-entry5-call-chain-p5-027")
    args = parser.parse_args()
    original = args.original.read_bytes()
    assert sha(original) == GAME_SHA
    leaf = original[0x2EC16:0x2EC47]
    assert len(leaf) == 49 and sha(leaf) == LEAF_SHA
    captures = {name: (args.capture_dir / name).read_bytes()
                for name in CAPTURE_SHA}
    for name, data in captures.items():
        assert len(data) == 0xA0000 and sha(data) == CAPTURE_SHA[name], name
    entry = captures["ram-031f-entry.bin"]
    after = captures["ram-080a-before-branch.bin"]
    loaded = entry[(0x2D14 << 4) + 0x031F:(0x2D14 << 4) + 0x0350]
    assert loaded == leaf
    assert [i for i, (a, b) in enumerate(zip(entry, after)) if a != b] == [
        0x20FF3, 0x20FF4, 0x20FF5]

    with tempfile.TemporaryDirectory(prefix="poolrad-u3bd-031f-") as temp:
        library = Path(temp) / "leaf.so"
        subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                        "-pedantic", "-shared", "-fPIC", "-I", str(ROOT / "include"),
                        str(ROOT / "src/recovered/overlay_u3bd_in_grid_031f.c"),
                        "-o", str(library)], check=True)
        model = ctypes.CDLL(str(library))
        function = model.pr_u3bd_in_grid_031f
        function.argtypes = [ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
                             ctypes.POINTER(Cpu)]
        function.restype = ctypes.c_int
        ram = (ctypes.c_uint8 * 0x100000)()
        ctypes.memmove(ram, entry, len(entry))
        cpu = Cpu(0x0004, 0, 4, 0x000D, 0x3E68, 0x004D, 0x3EB6, 0x3E96,
                  0x2D14, 0x031F, 0x14A0, 0x3DA1, 0x1D16, 0x7202)
        assert function(ram, len(ram), ctypes.byref(cpu)) == 0
        assert bytes(ram)[:len(after)] == after
        assert bytes(ram)[len(after):] == bytes(0x100000 - len(after))
        expected = (0x0001, 0, 4, 0x000D, 0x3E68, 0x004D, 0x3EB6, 0x3E9E,
                    0x2D14, 0x0808, 0x14A0, 0x3DA1, 0x1D16, 0x7202)
        assert tuple(getattr(cpu, name) for name, _ in Cpu._fields_) == expected

    print("U3BD +031F: original/loaded 49-byte SHA exact")
    print("saved native +031F -> +080A: 640 KiB RAM exact, 3 changed bytes")
    print("CPU leaf return matches manual post-OR EV tuple; no saved EV transcript")
    print("one in-range frame only; branch-complete original-byte parity remains open")


if __name__ == "__main__":
    main()
