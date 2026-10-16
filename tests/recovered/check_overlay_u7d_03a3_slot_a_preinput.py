#!/usr/bin/env python3
"""Check the saved slot-A pre-input RAM; never infer an E-key route."""

import argparse
from hashlib import sha256
import json
from pathlib import Path

EXE_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
OVR_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
PRE_SHA = "4c145e29c105ceb2207b6015965af630ebeae91505f60b7606155e2939b92cdc"
POST_UP_SHA = "38bd3cc701f326048175fa8378cc77d4c8459bbe6e4affd6270a3e63ab86c0d3"
ORIGINAL_CALL_SHA = "b588dcf500a335a6dc95ae0ca430a5ec17d6f1af71a4c94ca6ccf97f6915ae4f"
LOADED_CALL_SHA = "b280d21502c60f0f32bbfeea9e8ff2d36c2b21a1c7465b1052adee93e2ed7a2d"
LOADED_STUB_SHA = "2fcb835df8ea8b601f72d1110db582051d9a21fccbd9b3d33300e9f57732eb57"


def checked(data, size, digest, label):
    if len(data) != size or sha256(data).hexdigest() != digest:
        raise ValueError(f"{label} size or SHA-256 differs")


def physical(segment, offset):
    return ((segment << 4) + offset) & 0xFFFFF


def span(ram, segment, offset, size):
    at = physical(segment, offset)
    if at + size > len(ram):
        raise ValueError("captured RAM does not include requested address")
    return ram[at:at + size]


def word(ram, segment, offset):
    return int.from_bytes(span(ram, segment, offset, 2), "little")


def observed(ram):
    pointer = (word(ram, 0x14A0, 0x49D8), word(ram, 0x14A0, 0x49D6))
    if pointer != (0x3A16, 0x0000):
        raise ValueError("window-1 far pointer differs")
    window_word = word(ram, pointer[0], (pointer[1] + 0x0594) & 0xFFFF)
    stop = span(ram, 0x14A0, 0x49FF, 1)[0]
    if window_word != 0 or stop != 0:
        raise ValueError("documented pre/post-Up guard differs")
    call = span(ram, 0x3582, 0x3395, 5)
    checked(call, 5, LOADED_CALL_SHA, "loaded U1C +3395 call")
    if call != bytes.fromhex("9a2a009608"):
        raise ValueError("loaded input call does not target 0896:002A")
    return {"ds_stop_49ff": stop, "window1_segment": pointer[0],
            "window1_offset": pointer[1], "window1_word_0594": window_word,
            "loaded_call_sha256": LOADED_CALL_SHA}


def audit(exe, ovr, save, pre, post_up):
    checked(exe, 65231, EXE_SHA, "START.EXE")
    checked(ovr, 206105, OVR_SHA, "GAME.OVR")
    checked(save, 13137, SAVE_SHA, "slot-A save")
    checked(pre, 655360, PRE_SHA, "pre-input +3395 RAM")
    checked(post_up, 655360, POST_UP_SHA, "post-Up +33BD RAM")
    original = ovr[0x40DB:0x40E0]
    checked(original, 5, ORIGINAL_CALL_SHA, "original U1C +3395 call")
    if original != bytes.fromhex("9a2a007200"):
        raise ValueError("original input call does not target 0072:002A")
    stub = span(pre, 0x0896, 0x002A, 5)
    checked(stub, 5, LOADED_STUB_SHA, "loaded U68 entry-2 stub")
    if stub != bytes.fromhex("cd3f890900"):
        raise ValueError("loaded U68 stub is not the observed INT 3Fh form")
    return {"save_sha256": SAVE_SHA, "pre_input_ram_sha256": PRE_SHA,
            "post_up_ram_sha256": POST_UP_SHA,
            "original_call_sha256": ORIGINAL_CALL_SHA,
            "loaded_stub_sha256": LOADED_STUB_SHA,
            "pre_input": observed(pre), "post_up": observed(post_up),
            "captured_ram_bytes_per_stop": len(pre),
            "full_cpu14_captured_here": False, "e_route_executed": False}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "ovr", "save", "pre-input-ram", "post-up-ram"):
        parser.add_argument("--" + name, required=True, type=Path)
    args = parser.parse_args()
    print(json.dumps(audit(args.exe.read_bytes(), args.ovr.read_bytes(),
                           args.save.read_bytes(), args.pre_input_ram.read_bytes(),
                           args.post_up_ram.read_bytes()), indent=2))


if __name__ == "__main__":
    main()
