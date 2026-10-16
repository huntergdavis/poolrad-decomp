#!/usr/bin/env python3
"""Validate the ignored U19 host snapshot core and synthetic capture contracts."""

import argparse
from dataclasses import dataclass
from hashlib import sha256
from pathlib import Path
from typing import Callable

from tools.replay.cold_start_codeword_row14_internal_plane_resolver import HostMap, audit_source, planar_index


PRIVATE_FILES = {
    "plane_snapshot_core.hpp": "0557a1134652e65ec60310fa5e94defb6ff223b1e073a8a2364df3425ead07ea",
    "test_plane_snapshot_core.cpp": "e6f34c8bc55cc153d1c60f98a5a54a9e808672946ec82a74ba62ff9f709ef634",
    "plane_snapshot_dosbox_adapter.inc": "dbdea9df169ae5dcd20772a8350237d1a32ce1f7f03823980257f1cbd65f1077",
    "test_plane_snapshot_adapter.cpp": "abfa39ed23e5c72e2329db75864767b1de82987911496e0c4b355e3263a45ed4",
}
TARGET_LINEAR = 0xA118F


@dataclass(frozen=True)
class Snapshot:
    plane_index: int
    planes: bytes
    latch_before: int
    latch_after: int


def capture_backing(state: HostMap, backing: bytes | bytearray,
                    latch_read: Callable[[], int], *, guest_linear: int,
                    expected_read_bank: int, observed_write_bank: int,
                    expected_write_bank: int) -> Snapshot:
    """Read four backing bytes directly; never perform a guest VGA read."""
    if guest_linear != TARGET_LINEAR:
        raise ValueError("wrong selected guest target")
    if (state.bank_read_full != expected_read_bank or
            observed_write_bank != expected_write_bank):
        raise ValueError("changed host bank state")
    if len(backing) < state.memsize:
        raise ValueError("short host backing memory")
    index = planar_index(state)
    before = latch_read()
    planes = bytes(backing[4 * index:4 * index + 4])
    after = latch_read()
    if before != after:
        raise ValueError("latch changed during snapshot")
    return Snapshot(index, planes, before, after)


def audit_private(source: Path, helper_dir: Path) -> None:
    audit_source(source)
    for name, expected in PRIVATE_FILES.items():
        if sha256((helper_dir / name).read_bytes()).hexdigest() != expected:
            raise ValueError(f"ignored helper {name} differs")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--helper-dir", type=Path, required=True)
    args = parser.parse_args()
    audit_private(args.source, args.helper_dir)
    print("private source lock: one DOSBox-X file/four windows, four ignored helper files; PASS")
    print("ignored C++ helper is synthetic only; live host handler/latch/plane witness unmeasured")


if __name__ == "__main__":
    main()
