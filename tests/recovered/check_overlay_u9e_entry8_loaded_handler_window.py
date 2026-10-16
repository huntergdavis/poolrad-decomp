#!/usr/bin/env python3
"""Pin original and loaded INT 3Fh handler bytes at retained Up-run stops."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "overlay"))
from resident_loader_map import unpack  # noqa: E402

EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
RAM_SHA = (
    "4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc",
    "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3",
    "d5332052a2367728a88215e36ff182a4f4e7a2bbdde22a7ef2c74497e2f0cf7c",
)
ORIGINAL_SHA = "6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c"
LOADED_SHA = "5c69a0e6a0a9141fae2d0381a4ebe4252981d54cbad01d335a3f5f73380fd6ef"
U9E_COLD_STUB_SHA = "16e87d451726931b6e084c2a73e82f385b41e3886c5d7a9806ec0fd814f991f2"


def checked(data, size, digest, label):
    if len(data) != size or sha256(data).hexdigest() != digest:
        raise ValueError(f"{label}: size or SHA-256 differs")


def sample(ram, segment, offset, size):
    start = ((segment << 4) + offset) & 0xFFFFF
    if start + size > len(ram):
        raise ValueError("sample exceeds retained 640 KiB")
    return ram[start:start + size]


def audit(exe, rams):
    checked(exe, 65231, EXE_SHA, "START.EXE")
    image, _, _, _ = unpack(exe)
    original = bytes(image[0xAD06:0xAD58])
    checked(original, 82, ORIGINAL_SHA, "original 0AA2:02E6..0337")
    for label, ram, digest in zip(("pre-input", "post-Up", "later"), rams, RAM_SHA):
        checked(ram, 655360, digest, label + " RAM")
        loaded = sample(ram, 0x12C6, 0x02E6, 82)
        checked(loaded, 82, LOADED_SHA, label + " loaded INT 3Fh handler")
        differences = [0x02E6 + i for i, (a, b) in
                       enumerate(zip(original, loaded)) if a != b]
        if differences != [0x02F2, 0x02F3]:
            raise ValueError(label + ": loaded handler relocation differs")
        if original[12:14] != bytes.fromhex("7c0c"):
            raise ValueError("original handler DS immediate differs")
        if loaded[12:14] != bytes.fromhex("a014"):
            raise ValueError(label + ": loaded handler DS immediate differs")
        if ram[0xFC:0x100] != bytes.fromhex("e602c612"):
            raise ValueError(label + ": INT 3Fh vector differs")
        checked(sample(ram, 0x08D4, 0x0048, 5), 5, U9E_COLD_STUB_SHA,
                label + " U9E entry8 cold stub")
    return {
        "original_handler": "expanded START.EXE 0AA2:02E6..0337 (82 bytes)",
        "original_sha256": ORIGINAL_SHA,
        "loaded_handler": "12C6:02E6..0337, same at three retained Up-run stops",
        "loaded_sha256": LOADED_SHA,
        "loaded_ds_immediate": "14A0 at +02F2/+02F3; original 0C7C",
        "ivt_3f": "12C6:02E6",
        "u9e_stub": "08D4:0048 cold at all three stops",
        "captured_ram_bytes_per_stop": 655360,
        "u7f_0fde_call_observed": False,
        "u9e_loaded_body_cs_observed": False,
        "full_cpu14_and_1mib_same_run_available": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    for name in ("pre-input-ram", "post-up-ram", "later-ram"):
        parser.add_argument("--" + name, type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(audit(args.exe.read_bytes(),
                           [args.pre_input_ram.read_bytes(),
                            args.post_up_ram.read_bytes(),
                            args.later_ram.read_bytes()]), indent=2))


if __name__ == "__main__":
    main()
