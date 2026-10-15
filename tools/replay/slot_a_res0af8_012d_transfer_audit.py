#!/usr/bin/env python3
"""Audit loaded 131C:012D stack-built RETF transfer without running it."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
BODY_SHA = "f40fac73cf15f6a62c43395ce93c455dfda956e2d427db8477461658d9a01bb1"
NEXT_SHA = "05363f65c03f3827871b25a34a47bd5ddfc501c83e7513555d321949fb2779dc"
SOURCE = 0xADA3
LOADED = (0x131C << 4) + 0x012D
LENGTH = 19


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


def main() -> None:
    start = START.read_bytes()
    ram = RAM.read_bytes()
    assert len(start) == 65231 and sha256(start).hexdigest() == START_SHA
    assert len(ram) == 655360 and sha256(ram).hexdigest() == RAM_SHA
    original_image = (0x0AF8 << 4) + 0x012D
    assert original_image == 0xB0AD and LOADED == 0x132ED
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    body = start[SOURCE:SOURCE + LENGTH]
    assert len(body) == LENGTH and body == ram[LOADED:LOADED + LENGTH]
    assert sha256(body).hexdigest() == BODY_SHA
    assert [i for i in range(len(start) - LENGTH + 1)
            if start[i:i + LENGTH] == body] == [SOURCE]
    expected = [
        (0, "xor %ax,%ax"), (2, "mov %ax,0x4418"),
        (5, "mov %ax,0x441a"), (8, "mov %ax,0x4426"),
        (11, "mov $0x123,%ax"), (14, "push %cs"),
        (15, "push %ax"), (16, "push %es"),
        (17, "push %bx"), (18, "lret"),
    ]
    assert decode(START, SOURCE, SOURCE + LENGTH) == expected
    assert decode(RAM, LOADED, LOADED + LENGTH) == expected
    assert LOADED + LENGTH == 0x13300
    next_source = start[SOURCE + LENGTH:SOURCE + LENGTH + 3]
    next_loaded = ram[LOADED + LENGTH:LOADED + LENGTH + 3]
    assert next_source == next_loaded
    assert sha256(next_source).hexdigest() == NEXT_SHA
    assert decode(RAM, LOADED + LENGTH, LOADED + LENGTH + 3) == [
        (0, "mov $0x8508,%ax"),
    ]
    assert ((0x14A0 << 4) + 0x4418) == 0x18E18
    assert ((0x14A0 << 4) + 0x4426) == 0x18E26

    print(f"START.EXE bytes=65231 sha256={START_SHA}")
    print(f"loaded RAM bytes=655360 sha256={RAM_SHA}")
    print(f"body source [0x{SOURCE:05X},0x{SOURCE + LENGTH:05X}) unique_hits=1 "
          f"bytes=19 instructions=10 terminal=RETF sha256={BODY_SHA}")
    print(f"body loaded [0x{LOADED:05X},0x{LOADED + LENGTH:05X}) sha256={BODY_SHA}")
    print("DS:4418/441A/4426 word clears; pushes CS,0123,ES,BX; RETF target=ES:BX")
    print(f"sequential next source=0x{SOURCE + LENGTH:05X} "
          f"loaded=0x{LOADED + LENGTH:05X} bytes=3 sha256={NEXT_SHA}")
    print("source-only; no reached clear, target value, or far-transfer execution claim")


if __name__ == "__main__":
    main()
