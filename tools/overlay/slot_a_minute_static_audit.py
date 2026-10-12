#!/usr/bin/env python3
"""Address-only static boundary audit for the saved slot-A minute observation."""

import argparse
from collections import deque
from hashlib import sha256
import json
from pathlib import Path

from ecl_walk import OPERANDS, decode_one, operand_len
from overlay_inventory import find_units, parse_mz
from resident_loader_map import unpack


EXPECTED = {
    "START.EXE": "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd",
    "GAME.OVR": "872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614",
    "SAVGAMA.DAT": "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1",
}
SCRIPT_SHA = "bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8"
BASE = 0x9900
MINUTE_ADDRESSES = {0x49C7, 0x49C8, 0x49C9}
EXPECTED_SEARCH = dict(decoded_nodes=697, save_opcodes=104, ecl_clock_opcodes=0,
                       indirect_fixed_operand_tags=0, minute_operands=0,
                       minute_save_destinations=0)
EXPECTED_SPANS = {
    "picture_handler": (251, "51628c4f1c9879395122bd027bd60463b7ad58181b768d48be6f1b697ee17663"),
    "movement_entry1": (826, "a28055269b7373f984337022d88bfdea89993f8eb4a44ee69d6d8c23b2240c55"),
    "minute_static_writer": (165, "7bd8c6d35073b0d7101c4a3fbe2154e916a1f72424636636fcf0b48718f148a4"),
    "next_c_candidate": (67, "f757b69105918b46cfc641ccdfafd78a00f0680dd9e5280898517ea8ac86fd0d"),
}
DISPLAY_SHA = "197d6b93ad7761754f5cd680c5c16c12dccff53bcc6137764c09f16506172c78"


def verify_input(path, name):
    data = path.read_bytes()
    if sha256(data).hexdigest() != EXPECTED[name]:
        raise ValueError(f"{name}: SHA-256 differs from the audited DOS input")
    return data


def walk_search(script):
    """Overapproximate IF and counted-branch targets; never execute game effects."""
    pending = deque([0x9975])
    seen = {}
    while pending:
        pc = pending.popleft()
        if pc in seen:
            continue
        offset = pc - BASE
        if not 0 <= offset < len(script):
            raise ValueError(f"saved search target outside script at {pc:#06x}")
        end, operands, items = decode_one(script, offset)
        if end is None:
            raise ValueError(f"saved search decode stops at {pc:#06x}: {operands}")
        opcode = script[offset]
        next_pc = end + BASE
        seen[pc] = (opcode, operands)
        if opcode in (0x00, 0x13, 0x20, 0x33):
            continue
        if opcode in (0x25, 0x26):
            pending.extend(value for kind, value in items if kind == "word")
        if opcode in (0x01, 0x02) and operands and operands[0][0] == "word":
            pending.append(operands[0][1])
            if opcode == 0x01:
                continue
        if 0x16 <= opcode <= 0x1B:
            after_skip, _, _ = decode_one(script, end)
            if after_skip is None:
                raise ValueError(f"cannot size IF successor at {next_pc:#06x}")
            pending.append(after_skip + BASE)
        pending.append(next_pc)

    saves = 0
    clock = 0
    indirect_tags = 0
    minute_operands = 0
    minute_save_destinations = 0
    for pc, (opcode, operands) in seen.items():
        saves += opcode == 0x09
        clock += opcode == 0x34
        minute_operands += sum(kind == "word" and value in MINUTE_ADDRESSES
                               for kind, value in operands)
        if opcode == 0x09 and len(operands) > 1:
            minute_save_destinations += (operands[1][0] == "word" and
                                         operands[1][1] in MINUTE_ADDRESSES)
        pos = pc - BASE + 1
        for _ in range(OPERANDS[opcode]):
            indirect_tags += script[pos] == 0x03
            length, _, _ = operand_len(script, pos)
            if length is None:
                raise ValueError(f"bad operand at {pc:#06x}")
            pos += length
    return dict(decoded_nodes=len(seen), save_opcodes=saves, ecl_clock_opcodes=clock,
                indirect_fixed_operand_tags=indirect_tags, minute_operands=minute_operands,
                minute_save_destinations=minute_save_destinations)


def audit(directory):
    exe = verify_input(directory / "START.EXE", "START.EXE")
    ovr = verify_input(directory / "GAME.OVR", "GAME.OVR")
    save = verify_input(directory / "SAVGAMA.DAT", "SAVGAMA.DAT")
    saved_words = {
        f"0x{address:04X}": int.from_bytes(
            save[1 + 2 * (address - 0x4900):3 + 2 * (address - 0x4900)], "little"
        )
        for address in sorted(MINUTE_ADDRESSES)
    }
    if saved_words != {"0x49C7": 0, "0x49C8": 5, "0x49C9": 10}:
        raise ValueError(f"saved minute words differ: {saved_words}")
    script = save[0x1401:0x3201]
    if sha256(script).hexdigest() != SCRIPT_SHA:
        raise ValueError("saved ECL region hash differs")
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}
    spans = {}
    for label, unit, start, end in (
        ("picture_handler", 0x1C, 0x0856, 0x0951),
        ("movement_entry1", 0x68, 0x0D6D, 0x10A7),
        ("minute_static_writer", 0x7D, 0x03D5, 0x047A),
        ("next_c_candidate", 0x68, 0x05AB, 0x05EE),
    ):
        u = units[unit]
        body = ovr[u["ovr_offset"] + start:u["ovr_offset"] + end]
        spans[label] = dict(unit=f"0x{unit:03X}", start=f"0x{start:04X}",
                            end_exclusive=f"0x{end:04X}", bytes=len(body),
                            file_offset=f"0x{u['ovr_offset'] + start:05X}",
                            sha256=sha256(body).hexdigest())
    image, _, _, _ = unpack(exe)
    display = bytes(image[0x034AB:0x036FE])
    search = walk_search(script)
    if search != EXPECTED_SEARCH:
        raise ValueError(f"saved search audit changed: {search}")
    for label, (size, digest) in EXPECTED_SPANS.items():
        if (spans[label]["bytes"], spans[label]["sha256"]) != (size, digest):
            raise ValueError(f"{label}: audited code span differs")
    if len(display) != 595 or sha256(display).hexdigest() != DISPLAY_SHA:
        raise ValueError("resident display: audited code span differs")
    return dict(inputs={name: digest for name, digest in EXPECTED.items()},
                script_sha256=SCRIPT_SHA, saved_words=saved_words, search=search, spans=spans,
                resident_display=dict(image_start="0x034AB", bytes=len(display),
                                      sha256=DISPLAY_SHA))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("directory", type=Path, help="ignored DOS directory containing the three inputs")
    print(json.dumps(audit(parser.parse_args().directory), indent=2))


if __name__ == "__main__":
    main()
