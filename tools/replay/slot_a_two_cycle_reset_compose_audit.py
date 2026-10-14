#!/usr/bin/env python3
"""Private hash-locked audit of one C page across both slot-A printer cycles.

Only call 2's native entry page initializes the C page. Every subsequent
native page is a comparison target, including call 25's post-reset entry.
All original bytes, RAM, font, source text, and page payloads stay ignored.
"""

import argparse
from pathlib import Path
import struct
import subprocess

import slot_a_second_cycle_compose_audit as compose
import slot_a_second_cycle_immediate_audit as first

FIRST_RAM_SET_SHA = "e2b1caa5dc4a0f9ab0422f3bc48c954b1a1d7a96460340d0cffb513f7b17188e"
FIRST_PAGE_SET_SHA = "e7214cd8e42b5df641c2c69ccef46bcc173efa8bcd4b663e555528e5cd4999f1"
FIRST_INPUT_SHA = "606d838123a21edb436339b6fbf4fb2583ae79b38964d89f24e763cdb34c7c77"
ROW4_CALLER_SHA = "848bbadb5b82e407b54463fc811894962c9bc7e05d0777ce1fe457bda352c39c"
ROW4_OUTER_SHA = "86e2205dd0a5353cc40a783c33c5e6d0be167722a7d8aae56f9f819ff1ecceb1"
ROW4_RECT_SHA = "3ad791e7b0fa7bb18e62fc24acb74e6a9b19d8bfc177aa607c94a44486fa4d1d"
PAGE_STREAM_SHA = "a139cb8c478593ae14346f02f60a59b209535ca193fb54a2d86c13c421bbf1ac"
CALLER_PATH_IMAGE_SHA = "31ca672eb365796871962333732456524892e788e30cda9de7f6079f63f6ca57"
CALLER_PATH_RUNTIME_SHA = "440e336aeb4cfcf75d40ac752abdfa4e7ef138b5d7ec5b512595f04deffff15a"

# Return IP, Pascal source length, color, row, column, SS:SP.
FIRST_CALLS = (
    (0x07A1, 4, 15, 2, 17, 0x3EA6), (0x07BF, 6, 15, 2, 33, 0x3EA6),
    (0x0814, 14, 15, 4, 17, 0x3EA6), (0x096D, 1, 10, 4, 34, 0x3C96),
    (0x09C3, 2, 10, 4, 37, 0x3D94), (0x1858, 6, 11, 5, 17, 0x3E92),
    (0x096D, 1, 10, 5, 34, 0x3C96), (0x09C3, 1, 10, 5, 38, 0x3D94),
    (0x1858, 8, 11, 6, 17, 0x3E92), (0x096D, 1, 10, 6, 34, 0x3C96),
    (0x09C3, 1, 10, 6, 38, 0x3D94), (0x1858, 12, 11, 7, 17, 0x3E92),
    (0x096D, 1, 10, 7, 34, 0x3C96), (0x09C3, 2, 10, 7, 37, 0x3D94),
    (0x1858, 8, 11, 8, 17, 0x3E92), (0x096D, 1, 10, 8, 34, 0x3C96),
    (0x09C3, 1, 10, 8, 38, 0x3D94), (0x1858, 7, 11, 9, 17, 0x3E92),
    (0x096D, 1, 10, 9, 34, 0x3C96), (0x09C3, 1, 10, 9, 38, 0x3D94),
    (0x2B5A, 12, 10, 15, 17, 0x3C86),
    (0x07A1, 4, 15, 2, 17, 0x3EB0), (0x07BF, 6, 15, 2, 33, 0x3EB0),
)
CLEAR_BEFORE = {25: 4, **compose.CLEAR_BEFORE_CALL}
REPORT = ("calls=41 printer_cells=170 printer_stores=5440 "
          "printer_changed=3158 clear_calls=6 clear_stores=4224 "
          "clear_changed=1404")


def checked_ram(path: Path, digest: str, label: str) -> bytes:
    ram = path.read_bytes()
    if len(ram) != 0xA0000:
        raise ValueError(f"{label} length changed")
    first.check_hash(ram, digest, label)
    return ram


