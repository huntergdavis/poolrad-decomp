#!/usr/bin/env python3
"""Audit loaded 131C:060B POP/POP/RET from a supplied stack frame."""

from hashlib import sha256
from pathlib import Path
import re
import subprocess

START = Path("resources/DOS/POOLRAD/START.EXE")
RAM = Path("resources/oracle-runs/slot-a-ui-postdrawer-wait-p9-094/postdrawer-048e-ram-private.bin")
START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "a0f56961fbbf02c12c6f36b8617d51261af06dbfb88eba2b1b4a77b29189083f"
TAIL_SHA = "5e4a22596185d7238317b5a9ab7ed66b7baae2941db391e08cd8778fd944e889"
ANCHOR_SHA = "89ccf1ac7c0aad9c03d35681e115f8b2953d425add72c23a830b90f3f0a2043b"
NEXT_SHA = "6b10f8594bce57f6a45463f792808303f6d3a14d6da0baece99d85d4ac583408"
SOURCE = 0xB281
LOADED = (0x131C << 4) + 0x060B
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
    original_image = (0x0AF8 << 4) + 0x060B
    assert original_image == 0xB58B and LOADED == 0x137CB
    assert original_image - SOURCE == 0x30A
    assert LOADED - original_image == 0x8240
    tail = start[SOURCE:SOURCE + LENGTH]
    anchor = start[SOURCE - 7:SOURCE + LENGTH]
    next_source = start[SOURCE + LENGTH:SOURCE + LENGTH + 4]
    assert tail == ram[LOADED:LOADED + LENGTH]
    assert anchor == ram[LOADED - 7:LOADED + LENGTH]
    assert next_source == ram[LOADED + LENGTH:LOADED + LENGTH + 4]
    assert sha256(tail).hexdigest() == TAIL_SHA
    assert sha256(anchor).hexdigest() == ANCHOR_SHA
    assert sha256(next_source).hexdigest() == NEXT_SHA
    assert occurrences(start, tail) == [SOURCE]
    assert occurrences(start, anchor) == [SOURCE - 7]
    assert decode(START, SOURCE, SOURCE + LENGTH) == [
        (0, "pop %di"), (1, "pop %es"), (2, "ret")]
    assert decode(RAM, LOADED, LOADED + LENGTH) == [
        (0, "pop %di"), (1, "pop %es"), (2, "ret")]
    assert decode(START, SOURCE - 7, SOURCE) == [
        (0, "or %ax,%ax"), (2, "je 0xb281"),
        (4, "mov %ax,0x4426")]
    assert decode(RAM, LOADED - 7, LOADED) == [
        (0, "or %ax,%ax"), (2, "je 0x137cb"),
        (4, "mov %ax,0x4426")]
    print(f"START.EXE bytes=65231 sha256={START_SHA}")
    print(f"loaded RAM bytes=655360 sha256={RAM_SHA}")
    print(f"tail source [0x{SOURCE:05X},0x{SOURCE + LENGTH:05X}) "
          f"unique_hits=1 bytes=3 instructions=3 terminal=RET sha256={TAIL_SHA}")
    print(f"tail loaded [0x{LOADED:05X},0x{LOADED + LENGTH:05X}) sha256={TAIL_SHA}")
    print(f"unique return suffix bytes=10 sha256={ANCHOR_SHA}")
    print(f"sequential next 131C:060E source sha256={NEXT_SHA}")
    print("supplied SS:SP three words -> DI, ES, near return IP; SP advances 6 mod 65536")
    print("far-callee cleanup and actual stack contents unknown; no native return or C parity")


if __name__ == "__main__":
    main()
