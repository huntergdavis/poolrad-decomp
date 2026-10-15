#!/usr/bin/env python3
"""Pin loaded 131C:068C shared return with a unique preceding context."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
RETURN_SHA = "45755c99e39cfb753aa25ef66894dda3ef31d15961c9ce3ebf533bd8fb4145f1"
CONTEXT_SHA = "246f207460c20748409b24362da282047f90d7d76951a98e1d8963c3271b0715"
SOURCE = 0xB302
LOADED = 0x1384C
SIZE = 4
CONTEXT_SOURCE = 0xB2EC
CONTEXT_LOADED = 0x13836
CONTEXT_SIZE = 26


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


def hits(data: bytes, needle: bytes) -> list[int]:
    return [i for i in range(len(data) - len(needle) + 1)
            if data[i:i + len(needle)] == needle]


def main() -> None:
    start = START.read_bytes()
    ram = RAM.read_bytes()
    assert len(start) == 65231 and sha256(start).hexdigest() == START_SHA
    assert len(ram) == 655360 and sha256(ram).hexdigest() == RAM_SHA
    assert (0x0AF8 << 4) + 0x068C == 0xB60C
    assert (0x131C << 4) + 0x068C == LOADED
    assert 0xB60C - SOURCE == 0x30A
    assert LOADED - 0xB60C == 0x8240
    body = start[SOURCE:SOURCE + SIZE]
    assert len(body) == SIZE and body == ram[LOADED:LOADED + SIZE]
    assert sha256(body).hexdigest() == RETURN_SHA
    assert len(hits(start, body)) == 6
    assert decode(START, SOURCE, SOURCE + SIZE) == [
        (0, "pop %ds"),
        (1, "lret $0x4"),
    ]
    assert decode(RAM, LOADED, LOADED + SIZE) == [
        (0, "pop %ds"),
        (1, "lret $0x4"),
    ]
    context = start[CONTEXT_SOURCE:CONTEXT_SOURCE + CONTEXT_SIZE]
    assert len(context) == CONTEXT_SIZE
    assert context == ram[CONTEXT_LOADED:CONTEXT_LOADED + CONTEXT_SIZE]
    assert sha256(context).hexdigest() == CONTEXT_SHA
    assert hits(start, context) == [CONTEXT_SOURCE]
    assert CONTEXT_SOURCE + CONTEXT_SIZE == SOURCE + SIZE
    assert CONTEXT_LOADED + CONTEXT_SIZE == LOADED + SIZE
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+068C source [0x{SOURCE:05X},0x{SOURCE + SIZE:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + SIZE:05X}) "
          f"raw_hits=6 bytes=4 instructions=2 sha256={RETURN_SHA}")
    print(f"context [0x{CONTEXT_SOURCE:05X},0x{CONTEXT_SOURCE + CONTEXT_SIZE:05X}) "
          f"unique_hits=1 bytes=26 sha256={CONTEXT_SHA}")
    print("stack, return destination, native reach and path unmeasured")


if __name__ == "__main__":
    main()
