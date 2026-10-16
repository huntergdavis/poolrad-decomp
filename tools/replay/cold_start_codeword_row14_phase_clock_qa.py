"""Synthetic U19 phase-clock QA contract; current producers lack this API.

An eventual common host-clock hook must tag each sample with its actual
phase. This checker cannot authenticate a hook or infer a clock from guest
state. It deliberately compares only matching phases of the same event.
"""

from dataclasses import dataclass
import re


PHASES = frozenset(("latch_enter", "selected_store"))
TOKEN = re.compile(r"[A-Za-z0-9_-]{1,64}\Z")


@dataclass(frozen=True)
class PhaseClockSample:
    run_token: str
    plane_event_id: str
    arm_token: str
    store_ordinal: int
    phase: str
    host_ns: int


def check_phase_pair(expected_phase: str, host: PhaseClockSample,
                     passive: PhaseClockSample) -> None:
    """Require identical samples from one named hook, never cross-phase reuse."""
    if expected_phase not in PHASES:
        raise ValueError("unknown clock phase")
    if host.phase != expected_phase or passive.phase != expected_phase:
        raise ValueError("cross-phase clock sample")
    host_key = (host.run_token, host.plane_event_id, host.arm_token,
                host.store_ordinal)
    passive_key = (passive.run_token, passive.plane_event_id,
                   passive.arm_token, passive.store_ordinal)
    if (host_key != passive_key or
            any(not isinstance(value, str) or TOKEN.fullmatch(value) is None
                for value in host_key[:3]) or
            type(host.store_ordinal) is not int or host.store_ordinal < 1 or
            type(passive.store_ordinal) is not int):
        raise ValueError("phase-clock event key differs")
    if (type(host.host_ns) is not int or host.host_ns < 0 or
            type(passive.host_ns) is not int or passive.host_ns < 0 or
            host.host_ns != passive.host_ns):
        raise ValueError("same-phase common clock differs")


def check_latch_and_store(host_latch: PhaseClockSample,
                          passive_latch: PhaseClockSample,
                          host_store: PhaseClockSample,
                          passive_store: PhaseClockSample) -> str:
    """Exercise two actual-hook phase slots; no current live producer call."""
    check_phase_pair("latch_enter", host_latch, passive_latch)
    check_phase_pair("selected_store", host_store, passive_store)
    if (host_latch.run_token, host_latch.plane_event_id,
            host_latch.arm_token, host_latch.store_ordinal) != (
            host_store.run_token, host_store.plane_event_id,
            host_store.arm_token, host_store.store_ordinal):
        raise ValueError("phase-clock event key differs")
    return "SYNTHETIC_PHASE_CONTRACT_NO_LIVE_HOOK"
