#!/usr/bin/env python3
"""Audit loaded 131C:0615 through the first DS:DI word store."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
BODY_SHA = "7234769f818ccc0d93e92db72ded096dd584f1584c72eacc560f804ae64c8edb"
WRITE_SHA = "9789c11eed3409fb170ed9c09f5bb697149679344202c0653f1e06609201b454"
ANCHOR_SHA = "12586dd6325cca0490499dcc572c305f1f6d646052d506fe37e7f68dd8c2ae8c"
SOURCE = 0xB28B
LOADED = (0x131C << 4) + 0x0615


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
    original_image = (0x0AF8 << 4) + 0x0615
    assert original_image == 0xB595 and LOADED == 0x137D5
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    body = start[SOURCE:SOURCE + 4]
    assert len(body) == 4 and body == ram[LOADED:LOADED + 4]
    assert sha256(body).hexdigest() == BODY_SHA
    assert [i for i in range(len(start) - 3) if start[i:i + 4] == body] == [SOURCE]
    assert decode(START, SOURCE, SOURCE + 4) == [
        (0, "xor %cx,%cx"),
        (2, "mov %cx,(%di)"),
    ]
    assert decode(RAM, LOADED, LOADED + 4) == [
        (0, "xor %cx,%cx"),
        (2, "mov %cx,(%di)"),
    ]
    write = start[SOURCE + 2:SOURCE + 4]
    assert write == ram[LOADED + 2:LOADED + 4]
    assert sha256(write).hexdigest() == WRITE_SHA
    anchor = start[SOURCE - 7:SOURCE + 4]
    assert anchor == ram[LOADED - 7:LOADED + 4]
    assert len(anchor) == 11 and sha256(anchor).hexdigest() == ANCHOR_SHA
    assert LOADED + 4 - (0x131C << 4) == 0x0619
    print(f"START.EXE bytes={len(start)} sha256={START_SHA}")
    print(f"loaded RAM bytes={len(ram)} sha256={RAM_SHA}")
    print(f"body source [0x{SOURCE:05X},0x{SOURCE + 4:05X}) unique_hits=1 "
          f"bytes=4 instructions=2 stores=1 sha256={BODY_SHA}")
    print(f"body loaded [0x{LOADED:05X},0x{LOADED + 4:05X}) sha256={BODY_SHA}")
    print(f"word-store instruction sha256={WRITE_SHA} prior_anchor_sha256={ANCHOR_SHA}")
    print("next instruction=131C:0619; DS:DI caller frame and native reach unmeasured")


if __name__ == "__main__":
    main()
