"""Deterministic exact-state reader for the DOS code-word prompt: ASSISTED input (emulator side).

LABEL: ASSIST. Nothing this module produces is parity evidence or a game credit. It is out of the parity
baseline; the manual path with assist off is the baseline, and it is unchanged when this module is disabled.

The reader decides only from an exact snapshot that the emulator hands it at the INT 16h keyboard poll:
  * CS:IP and a keyboard-poll flag,
  * the BP chain on the stack (word and byte reads of SS, supplied as dicts),
  * the 512-byte record table at DS:0000 and its pinned SHA-256,
  * the BIOS keyboard head and tail words (0040:001A and 0040:001C),
  * the set of unit-0x19 offsets from which the resident 0709:0626 printer was called during this occurrence.
No screen pixels, no OCR, no host typing tool, no confidence score and no game-memory write exists here. Any
failed check is SILENCE. Only one answer plan is started per prompt occurrence; a broken guard aborts the plan and
nothing is ever resent.
"""
import hashlib
from dataclasses import dataclass
from typing import Dict, FrozenSet, Optional, Tuple, Union

ASSIST_LABEL = "ASSIST: emulator-side code-word input; not parity evidence"

# Run-time facts from the owner-verified native captures (sessions t574-t579). Every one is checked by the guard.
RES_0709_SEG = 0x0F2D          # resident 0709 segment word at run time (observed); the input routine's caller chain
UNIT_RET_IP = 0x0302           # the instruction after lcall 0709:09E3 at unit 0x19 +02FD
INDEX_OFFSET = 0x2A            # unit frame: selected index byte at BP-2Ah
ATTEMPT_OFFSET = 0x2B          # unit frame: attempt byte at BP-2Bh
RECORD_COUNT = 13              # indices 0..12
RECORD_BASE = 2                # record slot i starts at DS:0000 + 2 + 21*i
RECORD_STRIDE = 21
RECORD_TEXT_LEN = 6
TABLE_BYTES = 0x200
ATTEMPTS = (1, 2, 3)
MAX_FRAMES = 8
# Unit 0x19 offsets of the six fixed-row calls to 0709:0626 (docs/re/COLD_START_CODEWORD_SOURCE.md).
FIXED_ROW_CALLS = frozenset({0x0157, 0x0174, 0x0191, 0x0202, 0x0286, 0x02D8})
RUNTIME_TABLE_SHA256 = "0486c788451ab11470af8d9bcf79a4ff4c677bca4fa479dddc96d2e2e8f27ac7"
MAX_WAIT_STEPS = 200
RETURN = "RETURN"

# Standard PC set-1 make codes for the normal keyboard path (public keyboard facts, not game data).
MAKE_CODES = {
    "A": 0x1E, "B": 0x30, "C": 0x2E, "D": 0x20, "E": 0x12, "F": 0x21, "G": 0x22, "H": 0x23, "I": 0x17,
    "J": 0x24, "K": 0x25, "L": 0x26, "M": 0x32, "N": 0x31, "O": 0x18, "P": 0x19, "Q": 0x10, "R": 0x13,
    "S": 0x1F, "T": 0x14, "U": 0x16, "V": 0x2F, "W": 0x11, "X": 0x2D, "Y": 0x15, "Z": 0x2C,
    RETURN: 0x1C,
}


@dataclass(frozen=True)
class Snapshot:
    cs: int
    ip: int
    bp: int
    ss_word: Dict[int, int]        # SS-relative word reads, keyed by offset
    ss_byte: Dict[int, int]        # SS-relative byte reads, keyed by offset
    ds_table: Optional[bytes]      # DS:0000, TABLE_BYTES long, or None when unavailable
    bda_head: int
    bda_tail: int
    printer_sites: FrozenSet[int]  # unit offsets that called 0709:0626 during this occurrence
    keyboard_poll: bool            # True only at the INT 16h keyboard poll of the input loop


@dataclass(frozen=True)
class Frame:
    bp: int
    saved_bp: int
    ret_ip: int
    ret_cs: int


@dataclass(frozen=True)
class Prompt:
    occurrence: Tuple[int, int, int, int, int]   # (unit segment, unit BP, input frame BP, index, attempt)
    index: int
    attempt: int
    text: str


@dataclass(frozen=True)
class Action:
    kind: str                      # "SILENT", "WAIT" or "SEND"
    key: Optional[str]
    reason: str


def table_digest(table: bytes) -> str:
    return hashlib.sha256(table).hexdigest()


def record_slot(index: int) -> int:
    return RECORD_BASE + RECORD_STRIDE * index


def record_text(table: bytes, index: int) -> Optional[str]:
    """The six letters of slot `index`, or None when the slot is not a well-formed record."""
    start = record_slot(index)
    if start + RECORD_STRIDE > len(table) or table[start] != RECORD_TEXT_LEN:
        return None
    raw = table[start + 1:start + 1 + RECORD_TEXT_LEN]
    if any(not (65 <= b <= 90) for b in raw):
        return None
    return raw.decode("ascii")


