#!/usr/bin/env python3
"""Pin the loaded RES_0AF8:06C9 scan-loop source arm."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
SPAN_SHA = "4d4d9bd5f1aea8603bbf40a0846d5c3af27da0a9af337196c7587df371a9ec17"
SOURCE = 0xB33F
LOADED = 0x13889
SIZE = 10


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
    assert (0x0AF8 << 4) + 0x06C9 == 0xB649
    assert (0x131C << 4) + 0x06C9 == LOADED
    assert 0xB649 - SOURCE == 0x30A
    assert LOADED - 0xB649 == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == SPAN_SHA
    assert [i for i in range(len(start) - SIZE + 1)
            if start[i:i + SIZE] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + SIZE) == [
        (0, "cmpb $0x1a,0x80(%bx,%di)"),
        (5, "je 0xb349"),
        (7, "inc %bx"),
        (8, "jmp 0xb33b"),
    ]
    assert decode(RAM, LOADED, LOADED + SIZE) == [
        (0, "cmpb $0x1a,0x80(%bx,%di)"),
        (5, "je 0x13893"),
        (7, "inc %bx"),
        (8, "jmp 0x13885"),
    ]
    assert LOADED + SIZE == (0x131C << 4) + 0x06D3
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+06C9 source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"unique_hits=1 bytes={SIZE} instructions=4 branches=2 sha256={SPAN_SHA}")
    print("JE taken=131C:06D3; else BX increments, JMP=131C:06C5; native path unmeasured")


if __name__ == "__main__":
    main()
