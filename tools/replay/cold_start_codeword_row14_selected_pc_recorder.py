#!/usr/bin/env python3
"""Opt-in U19 selected-PC recorder model and private manifest auditor.

Inputs are supplied frames. This module does not hook the emulator or prove
that every native instruction was observed. Legacy cap-128 APIs are untouched.
"""

from dataclasses import dataclass
from hashlib import sha256
import json
import re

from tools.replay.cold_start_codeword_row14_candidate_coverage_gate import (
    MAX_JSONL_LINE, MAX_STREAM_RECORDS, ROUTE_CANDIDATES, SUCCESSOR,
    WITNESS_OFFSET, WITNESS_SEGMENT, candidate_frames,
)
from tools.replay.cold_start_codeword_row14_target_filter import Frame, parse_frame


SOURCE_DOMAIN = "src/debug/debug.cpp raw bytes"
MAX_MANIFEST_BYTES = 4096
TOKEN = re.compile(r"[A-Za-z0-9_-]{1,64}\Z")
HEX_SHA = re.compile(r"[0-9a-f]{64}\Z")
SITES = frozenset(SUCCESSOR)
MANIFEST_FIELDS = frozenset((
    "version", "run_token", "arm_token", "stream_id", "plane_event_id",
    "source_domain", "source_sha256", "raw_sha256", "segments",
    "candidate_count", "frame_count", "terminal", "stop_reason",
    "overflow", "truncated", "selected_event", "native_coverage_proved",
))
SELECTED_FIELDS = frozenset((
    "run_token", "arm_token", "stream_id", "plane_event_id", "raw_sha256",
    "candidate_ordinal", "pc", "successor_pc",
))
PC_WORDS = re.compile(r"[0-9A-F]{4}:[0-9A-F]{4}\Z")


@dataclass(frozen=True)
class Recording:
    raw_jsonl: bytes
    manifest: dict


@dataclass(frozen=True)
class AuditResult:
    status: str
    candidate_count: int
    selected_candidate_ordinal: int | None
    native_coverage_proved: bool = False


def _word(value: object) -> bool:
    return type(value) is int and 0 <= value <= 0xFFFF


def _token(value: object) -> bool:
    return isinstance(value, str) and TOKEN.fullmatch(value) is not None


def _frame_jsonl(frame: Frame) -> bytes:
    record = {name: f"{getattr(frame, name):04X}"
              for name in ("cs", "ip", "es", "di")}
    return (json.dumps(record, separators=(",", ":")) + "\n").encode("ascii")


def _segments(u19: int, printer: int, active: int, draw: int) -> dict:
    return {"u19": f"{u19:04X}", "printer": f"{printer:04X}",
            "active": f"{active:04X}", "draw": f"{draw:04X}"}


