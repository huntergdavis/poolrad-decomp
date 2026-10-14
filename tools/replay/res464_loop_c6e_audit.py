#!/usr/bin/env python3
"""Check one saved-A RES_0464:+0C6E backedge against bounded C.

Original executables, native CPU transcript, and RAM dumps remain in ignored
resources/. Public output contains only code hashes, addresses and counts.
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
SPANS = {
    (0x0C6E, 0x0C8C): "f4cfc827c666bf9247381d9502d84832cbbb78b09db52dfd10e53f9b48213b8c",
    (0x0C31, 0x0C3D): "24af6a6df972ee2c136ee6c4ff0621ad734cc715d4962da735ca68d951a46077",
}
CAPTURES = {
    "debugger-interactive.typescript": "dabbee03b043fff154ee74f7b45e174e66b361813ba4bf8431752c7e3965cced",
    "ram-c6e-pre.bin": "3b28a2ed5e4eb656bccc18a91814cd7fdf4f11de126eeae9b5d9b5dc3eb051bc",
    "ram-c3d-next.bin": "c023d5e38f31a6a23faf0ceb278730b67464d66394b955cbbb2f9a7d2cc437d2",
}
ORDER = "CS IP AX BX CX DX SI DI BP SP DS ES SS FLAGS".split()
EXECUTED = (
    0x0C6E, 0x0C71, 0x0C74, 0x0C78, 0x0C81, 0x0C84, 0x0C87,
    0x0C8A, 0x0C31, 0x0C35, 0x0C38, 0x0C3B,
)


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
    pre = next(row for row in rows if row["cs"] == 0x0C88 and row["ip"] == 0x0C6E)
    post = next(row for row in rows if row["cs"] == 0x0C88 and row["ip"] == 0x0C3D)
    return pre, post


def check(exe, run, library):
    from capstone import Cs, CS_ARCH_X86, CS_MODE_16

    assert sha256(exe).hexdigest() == START_SHA
    image, _, _, _ = unpack(exe)
    data = {name: (run / name).read_bytes() for name in CAPTURES}
    for name, digest in CAPTURES.items():
        assert sha256(data[name]).hexdigest() == digest, name
    before, after = data["ram-c6e-pre.bin"], data["ram-c3d-next.bin"]
    assert len(before) == len(after) == 0xA0000
    addresses = set()
    for (lo, hi), digest in SPANS.items():
        body = bytes(image[0x4640 + lo:0x4640 + hi])
        assert sha256(body).hexdigest() == digest
        live = before[(0x0C88 << 4) + lo:(0x0C88 << 4) + hi]
        assert live == body
        instructions = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(body, lo))
        assert instructions[-1].address + instructions[-1].size == hi
        addresses.update(i.address for i in instructions)
    assert all(address in addresses for address in EXECUTED)
    assert len(EXECUTED) == 12

    pre, post = cpu_rows(data["debugger-interactive.typescript"])
    assert pre["es"] == 0x3DE4 and pre["di"] == 0x1286
    assert post["es"] == 0x417B and post["di"] == 0x0183
    ram = (c.c_uint8 * len(before)).from_buffer_copy(before)
    state = State(CPU(**pre), ram, len(before))
    model = c.CDLL(str(library.resolve()))
    model.pr_r464_loop_c6e.argtypes = [c.POINTER(State)]
    model.pr_r464_loop_c6e.restype = c.c_int
    assert model.pr_r464_loop_c6e(c.byref(state)) == 0
    for name in pre:
        assert getattr(state.cpu, name) == post[name], name
    assert bytes(ram) == after, "complete 640 KiB RAM differs"
    changed = [i for i, (a, b) in enumerate(zip(before, after)) if a != b]
    assert changed == [0x1BAC7, 0x20FCF, 0x20FD6, 0x20FD8]
    print("RES_0464:+0C6E→+0C3D: 12 reached instructions, "
          "14 CPU fields, 640 KiB RAM, four changed addresses match")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--library", type=Path, required=True)
    args = parser.parse_args()
    check(args.exe.read_bytes(), args.run, args.library)
