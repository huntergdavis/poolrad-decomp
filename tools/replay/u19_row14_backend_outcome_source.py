"""Source-site classification of supplied U19 backend observations.

This pure synthetic model neither hooks DOSBox-X nor authenticates a supplied
completion witness. Its completion result is conditional on an independently
sourced same-frame witness that no current producer provides.
"""

from dataclasses import dataclass
from enum import Enum
import re


class Backend(str, Enum):
    SDL2_SURFACE = "sdl2_surface"
    SDL1_SURFACE = "sdl1_surface"
    OPENGL = "opengl"


class Outcome(str, Enum):
    NO_OUTPUT = "no_output"
    ATTEMPT_UNVERIFIED = "attempt_unverified"
    COMPLETION_VERIFIED = "completion_verified"


# Pinned DOSBox-X revision 5a9c7c5. Values are original source line numbers.
CALL_SITES = {
    Backend.SDL2_SURFACE: frozenset((687, 716, 737, 744, 771)),
    Backend.SDL1_SURFACE: frozenset((689, 718, 739, 746, 773)),
    Backend.OPENGL: frozenset((1026, 1132)),
}
RECT_SITES = frozenset(((Backend.SDL2_SURFACE, 771),
                        (Backend.SDL1_SURFACE, 773)))
VOID_SITES = frozenset(((Backend.SDL1_SURFACE, 773),
                        (Backend.OPENGL, 1026),
                        (Backend.OPENGL, 1132)))
TOKEN = re.compile(r"[A-Za-z0-9_-]{1,64}\Z")


@dataclass(frozen=True)
class Attempt:
    run_token: str
    frame_id: int
    backend_event_id: str | None
    backend: Backend
    site: int | None
    call_reached: bool
    selected_new_frame: bool
    render_aborted: bool
    rect_count: int | None = None
    api_result: int | None = None


@dataclass(frozen=True)
class CompletionWitness:
    run_token: str
    frame_id: int
    backend_event_id: str
    backend: Backend
    site: int
    source_domain: str
    verified: bool


def _valid_token(value: object) -> bool:
    return type(value) is str and TOKEN.fullmatch(value) is not None


def classify_backend_outcome(
        attempt: Attempt,
        witness: CompletionWitness | None = None) -> Outcome:
    """Classify supplied facts; this does not validate the witness's origin."""
    if (not isinstance(attempt, Attempt) or
            not _valid_token(attempt.run_token) or
            type(attempt.frame_id) is not int or attempt.frame_id < 1 or
            type(attempt.backend) is not Backend or
            type(attempt.call_reached) is not bool or
            type(attempt.selected_new_frame) is not bool or
            type(attempt.render_aborted) is not bool):
        raise ValueError("invalid backend attempt identity or state")
    if (attempt.site is not None and
            (type(attempt.site) is not int or
             attempt.site not in CALL_SITES[attempt.backend])):
        raise ValueError("call site does not belong to selected backend")
    rect_site = (attempt.backend, attempt.site) in RECT_SITES
    if (attempt.rect_count is not None and
            (type(attempt.rect_count) is not int or
             attempt.rect_count < 0 or not rect_site)):
        raise ValueError("invalid rectangle count for call site")
    if (attempt.api_result is not None and
            (type(attempt.api_result) is not int or
             not attempt.call_reached or
             (attempt.backend, attempt.site) in VOID_SITES)):
        raise ValueError("API result absent or void at this call site")
    if attempt.call_reached and attempt.site is None:
        raise ValueError("reached call lacks a source site")
    if rect_site and attempt.call_reached and (
            attempt.rect_count is None or attempt.rect_count == 0):
        raise ValueError("rectangle call cannot be reached with zero rectangles")

    no_output = (not attempt.selected_new_frame or attempt.render_aborted or
                 not attempt.call_reached or
                 (rect_site and attempt.rect_count == 0))
    if no_output:
        if witness is not None:
            raise ValueError("completion witness supplied for no output")
        if attempt.backend_event_id is not None:
            raise ValueError("backend event ID minted for no output")
        return Outcome.NO_OUTPUT

    if not _valid_token(attempt.backend_event_id):
        raise ValueError("reached outcome lacks backend event ID")
    if witness is None:
        return Outcome.ATTEMPT_UNVERIFIED
    if (not isinstance(witness, CompletionWitness) or
            not _valid_token(witness.run_token) or
            not _valid_token(witness.backend_event_id) or
            type(witness.frame_id) is not int or witness.frame_id < 1 or
            type(witness.backend) is not Backend or
            type(witness.site) is not int or
            type(witness.source_domain) is not str or
            type(witness.verified) is not bool or
            witness.source_domain != "independent_backend_completion" or
            (witness.run_token, witness.frame_id, witness.backend_event_id,
             witness.backend, witness.site) !=
            (attempt.run_token, attempt.frame_id, attempt.backend_event_id,
             attempt.backend, attempt.site)):
        raise ValueError("independent same-frame completion witness missing")
    if attempt.api_result is not None and attempt.api_result != 0:
        if witness.verified:
            raise ValueError("completion contradicts failed API result")
        return Outcome.ATTEMPT_UNVERIFIED
    if not witness.verified:
        return Outcome.ATTEMPT_UNVERIFIED
    return Outcome.COMPLETION_VERIFIED