def walk(snap: Snapshot):
    frames = []
    bp = snap.bp
    for _ in range(MAX_FRAMES):
        if bp not in snap.ss_word or bp + 4 not in snap.ss_word:
            break
        saved = snap.ss_word[bp]
        frames.append(Frame(bp, saved, snap.ss_word[bp + 2], snap.ss_word[bp + 4]))
        if saved <= bp:
            break
        bp = saved
    return frames


def recognise(snap: Snapshot, expected_sha256: str) -> Union[Prompt, str]:
    """A Prompt, or the reason the exact state is not a recognised prompt occurrence."""
    if not snap.keyboard_poll:
        return "not-keyboard-poll"
    frames = walk(snap)
    hits = [i for i, f in enumerate(frames) if f.ret_ip == UNIT_RET_IP]
    if len(hits) != 1:
        return "no-unique-input-return"
    i = hits[0]
    if i == 0:
        return "no-input-routine-child"
    frame, child = frames[i], frames[i - 1]
    if child.saved_bp != frame.bp or child.ret_cs != RES_0709_SEG:
        return "input-routine-chain-mismatch"
    unit_seg = frame.ret_cs
    unit_bp = frame.saved_bp
    if unit_bp - INDEX_OFFSET not in snap.ss_byte or unit_bp - ATTEMPT_OFFSET not in snap.ss_byte:
        return "frame-bytes-unavailable"
    index = snap.ss_byte[unit_bp - INDEX_OFFSET]
    attempt = snap.ss_byte[unit_bp - ATTEMPT_OFFSET]
    if index >= RECORD_COUNT:
        return "index-out-of-range"
    if attempt not in ATTEMPTS:
        return "attempt-out-of-range"
    if snap.ds_table is None or len(snap.ds_table) != TABLE_BYTES:
        return "table-unavailable"
    if table_digest(snap.ds_table) != expected_sha256:
        return "table-digest-mismatch"
    if not FIXED_ROW_CALLS <= snap.printer_sites:
        return "fixed-rows-not-printed"
    text = record_text(snap.ds_table, index)
    if text is None:
        return "record-malformed"
    return Prompt((unit_seg, unit_bp, frame.bp, index, attempt), index, attempt, text)


@dataclass
class _Plan:
    occurrence: Tuple[int, int, int, int, int]
    keys: list
    pending: Optional[Tuple[str, int, int]] = None   # (key, tail before send, waits)


class Gate:
    """One answer plan per prompt occurrence: letters of the record, then RETURN, one key at a time."""

    def __init__(self, enabled: bool, expected_sha256: str = RUNTIME_TABLE_SHA256):
        self.enabled = enabled
        self.expected_sha256 = expected_sha256
        self.started = set()
        self.aborted = set()
        self.plan: Optional[_Plan] = None

    def step(self, snap: Snapshot) -> Action:
        if not self.enabled:
            return Action("SILENT", None, "assist-off")
        rec = recognise(snap, self.expected_sha256) if snap.keyboard_poll else "not-keyboard-poll"
        p = self.plan
        if p is not None:
            if not p.keys and p.pending is None:
                self.plan = None
                return Action("SILENT", None, "completed")
            if isinstance(rec, str) or rec.occurrence != p.occurrence:
                self.aborted.add(p.occurrence)
                self.plan = None
                why = rec if isinstance(rec, str) else "occurrence-changed"
                return Action("SILENT", None, "abort:" + why)
            if p.pending is not None:
                key, tail_before, waits = p.pending
                if snap.bda_tail == tail_before:
                    if waits + 1 > MAX_WAIT_STEPS:
                        self.aborted.add(p.occurrence)
                        self.plan = None
                        return Action("SILENT", None, "abort:key-not-buffered")
                    p.pending = (key, tail_before, waits + 1)
                    return Action("WAIT", None, "key-not-yet-buffered")
                if snap.bda_head != snap.bda_tail:
                    return Action("WAIT", None, "key-not-yet-consumed")
                p.pending = None
            if not p.keys:
                self.plan = None
                return Action("SILENT", None, "completed")
            if snap.bda_head != snap.bda_tail:
                return Action("WAIT", None, "buffer-not-empty")
            key = p.keys.pop(0)
            p.pending = (key, snap.bda_tail, 0)
            return Action("SEND", key, "one-normal-key")
        if isinstance(rec, str):
            return Action("SILENT", None, rec)
        if rec.occurrence in self.started or rec.occurrence in self.aborted:
            return Action("SILENT", None, "occurrence-already-handled")
        if snap.bda_head != snap.bda_tail:
            return Action("WAIT", None, "buffer-not-empty")
        self.started.add(rec.occurrence)
        self.plan = _Plan(rec.occurrence, list(rec.text) + [RETURN])
        key = self.plan.keys.pop(0)
        self.plan.pending = (key, snap.bda_tail, 0)
        return Action("SEND", key, "one-normal-key")
