#!/usr/bin/env python3
"""Audit the loaded 131C:0608 store after the nonzero return decision."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
ANCHOR_SHA = "89ccf1ac7c0aad9c03d35681e115f8b2953d425add72c23a830b90f3f0a2043b"
DECISION_SHA = "66faa6ae049f7aa3904180632d00f46e3abd7aad7db42a98a4ec6c023c677858"
STORE_SHA = "bf754db91e10c283bf2b78bd69d4d97ccf871861a819899ad2648cdd9ba5722e"
NEXT_SHA = "5e4a22596185d7238317b5a9ab7ed66b7baae2941db391e08cd8778fd944e889"
ANCHOR_SOURCE = 0xB27A
SOURCE = 0xB27E
LOADED = (0x131C << 4) + 0x0608
LENGTH = 3


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


def occurrences(haystack: bytes, needle: bytes) -> list[int]:
    return [i for i in range(len(haystack) - len(needle) + 1)
            if haystack[i:i + len(needle)] == needle]


def main() -> None:
    start = START.read_bytes()
    ram = RAM.read_bytes()
    assert len(start) == 65231 and sha256(start).hexdigest() == START_SHA
    assert len(ram) == 655360 and sha256(ram).hexdigest() == RAM_SHA
    original_image = (0x0AF8 << 4) + 0x0608
    assert original_image == 0xB588 and LOADED == 0x137C8
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    anchor = start[ANCHOR_SOURCE:ANCHOR_SOURCE + 10]
    store = start[SOURCE:SOURCE + LENGTH]
    successor = start[SOURCE + LENGTH:SOURCE + LENGTH + 3]
    assert anchor == ram[LOADED - 4:LOADED + 6]
    assert store == ram[LOADED:LOADED + LENGTH]
    assert successor == ram[LOADED + LENGTH:LOADED + LENGTH + 3]
    assert sha256(anchor).hexdigest() == ANCHOR_SHA
    assert sha256(anchor[:4]).hexdigest() == DECISION_SHA
    assert sha256(store).hexdigest() == STORE_SHA
    assert sha256(successor).hexdigest() == NEXT_SHA
    assert occurrences(start, anchor) == [ANCHOR_SOURCE]
    store_hits = occurrences(start, store)
    assert len(store_hits) == 13 and SOURCE in store_hits
    assert decode(START, ANCHOR_SOURCE, SOURCE) == [
        (0, "or %ax,%ax"), (2, "je 0xb281")]
    assert decode(RAM, LOADED - 4, LOADED) == [
        (0, "or %ax,%ax"), (2, "je 0x137cb")]
    assert decode(START, SOURCE, SOURCE + LENGTH) == [(0, "mov %ax,0x4426")]
    assert decode(RAM, LOADED, LOADED + LENGTH) == [(0, "mov %ax,0x4426")]
    assert decode(START, SOURCE + LENGTH, SOURCE + LENGTH + 3) == [
        (0, "pop %di"), (1, "pop %es"), (2, "ret")]
    assert decode(RAM, LOADED + LENGTH, LOADED + LENGTH + 3) == [
        (0, "pop %di"), (1, "pop %es"), (2, "ret")]
    print(f"START.EXE bytes=65231 sha256={START_SHA}")
    print(f"loaded RAM bytes=655360 sha256={RAM_SHA}")
    print(f"return anchor source [0x{ANCHOR_SOURCE:05X},0x{ANCHOR_SOURCE + 10:05X}) "
          f"unique_hits=1 sha256={ANCHOR_SHA}")
    print(f"store source [0x{SOURCE:05X},0x{SOURCE + LENGTH:05X}) "
          f"source_hits=13 bytes=3 instructions=1 sha256={STORE_SHA}")
    print(f"store loaded [0x{LOADED:05X},0x{LOADED + LENGTH:05X}) sha256={STORE_SHA}")
    print(f"next 131C:060B stack-return source sha256={NEXT_SHA}")
    print("AX!=0 reaches uint16 DS:4426 store; AX=0 skips it")
    print("AX, DS writable memory, and SS:SP are unsupported; no observed write or C parity")


if __name__ == "__main__":
    main()
