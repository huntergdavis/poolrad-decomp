#!/usr/bin/env python3
"""Audit ignored slot-A DELAY/A40 IRQ stack snapshots; print numeric evidence only."""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RUN_ROOT = ROOT / "resources" / "oracle-runs"
RAM_SIZE = 0xA0000
SS = 0x1D16
ENTRY_SP = 0x3ED6
INPUT_HASHES = {
    "START.EXE": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "GAME.OVR": "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
    "SAVGAMA.DAT": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
}
RAM_HASHES = {
    "a40-entry-private.ram": "ace0a0ff3aac14152a331b505baa1c924ecde4a9755bb18e293675e648176a1b",
    "a40-first-irq-private.ram": "36415a7a4f584183a11703b44bb5a8410ea460ba879ff7f69caded5e239cd58a",
    "a40-return-private.ram": "ea096da21f27bceddf4f35fe5d9721b81eace9bf0aa853338e04297a7b35c92c",
}
LOADED_HASHES = (
    (0x3582, 0x271F, 14, "ecb6e7a981de47539c4809d0338832f0c107bf96a9e83d65e2847bf5a2f14ec2"),
    (0x0F2D, 0x0DBE, 21, "7830cf292711ac320d18f57cde8ddafdc977b6dd12d8438f1f449a34ca5635be"),
    (0x1264, 0x029C, 45, "d493452e23ca3a0e81d53c6c6e31171d56f543409f3881247d729c5e394e29cb"),
)


def require(condition: bool, label: str) -> None:
    if not condition:
        raise ValueError(label)


def locked(path: Path, digest: str) -> bytes:
    data = path.read_bytes()
    require(hashlib.sha256(data).hexdigest() == digest,
            f"{path.name}: SHA-256 mismatch")
    return data


def physical(segment: int, offset: int) -> int:
    return ((segment << 4) + (offset & 0xFFFF)) & 0xFFFFF


def word(memory: bytes, segment: int, offset: int) -> int:
    at = physical(segment, offset)
    require(at + 2 <= len(memory), "word outside RAM capture")
    return memory[at] | (memory[at + 1] << 8)


def dword(memory: bytes, segment: int, offset: int) -> int:
    return word(memory, segment, offset) | (word(memory, segment, offset + 2) << 16)


def intervals(left: bytes, right: bytes) -> list[tuple[int, int]]:
    require(len(left) == len(right), "RAM image lengths differ")
    result: list[tuple[int, int]] = []
    start = -1
    for address, (a, b) in enumerate(zip(left, right)):
        if a != b and start < 0:
            start = address
        elif a == b and start >= 0:
            result.append((start, address))
            start = -1
    if start >= 0:
        result.append((start, len(left)))
    return result


def verify_frame(entry: bytes, irq: bytes, returned: bytes,
                 ss: int = SS, sp: int = ENTRY_SP) -> tuple[int, int]:
    """Validate the near call, IRQ0 CPU frame, and bounded residual intervals."""
    require(len(entry) == len(irq) == len(returned), "RAM size mismatch")
    require((word(entry, ss, sp), word(entry, ss, sp + 2),
             word(entry, ss, sp + 4)) == (0x0DD1, 0x0F2D, 900),
            "A40 far return or argument")
    require(word(irq, ss, sp - 2) == 0x02B7,
            "A40 near-call return before IRQ0")
    require((word(irq, ss, sp - 8), word(irq, ss, sp - 6),
             word(irq, ss, sp - 4)) == (0x02C1, 0x1264, 0x7206),
            "IRQ0 saved IP/CS/FLAGS frame")
    first_byte = physical(ss, sp - 3)
    require((entry[first_byte], irq[first_byte]) == (0x04, 0x72),
            "first watched byte 3ED3")
    irq_changed = intervals(entry, irq)
    require(irq_changed == [(physical(ss, sp - 8), physical(ss, sp))],
            "first IRQ stop changed unexpected RAM")
    low = physical(ss, sp - 0x14)
    high = physical(ss, sp - 2)
    require(intervals(entry[low:high], returned[low:high]) ==
            [(0, 9), (10, 18)], "17 residual stack-byte bounds")
    require((word(returned, ss, sp), word(returned, ss, sp + 2)) ==
            (0x0DD1, 0x0F2D), "A40 far return changed")
    return 6, 17  # Six watched bytes in the first CPU frame; 17 at return.


def audit(run_dir: Path) -> None:
    require(run_dir.resolve().is_relative_to(RUN_ROOT.resolve()),
            "run must be under ignored resources/oracle-runs")
    for name, digest in INPUT_HASHES.items():
        locked(run_dir / "POOLRAD" / name, digest)
    ram = {name: locked(run_dir / name, digest)
           for name, digest in RAM_HASHES.items()}
    require(all(len(data) == RAM_SIZE for data in ram.values()),
            "RAM capture size")
    entry, irq, returned = (ram[name] for name in RAM_HASHES)
    for segment, offset, size, digest in LOADED_HASHES:
        at = physical(segment, offset)
        require(hashlib.sha256(entry[at:at + size]).hexdigest() == digest,
                f"loaded span {segment:04X}:{offset:04X}")
    first_count, residual_count = verify_frame(entry, irq, returned)
    tick_delta = (dword(returned, 0x40, 0x6C) -
                  dword(entry, 0x40, 0x6C)) & 0xFFFFFFFF
    require(tick_delta == 16, "BIOS tick delta")
    require(intervals(entry, returned) ==
            [(0x46C, 0x46D), (0x21022, 0x2102B), (0x2102C, 0x21036)],
            "A40 entry-to-return RAM intervals")
    print("A40 frame: SS:SP=1D16:3ED6 return=0F2D:0DD1 argument=900")
    print("first IRQ0 stop: F000:FEA5; saved 1264:02C1; SS:SP=1D16:3ECE")
    print(f"first IRQ0 stop: {first_count} watched bytes changed by CPU frame")
    print(f"A40 return: {residual_count} residual stack bytes differ; BIOS ticks +{tick_delta}")
    print("RAM capture: [0x00000,0xA0000); BIOS code above range")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: slot_a_delay_irq_stack_audit.py RUN_DIR")
    try:
        audit(Path(sys.argv[1]))
    except (OSError, ValueError) as exc:
        raise SystemExit(f"DELAY IRQ stack audit failed: {exc}") from exc
