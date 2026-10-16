"""Offline consistency checks for supplied U19 plane/frame/RGB observations.

This module neither captures nor authenticates native events. Matching tokens,
state declarations and ordering do not prove breakpoint coverage or C parity.
"""

from dataclasses import dataclass
from hashlib import sha256
import re

from tools.replay.cold_start_codeword_row14_target_filter import (
    filter_frames, parse_frame,
)
from tools.replay.cold_start_codeword_row14_plane_event_audit import (
    WINDOW_BYTES, WITNESS_AT, WITNESS_MASK,
)


def require(condition, message):
    if not condition:
        raise ValueError(message)


@dataclass(frozen=True)
class CaptureState:
    latch: int
    gc_index: int
    read_map: int
    seq_index: int
    map_mask: int
    memory_mode: int


@dataclass(frozen=True)
class PageIdentity:
    crtc_start: int
    display_start: int
    real_start: int
    palette_sha256: str


@dataclass(frozen=True)
class PlaneObservation:
    run_token: str
    event_index: int
    pc: str
    planes: tuple[bytes, ...]
    state_before: CaptureState
    state_after: CaptureState
    page: PageIdentity
    method: str = "host-internal"
    read_failures: int = 0


@dataclass(frozen=True)
class PresentedFrame:
    run_token: str
    frame_id: int
    start_event: int
    backend_event: int
    plane_event: int
    page: PageIdentity
    backend_completed: bool


@dataclass(frozen=True)
class RGB24Observation:
    run_token: str
    event_index: int
    frame_id: int
    page: PageIdentity
    width: int
    height: int
    row_stride: int
    pixels: bytes
    state_before: CaptureState
    state_after: CaptureState


@dataclass(frozen=True)
class RGB24Patch:
    """Page-neutral, tightly packed RGB24 patch; x/y are inclusive origins."""

    x: int
    y: int
    width: int
    height: int
    pixels: bytes


@dataclass(frozen=True)
class VisualWitness:
    changed_planes: tuple[int, ...]
    changed_witness_bits: tuple[int, ...]
    rgb_sha256: str


def passive(before, after):
    require(isinstance(before, CaptureState) and isinstance(after, CaptureState),
            "missing passive observation state")
    for state in (before, after):
        for value, maximum in zip(
                (state.latch, state.gc_index, state.read_map, state.seq_index,
                 state.map_mask, state.memory_mode),
                (0xFFFFFFFF, 15, 3, 255, 15, 255)):
            require(type(value) is int and 0 <= value <= maximum,
                    "invalid passive observation state")
    require(before == after, "observation changed latch or controller state")


def page_identity(page):
    require(isinstance(page, PageIdentity), "missing page identity")
    require(all(type(n) is int and 0 <= n <= 0xFFFFFFFF for n in
                (page.crtc_start, page.display_start, page.real_start)),
            "invalid page identity")
    require(isinstance(page.palette_sha256, str) and
            re.fullmatch(r"[0-9a-f]{64}", page.palette_sha256),
            "invalid palette identity")


def rgb_layout(rgb):
    require(all(type(n) is int and n > 0 for n in
                (rgb.width, rgb.height, rgb.row_stride)) and
            rgb.row_stride >= rgb.width * 3 and
            isinstance(rgb.pixels, bytes) and
            len(rgb.pixels) == rgb.row_stride * rgb.height,
            "invalid RGB24 layout")


def audit_visual_witness(cpu_doc, before, after, frame, rgb):
    """Join supplied selected-store planes to a later completed host frame.

    All event indices use one run-wide monotonic clock. The frame records the
    plane observation it claims to use. A real collector must separately prove
    that linkage, passive host access and actual backend completion.
    """
    require(isinstance(cpu_doc, dict) and cpu_doc.get("version") == 1,
            "unsupported CPU frame document")
    token = cpu_doc.get("run_token")
    require(isinstance(token, str) and
            re.fullmatch(r"[A-Za-z0-9_-]{1,64}", token), "invalid run token")
    require(isinstance(before, PlaneObservation) and
            isinstance(after, PlaneObservation) and
            isinstance(frame, PresentedFrame) and
            isinstance(rgb, RGB24Observation), "missing typed observation")
    require(all(record.run_token == token for record in
                (before, after, frame, rgb)), "same-run token mismatch")
    records = cpu_doc.get("frames")
    require(isinstance(records, list) and len(records) <= 130,
            "CPU frame stream missing or above bounded stop cap")
    selected = filter_frames([parse_frame(record) for record in records])
    require(selected.status == "PAIRED_CPU_SOURCE_ONLY", selected.status)
    require(before.pc == f"0F2D:{selected.target_pc:04X}" and
            after.pc == f"0F2D:{selected.next_pc:04X}",
            "planes do not match selected store and successor")
    for sample in (before, after):
        require(sample.method == "host-internal" and
                type(sample.read_failures) is int and sample.read_failures == 0,
                "nonperturbing successful plane observation required")
        require(isinstance(sample.planes, tuple) and len(sample.planes) == 4 and
                all(isinstance(p, bytes) and len(p) == WINDOW_BYTES
                    for p in sample.planes), "four complete plane windows required")
        passive(sample.state_before, sample.state_after)
        page_identity(sample.page)
    passive(rgb.state_before, rgb.state_after)
    page_identity(frame.page)
    page_identity(rgb.page)
    rgb_layout(rgb)
    require(all(type(n) is int and n > 0 for n in
                (before.event_index, after.event_index, frame.start_event,
                 frame.backend_event, frame.plane_event, rgb.event_index,
                 frame.frame_id, rgb.frame_id)), "invalid event or frame identity")
    require(before.event_index < after.event_index < frame.start_event <
            frame.backend_event < rgb.event_index,
            "store, frame, backend and RGB events out of order")
    require(frame.backend_completed is True, "backend completion missing")
    require(frame.plane_event == after.event_index and
            rgb.frame_id == frame.frame_id, "plane/frame/RGB identity mismatch")
    require(after.page == frame.page == rgb.page,
            "plane/frame/RGB page or palette mismatch")
    changed = tuple(p for p in range(4)
                    if before.planes[p][WITNESS_AT] != after.planes[p][WITNESS_AT])
    bits = tuple(p for p in changed if
                 (before.planes[p][WITNESS_AT] ^ after.planes[p][WITNESS_AT]) &
                 WITNESS_MASK)
    return VisualWitness(changed, bits, sha256(rgb.pixels).hexdigest())


def compare_patch(rgb, patch):
    """Count differing pixels in an independent expected patch, ignoring padding."""
    require(isinstance(rgb, RGB24Observation) and isinstance(patch, RGB24Patch),
            "typed RGB24 observation and patch required")
    rgb_layout(rgb)
    require(all(type(n) is int for n in
                (patch.x, patch.y, patch.width, patch.height)) and
            min(patch.x, patch.y) >= 0 and min(patch.width, patch.height) > 0 and
            patch.x + patch.width <= rgb.width and
            patch.y + patch.height <= rgb.height and
            isinstance(patch.pixels, bytes) and
            len(patch.pixels) == patch.width * patch.height * 3,
            "RGB24 patch outside host bounds or invalid length")
    differences = 0
    for y in range(patch.height):
        host = (patch.y + y) * rgb.row_stride + patch.x * 3
        expected = y * patch.width * 3
        differences += sum(
            rgb.pixels[host + x * 3:host + x * 3 + 3] !=
            patch.pixels[expected + x * 3:expected + x * 3 + 3]
            for x in range(patch.width))
    return differences
