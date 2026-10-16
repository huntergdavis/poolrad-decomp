#!/usr/bin/env python3
"""Offline U19 row-14 DEBUGBOX compatibility and witness-frame preflight.

This tool reads a pinned debugger binary and optional supplied JSON frames.
It never launches DOSBox-X, changes emulator state, or declares a plane write.
"""

import argparse
from dataclasses import dataclass
from hashlib import sha256
import json
from pathlib import Path
import re


HEAVY_SHA = "8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92"
FEATURES = (
    b"DEBUGBOX",
    b"MEMDUMPBIN",
    b"VGA Graphics controller info",
    b"VGA Sequencer info",
    b"VGA mode info",
    b"OUTP",
    b"BPM",
)
CALLER = (0x38D3, 0x02D8)
RETURN = (0x38D3, 0x02DD)
WITNESS = (0xA000, 0x118F)
WITNESS_BIT = 0x20
NEXT_PC = {
    0x0412: 0x0415,
    0x0438: 0x043B,
    0x0445: 0x0449,
    0x0451: 0x0455,
}
MAX_CANDIDATE_HITS = 128


@dataclass(frozen=True)
class Frame:
    cs: int
    ip: int
    es: int
    di: int


@dataclass(frozen=True)
class FilterResult:
    status: str
    candidate_hits: int = 0
    target_pc: int | None = None
    next_pc: int | None = None


def debugger_features(binary: bytes, expected_sha: str = HEAVY_SHA) -> tuple[str, tuple[str, ...]]:
    """Pin the compatible private debugger before any capture protocol is used."""
    missing = tuple(token.decode("ascii") for token in FEATURES if token not in binary)
    if sha256(binary).hexdigest() != expected_sha:
        return "STOP_DEBUGGER_SHA", missing
    if missing:
        return "STOP_DEBUGGER_FEATURE", missing
    return "SOURCE_PREFLIGHT_READY_NO_NATIVE", ()


def _word(value: object) -> int:
    """Debugger JSON stores hexadecimal WORDs as strings, never ambiguous decimal."""
    if not isinstance(value, str) or re.fullmatch(r"[0-9A-Fa-f]{1,4}", value) is None:
        raise ValueError("frame WORD must be one to four hex digits")
    return int(value, 16)


def parse_frame(record: object) -> Frame:
    if not isinstance(record, dict) or set(record) != {"cs", "ip", "es", "di"}:
        raise ValueError("frame needs exactly cs, ip, es, and di")
    return Frame(*(_word(record[name]) for name in ("cs", "ip", "es", "di")))


def filter_frames(frames: list[Frame], max_candidate_hits: int = MAX_CANDIDATE_HITS) -> FilterResult:
    """Find one paired witness instruction in a supplied same-run stop stream.

    A future driver must single-step the selected store and append its direct
    successor as the next frame. Unrelated candidate hits are skipped.
    This verifies frame selection only; no plane data or caller stack is here.
    """
    if not 1 <= max_candidate_hits <= MAX_CANDIDATE_HITS:
        raise ValueError("candidate-hit cap must be 1..128")
    armed = False
    hits = 0
    for i, frame in enumerate(frames):
        pc = (frame.cs, frame.ip)
        if not armed:
            if pc == CALLER:
                armed = True
            continue
        if pc == RETURN:
            return FilterResult("STOP_PRINTER_RETURN_BEFORE_WITNESS", hits)
        if frame.cs != 0x0F2D or frame.ip not in NEXT_PC:
            continue
        hits += 1
        if hits > max_candidate_hits:
            return FilterResult("STOP_CANDIDATE_HIT_CAP", hits)
        if (frame.es, frame.di) != WITNESS:
            continue
        successor = NEXT_PC[frame.ip]
        if i + 1 == len(frames):
            return FilterResult("STOP_MISSING_SINGLE_STEP", hits, frame.ip, successor)
        next_frame = frames[i + 1]
        if (next_frame.cs, next_frame.ip) != (0x0F2D, successor):
            return FilterResult("STOP_WRONG_SINGLE_STEP", hits, frame.ip, successor)
        return FilterResult("PAIRED_CPU_SOURCE_ONLY", hits, frame.ip, successor)
    if not armed:
        return FilterResult("STOP_NO_U19_CALLER")
    return FilterResult("STOP_NO_WITNESS_FRAME", hits)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--debugger", type=Path, required=True,
                        help="ignored pinned heavy DOSBox-X executable")
    parser.add_argument("--frames", type=Path,
                        help="optional supplied same-run JSON stop frames; no native capture")
    args = parser.parse_args()
    status, missing = debugger_features(args.debugger.read_bytes())
    print(f"debugger={status}; missing_feature_tokens={len(missing)}")
    if status != "SOURCE_PREFLIGHT_READY_NO_NATIVE":
        raise SystemExit(2)
    if args.frames:
        records = json.loads(args.frames.read_text())
        if not isinstance(records, list):
            raise SystemExit("STOP_FRAME_LIST")
        result = filter_frames([parse_frame(record) for record in records])
        print(f"frame_filter={result.status}; candidate_hits={result.candidate_hits}")
        if result.target_pc is not None:
            print(f"selected=0F2D:{result.target_pc:04X} -> "
                  f"0F2D:{result.next_pc:04X}; witness=A000:118F bit=20")
        if result.status != "PAIRED_CPU_SOURCE_ONLY":
            raise SystemExit(3)
    print("first value-changing EGA writer, page and RGB timing unmeasured")


if __name__ == "__main__":
    main()
