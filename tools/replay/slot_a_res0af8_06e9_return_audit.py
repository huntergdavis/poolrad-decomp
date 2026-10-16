#!/usr/bin/env python3
"""Pin loaded RES_0AF8:06E9 near RET with unique preceding context."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
RET_SHA = "ae3f4619b0413d70d3004b9131c3752153074e45725be13b9a148978895e359e"
CONTEXT_SHA = "6c68916d956424a3c1295f94615cda763edb04fbf32a0d6cc4fb6124efbb8ef6"
SOURCE = 0xB35F
LOADED = 0x138A9
CONTEXT_SOURCE = 0xB349
CONTEXT_LOADED = 0x13893
CONTEXT_SIZE = 23


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
    assert (0x0AF8 << 4) + 0x06E9 == 0xB669
    assert (0x131C << 4) + 0x06E9 == LOADED
    assert 0xB669 - SOURCE == 0x30A
    assert LOADED - 0xB669 == 0x8240
    body = start[SOURCE:SOURCE + 1]
    assert len(body) == 1 and body == ram[LOADED:LOADED + 1]
    assert sha256(body).hexdigest() == RET_SHA
    assert decode(START, SOURCE, SOURCE + 1) == [(0, "ret")]
    assert decode(RAM, LOADED, LOADED + 1) == [(0, "ret")]
    assert sum(start[i:i + 1] == body for i in range(len(start))) == 140
    assert CONTEXT_SOURCE + CONTEXT_SIZE == SOURCE + 1
    assert CONTEXT_LOADED + CONTEXT_SIZE == LOADED + 1
    context = start[CONTEXT_SOURCE:CONTEXT_SOURCE + CONTEXT_SIZE]
    assert context == ram[CONTEXT_LOADED:CONTEXT_LOADED + CONTEXT_SIZE]
    assert sha256(context).hexdigest() == CONTEXT_SHA
    assert [i for i in range(len(start) - CONTEXT_SIZE + 1)
            if start[i:i + CONTEXT_SIZE] == context] == [CONTEXT_SOURCE]
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"+06E9 source [0x{SOURCE:05X},0x{SOURCE + 1:05X}) "
          f"loaded [0x{LOADED:05X},0x{LOADED + 1:05X}) "
          f"bytes=1 instructions=1 start_hits=140 sha256={RET_SHA}")
    print(f"context source [0x{CONTEXT_SOURCE:05X},0x{CONTEXT_SOURCE + CONTEXT_SIZE:05X}) "
          f"loaded [0x{CONTEXT_LOADED:05X},0x{CONTEXT_LOADED + CONTEXT_SIZE:05X}) "
          f"unique_hits=1 bytes={CONTEXT_SIZE} sha256={CONTEXT_SHA}")
    print("near RET at 131C:06E9; native reach and return frame unmeasured")


if __name__ == "__main__":
    main()
