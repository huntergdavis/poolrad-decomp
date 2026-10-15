#!/usr/bin/env python3
"""Audit saved slot-A EGA values from second resident flush to prompt drawer."""

import argparse
from pathlib import Path
import re
import subprocess

import slot_a_u3b0_prompt_row_audit as prior


RAM_HASHES = {
    "second-flush-return": "909bc77480f1dfac6f7307819ffb80eca55ecc2be1a2c227c038eb610b48e7d2",
    "first-view-return": "909bc77480f1dfac6f7307819ffb80eca55ecc2be1a2c227c038eb610b48e7d2",
    "drawer-entry": "e592be642b4b2cb1fb2ee8dffa7af6f941861034feeab03debe4f4c803857061",
    "drawer-return": "ecefb60da63cb6281faa4eb4f9e5854fab87aff192cfb8ce9e5e9cfe3aa331c0",
}
FLUSH_SHA = "7b5347e6e6af0127e2d089c495dcf0e67e8882338e2a792e44a318d82b2c228c"
LOADED_FLUSH_SHA = "0c637b6b4a32b6c9ad098ca841c4a22f161eaac51bf45bab99309b55cdec4828"
TRACE_SHA = "ca678e59b58d0dcd85644bd04fa396eb53cbe0fe00a2e82b48d6c63547a65c38"


def page(run: Path, stop: str, expected: str) -> bytes:
    planes = [(run / f"a000-plane{i}-{stop}.bin").read_bytes()
              for i in range(4)]
    if any(len(plane) != 8000 for plane in planes):
        raise ValueError(f"{stop} has an incomplete EGA plane")
    return prior.checked(b"".join(planes), expected, stop + " page")


