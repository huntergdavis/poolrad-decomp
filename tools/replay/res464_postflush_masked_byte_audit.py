#!/usr/bin/env python3
"""Audit one saved first-view RES_0464 masked-byte instruction span.

Inputs remain under ignored resources/. Public output is address/count/hash
evidence only. Build the C source as a shared library and pass --library.
"""

import argparse
import ctypes as c
from hashlib import sha256
from pathlib import Path
import re
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[1] / "overlay"))
from resident_loader_map import unpack

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
SPAN_SHA = "b351be8dfc49d233bbf25a6a75e0fe42f05cbeaac0a0b0478c2ee0cb3c32e16e"
HASHES = {
    "debugger.typescript": "94bd094a46af7643cb2107fa9eefbcf06f9ca94ddb528eaef942ed2f0b39c7c0",
    "ram-05fb-postflush.bin": "9a75ef82fc1b40abbae5b07da222536ada56112bbd5ebf1b76e28d6117d9dfa1",
    "ram-first-tile-pre.bin": "a3b95229eef5a7196dd46211698ec88fdbc0fb0a51c4cd71267a495949e0a57a",
    "ram-first-tile-post.bin": "5d22699b7eef8ec1e41ef76316190ad8ffc52c2b8f1d9950d45e343c1e6d6d10",
    "ram-second-tile-pre.bin": "01a66a7733af0c5b8c9b5a089472afd13702551dfaecf39ba5d61f2ccc698c8f",
    "ram-writer-plane2-pre.bin": "593dba840d087788915c47fc65ef4c16c72bb54794f8b76a644bbf7f7d2d1158",
    "ram-writer-plane2-post.bin": "c2daf7aad210497fef3f90ab6d90a180b057f12ca23820685c2d327ddaaedff3",
}
ORDER = "CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS".split()


class CPU(c.Structure):
    _fields_ = [(n, c.c_uint16) for n in
                "ax bx cx dx si di bp sp cs ds es ss fs gs ip flags".split()]


class State(c.Structure):
    _fields_ = [("cpu", CPU), ("ram", c.POINTER(c.c_uint8)),
                ("ram_size", c.c_size_t)]


def cpu_rows(transcript):
    events = re.findall(
        r"LOG: EV of '([A-Z]+)' is:\s*LOG: ([0-9a-fA-F]+)",
        transcript.decode("utf-8", errors="replace"),
    )
    rows = []
    for i in range(len(events) - len(ORDER) + 1):
        frame = events[i:i + len(ORDER)]
        if [name for name, _ in frame] == ORDER:
            rows.append({name.lower(): int(value, 16) for name, value in frame})
    pre = next(row for row in rows if row["cs"] == 0x0C88 and row["ip"] == 0x0C3D)
    post = next(row for row in rows if row["cs"] == 0x0C88 and row["ip"] == 0x0C6E)
    assert len(rows) >= 2
    return pre, post


def check(exe, run, library):
    assert sha256(exe).hexdigest() == START_SHA
    image, _, _, _ = unpack(exe)
    span = bytes(image[0x527D:0x52AE])
    assert len(span) == 49 and sha256(span).hexdigest() == SPAN_SHA
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16
    instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, 0x0C3D))
    assert len(instructions) == 17
    assert instructions[-1].address + instructions[-1].size == 0x0C6E
    assert instructions[-1].mnemonic == "mov"

    data = {name: (run / name).read_bytes() for name in HASHES}
    for name, digest in HASHES.items():
        assert sha256(data[name]).hexdigest() == digest, name
    for name in HASHES:
        if name.endswith(".bin"):
            assert len(data[name]) == 0xA0000, name
    pre = data["ram-writer-plane2-pre.bin"]
    post = data["ram-writer-plane2-post.bin"]
    loaded = (0x0C88 << 4) + 0x0C3D
    assert pre[loaded:loaded + len(span)] == span
    first_pre = data["ram-first-tile-pre.bin"]
    first_post = data["ram-first-tile-post.bin"]
    object_base, object_size = 0x3DE48, 0x17 + 14112
    assert first_pre[object_base:object_base + object_size] == (
        first_post[object_base:object_base + object_size])
    assert data["ram-second-tile-pre.bin"][0x3F0DD] == pre[0x3F0DD]

    before_cpu, after_cpu = cpu_rows(data["debugger.typescript"])
    assert before_cpu["di"] == 0x0182 and before_cpu["es"] == 0x417B
    assert after_cpu["di"] == 0x1286 and after_cpu["es"] == 0x3DE4
    ram = (c.c_uint8 * len(pre)).from_buffer_copy(pre)
    cpu = CPU(**before_cpu)
    state = State(cpu, ram, len(pre))
    model = c.CDLL(str(library.resolve()))
    model.pr_r464_masked_byte_0c3d.argtypes = [c.POINTER(State)]
    model.pr_r464_masked_byte_0c3d.restype = c.c_int
    assert model.pr_r464_masked_byte_0c3d(c.byref(state)) == 0
    for name in before_cpu:
        assert getattr(state.cpu, name) == after_cpu[name], name
    assert bytes(ram) == post, "640 KiB RAM differs"
    changes = [i for i, a in enumerate(pre) if a != post[i]]
    assert changes == [0x3F0DD]
    assert (0x3F0DD - object_base - 0x17) % 4 == 2
    print("RES_0464:+0C3D..+0C6E: 17 instructions, 14 CPU fields, "
          "640 KiB RAM; one plane-2 byte changed at 0x3F0DD")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--library", type=Path, required=True)
    args = parser.parse_args()
    check(args.exe.read_bytes(), args.run, args.library)
