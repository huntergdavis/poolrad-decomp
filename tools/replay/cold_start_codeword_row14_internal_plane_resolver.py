#!/usr/bin/env python3
"""Source-locked address resolver for a future stopped-state U19 plane witness.

This does not capture a native store, read guest memory, or infer a page/RGB result.
"""

import argparse
from dataclasses import dataclass
from hashlib import sha256
from pathlib import Path
import subprocess


SOURCE_COMMIT = "5a9c7c53481c2e933948039c33256776653f4faf"
SOURCE_SHA = "f506017948503fa22ce40715205293c987473a938247cbd09ba409ffbb652c14"
WINDOWS = (
    (331, 385, "bdeed38164872c251e714d1a3b778a5f6c3a836e932f78e72a0209fb07e15efc"),
    (580, 665, "c68333a8194bdd04e78c09ed187dd3279ca4e17bd6585f18fc84ba37abb1b667"),
    (2491, 2564, "5ae149c01a7abe9eeb1081c22d6c9ee852d4ab02aad43dcf79223b2b79c93a7b"),
    (2815, 2840, "2eb7a3090afe61d6eb6b08e8d5fcd328238cd8c0949cca2f32ce0bb46ee8ced5"),
)


@dataclass(frozen=True)
class HostMap:
    handler: str
    physical_address: int
    page_mask: int
    bank_read_full: int
    memmask: int
    memsize: int
    seq_memory_mode: int
    gfx_miscellaneous: int
    compatible_chain4: bool = False
    ignore_odd_even: bool = False
    ignore_extended_memory_bit: bool = False
    is_vga_arch: bool = False


def planar_index(state: HostMap) -> int:
    """Resolve the four-byte backing word from a separately measured host state.

    physical_address is already the host paging translation of the guest address.
    The pinned unchained write handlers use bank_read_full even for writes.
    """
    if state.handler not in ("uvga", "uvga_fast"):
        raise ValueError("unsupported A000 page handler")
    if (state.memsize <= 0 or state.memsize % 4 or state.memmask < 0 or
            min(state.physical_address, state.page_mask, state.bank_read_full,
                state.seq_memory_mode, state.gfx_miscellaneous) < 0):
        raise ValueError("invalid host mapping state")
    base = (state.physical_address & state.page_mask) + state.bank_read_full
    if state.handler == "uvga_fast":
        index = base & (state.memmask >> 2)
    else:
        hobit = 16 if (state.seq_memory_mode & 2 or
                       (state.ignore_extended_memory_bit and state.is_vga_arch)) else 14
        high = 0 if state.compatible_chain4 else ~0xFFFF
        if state.gfx_miscellaneous & 2 and not state.ignore_odd_even:
            mask = high + (1 << hobit) - 2
            index = (base & mask & (state.memmask >> 2)) + ((base >> hobit) & 1)
        else:
            mask = high + (1 << hobit) - 1
            index = base & mask & (state.memmask >> 2)
    if index >= state.memsize // 4:
        raise ValueError("resolved four-plane word outside backing memory")
    return index


def audit_source(source: Path) -> None:
    commit = subprocess.check_output(
        ["git", "-C", str(source), "rev-parse", "HEAD"], text=True).strip()
    if commit != SOURCE_COMMIT:
        raise ValueError("private DOSBox-X revision differs")
    data = (source / "src/hardware/vga_memory.cpp").read_bytes()
    if sha256(data).hexdigest() != SOURCE_SHA:
        raise ValueError("private VGA memory source differs")
    lines = data.splitlines(keepends=True)
    for first, last, expected in WINDOWS:
        if sha256(b"".join(lines[first - 1:last])).hexdigest() != expected:
            raise ValueError(f"private VGA memory source window {first}-{last} differs")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    args = parser.parse_args()
    audit_source(args.source)
    print("source lock: one private file, four exact windows; PASS")
    print("uvga and uvga_fast mapping available for measured host state only")
    print("selected store, latch preservation, four-plane bytes and page/RGB unmeasured")


if __name__ == "__main__":
    main()
