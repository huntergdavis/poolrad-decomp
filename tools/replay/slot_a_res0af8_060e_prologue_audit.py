#!/usr/bin/env python3
"""Audit the source-only loaded 131C:060E prologue through its LDS."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
PREFIX_SHA = "de93a15f335abe117a5810f160b92897d3c8d8c069d01ccb16db4d862e148220"
ANCHOR_SHA = "543a9a9eb8255640058d16408842539adc8e66ec548e6b693a18122e4b4de9ba"
SOURCE = 0xB284
LOADED = (0x131C << 4) + 0x060E
ANCHOR_SOURCE = 0xB27A
ANCHOR_LOADED = (0x131C << 4) + 0x0604


def decode(path: Path, begin: int, end: int) -> list[tuple[int, str]]:
    result = subprocess.run(
        ["objdump", "-D", "-b", "binary", "-m", "i8086",
         "--no-show-raw-insn", f"--start-address={begin}",
         f"--stop-address={end}", str(path)],
        capture_output=True, text=True, check=True,
    )
    return [(int(m[1], 16) - begin, re.sub(r"\s+", " ", m[2].strip()))
            for line in result.stdout.splitlines()
            if (m := re.match(r"\s*([0-9a-f]+):\s+(.+)", line))]


def hits(haystack: bytes, needle: bytes) -> list[int]:
    return [i for i in range(len(haystack) - len(needle) + 1)
            if haystack[i:i + len(needle)] == needle]


def main() -> None:
    start = START.read_bytes()
    ram = RAM.read_bytes()
    assert len(start) == 65231 and sha256(start).hexdigest() == START_SHA
    assert len(ram) == 655360 and sha256(ram).hexdigest() == RAM_SHA
    original_image = (0x0AF8 << 4) + 0x060E
    assert original_image == 0xB58E and LOADED == 0x137CE
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    prefix = start[SOURCE:SOURCE + 7]
    assert len(prefix) == 7 and prefix == ram[LOADED:LOADED + 7]
    assert sha256(prefix).hexdigest() == PREFIX_SHA
    assert hits(start, prefix) == [0xA42A, 0xA455, SOURCE]
    assert decode(START, SOURCE, SOURCE + 7) == [
        (0, "mov %sp,%bx"),
        (2, "push %ds"),
        (3, "lds %ss:0x4(%bx),%di"),
    ]
    assert decode(RAM, LOADED, LOADED + 7) == [
        (0, "mov %sp,%bx"),
        (2, "push %ds"),
        (3, "lds %ss:0x4(%bx),%di"),
    ]
    anchor = start[ANCHOR_SOURCE:SOURCE + 7]
    assert len(anchor) == 17
    assert anchor == ram[ANCHOR_LOADED:LOADED + 7]
    assert sha256(anchor).hexdigest() == ANCHOR_SHA
    assert hits(start, anchor) == [ANCHOR_SOURCE]
    assert LOADED + 7 - (0x131C << 4) == 0x0615
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"prologue source [0x{SOURCE:05X},0x{SOURCE + 7:05X}) "
          f"bytes=7 instructions=3 source_hits=3 sha256={PREFIX_SHA}")
    print(f"prologue loaded [0x{LOADED:05X},0x{LOADED + 7:05X}) "
          f"sha256={PREFIX_SHA}")
    print(f"cross-boundary anchor source [0x{ANCHOR_SOURCE:05X},"
          f"0x{SOURCE + 7:05X}) unique_hits=1 sha256={ANCHOR_SHA}")
    print("next instruction=131C:0615; first data write is outside this audit")
    print("source-only; caller frame, native reach, and C parity unmeasured")


if __name__ == "__main__":
    main()
