#!/usr/bin/env python3
"""Hash-locked private audit of a persistent second-cycle C EGA page.

Only call 25's measured native entry page initializes the C page. Later
native entry/return pages are comparison targets, never composition inputs.
"""

import argparse
from hashlib import sha256
from pathlib import Path
import struct
import subprocess

import slot_a_second_cycle_immediate_audit as first

CALLER_RAM_SHA = {
    5: "a6c7ba2f84683a6d2db0fac17e91118cc397884de9eeeb07f3c435608347d0c2",
    6: "86603906a03793a87900b95b1b6fc2fbd632462781435703aa935abac6055633",
    7: "28c6b8b215d8e6b7c196a4647d65622185351d3197e353d64f26267fe8ac7539",
    8: "02233c08911761f46c243f6b2d2b278d919da18c2523c7c5727f46fcdbb0e66b",
    9: "d162a2ff5b0a8d773dc7f7c53cfee297a1a06758e9eb1ac4290787348e8838a8",
}
ROW9_RECT_ENTRY_SHA = "672e433c28362f3445af87c4a04c1c68b1d4cacd407fa958cbcc616f9e2153b5"
CLEAR_BEFORE_CALL = {28: 5, 31: 6, 34: 7, 37: 8, 40: 9}
EXPECTED_REPORT = (
    "calls=18 printer_cells=69 printer_stores=2208 "
    "printer_changed=1404 clear_calls=5 clear_stores=3520 "
    "clear_changed=994"
)


def caller_arguments(image: bytes, caller_run: Path):
    first.check_hash(image[0x4047:0x41F8], first.OUTER_IMAGE_SHA,
                     "original RES_03F1:0137 body")
    first.check_hash(image[0x7147:0x7201], first.RECT_IMAGE_SHA,
                     "original RES_0709:00B7 body")
    first.check_hash(image[0x4101:0x4106], first.RECT_CALL_IMAGE_SHA,
                     "original RES_03F1:01F1 far call")
    first.check_hash(image[0x138A:0x138F], first.OUTER_CALL_IMAGE_SHA,
                     "original RES_00BA:07EA far call")
    first.check_hash((caller_run / "POOLRAD/SAVGAMA.DAT").read_bytes(),
                     first.SAVE_SHA, "caller-run slot-A save")
    arguments = {}
    for row, digest in CALLER_RAM_SHA.items():
        ram = (caller_run / f"ram-row{row}-caller-private.bin").read_bytes()
        if len(ram) != 0xA0000:
            raise ValueError(f"row {row} caller RAM length changed")
        first.check_hash(ram, digest, f"row {row} caller RAM")
        first.check_hash(ram[0x0C150 + 0x0137:0x0C150 + 0x02E8],
                         first.OUTER_RUNTIME_SHA, f"row {row} loaded outer body")
        first.check_hash(ram[0x0F2D0 + 0x00B7:0x0F2D0 + 0x0171],
                         first.RECT_RUNTIME_SHA, f"row {row} loaded rectangle")
        first.check_hash(ram[0x0C150 + 0x01F1:0x0C150 + 0x01F6],
                         first.RECT_CALL_RUNTIME_SHA, f"row {row} loaded far call")
        first.check_hash(ram[0x08DE0 + 0x07EA:0x08DE0 + 0x07EF],
                         first.OUTER_CALL_RUNTIME_SHA,
                         f"row {row} loaded outer far call")
        ss = 0x1D160
        bp = 0x3EB0
        if struct.unpack_from("<2H", ram, ss + bp + 2) != (0x07EF, 0x08DE):
            raise ValueError(f"row {row} outer return frame changed")
        low_bytes = tuple(ram[ss + bp + offset]
                          for offset in (6, 8, 10, 12))
        if low_bytes != (row, 38, row, 17):
            raise ValueError(f"row {row} rectangle arguments changed")
        ds = 0x14A00
        if (ram[ds + 0x4A1E] != 1 or
                struct.unpack_from("<HH", ram, ds + 0x4A16) !=
                (0xA000, 0xA000)):
            raise ValueError(f"row {row} EGA page state changed")
        arguments[row] = (row, 17, 38, 0)
    entry = (caller_run / "ram-row9-rectangle-entry-private.bin").read_bytes()
    if len(entry) != 0xA0000:
        raise ValueError("row-nine rectangle entry RAM length changed")
    first.check_hash(entry, ROW9_RECT_ENTRY_SHA, "row-nine rectangle entry RAM")
    first.check_hash(entry[0x0F2D0 + 0x00B7:0x0F2D0 + 0x0171],
                     first.RECT_RUNTIME_SHA, "row-nine entered rectangle body")
    sp = 0x1D160 + 0x3E94
    if struct.unpack_from("<2H", entry, sp) != (0x01F6, 0x0C15):
        raise ValueError("row-nine rectangle callee return changed")
    if tuple(entry[sp + offset] for offset in (4, 6, 8, 10, 12, 14)) != (
            0, 0, 9, 38, 9, 17):
        raise ValueError("row-nine rectangle callee arguments changed")
    return arguments


