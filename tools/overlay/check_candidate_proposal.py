#!/usr/bin/env python3
"""Verify a candidate-scout proposal's span identity against the owner GAME.OVR.

A candidate proposal is a claim: "this body id lives at this unit-relative offset, is
this many bytes, at this file offset, with this SHA-256". This tool checks exactly that,
by reading the span from the game file and hashing it. Original bytes are not quoted in
the proposal, so nothing derived from the game is committed in the doc.

Proposal format, one block per candidate, anywhere in the document:

    BODY: OVL_001C_0292
    UNIT: 0x1c
    REL_OFFSET: 0x0292
    LENGTH: 78
    FILE_OFFSET: 4056
    SHA256: <64 hex>

RUN:
    python3 tools/overlay/check_candidate_proposal.py docs/re/CANDIDATE_SCOUT_DS41B_BATCH1.md

Prints one line per candidate:

    SPAN MATCH: 78 bytes at 4056 sha256 ...
    SHA256 MISMATCH: OVL_... claims ... but the span digest is ...
    FILE_OFFSET MISMATCH: OVL_... claims N but unit+offset resolve to M
    LENGTH OUT OF RANGE / NO SHA256 / UNIT UNRESOLVED / BAD FIELDS

A present SHA256 must match; a missing one is a failure, because the digest is the only
identity claim left once the bytes are not quoted. Exit 0 only if every candidate matches.
Media stays under ignored resources/.
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
FIELD_RE = re.compile(r"^(BODY|UNIT|REL_OFFSET|LENGTH|FILE_OFFSET|SHA256):\s*(\S+)\s*$")


def blocks(text):
    """Yield one field dict per BODY: block."""
    current = {}
    for line in text.splitlines():
        m = FIELD_RE.match(line)
        if not m:
            continue
        key, value = m.group(1), m.group(2)
        if key == "BODY" and current.get("BODY"):
            yield current
            current = {}
        current[key] = value
    if current.get("BODY"):
        yield current


def digest_matches(claimed, actual):
    """A present SHA256 must equal the span digest; an absent one is a failure."""
    return claimed is not None and claimed.lower() == actual


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
    for fields in blocks(text):
        found += 1
        body = fields["BODY"]
        try:
            unit = int(fields.get("UNIT", ""), 0)
            rel = int(fields.get("REL_OFFSET", ""), 0)
            length = int(fields.get("LENGTH", "0"), 0)
            file_offset = int(fields["FILE_OFFSET"], 0) if "FILE_OFFSET" in fields else None
        except ValueError as exc:
            print(f"BAD FIELDS: {body}: {exc}")
            failures += 1
            continue
        if unit not in units:
            print(f"UNIT UNRESOLVED: {body} unit {unit:#x} not in the inventory")
            failures += 1
            continue
        if rel < 0:
            print(f"NEGATIVE OFFSET: {body} REL_OFFSET {rel} is negative")
            failures += 1
            continue
        at = units[unit]["ovr_offset"] + rel
        if at < 0:
            print(f"NEGATIVE OFFSET: {body} resolves to a negative file offset {at}")
            failures += 1
            continue
        if file_offset is not None and file_offset < 0:
            print(f"NEGATIVE OFFSET: {body} FILE_OFFSET {file_offset} is negative")
            failures += 1
            continue
        if file_offset is not None and file_offset != at:
            print(f"FILE_OFFSET MISMATCH: {body} claims {file_offset} but unit+offset resolve to {at}")
            failures += 1
            continue
        # Validate the whole interval BEFORE slicing, then slice once.
        if length <= 0:
            print(f"BAD INTERVAL: {body} LENGTH {length} must be positive")
            failures += 1
            continue
        if at + length > len(ovr):
            print(f"BAD INTERVAL: {body} [{at},{at + length}) exceeds the file ({len(ovr)} bytes)")
            failures += 1
            continue
        span = ovr[at:at + length]
        if len(span) != length:
            print(f"BAD INTERVAL: {body} requested {length} bytes but the slice has {len(span)}")
            failures += 1
            continue
        digest = sha256(span).hexdigest()
        claimed = fields.get("SHA256")
        if claimed is None:
            print(f"NO SHA256: {body} has no digest claim")
            failures += 1
            continue
        if not digest_matches(claimed, digest):
            print(f"SHA256 MISMATCH: {body} claims {claimed} but the span digest is {digest}")
            failures += 1
            continue
        print(f"SPAN MATCH: {length} bytes at {at} sha256 {digest}")
    if not found:
        print("NO CANDIDATES: the proposal has no BODY: blocks", file=sys.stderr)
        return 2
    return 0 if failures == 0 else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