def check_row4(image: bytes, reset: Path, second: Path) -> None:
    first.check_hash((reset / "POOLRAD/SAVGAMA.DAT").read_bytes(),
                     first.SAVE_SHA, "reset-run slot-A save")
    caller = checked_ram(reset / "ram-row4-caller-private.bin",
                         ROW4_CALLER_SHA, "row-four caller RAM")
    outer = checked_ram(reset / "ram-row4-outer-call-private.bin",
                        ROW4_OUTER_SHA, "row-four outer RAM")
    rectangle = checked_ram(reset / "ram-row4-rectangle-entry-private.bin",
                            ROW4_RECT_SHA, "row-four rectangle RAM")
    spans = (
        (image, 0xBA0 + 0x07BF, 0xBA0 + 0x07EF,
         CALLER_PATH_IMAGE_SHA),
        (caller, 0x08DE0 + 0x07BF, 0x08DE0 + 0x07EF,
         CALLER_PATH_RUNTIME_SHA),
        (image, 0x4047, 0x41F8, first.OUTER_IMAGE_SHA),
        (image, 0x7147, 0x7201, first.RECT_IMAGE_SHA),
        (image, 0x4101, 0x4106, first.RECT_CALL_IMAGE_SHA),
        (image, 0x138A, 0x138F, first.OUTER_CALL_IMAGE_SHA),
        (caller, 0x08DE0 + 0x07EA, 0x08DE0 + 0x07EF,
         first.OUTER_CALL_RUNTIME_SHA),
        (outer, 0x0C150 + 0x0137, 0x0C150 + 0x02E8,
         first.OUTER_RUNTIME_SHA),
        (outer, 0x0C150 + 0x01F1, 0x0C150 + 0x01F6,
         first.RECT_CALL_RUNTIME_SHA),
        (rectangle, 0x0F2D0 + 0x00B7, 0x0F2D0 + 0x0171,
         first.RECT_RUNTIME_SHA),
    )
    for data, start, end, digest in spans:
        first.check_hash(data[start:end], digest, f"row-four span {start:#x}")
    ss = 0x1D160
    if tuple(caller[ss + 0x3EB6 + i] for i in (0, 2, 4, 6)) != (
            4, 38, 4, 17):
        raise ValueError("row-four caller pushes changed")
    if (caller[ss + 0x3ECC - 6], caller[ss + 0x3ECC - 5]) != (4, 17):
        raise ValueError("row-four caller locals changed")
    if (struct.unpack_from("<2H", caller, 0x14A00 + 0x5D96) !=
            (0x0008, 0x449A) or
            struct.unpack_from("<2H", caller, ss + 0x3ECC - 4) !=
            (0x0008, 0x449A)):
        raise ValueError("row-four nonzero far-pointer branch changed")
    if struct.unpack_from("<2H", outer, ss + 0x3EB0 + 2) != (0x07EF, 0x08DE):
        raise ValueError("row-four outer return changed")
    if tuple(outer[ss + 0x3EB0 + i] for i in (6, 8, 10, 12)) != (
            4, 38, 4, 17):
        raise ValueError("row-four outer arguments changed")
    if struct.unpack_from("<2H", rectangle, ss + 0x3E94) != (0x01F6, 0x0C15):
        raise ValueError("row-four rectangle return changed")
    if tuple(rectangle[ss + 0x3E94 + i] for i in (4, 6, 8, 10, 12, 14)) != (
            0, 0, 4, 38, 4, 17):
        raise ValueError("row-four rectangle arguments changed")
    ds = 0x14A00
    if (outer[ds + 0x4A1E] != 1 or
            struct.unpack_from("<2H", outer, ds + 0x4A16) !=
            (0xA000, 0xA000)):
        raise ValueError("row-four EGA page state changed")
    returned = b"".join(
        (reset / f"page-row4-return-plane{p}-private.bin").read_bytes()
        for p in range(4))
    before = first.page(second, "call024-return")
    after = first.page(second, "call025-entry")
    first.check_hash(before, first.FIRST_CYCLE_END_SHA, "row-four pre-page")
    first.check_hash(returned, first.SECOND_CYCLE_START_SHA,
                     "row-four immediate return page")
    if returned != after:
        raise ValueError("other inter-cycle page writes followed row-four return")
    changes = [i for i, (a, b) in enumerate(zip(before, returned)) if a != b]
    if (len(changes) != 410 or
            any(before[i] == 0 or returned[i] != 0 or
                (i % 8000) // 320 != 4 or not 17 <= i % 40 <= 38
                for i in changes)):
        raise ValueError("row-four reset geometry changed")
    modeled = bytearray(before)
    for plane in range(4):
        for scanline in range(32, 40):
            at = plane * 8000 + scanline * 40 + 17
            modeled[at:at + 22] = bytes(22)
    if modeled != returned:
        raise ValueError("row-four clear does not explain the full page")


