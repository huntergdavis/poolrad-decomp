#!/usr/bin/env python3
"""Audit the reached DELAY return frame; emit address/numeric evidence only."""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
RUN_ROOT = ROOT / "resources" / "oracle-runs"
INPUT_HASHES = {
    "START.EXE": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "GAME.OVR": "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
    "SAVGAMA.DAT": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
}
RETURN_RAM_HASH = "ea096da21f27bceddf4f35fe5d9721b81eace9bf0aa853338e04297a7b35c92c"
EPILOGUE_HASH = "05303d46935f1d6bc7f492470521a209a51050b261d452a788f042866b5d72cd"
SS = 0x1D16
A40_RETURN_SP = 0x3EDC
RESIDUAL = ((0x3EC2, 0x3ECB), (0x3ECC, 0x3ED4))  # Half-open SS offsets.


def require(condition: bool, label: str) -> None:
    if not condition:
        raise ValueError(label)


def physical(segment: int, offset: int) -> int:
    return ((segment << 4) + (offset & 0xFFFF)) & 0xFFFFF


def word(memory: bytes, segment: int, offset: int) -> int:
    at = physical(segment, offset)
    require(at + 2 <= len(memory), "return frame outside RAM capture")
    return memory[at] | (memory[at + 1] << 8)


def returned_reads(memory: bytes, ss: int = SS,
                   sp: int = A40_RETURN_SP) -> tuple[tuple[int, int], ...]:
    """Check two POP BP; RETF frames and return their half-open read ranges."""
    require((word(memory, ss, sp), word(memory, ss, sp + 2),
             word(memory, ss, sp + 4)) == (sp + 6, 0x272B, 0x3582),
            "resident wrapper return frame")
    require((word(memory, ss, sp + 6), word(memory, ss, sp + 8),
             word(memory, ss, sp + 10)) == (sp + 12, 0x3151, 0x3582),
            "U1C return frame")
    reads = ((sp, sp + 2), (sp + 2, sp + 6),
             (sp + 6, sp + 8), (sp + 8, sp + 12))
    for start, end in reads:
        require(all(end <= low or start >= high for low, high in RESIDUAL),
                "epilogue read overlaps residual stack")
    return reads


def locked(path: Path, expected: str) -> bytes:
    data = path.read_bytes()
    require(hashlib.sha256(data).hexdigest() == expected,
            f"{path.name}: SHA-256 mismatch")
    return data


def audit(run_dir: Path) -> None:
    require(run_dir.resolve().is_relative_to(RUN_ROOT.resolve()),
            "run must be under ignored resources/oracle-runs")
    for name, digest in INPUT_HASHES.items():
        locked(run_dir / "POOLRAD" / name, digest)
    memory = locked(run_dir / "a40-return-private.ram", RETURN_RAM_HASH)
    require(len(memory) == 0xA0000, "RAM capture size")
    for segment, offset in ((0x0F2D, 0x0DD1), (0x3582, 0x272B)):
        at = physical(segment, offset)
        require(hashlib.sha256(memory[at:at + 2]).hexdigest() == EPILOGUE_HASH,
                f"loaded epilogue {segment:04X}:{offset:04X}")
    reads = returned_reads(memory)
    print("A40 return: SS:SP=1D16:3EDC; residual bytes=17")
    print("wrapper: 0F2D:0DD1 POP BP, RETF -> 3582:272B; SP=3EE2")
    print("U1C: 3582:272B POP BP, RETF -> 3582:3151; SP=3EE8")
    print("epilogue read ranges: " + ", ".join(
        f"SS:[{start:04X},{end:04X})" for start, end in reads))
    print("first residual read after 3582:3151: unmeasured")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: slot_a_delay_residual_read_boundary_audit.py RUN_DIR")
    try:
        audit(Path(sys.argv[1]))
    except (OSError, ValueError) as exc:
        raise SystemExit(f"DELAY residual read boundary audit failed: {exc}") from exc
