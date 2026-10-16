#!/usr/bin/env python3
"""Audit retained Up-run U9E stub state, without claiming a U7F call."""

import argparse
from hashlib import sha256
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "tools" / "overlay"))
from overlay_inventory import find_units, parse_mz  # noqa: E402
from resident_loader_map import unpack  # noqa: E402

EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
RAM_SHA = (
    "4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc",
    "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3",
    "d5332052a2367728a88215e36ff182a4f4e7a2bbdde22a7ef2c74497e2f0cf7c",
)
STUB_SHA = "16e87d451726931b6e084c2a73e82f385b41e3886c5d7a9806ec0fd814f991f2"
HANDLER_SHA = "6269d22ce3aa55966403db7c09fcad2c9b2a717b8d0c53986a3e9ad52121264c"
ENTRY_SHA = "7df225682ebb94351984048ef7a4fdccc8de60f9ef2450751a696fde74c583ba"
U68_COLD_SHA = "2fcb835df8ea8b601f72d1110db582051d9a21fccbd9b3d33300e9f57732eb57"
U68_WARM_SHA = "8e64482ee3d0380a79e4d507887df358333da6f04f51c4b19007e857594aae96"


def checked(data, size, digest, label):
    if len(data) != size or sha256(data).hexdigest() != digest:
        raise ValueError(f"{label} size or SHA-256 differs")


def sample(ram, segment, offset, size):
    at = ((segment << 4) + offset) & 0xFFFFF
    if at + size > len(ram):
        raise ValueError("requested address outside 640 KiB capture")
    return ram[at:at + size]


def audit(exe, ovr, pre, post_up, later):
    checked(exe, 65231, EXE_SHA, "START.EXE")
    checked(ovr, 206105, OVR_SHA, "GAME.OVR")
    for label, ram, digest in zip(("pre", "post-Up", "later"),
                                  (pre, post_up, later), RAM_SHA):
        checked(ram, 655360, digest, label + " RAM")

    header = parse_mz(exe)["header_bytes"]
    units = {u["stub_para"]: u for u in find_units(exe[header:], ovr)}
    u9e = units[0x9E]
    if (u9e["memory_segment"] != 0xB0 or
            u9e["entries"][8:10] != [0x0DE3, 0x0E2E]):
        raise ValueError("U9E descriptor boundary differs")
    entry = ovr[u9e["ovr_offset"] + 0x0DE3:u9e["ovr_offset"] + 0x0E2E]
    checked(entry, 75, ENTRY_SHA, "U9E entry8 original body")
    image, _, _, _ = unpack(exe)
    checked(bytes(image[0xB48:0xB4D]), 5, STUB_SHA,
            "pre-relocation expanded U9E stub")
    checked(bytes(image[0xAD06:0xAD58]), 82, HANDLER_SHA,
            "original INT 3Fh handler")

    for label, ram in (("pre", pre), ("post-Up", post_up), ("later", later)):
        checked(sample(ram, 0x08D4, 0x0048, 5), 5, STUB_SHA,
                label + " relocated U9E stub")
        if sample(ram, 0x00B0, 0x0048, 5) != b"\x00" * 5:
            raise ValueError(label + " unrelocated B0 bytes differ")
        if ram[0xFC:0x100] != bytes.fromhex("e602c612"):
            raise ValueError(label + " INT 3Fh vector differs")

    checked(sample(pre, 0x0896, 0x002A, 5), 5, U68_COLD_SHA,
            "pre-input U68 cold stub")
    for label, ram in (("post-Up", post_up), ("later", later)):
        checked(sample(ram, 0x0896, 0x002A, 5), 5, U68_WARM_SHA,
                label + " U68 patched stub")
    return {
        "private_ram_sha256": RAM_SHA,
        "original_u9e_entry_sha256": ENTRY_SHA,
        "original_handler_sha256": HANDLER_SHA,
        "source_stub_image": 0xB48,
        "observed_relocated_stub_segment": 0x08D4,
        "observed_relocated_stub_offset": 0x0048,
        "observed_relocated_stub_sha256": STUB_SHA,
        "observed_ivt3f": "12C6:02E6",
        "u68_positive_control": "cold at pre-input; patched after Up",
        "captured_ram_bytes_per_stop": 655360,
        "u7f_caller_reached": False,
        "u9e_loaded_body_cs_observed": False,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "ovr", "pre-input-ram", "post-up-ram", "later-ram"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(audit(args.exe.read_bytes(), args.ovr.read_bytes(),
                           args.pre_input_ram.read_bytes(),
                           args.post_up_ram.read_bytes(),
                           args.later_ram.read_bytes()), indent=2))


if __name__ == "__main__":
    main()
