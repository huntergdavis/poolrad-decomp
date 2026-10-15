#!/usr/bin/env python3
"""Pin the source-only 131C:063F type decision and two successors."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
SPAN_SHA = "09644656af005a31725db0d39af0e866be844ceb62fbadac6809def3a4879097"
JE_SHA = "49f25187c36d64f88fdd63a8b0d1bc3c8aabcb4c0e3bd565efa1bea5fc6e8a6a"
SOURCE = 0xB2B5
LOADED = 0x137FF
SIZE = 14


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
    assert (0x0AF8 << 4) + 0x063F == 0xB5BF
    assert (0x131C << 4) + 0x063F == LOADED
    assert 0xB5BF - SOURCE == 0x30A
    assert LOADED - 0xB5BF == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == SPAN_SHA
    assert [i for i in range(len(start) - SIZE + 1)
            if start[i:i + SIZE] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + SIZE) == [
        (0, "mov $0x6ea,%ax"),
        (3, "xor %cx,%cx"),
        (5, "mov %cx,%bx"),
        (7, "cmpw $0xd7b1,0x2(%di)"),
        (12, "je 0xb2ec"),
    ]
    assert decode(RAM, LOADED, LOADED + SIZE) == [
        (0, "mov $0x6ea,%ax"),
        (3, "xor %cx,%cx"),
        (5, "mov %cx,%bx"),
        (7, "cmpw $0xd7b1,0x2(%di)"),
        (12, "je 0x13836"),
    ]
    assert sha256(start[SOURCE + 12:SOURCE + 14]).hexdigest() == JE_SHA
    assert LOADED + SIZE == (0x131C << 4) + 0x064D
    assert 0x13836 - (0x131C << 4) == 0x0676
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+063F source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"unique_hits=1 bytes=14 instructions=5 branches=1 sha256={SPAN_SHA}")
    print(f"JE sha256={JE_SHA} taken=131C:0676 fallthrough=131C:064D")
    print("DS:DI, type word, native reach and branch outcome unmeasured")


if __name__ == "__main__":
    main()
