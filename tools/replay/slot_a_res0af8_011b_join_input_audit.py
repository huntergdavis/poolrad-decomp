#!/usr/bin/env python3
"""Audit both loaded 011B entries through their first far-pointer input."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
PREFIX_SHA = "38baf501b4f10a9cddf575df137673a0496716ef0d8f526124c89023f78e4ab7"
STORES_SHA = "751d75c900ef962f95f19f4111459c32cf118af5a3d3a4abd7853db1ebf0249d"
READ_SHA = "e546db32a2cc6329368ab7a74f1dfe0641fd39c0bfa0a5cd1c9ee36ea918a8a9"
ZERO_SHA = "b78dfd99ac48889c52dc5d0afada44a893f8555305ace89dcd3d881e5eabd4ae"
ES_SHA = "df10903d8d3dc5bf4ea933866fc67847413b7f440d38ff2e6f4cb71ba12f7f2b"
SOURCE = 0xAD91
LOADED = (0x131C << 4) + 0x011B
LENGTH = 12


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
    original_image = (0x0AF8 << 4) + 0x011B
    assert original_image == 0xB09B and LOADED == 0x132DB
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    prefix = start[SOURCE:SOURCE + LENGTH]
    assert len(prefix) == LENGTH and prefix == ram[LOADED:LOADED + LENGTH]
    assert sha256(prefix).hexdigest() == PREFIX_SHA
    assert [i for i in range(len(start) - LENGTH + 1)
            if start[i:i + LENGTH] == prefix] == [SOURCE]
    assert sha256(prefix[:8]).hexdigest() == STORES_SHA
    assert sha256(prefix[8:]).hexdigest() == READ_SHA
    expected = [(0, "mov %cx,0x441e"), (4, "mov %bx,0x4420"),
                (8, "les 0x4418,%bx")]
    assert decode(START, SOURCE, SOURCE + LENGTH) == expected
    assert decode(RAM, LOADED, LOADED + LENGTH) == expected
    assert sha256(start[0xAD8A:0xAD91]).hexdigest() == ZERO_SHA
    assert sha256(start[0xAD88:0xAD91]).hexdigest() == ES_SHA
    assert decode(START, 0xAD75, 0xAD79)[1] == (2, "je 0xad8a")
    assert decode(START, 0xAD79, 0xAD82)[2] == (7, "je 0xad88")
    assert decode(RAM, 0x132BF, 0x132C3)[1] == (2, "je 0x132d4")
    assert decode(RAM, 0x132C3, 0x132CC)[2] == (7, "je 0x132d2")
    print(f"START.EXE bytes=65231 sha256={START_SHA}")
    print(f"loaded RAM bytes=655360 sha256={RAM_SHA}")
    print(f"join source [0x{SOURCE:05X},0x{SOURCE + LENGTH:05X}) "
          f"unique_hits=1 bytes=12 instructions=3 sha256={PREFIX_SHA}")
    print(f"join loaded [0x{LOADED:05X},0x{LOADED + LENGTH:05X}) sha256={PREFIX_SHA}")
    print(f"two stores bytes=8 sha256={STORES_SHA}; "
          f"first far-pointer read bytes=4 sha256={READ_SHA}")
    print("0114 incoming BX formula and 0112 ES-copy formula both join at 011B")
    print("typed unsupported input=far pointer DS:4418; no branch outcome or C parity claim")


if __name__ == "__main__":
    main()
