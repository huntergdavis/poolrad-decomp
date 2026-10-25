#!/usr/bin/env python3
"""Byte-compare a candidate-scout proposal against the owner GAME.OVR.

A candidate proposal is a claim: "this body id lives at this unit-relative
offset, is this many bytes, and these are its original bytes". The claim is
mechanically checkable and this tool checks exactly that, nothing else. It does
NOT verify the semantic C, the caller/callee contracts, or any credit decision.

Proposal format, one block per candidate, anywhere in the document:

    BODY: OVL_001C_0292
    UNIT: 0x1c
    REL_OFFSET: 0x0292
    LENGTH: 78
    SHA256: <64 hex>
    BYTES:
    ```bytes
    55 89 e5 ...
    ```

RUN:
    python3 tools/overlay/check_candidate_proposal.py docs/re/CANDIDATE_SCOUT_DS41B_BATCH1.md

Prints one line per candidate:

    BYTES MATCH: 78 bytes at 4056 are byte-identical to GAME.OVR (sha256 ...)
    BYTES DIFFER: OVL_... first difference at +N (offset F): file xx, claimed yy
    NO BYTES: OVL_... has no `bytes` block
    UNIT UNRESOLVED: OVL_... unit 0x1c not in the inventory

Exit 0 only if every candidate matches. Media stays under ignored resources/.
"""
from __future__ import annotations

import argparse
import re
import sys
from hashlib import sha256
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from overlay_inventory import find_units, parse_mz  # noqa: E402

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
DEFAULT_EXE = ROOT / "resources/DOS/POOLRAD/START.EXE"
FIELD_RE = re.compile(r"^(BODY|UNIT|REL_OFFSET|LENGTH|SHA256):\s*(\S+)\s*$")
BYTES_RE = re.compile(r"^```bytes\s*$([\s\S]*?)^```\s*$", re.M)


def blocks(text):
    """Yield (fields, bytes_text) for each BODY: block that precedes a bytes fence."""
    lines = text.splitlines()
    current = {}
    for lineno, line in enumerate(lines):
        m = FIELD_RE.match(line)
        if m:
            key, value = m.group(1), m.group(2)
            if key == "BODY" and current.get("BODY"):
                yield current, None
                current = {}
            current[key] = value
            continue
        if line.strip() == "BYTES:" and current.get("BODY"):
            m2 = BYTES_RE.search("\n".join(lines[lineno:]))
            dump = m2.group(1) if m2 else None
            yield current, dump
            current = {}
    if current.get("BODY"):
        yield current, None


def hexbytes(dump):
    if not dump:
        return None
    raw = re.sub(r"[^0-9A-Fa-f]", "", dump)
    if len(raw) % 2:
        raw = raw[:-1]
    return bytes.fromhex(raw)


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("proposal")
    parser.add_argument("--ovr", default=str(DEFAULT_OVR))
    parser.add_argument("--exe", default=str(DEFAULT_EXE))
    args = parser.parse_args(argv)

    ovr_path, exe_path = Path(args.ovr), Path(args.exe)
    for path in (ovr_path, exe_path):
        if not path.exists():
            print(f"REFUSED: no media at {path}", file=sys.stderr)
            return 2
    ovr = ovr_path.read_bytes()
    exe = exe_path.read_bytes()
    units = {u["stub_para"]: u for u in find_units(exe[parse_mz(exe)["header_bytes"]:], ovr)}

    text = Path(args.proposal).read_text(errors="replace")
    failures = 0
    found = 0
    for fields, dump in blocks(text):
        found += 1
        body = fields["BODY"]
        expected = hexbytes(dump)
        if expected is None:
            print(f"NO BYTES: {body} has no `bytes` block")
            failures += 1
            continue
        try:
            unit = int(fields.get("UNIT", ""), 0)
            rel = int(fields.get("REL_OFFSET", ""), 0)
            length = int(fields.get("LENGTH", "0"), 0)
        except ValueError as exc:
            print(f"BAD FIELDS: {body}: {exc}")
            failures += 1
            continue
        if unit not in units:
            print(f"UNIT UNRESOLVED: {body} unit {unit:#x} not in the inventory")
            failures += 1
            continue
        at = units[unit]["ovr_offset"] + rel
        if length and len(expected) != length:
            print(f"LENGTH MISMATCH: {body} claims {length} bytes, block has {len(expected)}")
            failures += 1
        file_bytes = ovr[at:at + len(expected)]
        digest = sha256(file_bytes).hexdigest()
        if file_bytes == expected:
            extra = ""
            if fields.get("SHA256") and fields["SHA256"].lower() != digest:
                extra = f" WARNING: claimed sha256 {fields['SHA256'][:16]}... != {digest[:16]}..."
            print(f"BYTES MATCH: {len(expected)} bytes at {at} are byte-identical to GAME.OVR"
                  f" (sha256 {digest}{extra})")
        else:
            failures += 1
            for i, (x, y) in enumerate(zip(file_bytes, expected)):
                if x != y:
                    print(f"BYTES DIFFER: {body} first difference at +{i} (offset {at + i}):"
                          f" file {x:02x}, claimed {y:02x}")
                    print(f"   file   : {' '.join(f'{v:02x}' for v in file_bytes[:48])}")
                    print(f"   claimed: {' '.join(f'{v:02x}' for v in expected[:48])}")
                    break
            else:
                print(f"BYTES DIFFER: {body} claimed block is longer than the file slice")
    if not found:
        print("NO CANDIDATES: the proposal has no BODY: blocks", file=sys.stderr)
        return 2
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
