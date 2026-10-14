#!/usr/bin/env python3
"""Check one reached DOS RES_0464:0000 frame against C, without emitting media."""

import argparse
import ctypes as c
from hashlib import sha256
from pathlib import Path

from check_resident_464_reverse_1484_x86 import CPU, State
from resident_464_bitmask_audit import collect
from resident_loader_map import unpack

HASHES = {
    "cpu-entry.txt": "c974b3b8d3ae0e4dc41a75815d5e4104dbf47c687fa8939fe574aa1bb2cc6a3f",
    "cpu-return.txt": "521778e9e26e8f6e48db5774fb4f87c53ad4777524cadec4ada17167a7cd6039",
    "ram-entry.bin": "68d9966d6075358b0bea1fe7eaaa8f0b507e7ef69e4cefd2010d2508b374722f",
    "ram-return.bin": "84ddb66f11e2b5c4b95db03247a247923bbaf6fca17f0c912ad0c750f9753bf7",
}
ORDER = "cs ip ds es ss fs gs sp bp ax bx cx dx si di flags".split()


def cpu_row(data, label):
    fields = data.decode("ascii").split()
    assert len(fields) == 17 and fields[0] == label
    return CPU(**dict(zip(ORDER, (int(v, 16) for v in fields[1:]))))


def check(root, library):
    data = {name: (root / name).read_bytes() for name in HASHES}
    for name, digest in HASHES.items():
        assert sha256(data[name]).hexdigest() == digest, name
    before, after = data["ram-entry.bin"], data["ram-return.bin"]
    assert len(before) == len(after) == 0x100000
    cpu = cpu_row(data["cpu-entry.txt"], "entry")
    expected_cpu = cpu_row(data["cpu-return.txt"], "return")
    exe = (root / "POOLRAD/START.EXE").read_bytes()
    evidence = collect(exe)
    image, _, _, _ = unpack(exe)
    assert cpu.cs == expected_cpu.cs == 0x0c88
    assert cpu.ip == 0 and expected_cpu.ip == 0x067e
    loaded = cpu.cs << 4
    assert before[loaded:loaded + 123] == bytes(image[0x4640:0x46bb])
    assert sha256(before[loaded:loaded + 123]).hexdigest() == evidence["sha256"]
    assert before[loaded + 0x067b:loaded + 0x067e] == bytes(image[0x4cbb:0x4cbe])
    stack = (cpu.ss << 4) + cpu.sp
    assert int.from_bytes(before[stack:stack + 2], "little") == 0x067e
    assert int.from_bytes(before[stack + 2:stack + 4], "little") == cpu.cs
    assert before[(cpu.ds << 4) + 0x4a1e] == 1
    ram = (c.c_uint8 * len(before)).from_buffer_copy(before)
    state = State(cpu, ram, len(before))
    model = c.CDLL(str(library.resolve()))
    model.pr_r464_bitmask_frame.argtypes = [c.POINTER(State)]
    model.pr_r464_bitmask_frame.restype = c.c_int
    assert model.pr_r464_bitmask_frame(c.byref(state)) == 1
    for name, _ in CPU._fields_:
        assert getattr(state.cpu, name) == getattr(expected_cpu, name), name
    assert bytes(ram) == after, "complete 1 MiB RAM differs"
    changes = {i for i, (a, b) in enumerate(zip(before, after)) if a != b}
    assert changes == set(range(0x20bb9, 0x20bc0))
    print("reached RES_0464:0000 native/C parity: 16 CPU fields and 1 MiB RAM match")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, required=True)
    parser.add_argument("--library", type=Path, required=True)
    args = parser.parse_args()
    check(args.root, args.library)
