#!/usr/bin/env python3
"""Pin loaded RES_0AF8:06D3 through its two DOS interrupt calls."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
SPAN_SHA = "0c1874990854d17cc19562456b2816d737443d20da7281b24b990ee3fe747ad7"
SOURCE = 0xB349
LOADED = 0x13893
SIZE = 22


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
    assert (0x0AF8 << 4) + 0x06D3 == 0xB653
    assert (0x131C << 4) + 0x06D3 == LOADED
    assert 0xB653 - SOURCE == 0x30A
    assert LOADED - 0xB653 == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == SPAN_SHA
    assert [i for i in range(len(start) - SIZE + 1)
            if start[i:i + SIZE] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + SIZE) == [
        (0, "mov %bx,%dx"),
        (2, "sub %ax,%dx"),
        (4, "mov $0xffff,%cx"),
        (7, "mov (%di),%bx"),
        (9, "mov $0x4202,%ax"),
        (12, "int $0x21"),
        (14, "xor %cx,%cx"),
        (16, "mov (%di),%bx"),
        (18, "mov $0x40,%ah"),
        (20, "int $0x21"),
    ]
    assert decode(RAM, LOADED, LOADED + SIZE) == [
        (0, "mov %bx,%dx"),
        (2, "sub %ax,%dx"),
        (4, "mov $0xffff,%cx"),
        (7, "mov (%di),%bx"),
        (9, "mov $0x4202,%ax"),
        (12, "int $0x21"),
        (14, "xor %cx,%cx"),
        (16, "mov (%di),%bx"),
        (18, "mov $0x40,%ah"),
        (20, "int $0x21"),
    ]
    assert LOADED + SIZE == (0x131C << 4) + 0x06E9
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+06D3 source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"unique_hits=1 bytes={SIZE} instructions=10 interrupts=2 sha256={SPAN_SHA}")
    print("INT21 AH=42 then AH=40; next=131C:06E9; native path and DOS results unmeasured")


if __name__ == "__main__":
    main()
