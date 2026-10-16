#!/usr/bin/env python3
"""Check a private paired U19 EGA store's source, CPU, and conventional RAM.

This supplements the four-plane event checker. A supplied pair can establish
its own instruction boundary; neither checker proves native trace coverage or
the first actual value-changing guest writer.
"""

import argparse
from collections import Counter
from dataclasses import dataclass
import hashlib
import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from tools.replay.cold_start_codeword_row14_plane_event_audit import (  # noqa: E402
    RAM_BYTES, _private_file, audit as plane_audit, require,
)
from tools.replay.slot_a_printer_page_audit import unpack  # noqa: E402


START_SHA = "d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd"
DRAWER_SHA = "3c063aa6e79da0ba15b3561c9312750b18d8445c784ef1571ff3e990ce7b2ac4"
GLYPH_SHA = "4aebe178bf3ad2484c62381353ff8bdaf04cf4204f742af578350e642510f58e"
ZERO_SHA = "5c4fa34099b77e7252dd0389cac1d60bd397e30d44577efd894a5326617ced29"
DRAWER_BASE = 0x0F2D << 4
WITNESS_AT = 0x118F
STATUS_FLAGS = 0x0ED5  # CF/PF/AF/ZF/SF/IF/DF/OF; debugger TF is excluded.
CPU_WORDS = ("ax", "bx", "cx", "dx", "si", "di", "bp", "sp",
             "cs", "ds", "es", "ss", "ip", "flags")
SITES = {
    "0F2D:0412": (0x74A2, 3, GLYPH_SHA, 0x0415, "glyph"),
    "0F2D:0438": (0x74C8, 3, GLYPH_SHA, 0x043B, "glyph"),
    "0F2D:0445": (0x74D5, 4, ZERO_SHA, 0x0449, "zero"),
    "0F2D:0451": (0x74E1, 4, ZERO_SHA, 0x0455, "zero"),
}


@dataclass(frozen=True)
class CandidateStore:
    sequence: int
    pc: str
    source_byte: int  # CPU operand, not the resulting EGA plane byte.


def digest(data):
    return hashlib.sha256(data).hexdigest()


def validate_source(exe, loaded_ram):
    """Hash-lock original instruction bytes and their retained loaded image."""
    require(digest(exe) == START_SHA, "original START identity differs")
    image, *_ = unpack(exe)
    original = image[0x7417:0x7501]
    loaded = loaded_ram[DRAWER_BASE + 0x0387:DRAWER_BASE + 0x0471]
    require(len(original) == len(loaded) == 234 and
            digest(original) == DRAWER_SHA and original == loaded,
            "original/loaded drawer identity differs")
    for pc, (at, length, expected, _next, _kind) in SITES.items():
        require(digest(image[at:at + length]) == expected,
                f"original store bytes differ at {pc}")


def validate_cpu(cpu, label):
    require(isinstance(cpu, dict) and all(
        isinstance(cpu.get(name), int) and 0 <= cpu[name] <= 0xFFFF
        for name in CPU_WORDS), f"{label} needs complete 16-bit CPU words")


def validate_pair(pair, before_ram, after_ram):
    """Validate one supplied native instruction pair, without proving reach."""
    require(isinstance(pair, dict) and pair.get("pc") in SITES,
            "pair is not one of four bounded drawer stores")
    pc = pair["pc"]
    _at, _length, _sha, next_ip, kind = SITES[pc]
    require(pair.get("next_pc") == f"0F2D:{next_ip:04X}",
            "candidate instruction successor differs")
    before = pair.get("before")
    after = pair.get("after")
    require(isinstance(before, dict) and isinstance(after, dict),
            "pair needs before and after samples")
    pre = before.get("cpu")
    post = after.get("cpu")
    validate_cpu(pre, "before")
    validate_cpu(post, "after")
    require(pre["cs"] == post["cs"] == 0x0F2D and
            pre["ip"] == int(pc[-4:], 16) and post["ip"] == next_ip,
            "CPU stop is not the pinned instruction step")
    require(pre["es"] == post["es"] == 0xA000 and
            pre["di"] == post["di"] == WITNESS_AT,
            "CPU ES:DI is not the row-14 witness")
    require(all(pre[name] == post[name] for name in CPU_WORDS
                if name not in ("ip", "flags")) and
            (pre["flags"] & STATUS_FLAGS) ==
            (post["flags"] & STATUS_FLAGS),
            "single MOV changed a CPU register or status flag")
    require(len(before_ram) == len(after_ram) == RAM_BYTES and
            before_ram == after_ram,
            "single EGA MOV changed captured conventional RAM")
    source_byte = pre["dx"] & 0xFF if kind == "glyph" else 0
    return CandidateStore(pair["sequence"], pc, source_byte)


def audit(root, manifest, exe):
    """Apply source and CPU/RAM checks after the existing plane-event gate."""
    changes = plane_audit(root, manifest)
    pairs = manifest["events"]
    first_ram = _private_file(root, pairs[0]["before"]["ram"]).read_bytes()
    validate_source(exe, first_ram)
    stores = []
    for pair in pairs:
        before_ram = _private_file(root, pair["before"]["ram"]).read_bytes()
        after_ram = _private_file(root, pair["after"]["ram"]).read_bytes()
        stores.append(validate_pair(pair, before_ram, after_ram))
    return stores, changes


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--exe", type=Path, required=True)
    parser.add_argument("--run", type=Path, required=True)
    parser.add_argument("--manifest", type=Path, required=True)
    args = parser.parse_args()
    require(args.manifest.resolve().is_relative_to(args.run.resolve()),
            "manifest must remain under ignored run directory")
    stores, changes = audit(
        args.run, json.loads(args.manifest.read_text()), args.exe.read_bytes())
    counts = Counter(store.pc for store in stores)
    print(f"source-locked paired CPU/RAM stores={len(stores)}")
    for pc in SITES:
        print(f"{pc} supplied pairs={counts[pc]}")
    print(f"supplied witness-byte changes={sum(bool(c.changed_planes) for c in changes)}")
    print("native trace completeness and first actual guest writer unproved")


if __name__ == "__main__":
    main()
