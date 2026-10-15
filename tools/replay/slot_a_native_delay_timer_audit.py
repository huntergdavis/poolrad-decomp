#!/usr/bin/env python3
"""Audit address/hash/numeric evidence from an ignored native DELAY replay.

Usage: python3 tools/replay/slot_a_native_delay_timer_audit.py RUN_DIR
RUN_DIR is the private oracle-runs/slot-a-native-delay-reach-p9-099 copy.
No original bytes or RAM contents are printed.
"""

from __future__ import annotations

import hashlib
import sys
from pathlib import Path


INPUT_HASHES = {
    "START.EXE": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "GAME.OVR": "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
    "SAVGAMA.DAT": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
}

RAM_HASHES = {
    "approach-entry-private.ram": "19ece22a5727727127190d2d3ef37548949308c4a91a08112daffd0f174b97b6",
    "delay-entry-private.ram": "007c1679b4899123e0b5ad29a274534befbcd8dc76e551c5faac6f64e79660b4",
    "wrapper-entry-private.ram": "1675857cbf3c0a2e729d0fc134ae7799f02e5290946660cabdcf2015835c84af",
    "a40-entry-private.ram": "c088ddf8cbc7cf3321a3afdf8a79e012be5cb996ff19a5d2c0a4fe46fd8a7162",
    "a40-return-private.ram": "7ff60cfe5abe05bf78fc0d0bf78277c792c4bd22bb024a063f153edd5215e84a",
    "u1c-return-private.ram": "7ff60cfe5abe05bf78fc0d0bf78277c792c4bd22bb024a063f153edd5215e84a",
}

# Loaded ranges from the original-byte decode. All ranges end exclusively.
LOADED = (
    ("APPROACH", 0x3582, 0x0817, 58, "b18c884571cedd3093b2db99ab977f8f6d859d0bba838d0c6de65cbab72c1b98"),
    ("U1C DELAY", 0x3582, 0x271F, 14, "ecb6e7a981de47539c4809d0338832f0c107bf96a9e83d65e2847bf5a2f14ec2"),
    ("0709 wrapper", 0x0F2D, 0x0DBE, 21, "7830cf292711ac320d18f57cde8ddafdc977b6dd12d8438f1f449a34ca5635be"),
    ("A40 delay body", 0x1264, 0x029C, 33, "7ee3c156a50e06d1f23e4da738158d3cb48179223d3135c55b7976eae426e75e"),
    ("A40 spin helper", 0x1264, 0x02BD, 12, "d2df9bf05a361b9f27a4fe2591ea9938c31fb9d319ea1640d73bfbc52d93a2ea"),
)


def require(ok: bool, label: str) -> None:
    if not ok:
        raise ValueError(label)


def read_locked(path: Path, expected_hash: str, expected_size: int | None = None) -> bytes:
    data = path.read_bytes()
    if expected_size is not None:
        require(len(data) == expected_size, f"{path.name}: size mismatch")
    require(hashlib.sha256(data).hexdigest() == expected_hash,
            f"{path.name}: SHA-256 mismatch")
    return data


def address(segment: int, offset: int) -> int:
    return ((segment << 4) + (offset & 0xFFFF)) & 0xFFFFF


def byte(memory: bytes, segment: int, offset: int) -> int:
    return memory[address(segment, offset)]


def word(memory: bytes, segment: int, offset: int) -> int:
    return byte(memory, segment, offset) | (byte(memory, segment, offset + 1) << 8)


def dword(memory: bytes, segment: int, offset: int) -> int:
    return word(memory, segment, offset) | (word(memory, segment, offset + 2) << 16)


def entry_frame(memory: bytes, sp: int, saved_bp: int, ip: int, cs: int) -> None:
    ss = 0x1D16
    require(word(memory, ss, sp) == ip, "far return IP mismatch")
    require(word(memory, ss, sp + 2) == cs, "far return CS mismatch")
    require(word(memory, ss, sp + 4) == saved_bp, "entry saved BP mismatch")


def return_frame(memory: bytes, sp: int, saved_bp: int, ip: int, cs: int) -> None:
    ss = 0x1D16
    require(word(memory, ss, sp) == saved_bp, "return saved BP mismatch")
    require(word(memory, ss, sp + 2) == ip, "return IP mismatch")
    require(word(memory, ss, sp + 4) == cs, "return CS mismatch")


