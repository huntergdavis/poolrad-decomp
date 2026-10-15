#!/usr/bin/env python3
"""Pin the source-only 131C:0636 DOS interrupt and carry branch."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
SPAN_SHA = "e3b89529f92d647af55495aff2aa884615aab6b031529082362eec35885dd158"
JC_SHA = "4a35e73f0b860ba321735b7350646cb0c126f98ee7386eef3bcba5fdc7ea17f5"
SOURCE = 0xB2AC
LOADED = 0x137F6
SIZE = 9


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
    assert (0x0AF8 << 4) + 0x0636 == 0xB5B6
    assert (0x131C << 4) + 0x0636 == LOADED
    assert 0xB5B6 - SOURCE == 0x30A
    assert LOADED - 0xB5B6 == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == SPAN_SHA
    assert [i for i in range(len(start) - SIZE + 1)
            if start[i:i + SIZE] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + SIZE) == [
        (0, "lea 0x30(%di),%dx"),
        (3, "int $0x21"),
        (5, "jb 0xb302"),
        (7, "mov %ax,(%di)"),
    ]
    assert decode(RAM, LOADED, LOADED + SIZE) == [
        (0, "lea 0x30(%di),%dx"),
        (3, "int $0x21"),
        (5, "jb 0x1384c"),
        (7, "mov %ax,(%di)"),
    ]
    assert sha256(start[SOURCE + 5:SOURCE + 7]).hexdigest() == JC_SHA
    assert LOADED + SIZE == (0x131C << 4) + 0x063F
    assert 0x1384C - (0x131C << 4) == 0x068C
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+0636 source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"unique_hits=1 bytes=9 instructions=4 branches=1 interrupts=1 sha256={SPAN_SHA}")
    print(f"JC sha256={JC_SHA} taken=131C:068C fallthrough=131C:063D "
          "next=131C:063F")
    print("AH, DS:DI, interrupt result, native reach and branch outcome unmeasured")


if __name__ == "__main__":
    main()