def audit(exe: Path, ovr: Path, emulator: Path, first_run: Path,
          second_run: Path, reset: Path, caller_run: Path,
          probe: Path) -> None:
    raw = exe.read_bytes()
    first.check_hash(raw, first.START_SHA, "START.EXE")
    first.check_hash(ovr.read_bytes(), first.GAME_SHA, "GAME.OVR")
    first.check_hash(emulator.read_bytes(), first.EMULATOR_SHA,
                     "heavy debugger")
    image, *_ = first.unpack(raw)
    first.check_hash(image[0x76B6:0x7737], first.PRINTER_IMAGE_SHA,
                     "original printer")
    for run in (first_run, second_run):
        first.check_hash((run / "POOLRAD/SAVGAMA.DAT").read_bytes(),
                         first.SAVE_SHA, f"{run.name} slot-A save")
    first.check_set((first_run / f"ram-call{n:03d}-entry-private.bin"
                     for n in range(2, 25)), FIRST_RAM_SET_SHA, 23,
                    23 * 0xA0000, "first-cycle RAM")
    first.check_set((first_run / f"page-call{n:03d}-{phase}-plane{p}.bin"
                     for n in range(2, 25)
                     for phase in ("entry", "return") for p in range(4)),
                    FIRST_PAGE_SET_SHA, 184, 184 * 8000,
                    "first-cycle page set")
    first.check_set((second_run / f"ram-call{n:03d}-entry-private.bin"
                     for n in range(25, 43)), first.RAM_SET_SHA, 18,
                    18 * 0xA0000, "second-cycle RAM")
    first.check_set((second_run / f"page-call{n:03d}-{phase}-plane{p}.bin"
                     for n in range(25, 43)
                     for phase in ("entry", "return") for p in range(4)),
                    first.PAGE_SET_SHA, 144, 144 * 8000,
                    "second-cycle page set")
    check_row4(image, reset, second_run)
    clears = compose.caller_arguments(image, caller_run)
    clears[4] = (4, 17, 38, 0)
    initial = first.page(first_run, "call002-entry")
    first.check_hash(initial, FIRST_INPUT_SHA, "only native C page input")
    if first.page(first_run, "call024-return") != first.page(
            second_run, "call024-return"):
        raise ValueError("independent first-cycle terminal page changed")
    records = []
    font_first = None
    for number, expected in list(enumerate(FIRST_CALLS, 2)) + list(
            enumerate(first.CALLS, 25)):
        run = first_run if number < 25 else second_run
        ram = (run / f"ram-call{number:03d}-entry-private.bin").read_bytes()
        font, source = first.call_input(ram, image, number, expected)
        if font_first is None:
            font_first = font
        elif font != font_first:
            raise ValueError(f"font changed at call {number}")
        if number in CLEAR_BEFORE:
            clear_meta = bytes((1, *clears[CLEAR_BEFORE[number]]))
        else:
            clear_meta = bytes(5)
        _, length, color, row, column, _ = expected
        records.append(clear_meta + bytes((length, column, row, color)) +
                       source + bytes(255 - length))
    private_input = reset / "two-cycle-composed-input-private.bin"
    private_output = reset / "two-cycle-composed-output-private.bin"
    private_input.write_bytes(font_first + initial + b"".join(records))
    result = subprocess.run([str(probe.resolve()), str(private_input),
                             str(private_output)], check=True,
                            capture_output=True, text=True)
    if result.stdout.strip() != REPORT:
        raise ValueError(f"C report changed: {result.stdout.strip()}")
    composed = private_output.read_bytes()
    if len(composed) != 41 * 2 * 32000:
        raise ValueError("composed page stream length changed")
    first.check_hash(composed, PAGE_STREAM_SHA, "two-cycle C page stream")
    offset = 0
    for number in range(2, 43):
        run = first_run if number < 25 else second_run
        for phase in ("entry", "return"):
            actual = composed[offset:offset + 32000]
            native = first.page(run, f"call{number:03d}-{phase}")
            if actual != native:
                count = sum(a != b for a, b in zip(actual, native))
                raise ValueError(f"call {number} {phase} differs by {count}")
            offset += 32000
    print("row-four live caller/callee and 410-byte immediate reset: verified")
    print("one C page from call 2 entry through call 42 return: "
          "82/82 native checkpoints, each 32000/32000 bytes")
    print("page stream SHA-256", PAGE_STREAM_SHA)
    print(result.stdout.strip())
    print("only native page seed: call 2 entry; call 25 and later are comparisons")
    print("pre-cycle transfer, request production, full C frame remain open")


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    for name in ("exe", "ovr", "emulator", "first-run", "second-run",
                 "reset", "caller-run", "probe"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    audit(args.exe, args.ovr, args.emulator, args.first_run,
          args.second_run, args.reset, args.caller_run, args.probe)
