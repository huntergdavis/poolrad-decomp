#!/usr/bin/env python3
"""Pin the loaded U1C E-key decision bytes in retained Up-run RAM only."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
RAM_SHA = (
    "4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc",
    "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3",
    "d5332052a2367728a88215e36ff182a4f4e7a2bbdde22a7ef2c74497e2f0cf7c",
)
ORIGINAL_SHA = "cfade238c89c0ca9be90dbd2581b61cd8c3fcffd651a544c719d4fd947ba71f7"
LOADED_SHA = "75a88ab71babe0b9689c459186b3ff1082d0f3f5cd2e69c47a590bbfa46b5a21"
FIXUP_OFFSETS = (0x33CC, 0x33CD, 0x33E0, 0x33E1)
ORIGINAL_FIXUPS = bytes.fromhex("f80af80a")
LOADED_FIXUPS = bytes.fromhex("1c131c13")


def checked(data, size, digest, label):
    if len(data) != size or sha256(data).hexdigest() != digest:
        raise ValueError(f"{label}: size or SHA-256 differs")


def loaded_span(ram):
    at = (0x3582 << 4) + 0x33BD
    return ram[at:at + 46]


def audit(ovr, rams):
    checked(ovr, 206105, OVR_SHA, "GAME.OVR")
    # U1C file base 0x0D46; this 46-byte span ends before the +33EB successor.
    original = ovr[0x0D46 + 0x33BD:0x0D46 + 0x33EB]
    checked(original, 46, ORIGINAL_SHA, "original U1C +33BD..+33EA")
    for label, ram, digest in zip(("pre-input", "post-Up", "later"), rams, RAM_SHA):
        checked(ram, 655360, digest, label + " RAM")
        loaded = loaded_span(ram)
        checked(loaded, 46, LOADED_SHA, label + " loaded U1C E window")
        differences = tuple(0x33BD + i for i, (a, b) in
                            enumerate(zip(original, loaded)) if a != b)
        if differences != FIXUP_OFFSETS:
            raise ValueError(label + ": relocation positions differ")
        indexes = [offset - 0x33BD for offset in FIXUP_OFFSETS]
        if bytes(original[i] for i in indexes) != ORIGINAL_FIXUPS:
            raise ValueError("original far-call fixup values differ")
        if bytes(loaded[i] for i in indexes) != LOADED_FIXUPS:
            raise ValueError(label + ": loaded far-call fixup values differ")
        # Both E comparisons are in this window, but no E key was entered here.
        if loaded[0x33CE - 0x33BD:0x33D0 - 0x33BD] != bytes.fromhex("3c45"):
            raise ValueError(label + ": first E comparison differs")
        if loaded[0x33E2 - 0x33BD:0x33E4 - 0x33BD] != bytes.fromhex("3c45"):
            raise ValueError(label + ": second E comparison differs")
    return {
        "original_window": "U1C +33BD..+33EA (46 bytes)",
        "original_sha256": ORIGINAL_SHA,
        "loaded_window": "3582:33BD..33EA, same at three retained Up-run stops",
        "loaded_sha256": LOADED_SHA,
        "relocated_byte_offsets": [f"+{offset:04X}" for offset in FIXUP_OFFSETS],
        "captured_ram_bytes_per_stop": 655360,
        "e_key_entered_in_this_capture": False,
        "u7d_03a3_natural_call_observed": False,
        "full_cpu14_and_1mib_same_run_available": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--ovr", type=Path, required=True)
    for name in ("pre-input-ram", "post-up-ram", "later-ram"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.ovr.read_bytes(),
                           [args.pre_input_ram.read_bytes(),
                            args.post_up_ram.read_bytes(),
                            args.later_ram.read_bytes()]), indent=2))


if __name__ == "__main__":
    main()