def audit(run_dir: Path) -> None:
    root = Path(__file__).resolve().parents[2] / "resources" / "oracle-runs"
    require(run_dir.resolve().is_relative_to(root.resolve()),
            "run must be under ignored resources/oracle-runs")
    for name, digest in INPUT_HASHES.items():
        read_locked(run_dir / "POOLRAD" / name, digest)
    ram = {name: read_locked(run_dir / name, digest, 655360)
           for name, digest in RAM_HASHES.items()}
    approach = ram["approach-entry-private.ram"]
    delay = ram["delay-entry-private.ram"]
    wrapper = ram["wrapper-entry-private.ram"]
    entry = ram["a40-entry-private.ram"]
    returned = ram["a40-return-private.ram"]
    u1c_return = ram["u1c-return-private.ram"]

    for name, segment, offset, size, digest in LOADED:
        start = address(segment, offset)
        require(hashlib.sha256(entry[start:start + size]).hexdigest() == digest,
                f"{name}: loaded SHA-256 mismatch")

    ds = 0x14A0
    require(word(approach, ds, 0x49ED) == 0x9C27, "APPROACH PC")
    require(word(delay, ds, 0x49ED) == 0x9C28, "DELAY PC")
    require(word(wrapper, ds, 0x49ED) == 0x9C29, "wrapper PC")
    require(word(entry, ds, 0x49ED) == 0x9C29, "A40 entry PC")
    require(word(returned, ds, 0x49ED) == 0x9C29, "A40 return PC")
    require(tuple(byte(delay, ds, x) for x in (0x6AAD, 0x6AAE, 0x6AAF))
            == (14, 4, 6), "party tuple")
    require(byte(entry, ds, 0x49E2) == 4, "delay factor")
    require(word(entry, ds, 0x84FC) == 269, "calibration")

    # A40 uses ES=0040, DI=0: this is the serial-port byte, not the clock.
    require(byte(entry, 0x40, 0) == byte(returned, 0x40, 0) == 248,
            "BIOS baseline byte")
    require(word(entry, 0x1D16, 0x3EDA) == 900, "A40 argument")
    entry_frame(approach, 0x3EE4, 0x3EEE, 0x2FB5, 0x3582)
    entry_frame(delay, 0x3EE4, 0x3EEE, 0x3151, 0x3582)
    entry_frame(wrapper, 0x3EDE, 0x3EE8, 0x272B, 0x3582)
    require(word(entry, 0x1D16, 0x3ED6) == 0x0DD1, "A40 return IP")
    require(word(entry, 0x1D16, 0x3ED8) == 0x0F2D, "A40 return CS")
    require(word(entry, 0x1D16, 0x3EDA) == 900, "A40 argument frame")
    require(word(entry, 0x1D16, 0x3EDC) == 0x3EE2, "A40 saved BP")
    return_frame(returned, 0x3EDC, 0x3EE2, 0x272B, 0x3582)
    return_frame(u1c_return, 0x3EE2, 0x3EE8, 0x3151, 0x3582)
    require(returned == u1c_return, "RAM changed across wrapper return")

    ticks_before = dword(entry, 0x40, 0x6C)
    ticks_after = dword(returned, 0x40, 0x6C)
    delta = (ticks_after - ticks_before) & 0xFFFFFFFF
    require((ticks_before, ticks_after, delta) == (1090044, 1090061, 17),
            "BIOS tick delta")
    changed = [i for i, pair in enumerate(zip(entry, returned))
               if pair[0] != pair[1]]
    require(len(changed) == 21, "A40 RAM change count")
    expected_changed = (
        list(range(0x46C, 0x46E))
        + list(range(0x21022, 0x2102B))
        + list(range(0x2102C, 0x21036))
    )
    require(changed == expected_changed, "A40 RAM diff intervals")

    print("inputs: START/GAME/slot-A SHA-256 locked")
    print("loaded chain: APPROACH, DELAY, wrapper, A40 body/helper SHA-256 locked")
    print("route: PC 9C27 -> 9C28 -> 9C29; party 14,4,6; factor 4; A40 argument 900")
    print("frames: APPROACH, DELAY, wrapper, A40 entry, wrapper and U1C returns verified")
    print(f"timer: BIOS 0040:006C {ticks_before} -> {ticks_after}, delta {delta} ticks")
    print("RAM: 21 changed bytes confined to BIOS timer and guest stack; U1C return unchanged")


if __name__ == "__main__":
    if len(sys.argv) != 2:
        raise SystemExit("usage: slot_a_native_delay_timer_audit.py RUN_DIR")
    try:
        audit(Path(sys.argv[1]))
    except (OSError, ValueError) as exc:
        raise SystemExit(f"audit failed: {exc}") from exc