class CandidateRecorder:
    """Consume one supplied caller/candidate/successor/return stream.

    The caller must opt in explicitly. An authentic native collector must
    separately prove that it supplied *every* reached candidate PC.
    """

    def __init__(self, *, run_token: str, arm_token: str, stream_id: str,
                 plane_event_id: str, source_bytes: bytes, u19_segment: int,
                 printer_segment: int, active_segment: int, draw_segment: int,
                 enabled: bool = False):
        if not all(_token(value) for value in
                   (run_token, arm_token, stream_id, plane_event_id)):
            raise ValueError("invalid run, arm, stream, or plane-event token")
        if not isinstance(source_bytes, bytes) or not all(_word(word) for word in
                (u19_segment, printer_segment, active_segment, draw_segment)):
            raise ValueError("source bytes and four bounded segments required")
        if type(enabled) is not bool:
            raise ValueError("enabled must be an exact bool")
        self.run_token = run_token
        self.arm_token = arm_token
        self.stream_id = stream_id
        self.plane_event_id = plane_event_id
        self.source_sha256 = sha256(source_bytes).hexdigest()
        self.u19 = u19_segment
        self.printer = printer_segment
        self.active = active_segment
        self.draw = draw_segment
        self.enabled = enabled
        self.expected = tuple(candidate_frames(printer_segment, active_segment,
                                               draw_segment))
        self.frames: list[Frame] = []
        self.candidate_count = 0
        self.selected_ordinal: int | None = None
        self.selected_pc: int | None = None
        self.selected_frame: Frame | None = None
        self.await_successor = False
        self.terminal: str | None = None
        self.stop_reason: str | None = None
        self.overflow = False
        self.truncated = False

    def _stop(self, reason: str) -> str:
        self.stop_reason = reason
        self.terminal = "stopped"
        return reason

    def push(self, frame: Frame) -> str:
        """Append one supplied relevant instruction frame, bounded on entry."""
        if not self.enabled:
            return "DISABLED"
        if not isinstance(frame, Frame) or not all(_word(value) for value in
                (frame.cs, frame.ip, frame.es, frame.di)):
            return self._stop("STOP_FRAME_WORD")
        if self.terminal is not None:
            return self._stop("STOP_AFTER_END")
        if not self.frames:
            if (frame.cs, frame.ip) != (self.u19, 0x02D8):
                return self._stop("STOP_CALLER_MISSING")
            self.frames.append(frame)
            return "CALLER"
        if self.await_successor:
            assert self.selected_frame is not None and self.selected_pc is not None
            target = Frame(self.printer, SUCCESSOR[self.selected_pc],
                           self.selected_frame.es, self.selected_frame.di)
            if frame != target:
                return self._stop("STOP_WRONG_SINGLE_STEP")
            self.frames.append(frame)
            self.await_successor = False
            return "SUCCESSOR"
        if (frame.cs, frame.ip) == (self.u19, 0x02DD):
            if self.candidate_count != ROUTE_CANDIDATES:
                return self._stop("STOP_INCOMPLETE_ROUTE")
            self.frames.append(frame)
            if self.selected_ordinal is None:
                return self._stop("STOP_TARGET_PAGE_ABSENT")
            self.terminal = "full_return"
            return "FULL_RETURN"
        if frame.cs != self.printer or frame.ip not in SITES:
            return self._stop("STOP_UNEXPECTED_FRAME")
        self.candidate_count += 1
        if self.candidate_count > ROUTE_CANDIDATES:
            self.overflow = True
            return self._stop("STOP_CANDIDATE_OVERFLOW")
        if frame != self.expected[self.candidate_count - 1]:
            return self._stop("STOP_CANDIDATE_SEQUENCE")
        self.frames.append(frame)
        if (self.selected_ordinal is None and frame.es == WITNESS_SEGMENT and
                frame.di == WITNESS_OFFSET):
            self.selected_ordinal = self.candidate_count
            self.selected_pc = frame.ip
            self.selected_frame = frame
            self.await_successor = True
        return "CANDIDATE"

    def finish(self, *, selected_prefix: bool = False) -> Recording:
        if not self.enabled:
            raise ValueError("recorder is disabled")
        if self.terminal is None:
            if selected_prefix and self.selected_ordinal is not None and not self.await_successor:
                self.terminal = "selected_prefix"
            else:
                self.truncated = True
                self._stop("STOP_TRUNCATED")
        raw = b"".join(_frame_jsonl(frame) for frame in self.frames)
        if len(self.frames) > MAX_STREAM_RECORDS or any(
                len(_frame_jsonl(frame)) > MAX_JSONL_LINE for frame in self.frames):
            raise AssertionError("recorder exceeded raw stream bound")
        digest = sha256(raw).hexdigest()
        selected = None
        if self.terminal in ("full_return", "selected_prefix") and self.selected_ordinal is not None:
            selected = {
                "run_token": self.run_token, "arm_token": self.arm_token,
                "stream_id": self.stream_id,
                "plane_event_id": self.plane_event_id,
                "raw_sha256": digest,
                "candidate_ordinal": self.selected_ordinal,
                "pc": f"{self.printer:04X}:{self.selected_pc:04X}",
                "successor_pc": f"{self.printer:04X}:{SUCCESSOR[self.selected_pc]:04X}",
            }
        manifest = {
            "version": 1, "run_token": self.run_token,
            "arm_token": self.arm_token, "stream_id": self.stream_id,
            "plane_event_id": self.plane_event_id,
            "source_domain": SOURCE_DOMAIN,
            "source_sha256": self.source_sha256,
            "raw_sha256": digest,
            "segments": _segments(self.u19, self.printer, self.active, self.draw),
            "candidate_count": self.candidate_count,
            "frame_count": len(self.frames),
            "terminal": self.terminal, "stop_reason": self.stop_reason,
            "overflow": self.overflow, "truncated": self.truncated,
            "selected_event": selected, "native_coverage_proved": False,
        }
        if len(json.dumps(manifest, separators=(",", ":")).encode("utf-8")) > MAX_MANIFEST_BYTES:
            raise AssertionError("recorder exceeded manifest bound")
        return Recording(raw, manifest)


