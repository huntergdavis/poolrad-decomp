#!/usr/bin/env python3
"""Measure the saved u6F +1574 cold dispatch boundary without publishing RAM."""

import argparse
from pathlib import Path

from u6f_u9b_transfer_audit import audit as audit_original, instructions
from u9b_runtime_dispatch_audit import audit_native


CHECKPOINTS = (
    "pre-call-1574-ram.bin",
    "handler-entry-02e6-ram.bin",
    "pre-loader-0314-ram.bin",
    "pre-indirect-032a-ram.bin",
    "u9b-entry-0000-ram.bin",
)
STAGES = (
    (10, 1),
    (23, 2),
    (1188, 22),
    (3, 2),
)
NET = (1214, 20)


def changed_runs(before, after):
    """Return half-open changed address runs and the changed-byte count."""
    if len(before) != len(after):
        raise ValueError("snapshot sizes differ")
    runs = []
    count = 0
    start = None
    for address, (old, new) in enumerate(zip(before, after)):
        if old != new:
            count += 1
            if start is None:
                start = address
        elif start is not None:
            runs.append((start, address))
            start = None
    if start is not None:
        runs.append((start, len(before)))
    return runs, count


def audit_boundary(directory):
    # The existing audit verifies all nine capture hashes, saved stack words,
    # patched stub, loaded entry body, and the u9B return checkpoint.
    audit_native(directory)
    snapshots = [directory.joinpath(name).read_bytes() for name in CHECKPOINTS]

    for index, expected in enumerate(STAGES):
        runs, count = changed_runs(snapshots[index], snapshots[index + 1])
        if (count, len(runs)) != expected:
            raise ValueError(f"stage {index} changed: {count} bytes, {len(runs)} runs")
        print(f"{CHECKPOINTS[index]} -> {CHECKPOINTS[index + 1]}: "
              f"{count} changed RAM bytes in {len(runs)} runs")

    runs, count = changed_runs(snapshots[0], snapshots[-1])
    if (count, len(runs)) != NET:
        raise ValueError(f"pre-call to entry changed: {count} bytes, {len(runs)} runs")
    if not any(start < 0x2D133 and end > 0x2CCD0 for start, end in runs):
        raise ValueError("cold dispatch lacks the loaded overlay image")
    print(f"net pre-call +1574 -> u9B +0000 entry: {count} changed RAM "
          f"bytes in {len(runs)} runs")
    print("net changed physical address runs, inclusive:")
    for start, end in runs:
        print(f"  {start:05X}..{end - 1:05X} ({end - start})")


def main():
    root = Path(__file__).resolve().parents[2]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path,
                        default=root / "resources/DOS/POOLRAD/START.EXE")
    parser.add_argument("--ovr", type=Path,
                        default=root / "resources/DOS/POOLRAD/GAME.OVR")
    parser.add_argument("--native-dir", type=Path, required=True,
                        help="ignored native first-insertion capture directory")
    args = parser.parse_args()
    audit_original(args.exe, args.ovr)
    caller = args.ovr.read_bytes()[0x1F42C:0x1F565]
    decoded = instructions(caller, 0x1446)
    calls = [(item.address, item.op_str) for item in decoded.values()
             if "call" in item.mnemonic]
    if len(decoded) != 99 or calls != [
            (0x14CA, "0xaf8, 0x16a8"),
            (0x14F2, "0xaf8, 0xdc0"),
            (0x1574, "0xac, 0x25")]:
        raise ValueError("u6F insertion decode or call sites changed")
    print("u6F +1446..+157E: 99 instructions, three far calls")
    audit_boundary(args.native_dir)


if __name__ == "__main__":
    main()
