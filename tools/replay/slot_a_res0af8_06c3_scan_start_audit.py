#!/usr/bin/env python3
"""Pin the loaded RES_0AF8:06C3 scan-start source branch."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
SPAN_SHA = "f8aa5307503265eb9d552a2ec4bef725fa26ac5a516b1611d5c8bbe2db82d1e8"
SOURCE = 0xB339
LOADED = 0x13883
SIZE = 6


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
    assert (0x0AF8 << 4) + 0x06C3 == 0xB643
    assert (0x131C << 4) + 0x06C3 == LOADED
    assert 0xB643 - SOURCE == 0x30A
    assert LOADED - 0xB643 == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == SPAN_SHA
    assert [i for i in range(len(start) - SIZE + 1)
            if start[i:i + SIZE] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + SIZE) == [
        (0, "xor %bx,%bx"),
        (2, "cmp %ax,%bx"),
        (4, "je 0xb35f"),
    ]
    assert decode(RAM, LOADED, LOADED + SIZE) == [
        (0, "xor %bx,%bx"),
        (2, "cmp %ax,%bx"),
        (4, "je 0x138a9"),
    ]
    assert LOADED + SIZE == (0x131C << 4) + 0x06C9
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+06C3 source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"unique_hits=1 bytes={SIZE} instructions=3 branches=1 sha256={SPAN_SHA}")
    print("JE taken=131C:06E9 fallthrough=131C:06C9; native path unmeasured")


if __name__ == "__main__":
    main()
