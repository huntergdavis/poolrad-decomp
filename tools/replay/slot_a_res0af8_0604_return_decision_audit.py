#!/usr/bin/env python3
"""Audit loaded 131C:0604 decision from an independently supplied return frame."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
PARENT_SHA = "a0f2252dd20ab5c8e6209a7a334f97d893ad1ac671d8ceb2fb825674c1b2e4b1"
SUFFIX_SHA = "89ccf1ac7c0aad9c03d35681e115f8b2953d425add72c23a830b90f3f0a2043b"
DECISION_SHA = "66faa6ae049f7aa3904180632d00f46e3abd7aad7db42a98a4ec6c023c677858"
STORE_SHA = "bf754db91e10c283bf2b78bd69d4d97ccf871861a819899ad2648cdd9ba5722e"
PARENT_SOURCE = 0xB273
SOURCE = 0xB27A
LOADED = (0x131C << 4) + 0x0604


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


def occurrences(haystack: bytes, needle: bytes) -> list[int]:
    return [i for i in range(len(haystack) - len(needle) + 1)
            if haystack[i:i + len(needle)] == needle]


def main() -> None:
    start = START.read_bytes()
    ram = RAM.read_bytes()
    assert len(start) == 65231 and sha256(start).hexdigest() == START_SHA
    assert len(ram) == 655360 and sha256(ram).hexdigest() == RAM_SHA
    original_image = (0x0AF8 << 4) + 0x0604
    assert original_image == 0xB584 and LOADED == 0x137C4
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    parent = start[PARENT_SOURCE:PARENT_SOURCE + 17]
    suffix = start[SOURCE:SOURCE + 10]
    decision = suffix[:4]
    store = suffix[4:7]
    assert parent == ram[LOADED - 7:LOADED + 10]
    assert suffix == ram[LOADED:LOADED + 10]
    assert sha256(parent).hexdigest() == PARENT_SHA
    assert sha256(suffix).hexdigest() == SUFFIX_SHA
    assert sha256(decision).hexdigest() == DECISION_SHA
    assert sha256(store).hexdigest() == STORE_SHA
    assert occurrences(start, parent) == [PARENT_SOURCE]
    assert occurrences(start, suffix) == [SOURCE]
    decision_hits = occurrences(start, decision)
    assert len(decision_hits) == 3 and SOURCE in decision_hits
    assert decode(START, SOURCE, SOURCE + 10) == [
        (0, "or %ax,%ax"), (2, "je 0xb281"),
        (4, "mov %ax,0x4426"), (7, "pop %di"),
        (8, "pop %es"), (9, "ret"),
    ]
    assert decode(RAM, LOADED, LOADED + 10) == [
        (0, "or %ax,%ax"), (2, "je 0x137cb"),
        (4, "mov %ax,0x4426"), (7, "pop %di"),
        (8, "pop %es"), (9, "ret"),
    ]
    print(f"START.EXE bytes=65231 sha256={START_SHA}")
    print(f"loaded RAM bytes=655360 sha256={RAM_SHA}")
    print(f"helper source [0x{PARENT_SOURCE:05X},0x{PARENT_SOURCE + 17:05X}) "
          f"unique_hits=1 sha256={PARENT_SHA}")
    print(f"return suffix source [0x{SOURCE:05X},0x{SOURCE + 10:05X}) "
          f"unique_hits=1 bytes=10 instructions=6 sha256={SUFFIX_SHA}")
    print(f"return suffix loaded [0x{LOADED:05X},0x{LOADED + 10:05X}) sha256={SUFFIX_SHA}")
    print(f"decision bytes=4 source_hits=3 sha256={DECISION_SHA}; "
          "AX=0 -> 131C:060B; AX!=0 -> 131C:0608")
    print(f"nonzero successor DS:4426 source bytes=3 sha256={STORE_SHA}")
    print("post-call AX, DS, and SS:SP are supplied boundaries; no observed effects or C parity")


if __name__ == "__main__":
    main()
