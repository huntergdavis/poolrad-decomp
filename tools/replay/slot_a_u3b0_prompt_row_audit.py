#!/usr/bin/env python3
"""Private, hash-locked slot-A prompt-row page audit; emits counts and hashes only."""

import argparse
import hashlib
from pathlib import Path
import struct
import subprocess

START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
GAME_SHA = "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614"
SAVE_SHA = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
EMULATOR_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
CALL_SHA = "a47ae6e9d1136bdf24bdde8a222cffe3c2c5df0d98fec86529b0650bd2bdf4f3"
BODY_SHA = "8676c97ae6dcaa88c9f7d68d75c75f81e4936572b517bd84a7e19876c58d96ce"
LOADED_BODY_SHA = "a6a772f3cf170d69c3e73cac96056ab2689f25811ae148c5e4026c78ac0f02b8"
ENTRY_RAM_SHA = "31aaf5f659370812d52f25aa37f0dd1a97c19e5ab645417c69a15b8be602d26d"
RETURN_RAM_SHA = "456a2246da93bfa6d0b5bd32b161bec8eea3d0340d6497d5b05b8a112db1f1d6"
WAIT_RAM_SHA = "b86fa79139788784b40366fad483fd7b04dee7b4aef8bdf3e78e2fc92498b1b6"
BEFORE_PAGE_SHA = "3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738"
AFTER_PAGE_SHA = "09926b914ef24ca2c77eca35c49fd526b54fc27cafbc2d985eefecbcdc61500a"
FONT_SHA = "fca3a918bd20e24456f579d20341278a21d3b5f5b4e205384fadeb551ce091c1"
SET_SHA = "2d66b9ae33311a4e813f8a5263360a0aaeb0e64d86f53bdeeb2f0849d5e2cbe0"
TEXT_SHA = "1aade9e35370a1f2ee8d8ab9f41906f03d4732644424c627089ba51397db19b4"