def audit(args: argparse.Namespace) -> None:
    trace = prior.checked((args.run / "debugger-interactive.typescript").read_bytes(),
                          TRACE_SHA, "ordered native debugger transcript").decode("latin1")
    def register(name: str) -> list[int]:
        return [int(value, 16) for value in re.findall(
            r"EV of '" + name + r"' is:\s*LOG:\s*([0-9a-f]+)", trace, re.I)]
    stops = list(zip(register("CS"), register("IP")))
    if stops[-6:] != [(0x0C88, 0x1B52), (0x0C88, 0x1B52),
                      (0x2D14, 0x05FB), (0x3582, 0x3387),
                      (0x2116, 0x048B), (0x2116, 0x048E)]:
        raise ValueError("native stop order or address changed")
    if register("SP")[-5:] != [0x3EBE, 0x3EC2, 0x3EF6, 0x3E02, 0x3E08]:
        raise ValueError("second-flush or drawer stack signature changed")
    exe = prior.checked(args.exe.read_bytes(), prior.START_SHA, "START.EXE")
    game = prior.checked(args.game.read_bytes(), prior.GAME_SHA, "GAME.OVR")
    prior.checked(args.save.read_bytes(), prior.SAVE_SHA, "slot-A save")
    prior.checked(args.emulator.read_bytes(), prior.EMULATOR_SHA, "heavy DOSBox-X")
    prior.checked(exe[0x61CB:0x64D5], FLUSH_SHA, "original resident flush")
    prior.checked(game[182479 + 0x1DA:182479 + 0x37D],
                  prior.BODY_SHA, "original prompt drawer")
    prior.checked(game[182479 + 0x48B:182479 + 0x48E],
                  prior.CALL_SHA, "original drawer call")

    ram = {stop: prior.checked((args.run / f"ram-{stop}.bin").read_bytes(),
                               expected, stop + " RAM")
           for stop, expected in RAM_HASHES.items()}
    if any(len(image) != 655360 for image in ram.values()):
        raise ValueError("incomplete native RAM capture")
    prior.checked(ram["second-flush-return"][0xC880 + 0x1B52:
                                               0xC880 + 0x1B52 + 778],
                  LOADED_FLUSH_SHA, "loaded resident flush")
    if ram["second-flush-return"] != ram["first-view-return"]:
        raise ValueError("RAM changed across flush-to-view return")

    before = page(args.run, "second-return", prior.BEFORE_PAGE_SHA)
    for stop in ("first-view-return", "drawer-entry"):
        if page(args.run, stop, prior.BEFORE_PAGE_SHA) != before:
            raise ValueError(f"{stop} page changed before the drawer")
    after = page(args.run, "drawer-return", prior.AFTER_PAGE_SHA)
    changes = [i for i, (old, new) in enumerate(zip(before, after))
               if old != new]
    per_plane = tuple(sum(i // 8000 == plane for i in changes)
                      for plane in range(4))
    if len(changes) != 518 or per_plane != (63, 196, 63, 196):
        raise ValueError("same-run prompt-row page delta changed")
    if (min(i % 8000 // 40 for i in changes),
        max(i % 8000 // 40 for i in changes),
        min(i % 40 for i in changes),
        max(i % 40 for i in changes)) != (192, 198, 0, 32):
        raise ValueError("same-run prompt-row geometry changed")

    entry, returned = ram["drawer-entry"], ram["drawer-return"]
    cs, ds, ss, bp = 0x21160, 0x14A00, 0x1D160, 0x3E98
    prior.checked(entry[cs + 0x48B:cs + 0x48E], prior.CALL_SHA,
                  "loaded drawer call")
    prior.checked(entry[cs + 0x1DA:cs + 0x37D], prior.LOADED_BODY_SHA,
                  "loaded drawer")
    expected_state = (15, 4, 6, 0, 5, 10, 20, 0x9AA5, 1)
    if any(prior.state(image) != expected_state for image in ram.values()):
        raise ValueError("party, minute, ECL, or selection state changed")
    ram_changes = [i for i, (old, new) in enumerate(zip(entry, returned))
                   if old != new]
    if len(ram_changes) != 32 or any(not ss <= i < ss + 0x10000
                                     for i in ram_changes):
        raise ValueError("drawer RAM side effects changed")

    selection = entry[ds + 0x6B2B]
    first = entry[ss + bp - 0x8F + 2 * selection]
    last = entry[ss + bp - 0x8E + 2 * selection]
    length = entry[ss + bp - 0x64]
    column = entry[ss + bp - 0x65]
    color_selected = entry[ss + bp + 0xE]
    color_ordinary = entry[ss + bp + 0x10]
    if (selection, first, last, length, column,
        color_selected, color_ordinary) != (1, 1, 4, 33, 0, 15, 10):
        raise ValueError("live prompt drawer frame changed")
    pascal = prior.checked(entry[ss + bp - 0x53:
                                 ss + bp - 0x53 + length + 1],
                           prior.TEXT_SHA, "private copied input")
    classifier = prior.checked(entry[cs + 0x1BA:cs + 0x1DA],
                               prior.SET_SHA, "private classifier set")
    font_off = prior.word(entry, ds + 0x5E20)
    font_seg = prior.word(entry, ds + 0x5E22)
    font = prior.checked(entry[16 * font_seg + font_off:
                               16 * font_seg + font_off + 1416],
                         prior.FONT_SHA, "private font")
    private_input = (font + classifier +
                     bytes((column, length, first, last,
                            color_selected, color_ordinary)) +
                     pascal[1:].ljust(255, b"\0") + before)
    input_path = args.run / "postflush-prompt-probe-input-private.bin"
    output_path = args.run / "postflush-prompt-probe-output-private.bin"
    input_path.write_bytes(private_input)
    result = subprocess.run((str(args.probe), str(input_path), str(output_path)),
                            check=True, capture_output=True, text=True)
    output = prior.checked(output_path.read_bytes(), prior.AFTER_PAGE_SHA,
                           "bounded C drawer page")
    if output != after or result.stdout.strip() != "cells=39 stores=1248 changed=518":
        raise ValueError("bounded C/native EGA page mismatch")
    print("same-run second flush -> view return -> drawer entry: 32000/32000 EGA bytes equal")
    print("drawer +01DA return: C/native 32000/32000 bytes equal; changed=518")
    print("changed planes=63,196,63,196; scanlines=192..198; columns=0..32")
    print("RAM: flush/view equal; drawer changed 32 stack bytes; party/minute/ECL unchanged")
    print("pre-page SHA-256", prior.BEFORE_PAGE_SHA)
    print("post-page SHA-256", prior.AFTER_PAGE_SHA)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ("exe", "game", "save", "emulator", "run", "probe"):
        parser.add_argument("--" + name, type=Path, required=True)
    audit(parser.parse_args())


if __name__ == "__main__":
    main()
