#!/usr/bin/env python3
"""Pin the first Move byte while leaving all EGA hardware inputs unknown."""

from __future__ import annotations

from hashlib import sha256
from pathlib import Path
import re
import subprocess
from tempfile import TemporaryDirectory


ROOT = Path(__file__).resolve().parents[2]
EXE = ROOT / "resources/DOS/POOLRAD/START.EXE"
RUN = ROOT / "resources/oracle-runs/slot-a-u40c-all-dirty-p5-001"
EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = "25ae4cdbceb249e04ec413b49e6cbfc6f14ff4210378bbdd34e19d37389266d1"
GC_SHA = "4a474c34200525498a5a223630e9a858db3902bb713d4191c70b7214d5bd6ce5"
FLUSH_SHA = "7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c"
SETUP_SHA = "79fe951cb779d3f656e78693b928c652483f415b8c8272792f7b2eb4b74d7851"
MOVE_SHA = "1941e7e4eafb9300636ac7ff777605d8ea0b96654893d1198ca3d5f0bfd93972"


def pinned(path: Path, expected: str) -> bytes:
    resolved = path.resolve(strict=True)
    if not resolved.is_relative_to((ROOT / "resources").resolve()):
        raise ValueError("input escaped ignored resources/")
    data = resolved.read_bytes()
    if sha256(data).hexdigest() != expected:
        raise ValueError(f"private hash differs: {path.name}")
    return data


def decode(span: bytes, vma: int) -> dict[int, tuple[str, str]]:
    with TemporaryDirectory(prefix="one-byte-source-", dir=ROOT / "resources") as tmp:
        path = Path(tmp) / "span.bin"
        path.write_bytes(span)
        output = subprocess.run(
            ["objdump", "-D", "-b", "binary", "-m", "i8086",
             f"--adjust-vma=0x{vma:X}", "--no-show-raw-insn", str(path)],
            capture_output=True, text=True, check=True,
        ).stdout
    pattern = re.compile(r"^\s*([0-9a-f]+):\s*([a-z][a-z0-9]*)\s*(.*?)\s*$")
    found = {}
    for line in output.splitlines():
        match = pattern.match(line)
        if match:
            found[int(match.group(1), 16)] = (
                match.group(2), re.sub(r"\s+", "", match.group(3))
            )
    return found


def word(ram: bytes, address: int) -> int:
    return ram[address] | (ram[address + 1] << 8)


def main() -> None:
    original = pinned(EXE, EXE_SHA)
    for label, lo, hi, expected in (
        ("flush", 0x61CB, 0x64D5, FLUSH_SHA),
        ("setup", 0x63AA, 0x63CA, SETUP_SHA),
        ("Move", 0xC2FB, 0xC31E, MOVE_SHA),
    ):
        span = original[lo:hi]
        if len(span) != hi - lo or sha256(span).hexdigest() != expected:
            raise ValueError(f"{label} extent/hash differs")
    setup = decode(original[0x63AA:0x63CA], 0x1D31)
    move = decode(original[0xC2FB:0xC31E], 0x1685)
    expected = {
        0x1D31: ("mov", "$0xf,%al"),
        0x1D36: ("out", "%al,(%dx)"),
        0x1D44: ("mov", "$0x1,%al"),
        0x1D49: ("out", "%al,(%dx)"),
        0x1695: ("cld", ""),
        0x1698: ("jae", "0x16a1"),
        0x16A1: ("rep", "movsb%ds:(%si),%es:(%di)"),
        0x16A3: ("mov", "%dx,%ds"),
        0x16A5: ("lret", "$0xa"),
    }
    for address, instruction in expected.items():
        if (setup | move).get(address) != instruction:
            raise ValueError(f"original operand differs at +{address:04X}")
    ram = pinned(RUN / "ram-flush-entry.bin", RAM_SHA)
    gc = pinned(RUN / "gc-flush-entry.txt", GC_SHA).decode("ascii").splitlines()
    ds = 0x14A00
    row = next((r for r in range(168) if ram[ds + 0x4A28 + r]), None)
    first = word(ram, ds + 0x4C20 + 2 * 16)
    last = word(ram, ds + 0x4D70 + 2 * 16)
    low = word(ram, ds + 0x4AD0 + 2 * 16)
    start = (word(ram, ds + 0x5060) + 1 + 16 * 40 + low) & 0xFFFF
    if (len(ram), row, first, last, low, start,
            (((last - first) & 0xFFFF) >> 2) + 1) != (
            0xA0000, 16, 1352, 1395, 2, 0x03C3, 11):
        raise ValueError("first-row input geometry differs")
    if (len(gc) != 17 or
            tuple(gc[4 + i * 2] for i in range(6)) != tuple(
                f"OUTP 3CE {r:02X}" for r in (0, 1, 3, 4, 5, 8))):
        raise ValueError("entry GC capture set differs")
    print("START/RAM/GC and flush/setup/Move hashes; nine operands checked")
    print("conditional first host byte A200:03C3 -> A000:03C3, count11")
    print("STOP: GC6/Seq4/handler/banks/source/page planes at +16A1 unmeasured")


if __name__ == "__main__":
    main()