def audit(exe: Path, ovr: Path, emulator: Path, run: Path,
          watch: Path, caller_run: Path, printer_probe: Path,
          compose_probe: Path) -> None:
    # Preserve the independent per-call and first watched-writer gates.
    first.audit(exe, ovr, emulator, run, watch, printer_probe)
    image, *_ = first.unpack(exe.read_bytes())
    clears = caller_arguments(image, caller_run)

    initial = first.page(run, "call025-entry")
    first.check_hash(initial, first.SECOND_CYCLE_START_SHA,
                     "only native C page input")
    records = []
    font_first = None
    for number, expected in enumerate(first.CALLS, 25):
        ram = (run / f"ram-call{number:03d}-entry-private.bin").read_bytes()
        font, source = first.call_input(ram, image, number, expected)
        if font_first is None:
            font_first = font
        elif font != font_first:
            raise ValueError(f"call {number} font changed during second cycle")
        if number in CLEAR_BEFORE_CALL:
            row = CLEAR_BEFORE_CALL[number]
            clear_meta = bytes((1, *clears[row]))
        else:
            clear_meta = bytes(5)
        _, length, color, row, column, _ = expected
        records.append(clear_meta + bytes((length, column, row, color)) +
                       source + bytes(255 - length))
    if font_first is None:
        raise ValueError("second-cycle font missing")
    private_input = run / "second-cycle-composed-input-private.bin"
    private_output = run / "second-cycle-composed-output-private.bin"
    private_input.write_bytes(font_first + initial + b"".join(records))
    result = subprocess.run([str(compose_probe.resolve()),
                             str(private_input), str(private_output)],
                            check=True, capture_output=True, text=True)
    if result.stdout.strip() != EXPECTED_REPORT:
        raise ValueError(f"persistent C report changed: {result.stdout.strip()}")
    composed = private_output.read_bytes()
    if len(composed) != 18 * 2 * 32000:
        raise ValueError("persistent C page stream length changed")
    offset = 0
    for number in range(25, 43):
        for phase in ("entry", "return"):
            actual = composed[offset:offset + 32000]
            native = first.page(run, f"call{number:03d}-{phase}")
            if actual != native:
                mismatches = sum(a != b for a, b in zip(actual, native))
                raise ValueError(f"call {number} {phase} persistent C mismatch: "
                                 f"{mismatches} plane bytes")
            offset += 32000
    first.check_hash(composed, first.PAGE_SET_SHA,
                     "persistent C entry/return page stream")
    print("five reached row-clear callers: rows 5..9, each (row,38,row,17)")
    print("persistent C second-cycle page: 36/36 entry/return checks, "
          "each 32000/32000 bytes")
    print("persistent C page stream SHA-256", sha256(composed).hexdigest())
    print("one native page input only: call25 entry; no later native page copies")
    print("whole first-view static C frame and full resident routine remain open")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--ovr", type=Path, required=True)
    parser.add_argument("--emulator", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--watch", type=Path, required=True)
    parser.add_argument("--caller-run", type=Path, required=True)
    parser.add_argument("--printer-probe", type=Path, required=True)
    parser.add_argument("--compose-probe", type=Path, required=True)
    options = parser.parse_args()
    audit(options.exe, options.ovr, options.emulator, options.run,
          options.watch, options.caller_run, options.printer_probe,
          options.compose_probe)
