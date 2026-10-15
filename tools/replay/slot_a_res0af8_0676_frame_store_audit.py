#!/usr/bin/env python3
"""Pin the source-only 131C:0676 frame stores before the shared return."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
SPAN_SHA = "af01f1858dfabab8997d4182ac0c7b3f8c97c832e97a6bd41dcc3521342b2289"
NEXT_SHA = "45755c99e39cfb753aa25ef66894dda3ef31d15961c9ce3ebf533bd8fb4145f1"
SOURCE = 0xB2EC
LOADED = 0x13836
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
    assert (0x0AF8 << 4) + 0x0676 == 0xB5F6
    assert (0x131C << 4) + 0x0676 == LOADED
    assert 0xB5F6 - SOURCE == 0x30A
    assert LOADED - 0xB5F6 == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == SPAN_SHA
    assert [i for i in range(len(start) - SIZE + 1)
            if start[i:i + SIZE] == body] == [SOURCE]
    expected = [
        (0, "mov %ax,0x14(%di)"),
        (3, "mov %cs,0x16(%di)"),
        (6, "mov %cx,0x18(%di)"),
        (9, "mov %bx,0x1a(%di)"),
        (12, "movw $0x75f,0x1c(%di)"),
        (17, "mov %cs,0x1e(%di)"),
        (20, "xor %ax,%ax"),
    ]
    assert decode(START, SOURCE, SOURCE + SIZE) == expected
    assert decode(RAM, LOADED, LOADED + SIZE) == expected
    assert LOADED + SIZE == (0x131C << 4) + 0x068C
    assert sha256(start[SOURCE + SIZE:SOURCE + SIZE + 4]).hexdigest() == NEXT_SHA
    assert start[SOURCE + SIZE:SOURCE + SIZE + 4] == ram[LOADED + SIZE:LOADED + SIZE + 4]
    assert decode(START, SOURCE + SIZE, SOURCE + SIZE + 4) == [
        (0, "pop %ds"),
        (1, "lret $0x4"),
    ]
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+0676 source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"unique_hits=1 bytes=22 instructions=7 branches=0 stores=6 sha256={SPAN_SHA}")
    print(f"next=131C:068C shared_return_sha256={NEXT_SHA}")
    print("DS:DI, stored register values, native reach and path unmeasured")


if __name__ == "__main__":
    main()
