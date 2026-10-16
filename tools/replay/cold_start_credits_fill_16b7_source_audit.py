#!/usr/bin/env python3
"""Pin the credits FillChar candidate's source operands, not native reach."""

from __future__ import annotations

import argparse
from hashlib import sha256
from pathlib import Path
import re
import subprocess
import sys
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools/overlay"))
from resident_loader_map import unpack  # noqa: E402

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
SPANS = (
    ("EGA reset", 0x0709, 0x0000, 0x0071, 0x070C9, 0x0713A,
     "5a5fd5c3e982c984f5fb659dfc6f1a47e673ddcfebbbeebd4ecb8c40c0380de5"),
    ("GC write mode 2", 0x0709, 0x0103, 0x010F, 0x071CC, 0x071D8,
     "1bc31bc16742523ae70e987f6d344bda3144ee1717f2230d5ea6f0362dd06337"),
    ("FillChar call setup", 0x0709, 0x0124, 0x0145, 0x071ED, 0x0720E,
     "58ac0462a1c5d546fa5771ef003f227fa20acb99e6815a2858d21ae15a122df3"),
    ("FillChar body", 0x0AF8, 0x16A8, 0x16BC, 0x0C31E, 0x0C332,
     "f1c5fade96970aa1dd1a9ffc2d876c7aea57bef8680d929d9f65d6c5190e17f7"),
)
STOSB_SHA = "50b2d3be54fb620b49268d3b4bd0c759531794c4c735fc2916071807cf9ee7a6"


def checked_span(raw: bytes, image: bytes, origin: list[int | None],
                 item: tuple[str, int, int, int, int, int, str]) -> bytes:
    label, segment, lo, hi, file_lo, file_hi, expected_sha = item
    start, end = (segment << 4) + lo, (segment << 4) + hi
    span = bytes(image[start:end])
    sources = origin[start:end]
    if (len(span) != hi - lo or len(sources) != len(span) or
            any(source is None for source in sources) or
            [0x200 + source for source in sources] != list(range(file_lo, file_hi)) or
            raw[file_lo:file_hi] != span or
            sha256(span).hexdigest() != expected_sha):
        raise ValueError(f"{label} source extent, origin, or hash differs")
    return span


def decode(span: bytes, vma: int) -> dict[int, tuple[str, str]]:
    # Temporary original bytes stay under ignored resources/ and are removed.
    with TemporaryDirectory(prefix="credits-fill-source-", dir=ROOT / "resources") as tmp:
        path = Path(tmp) / "span.bin"
        path.write_bytes(span)
        output = subprocess.run(
            ["objdump", "-D", "-b", "binary", "-m", "i8086",
             "--no-show-raw-insn", f"--adjust-vma=0x{vma:X}", str(path)],
            check=True, capture_output=True, text=True,
        ).stdout
    found = {}
    for line in output.splitlines():
        match = re.match(r"^\s*([0-9a-f]+):\s*([a-z][a-z0-9]*)\s*(.*?)\s*$", line)
        if match:
            found[int(match[1], 16)] = (match[2], re.sub(r"\s+", "", match[3]))
    return found


def require_operands(found: dict[int, tuple[str, str]],
                     expected: dict[int, tuple[str, str]]) -> None:
    for offset, pair in expected.items():
        if found.get(offset) != pair:
            raise ValueError(f"decoded source operand differs at +{offset:04X}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path,
                        default=ROOT / "resources/DOS/POOLRAD/START.EXE")
    args = parser.parse_args()
    source = args.exe.resolve(strict=True)
    if not source.is_relative_to((ROOT / "resources").resolve()):
        raise ValueError("original input must remain under ignored resources/")
    raw = source.read_bytes()
    if len(raw) != 65231 or sha256(raw).hexdigest() != START_SHA:
        raise ValueError("unsupported private START.EXE")
    image, origin, *_ = unpack(raw)
    reset, mode, call, body = (checked_span(raw, image, origin, item)
                               for item in SPANS)
    reset_insn = decode(reset, 0x0000)
    mode_insn = decode(mode, 0x0103)
    call_insn = decode(call, 0x0124)
    body_insn = decode(body, 0x16A8)

    require_operands(reset_insn, {
        0x0057: ("mov", "$0x8,%al"),
        0x0059: ("mov", "$0x3ce,%dx"),
        0x005C: ("out", "%al,(%dx)"),
        0x005D: ("mov", "$0xff,%al"),
        0x005F: ("mov", "$0x3cf,%dx"),
        0x0062: ("out", "%al,(%dx)"),
        0x0063: ("mov", "$0x2,%al"),
        0x0065: ("mov", "$0x3c4,%dx"),
        0x0068: ("out", "%al,(%dx)"),
        0x0069: ("mov", "$0xf,%al"),
        0x006B: ("mov", "$0x3c5,%dx"),
        0x006E: ("out", "%al,(%dx)"),
    })
    require_operands(mode_insn, {
        0x0103: ("mov", "$0x5,%al"),
        0x0105: ("mov", "$0x3ce,%dx"),
        0x0109: ("mov", "$0x2,%al"),
        0x010B: ("mov", "$0x3cf,%dx"),
        0x010E: ("out", "%al,(%dx)"),
    })
    require_operands(call_insn, {
        0x0124: ("mov", "0x4a18,%ax"),
        0x012C: ("push", "%es"),
        0x012D: ("push", "%di"),
        0x0134: ("mov", "0xc(%bp),%al"),
        0x013B: ("push", "%ax"),
        0x013C: ("mov", "0x6(%bp),%al"),
        0x013F: ("push", "%ax"),
        0x0140: ("lcall", "$0xaf8,$0x16a8"),
    })
    require_operands(body_insn, {
        0x16A8: ("mov", "%sp,%bx"),
        0x16AA: ("les", "%ss:0x8(%bx),%di"),
        0x16AE: ("mov", "%ss:0x6(%bx),%cx"),
        0x16B2: ("mov", "%ss:0x4(%bx),%al"),
        0x16B6: ("cld", ""),
        0x16B7: ("rep", "stos%al,%es:(%di)"),
        0x16B9: ("lret", "$0x8"),
    })
    stosb = body[0x16B7 - 0x16A8:0x16B9 - 0x16A8]
    if len(stosb) != 2 or sha256(stosb).hexdigest() != STOSB_SHA:
        raise ValueError("REP STOSB opcode hash differs")
    print("START.EXE and four mapped source spans pinned; selected operands checked")
    print("GC8=FF Seq2=0F GC5=02; ES:DI/CX/AL stack operands; REP STOSB")
    print("STOP: credits execution, GC6/Seq4, first planar write unmeasured")


if __name__ == "__main__":
    main()
