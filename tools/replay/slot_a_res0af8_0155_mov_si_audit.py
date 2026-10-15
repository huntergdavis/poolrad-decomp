#!/usr/bin/env python3
"""Audit the loaded 131C:0155 MOV SI after a hypothetical call return."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
BODY_SHA = "6a9d0beb81be4f631820c683e61cfb1734964c19ae6e4d6504596e5b5e74dc8d"
ANCHOR_SHA = "69f3c75744af408ea7740c12131cf78eb49f55db934c3d1f5cb408e198ba717b"
SOURCE = 0xADCB
LOADED = (0x131C << 4) + 0x0155
ANCHOR_SOURCE = 0xADBF
ANCHOR_LOADED = (0x131C << 4) + 0x0149


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
    original_image = (0x0AF8 << 4) + 0x0155
    assert original_image == 0xB0D5 and LOADED == 0x13315
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    body = start[SOURCE:SOURCE + 3]
    assert len(body) == 3 and body == ram[LOADED:LOADED + 3]
    assert sha256(body).hexdigest() == BODY_SHA
    assert hits(start, body) == [0xACBA, SOURCE]
    assert decode(START, SOURCE, SOURCE + 3) == [(0, "mov $0x1ee,%si")]
    assert decode(RAM, LOADED, LOADED + 3) == [(0, "mov $0x1ee,%si")]
    anchor = start[ANCHOR_SOURCE:SOURCE + 3]
    assert len(anchor) == 15 and anchor == ram[ANCHOR_LOADED:LOADED + 3]
    assert sha256(anchor).hexdigest() == ANCHOR_SHA
    assert hits(start, anchor) == [ANCHOR_SOURCE]
    assert LOADED + 3 - (0x131C << 4) == 0x0158
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"body source [0x{SOURCE:05X},0x{SOURCE + 3:05X}) "
          f"bytes=3 instructions=1 source_hits=2 sha256={BODY_SHA}")
    print(f"body loaded [0x{LOADED:05X},0x{LOADED + 3:05X}) sha256={BODY_SHA}")
    print(f"prior-call anchor source [0x{ANCHOR_SOURCE:05X},0x{SOURCE + 3:05X}) "
          f"unique_hits=1 sha256={ANCHOR_SHA}")
    print("next instruction=131C:0158; prior call return and native reach unmeasured")


if __name__ == "__main__":
    main()