def audit_recording(raw_jsonl: bytes, manifest: object,
                    source_bytes: bytes) -> AuditResult:
    """Parse and recompute a supplied source-only private transcript."""
    if not isinstance(raw_jsonl, bytes) or len(raw_jsonl) > MAX_STREAM_RECORDS * MAX_JSONL_LINE:
        raise ValueError("STOP_RAW_SIZE_CAP")
    if not isinstance(manifest, dict) or set(manifest) != MANIFEST_FIELDS or len(
            json.dumps(manifest, separators=(",", ":")).encode("utf-8")) > MAX_MANIFEST_BYTES:
        raise ValueError("STOP_MANIFEST_SHAPE")
    if not isinstance(source_bytes, bytes) or manifest["source_domain"] != SOURCE_DOMAIN or (
            manifest["source_sha256"] != sha256(source_bytes).hexdigest()):
        raise ValueError("STOP_SOURCE_SHA")
    if type(manifest["version"]) is not int or manifest["version"] != 1 or (
            manifest["native_coverage_proved"] is not False) or not all(
            _token(manifest[name]) for name in
            ("run_token", "arm_token", "stream_id", "plane_event_id")):
        raise ValueError("STOP_MANIFEST_VERSION_OR_TOKEN")
    if not isinstance(manifest["raw_sha256"], str) or HEX_SHA.fullmatch(
            manifest["raw_sha256"]) is None or manifest["raw_sha256"] != sha256(raw_jsonl).hexdigest():
        raise ValueError("STOP_RAW_SHA")
    segments = manifest["segments"]
    if not isinstance(segments, dict) or set(segments) != {"u19", "printer", "active", "draw"}:
        raise ValueError("STOP_SEGMENTS")
    try:
        words = tuple(int(segments[name], 16) for name in
                      ("u19", "printer", "active", "draw"))
    except (TypeError, ValueError) as exc:
        raise ValueError("STOP_SEGMENTS") from exc
    if not all(isinstance(segments[name], str) and
               re.fullmatch(r"[0-9A-F]{4}", segments[name]) and _word(word)
               for name, word in zip(("u19", "printer", "active", "draw"), words)):
        raise ValueError("STOP_SEGMENTS")
    lines = raw_jsonl.splitlines(keepends=True)
    if (not lines or len(lines) > MAX_STREAM_RECORDS or
            any(not line.endswith(b"\n") or len(line) > MAX_JSONL_LINE for line in lines)):
        raise ValueError("STOP_RAW_RECORD_CAP")
    frames = []
    for line in lines:
        try:
            frame = parse_frame(json.loads(line))
        except (UnicodeError, ValueError, TypeError) as exc:
            raise ValueError("STOP_RAW_FRAME") from exc
        if line != _frame_jsonl(frame):
            raise ValueError("STOP_NONCANONICAL_FRAME")
        frames.append(frame)
    if type(manifest["frame_count"]) is not int or manifest["frame_count"] != len(frames) or (
            type(manifest["candidate_count"]) is not int or
            manifest["candidate_count"] < 0):
        raise ValueError("STOP_FRAME_COUNT")
    selected = manifest["selected_event"]
    if selected is not None:
        if (not isinstance(selected, dict) or set(selected) != SELECTED_FIELDS or
                not all(_token(selected[name]) for name in
                        ("run_token", "arm_token", "stream_id", "plane_event_id")) or
                not isinstance(selected["raw_sha256"], str) or
                HEX_SHA.fullmatch(selected["raw_sha256"]) is None or
                type(selected["candidate_ordinal"]) is not int or
                not 1 <= selected["candidate_ordinal"] <= ROUTE_CANDIDATES or
                not all(isinstance(selected[name], str) and
                        PC_WORDS.fullmatch(selected[name]) is not None
                        for name in ("pc", "successor_pc"))):
            raise ValueError("STOP_SELECTED_EVENT_SCHEMA")
    replay = CandidateRecorder(
        run_token=manifest["run_token"], arm_token=manifest["arm_token"],
        stream_id=manifest["stream_id"], plane_event_id=manifest["plane_event_id"],
        source_bytes=source_bytes, u19_segment=words[0], printer_segment=words[1],
        active_segment=words[2], draw_segment=words[3], enabled=True,
    )
    for index, frame in enumerate(frames):
        outcome = replay.push(frame)
        if outcome == "STOP_TARGET_PAGE_ABSENT" and index == len(frames) - 1:
            continue
        if outcome.startswith("STOP_"):
            raise ValueError("STOP_RAW_SEQUENCE")
    if manifest["stop_reason"] == "STOP_TARGET_PAGE_ABSENT":
        if (manifest["terminal"] != "stopped" or manifest["overflow"] is not False or
                manifest["truncated"] is not False or selected is not None or
                replay.stop_reason != "STOP_TARGET_PAGE_ABSENT" or
                replay.candidate_count != ROUTE_CANDIDATES or
                replay.finish().manifest != manifest):
            raise ValueError("STOP_NO_TARGET_MANIFEST")
        return AuditResult("STOP_TARGET_PAGE_ABSENT", ROUTE_CANDIDATES, None)
    if manifest["overflow"] is True:
        if (manifest["terminal"] != "stopped" or
                manifest["stop_reason"] != "STOP_CANDIDATE_OVERFLOW" or
                manifest["truncated"] is not False or
                manifest["candidate_count"] != ROUTE_CANDIDATES + 1 or
                manifest["selected_event"] is not None):
            raise ValueError("STOP_OVERFLOW_MANIFEST")
        if (replay.candidate_count != ROUTE_CANDIDATES or
                replay.terminal is not None or replay.await_successor):
            raise ValueError("STOP_OVERFLOW_PREFIX")
        replay.push(Frame(words[1], min(SITES), 0, 0))
        if replay.finish().manifest != manifest:
            raise ValueError("STOP_OVERFLOW_MANIFEST")
        return AuditResult("STOP_CANDIDATE_OVERFLOW", manifest["candidate_count"], None)
    if manifest["truncated"] is True:
        if (manifest["terminal"] != "stopped" or
                manifest["stop_reason"] != "STOP_TRUNCATED" or
                manifest["overflow"] is not False or
                manifest["selected_event"] is not None):
            raise ValueError("STOP_TRUNCATED_MANIFEST")
        if replay.terminal is not None or replay.candidate_count != manifest["candidate_count"]:
            raise ValueError("STOP_TRUNCATED_PREFIX")
        if replay.finish().manifest != manifest:
            raise ValueError("STOP_TRUNCATED_MANIFEST")
        return AuditResult("STOP_TRUNCATED", manifest["candidate_count"], None)
    if manifest["overflow"] is not False or manifest["truncated"] is not False or (
            manifest["terminal"] not in ("full_return", "selected_prefix")):
        raise ValueError("STOP_TERMINAL")
    if selected is None:
        raise ValueError("STOP_SELECTED_EVENT_MISSING")
    rebuilt = replay.finish(selected_prefix=manifest["terminal"] == "selected_prefix")
    if rebuilt.raw_jsonl != raw_jsonl or rebuilt.manifest != manifest:
        raise ValueError("STOP_MANIFEST_MISMATCH")
    return AuditResult("SOURCE_ONLY_" + manifest["terminal"].upper() + "_UNAUTHENTICATED",
                       replay.candidate_count, replay.selected_ordinal)
