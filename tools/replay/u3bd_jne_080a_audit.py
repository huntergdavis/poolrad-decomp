#!/usr/bin/env python3
"""Hash-locked saved-route U3BD +080A branch and +0211 call handoff."""

from __future__ import annotations

import ctypes
import hashlib
from pathlib import Path
import re
import subprocess
import tempfile

from capstone import Cs, CS_ARCH_X86, CS_MODE_16


ROOT = Path(__file__).resolve().parents[2]
ORIGINAL = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
RUN = ROOT / "resources/oracle-runs/slot-a-u3bd-jne-080a-p5-028"
SOURCE = ROOT / "src/recovered/overlay_u3bd_jne_080a.c"
ORIGINAL_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SPAN_SHA = "623e419028f0b9f65c3db7716c85912668fea14f8d4099eceb9210343908263b"
TRANSCRIPT_SHA = "27de497745a0e8f588f0a436acae267e9c15585c6695f279aa3719680d0a7885"
RAM_SHA = (
    "0d06b515e83e796ade8e41d3c93e3c999a43bcd291bb0efaebd2ee58ab74842a",
    "0d86321fa96c9a0bc11fedd625d15f483ed838c53062cbfea44c317ad7cc658a",
    "1c4354e8a6ad27344b98d7615b4ccce6b37f055f5a5c403d745d09d28b72464b",
)
FIELDS = ("AX", "BX", "CX", "DX", "SI", "DI", "BP", "SP",
          "DS", "ES", "SS", "CS", "FLAGS")
EV = re.compile(rb"LOG: EV of '([A-Z]+)' is:\r?\nLOG: ([0-9a-f]+)\r?\n")


class Frame(ctypes.Structure):
    _fields_ = [(name.lower(), ctypes.c_uint16) for name in FIELDS]


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def physical(seg: int, off: int) -> int:
    return ((seg << 4) + (off & 0xFFFF)) & 0xFFFFF


def word(ram: bytes, seg: int, off: int) -> int:
    return ram[physical(seg, off)] | (ram[physical(seg, off + 1)] << 8)


def cpu_rows(transcript: bytes) -> list[dict[str, int]]:
    pairs = [(match.group(1).decode(), int(match.group(2), 16))
             for match in EV.finditer(transcript)]
    names = ["CS", "IP", *FIELDS[:-3], "SS", "FLAGS"]
    assert len(names) == 14
    assert len(pairs) == 42
    rows = []
    for start in (0, 14, 28):
        row = pairs[start:start + 14]
        assert [name for name, _ in row] == names
        rows.append(dict(row))
    assert transcript.count(b"LOG: DEBUG: Memory dump binary success.") == 3
    assert [(row["CS"], row["IP"]) for row in rows] == [
        (0x2D14, 0x080A), (0x2D14, 0x0860), (0x2D14, 0x0211)
    ]
    return rows


def changed(left: bytes, right: bytes) -> list[int]:
    return [at for at, (a, b) in enumerate(zip(left, right)) if a != b]


def model_phase(model: ctypes.CDLL, before: bytes, expected: bytes,
                start: dict[str, int], finish: dict[str, int],
                stop_before_call: int, expected_status: int) -> None:
    ram = (ctypes.c_uint8 * 0x100000)()
    ctypes.memmove(ram, before, len(before))
    frame = Frame(*(start[name] for name in FIELDS))
    ip = ctypes.c_uint16(start["IP"])
    status = model.pr_u3bd_jne_080a(
        ram, len(ram), ctypes.byref(frame), ctypes.byref(ip), stop_before_call
    )
    assert status == expected_status
    assert bytes(ram)[:len(expected)] == expected
    assert ip.value == finish["IP"]
    assert {name: getattr(frame, name.lower()) for name in FIELDS} == {
        name: finish[name] for name in FIELDS
    }


def main() -> None:
    original = ORIGINAL.read_bytes()
    transcript = (RUN / "debugger.typescript").read_bytes()
    rams = [(RUN / name).read_bytes() for name in (
        "ram-080a-before-jne.bin", "ram-0860-before-call.bin",
        "ram-0211-callee-entry.bin"
    )]
    assert digest(original) == ORIGINAL_SHA
    assert digest(transcript) == TRANSCRIPT_SHA
    assert all(len(ram) == 0xA0000 for ram in rams)
    assert tuple(map(digest, rams)) == RAM_SHA
    rows = cpu_rows(transcript)

    start, end = 0x080A, 0x0863
    span = original[0x2E8F7 + start:0x2E8F7 + end]
    assert len(span) == 89 and digest(span) == SPAN_SHA
    loaded = physical(0x2D14, start)
    assert rams[0][loaded:loaded + len(span)] == span
    ins = list(Cs(CS_ARCH_X86, CS_MODE_16).disasm(span, start))
    assert len(ins) == 41 and sum(item.size for item in ins) == len(span)
    assert ins[0].mnemonic == "jne" and ins[0].op_str == "0x824"
    assert ins[-1].address == 0x0860 and ins[-1].op_str == "0x211"

    assert changed(rams[0], rams[1]) == [
        0x20FF4, 0x20FF5, 0x20FF6, 0x20FF7, 0x20FF8,
        0x20FF9, 0x20FFC, 0x20FFF
    ]
    assert changed(rams[1], rams[2]) == [0x20FF2, 0x20FF3]
    assert rams[1][physical(rows[1]["SS"], rows[1]["BP"] - 0x17)] == 0x0E
    assert [word(rams[1], rows[1]["SS"], rows[1]["SP"] + n)
            for n in (0, 2, 4, 6, 8)] == [0x2D14, 0, 0x000E, 4, 5]
    assert word(rams[2], rows[2]["SS"], rows[2]["SP"]) == 0x0863

    with tempfile.TemporaryDirectory(prefix="poolrad-u3bd-080a-") as tmp:
        library = Path(tmp) / "model.so"
        subprocess.run([
            "cc", "-std=c99", "-Wall", "-Wextra", "-Werror", "-pedantic",
            "-fPIC", "-shared", "-I", str(ROOT / "include"),
            str(SOURCE), "-o", str(library),
        ], check=True)
        model = ctypes.CDLL(str(library))
        model.pr_u3bd_jne_080a.argtypes = [
            ctypes.POINTER(ctypes.c_uint8), ctypes.c_size_t,
            ctypes.POINTER(Frame), ctypes.POINTER(ctypes.c_uint16), ctypes.c_int
        ]
        model.pr_u3bd_jne_080a.restype = ctypes.c_int
        model_phase(model, rams[0], rams[1], rows[0], rows[1], 1, 1)
        model_phase(model, rams[0], rams[2], rows[0], rows[2], 0, 2)

    print("U3BD +080A..+0863: 89 bytes/41 instructions original and loaded hash locked")
    print("same-run transcript-bound native/C at +0860 and +0211:")
    print("640 KiB RAM, 13 CPU fields, and IP exact at both boundaries")
    print("saved branch +080A -> +0824 -> +0847 -> +0860 CALL +0211")


if __name__ == "__main__":
    main()
