#!/usr/bin/env python3
"""Audit one saved-slot run from call 42 through the prompt-row page effect."""

import argparse
from pathlib import Path
import struct
import subprocess

import slot_a_u3b0_prompt_row_audit as prior


RAM_HASHES = {
    "call024-return": "b4307486e3cdcc47f1a4987dcfbe256126615a44ea6effa471e9a6f5332752b7",
    "call042-return": "90263e56f3b3856e3764e581cf662d65de8909652525de5443bde04c8c5d6dc2",
    "first-view-return": "64e776c9de01fc3635fe2d864c8ea10fcc0696c67b158e03e5accda8915d2d44",
    "drawer-entry": "68524f64e2e8d9726d0129c2b6e8d3ab3c5a640b7503b598210f2fd748df3062",
    "drawer-return": "34721afe87430b2fc3bfc3fb6de597b0f5dbd621fbac858b808d8be190f0aafe",
}


def audit(args: argparse.Namespace) -> None:
    prior.checked(args.exe.read_bytes(), prior.START_SHA, "START.EXE")
    game = prior.checked(args.game.read_bytes(), prior.GAME_SHA, "GAME.OVR")
    prior.checked(args.save.read_bytes(), prior.SAVE_SHA, "slot-A save")
    prior.checked(args.emulator.read_bytes(), prior.EMULATOR_SHA,
                  "heavy DOSBox-X")
    base = 182479
    call = prior.checked(game[base + 0x48B:base + 0x48E], prior.CALL_SHA,
                         "original prompt-row near call")
    if 0x48E + struct.unpack("<h", call[1:])[0] != 0x1DA:
        raise ValueError("prompt-row near-call target changed")
    prior.checked(game[base + 0x1DA:base + 0x37D], prior.BODY_SHA,
                  "original bounded prompt-row helper")

    ram = {name: prior.checked((args.run / f"{name}-ram-private.bin").read_bytes(),
                               expected, name + " RAM")
           for name, expected in RAM_HASHES.items()}
    if any(len(image) != 655360 for image in ram.values()):
        raise ValueError("native RAM size changed")
    before = prior.page(args.run, "call024-return", prior.BEFORE_PAGE_SHA)
    for name in ("call042-return", "first-view-return", "drawer-entry"):
        if prior.page(args.run, name, prior.BEFORE_PAGE_SHA) != before:
            raise ValueError(f"{name} page differs before prompt drawer")
    after = prior.page(args.run, "drawer-return", prior.AFTER_PAGE_SHA)
    changes = [i for i, (a, b) in enumerate(zip(before, after)) if a != b]
    per_plane = tuple(sum(i // 8000 == p for i in changes) for p in range(4))
    if len(changes) != 518 or per_plane != (63, 196, 63, 196):
        raise ValueError("same-run prompt-row page delta changed")
    if (min((i % 8000) // 40 for i in changes),
        max((i % 8000) // 40 for i in changes),
        min((i % 8000) % 40 for i in changes),
        max((i % 8000) % 40 for i in changes)) != (192, 198, 0, 32):
        raise ValueError("same-run prompt-row change geometry changed")

    entry, returned = ram["drawer-entry"], ram["drawer-return"]
    cs, ds, ss, bp = 0x21160, 0x14A00, 0x1D160, 0x3E98
    prior.checked(entry[cs + 0x48B:cs + 0x48E], prior.CALL_SHA,
                  "loaded prompt-row near call")
    prior.checked(entry[cs + 0x1DA:cs + 0x37D], prior.LOADED_BODY_SHA,
                  "loaded bounded prompt-row helper")
    expected_state = (15, 4, 6, 0, 5, 10, 20, 0x9AA5, 1)
    if prior.state(entry) != expected_state or prior.state(returned) != expected_state:
        raise ValueError("party, clock, script, or selection changed in drawer")
    ram_changes = [i for i, (a, b) in enumerate(zip(entry, returned)) if a != b]
    if len(ram_changes) != 32 or any(not ss <= i < ss + 0x10000
                                     for i in ram_changes):
        raise ValueError("bounded drawer RAM side effects changed")

    selected = entry[ds + 0x6B2B]
    first = entry[ss + bp - 0x8F + 2 * selected]
    last = entry[ss + bp - 0x8E + 2 * selected]
    length = entry[ss + bp - 0x64]
    column = entry[ss + bp - 0x65]
    pascal = prior.checked(entry[ss + bp - 0x53:ss + bp - 0x53 + length + 1],
                           prior.TEXT_SHA, "copied Pascal input")
    set_bits = prior.checked(entry[cs + 0x1BA:cs + 0x1DA], prior.SET_SHA,
                             "loaded classifier set")
    font_off = prior.word(entry, ds + 0x5E20)
    font_seg = prior.word(entry, ds + 0x5E22)
    font = prior.checked(entry[16 * font_seg + font_off:
                               16 * font_seg + font_off + 1416],
                         prior.FONT_SHA, "live font")
    colors = (entry[ss + bp + 0xE], entry[ss + bp + 0x10])
    if (length, column, selected, first, last, colors, pascal[0]) != (
            33, 0, 1, 1, 4, (15, 10), 33):
        raise ValueError("same-run prompt-row frame changed")

    private_input = (font + set_bits +
                     bytes((column, length, first, last, *colors)) +
                     pascal[1:].ljust(255, b"\0") +
                     prior.page(args.run, "drawer-entry", prior.BEFORE_PAGE_SHA))
    input_path = args.run / "post42-probe-input-private.bin"
    output_path = args.run / "post42-c-page-private.bin"
    input_path.write_bytes(private_input)
    result = subprocess.run((str(args.probe), str(input_path), str(output_path)),
                            check=True, capture_output=True, text=True)
    output = prior.checked(output_path.read_bytes(), prior.AFTER_PAGE_SHA,
                           "same-run C page")
    if output != after or result.stdout.strip() != "cells=39 stores=1248 changed=518":
        raise ValueError("same-run C/native prompt-row page mismatch")
    print("same-run stops: call24, call42, first-view, drawer entry/return")
    print("pre-drawer pages: 4/4 equal, each 32000 bytes")
    print("drawer C/native page: 32000/32000 bytes; changed=518; planes=63,196,63,196")
    print("drawer RAM: 32 changed stack bytes; party/time/script/selection unchanged")
    print("prompt-row page SHA-256", prior.digest(output))
    print("CPU execution and ECL metadata before the drawer remain separate boundaries")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "game", "save", "emulator", "run", "probe"):
        parser.add_argument("--" + name, type=Path, required=True)
    audit(parser.parse_args())


if __name__ == "__main__":
    main()