def digest(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def checked(data: bytes, expected: str, label: str) -> bytes:
    if digest(data) != expected:
        raise ValueError(f"{label} SHA-256 mismatch")
    return data


def page(directory: Path, name: str, expected: str) -> bytes:
    planes = [(directory / f"{name}-plane{i}-private.bin").read_bytes()
              for i in range(4)]
    if any(len(plane) != 8000 for plane in planes):
        raise ValueError(f"{name} plane size changed")
    return checked(b"".join(planes), expected, name + " page")


def word(data: bytes, at: int) -> int:
    return struct.unpack_from("<H", data, at)[0]


def state(ram: bytes) -> tuple[int, ...]:
    ds = 0x14A00
    window = 16 * word(ram, ds + 0x49D4) + word(ram, ds + 0x49D2)
    return (*ram[ds + 0x6AAD:ds + 0x6AB0],
            *(word(ram, window + offset) for offset in (0x18E, 0x190, 0x192)),
            ram[ds + 0x84DC], word(ram, ds + 0x49ED), ram[ds + 0x6B2B])


def audit(args: argparse.Namespace) -> None:
    checked(args.exe.read_bytes(), START_SHA, "START.EXE")
    game = checked(args.game.read_bytes(), GAME_SHA, "GAME.OVR")
    checked(args.save.read_bytes(), SAVE_SHA, "slot-A save")
    checked(args.emulator.read_bytes(), EMULATOR_SHA, "heavy DOSBox-X")
    base = 182479
    call = checked(game[base + 0x48B:base + 0x48E], CALL_SHA,
                   "original near call")
    if 0x48E + struct.unpack("<h", call[1:])[0] != 0x1DA:
        raise ValueError("near call no longer targets +01DA")
    body = checked(game[base + 0x1DA:base + 0x37D], BODY_SHA,
                   "original bounded helper")
    if len(body) != 419:
        raise ValueError("bounded helper return changed")
    writer, wait = args.writer_run, args.wait_run
    entry_ram = checked((writer / "drawer-entry-ram-private.bin").read_bytes(),
                        ENTRY_RAM_SHA, "drawer entry RAM")
    return_ram = checked((writer / "drawer-return-ram-private.bin").read_bytes(),
                         RETURN_RAM_SHA, "drawer return RAM")
    wait_ram = checked((wait / "wait-poll-ram-private.bin").read_bytes(),
                       WAIT_RAM_SHA, "keyboard wait RAM")
    repeat_ram = checked((wait / "wait-repeat-ram-private.bin").read_bytes(),
                         WAIT_RAM_SHA, "repeated keyboard wait RAM")
    if any(len(ram) != 655360 for ram in (entry_ram, return_ram, wait_ram,
                                          repeat_ram)):
        raise ValueError("native RAM size changed")
    cs = 0x21160
    checked(entry_ram[cs + 0x48B:cs + 0x48E], CALL_SHA, "loaded near call")
    checked(entry_ram[cs + 0x1DA:cs + 0x37D], LOADED_BODY_SHA,
            "loaded helper body")
    before = page(writer, "drawer-entry", BEFORE_PAGE_SHA)
    after = page(writer, "drawer-return", AFTER_PAGE_SHA)
    at_wait = page(wait, "wait-poll", AFTER_PAGE_SHA)
    repeated = page(wait, "wait-repeat", AFTER_PAGE_SHA)
    if after != at_wait or after != repeated:
        raise ValueError("post-drawer and no-key wait pages differ")
    changes = [i for i, (a, b) in enumerate(zip(before, after)) if a != b]
    per_plane = tuple(sum(i // 8000 == p for i in changes) for p in range(4))
    if len(changes) != 518 or per_plane != (63, 196, 63, 196):
        raise ValueError("native page delta changed")
    ram_changes = [i for i, (a, b) in enumerate(zip(entry_ram, return_ram))
                   if a != b]
    if len(ram_changes) != 44 or [i for i in ram_changes
                                  if not 0x1D160 <= i < 0x2D160] != [0x0046C]:
        raise ValueError("native non-page RAM effects changed")
    expected_state = (15, 4, 6, 0, 5, 10, 20, 0x9AA5, 1)
    if any(state(ram) != expected_state for ram in (entry_ram, return_ram,
                                                     wait_ram, repeat_ram)):
        raise ValueError("party, clock, script, or selection boundary changed")
    ds, ss, bp = 0x14A00, 0x1D160, 0x3E98
    selected = entry_ram[ds + 0x6B2B]
    first = entry_ram[ss + bp - 0x8F + 2 * selected]
    last = entry_ram[ss + bp - 0x8E + 2 * selected]
    length = entry_ram[ss + bp - 0x64]
    column = entry_ram[ss + bp - 0x65]
    pascal = checked(entry_ram[ss + bp - 0x53:ss + bp - 0x53 + length + 1],
                     TEXT_SHA, "copied Pascal input")
    set_bits = checked(entry_ram[cs + 0x1BA:cs + 0x1DA], SET_SHA,
                       "loaded classifier set")
    font_off = word(entry_ram, ds + 0x5E20)
    font_seg = word(entry_ram, ds + 0x5E22)
    font = checked(entry_ram[16 * font_seg + font_off:
                             16 * font_seg + font_off + 1416], FONT_SHA,
                   "live font")
    colors = (entry_ram[ss + bp + 0xE], entry_ram[ss + bp + 0x10])
    if (length, column, selected, first, last, colors, pascal[0]) != (
            33, 0, 1, 1, 4, (15, 10), 33):
        raise ValueError("live drawer frame changed")
    text = pascal[1:]
    selected_count = sum(first <= i <= last for i in range(1, length + 1))
    set_count = sum(not first <= i <= last and
                    bool(set_bits[ch >> 3] & (1 << (ch & 7)))
                    for i, ch in enumerate(text, 1))
    if (selected_count, set_count, length - selected_count - set_count) != (
            4, 5, 24):
        raise ValueError("source branch inputs changed")
    private_input = (font + set_bits +
                     bytes((column, length, first, last, *colors)) +
                     text.ljust(255, b"\0") + before)
    input_path = writer / "prompt-row-probe-input-private.bin"
    output_path = writer / "prompt-row-c-page-private.bin"
    input_path.write_bytes(private_input)
    result = subprocess.run((str(args.probe), str(input_path), str(output_path)),
                            check=True, capture_output=True, text=True)
    output = checked(output_path.read_bytes(), AFTER_PAGE_SHA, "C page")
    if output != after or output != at_wait or output != repeated:
        raise ValueError("C/native page mismatch")
    if result.stdout.strip() != "cells=39 stores=1248 changed=518":
        raise ValueError("C report changed")
    print("unit 0x3B0:+01DA..+037C: 419 bytes, 172 instructions, page-only model")
    print("branch inputs: selected=4 set=5 ordinary=24 trailing=6")
    print("C/native drawer return and repeated no-key wait: 32000/32000 page bytes")
    print("page changed=518 per-plane=63,196,63,196; RAM=43 stack + 1 BIOS tick")
    print("native/C page SHA-256", digest(output))


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "game", "save", "emulator", "writer_run", "wait_run",
                 "probe"):
        parser.add_argument("--" + name.replace("_", "-"), type=Path, required=True)
    audit(parser.parse_args())


if __name__ == "__main__":
    main()
