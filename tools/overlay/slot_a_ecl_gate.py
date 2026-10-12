#!/usr/bin/env python3
"""Hash-locked, address-only audit of the saved slot-A run-entry gate.

The ignored SAVGAMA.DAT is read, never modified. This audit deliberately stops
at the next ECL handler boundary; it is not a DOS execution trace.
"""

import argparse
import hashlib
import json
from pathlib import Path
import struct

from ecl_walk import decode_one


SAVE_SIZE = 0x3351
SAVE_SHA256 = "abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1"
SCRIPT_SHA256 = "bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8"
SCRIPT_START = 0x1401
SCRIPT_END = 0x3201
BASE = 0x9900

# (opcode, next PC, operands). These are decoded values, not saved game bytes.
RUN_LAYOUT = {
    0x9914: (0x09, 0x991A, (("byte", 0x0B), ("word", 0x6E7D))),
    0x991A: (0x03, 0x9920, (("word", 0x49C9), ("byte", 0x0E))),
    0x9920: (0x1B, 0x9921, ()),
    0x9921: (0x09, 0x9927, (("byte", 0x08), ("word", 0x6E7D))),
    0x9927: (0x09, 0x992E, (("word", 0x6E7D), ("word", 0x49FD))),
    0x992E: (0x09, 0x9934, (("byte", 0x09), ("word", 0x49FE))),
    0x9934: (0x03, 0x993A, (("word", 0x6DD5), ("byte", 0x00))),
    0x993A: (0x16, 0x993B, ()),
    0x993B: (0x01, 0x993F, (("word", 0x996B),)),
    0x993F: (0x09, 0x9945, (("byte", 0x00), ("word", 0x49FB))),
    0x9945: (0x2D, 0x9949, (("word", 0xC01E),)),
    0x996B: (0x2F, 0x9974, (("byte", 0x7F), ("word", 0xC04F), ("word", 0x6E82))),
    0x9974: (0x00, 0x9975, ()),
    0x9975: (0x09, 0x997B, (("byte", 0xFF), ("word", 0x6DE1))),
    0x997B: (0x0E, 0x997E, (("byte", 0xFF),)),
}


def validate_layout(decoded):
    """Reject a changed opcode, operand, or instruction boundary at every gate site."""
    for pc, expected in RUN_LAYOUT.items():
        if decoded[pc] != expected:
            raise ValueError(f"saved ECL layout mismatch at {pc:#06x}")


def decode_layout(script):
    decoded = {}
    for pc in RUN_LAYOUT:
        offset = pc - BASE
        end, vals, items = decode_one(script, offset)
        if end is None or items:
            raise ValueError(f"cannot decode saved ECL at {pc:#06x}")
        decoded[pc] = (script[offset], end + BASE, tuple(vals))
    validate_layout(decoded)
    return decoded


def branch_path(hour, gate):
    """Apply the merged COMPARE and IF skip-next contracts at the two gates."""
    path = [0x9914, 0x991A, 0x9920]
    if hour >= 0x0E:
        path.append(0x9921)
    path.extend((0x9927, 0x992E, 0x9934, 0x993A))
    if gate == 0:
        path.extend((0x993B, 0x996B, 0x9974))
    else:
        path.extend((0x993F, 0x9945))
    return path


def audit(data):
    if len(data) != SAVE_SIZE or hashlib.sha256(data).hexdigest() != SAVE_SHA256:
        raise ValueError("slot-A save size or SHA-256 differs from the audited input")
    script = data[SCRIPT_START:SCRIPT_END]
    script_hash = hashlib.sha256(script).hexdigest()
    if script_hash != SCRIPT_SHA256:
        raise ValueError("saved ECL SHA-256 differs from the audited input")
    decode_layout(script)
    entries = tuple(struct.unpack_from("<H", script, i + 2)[0] for i in (0, 4, 8, 12, 16))
    if entries != (0x9914, 0x9975, 0x9A0E, 0x9A49, 0x9A5E):
        raise ValueError("saved ECL entry words differ")
    hour = struct.unpack_from("<H", data, 0x0001 + 2 * (0x49C9 - 0x4900))[0]
    gate = struct.unpack_from("<H", data, 0x0801 + 2 * (0x6DD5 - 0x6B00))[0]
    path = branch_path(hour, gate)
    return {
        "save_sha256": SAVE_SHA256,
        "script_sha256": script_hash,
        "entry_pcs": [f"0x{x:04X}" for x in entries],
        "saved_hour_word_49c9": hour,
        "saved_gate_word_6dd5": gate,
        "run_path_pcs": [f"0x{x:04X}" for x in path],
        "call_9945_reached_for_saved_gate": 0x9945 in path,
        "continuation_pc": "0x996B" if gate == 0 else "0x993F",
        "continuation_and_destination": "0x6E82" if gate == 0 else None,
        "run_exit_pc": "0x9974" if gate == 0 else None,
        "later_search_entry_pc": "0x9975",
        "search_prefix_pcs": ["0x9975", "0x997B"],
        "first_untraced_search_handler": "0x997B",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("save", type=Path, help="ignored, owner-supplied SAVGAMA.DAT")
    args = parser.parse_args()
    print(json.dumps(audit(args.save.read_bytes()), indent=2))


if __name__ == "__main__":
    main()
