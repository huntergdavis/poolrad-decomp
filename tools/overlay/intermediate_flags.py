"""Exact intermediate FLAGS observations and fail-closed mutation witnesses.

An audit supplies reviewed observation sites before each relevant flag overwrite.
This module does not discover all arithmetic producers or instrument C implicitly.
Record C state before invoking a callback; install the Unicorn hook before any
hook which substitutes that callback. Final CPU/RAM comparison remains mandatory.
"""
from dataclasses import dataclass

ARITH = 0x08D5


class FlagObservationError(ValueError):
    pass


@dataclass(frozen=True)
class Event:
    site: str
    flags: int


class Trace:
    """Bounded recorder safe inside ctypes callbacks (errors retained, not raised)."""

    def __init__(self, sites, limit=100000):
        self.sites = frozenset(sites)
        if not self.sites or any(not isinstance(s, str) or not s for s in self.sites):
            raise FlagObservationError("nonempty named observation sites required")
        if limit < 1:
            raise FlagObservationError("positive event limit required")
        self.limit = limit
        self.events = []
        self.error = None

    def record(self, site, flags):
        if self.error is not None:
            return
        if site not in self.sites:
            self.error = f"undeclared observation site: {site}"
        elif not isinstance(flags, int) or not 0 <= flags <= 0xFFFF:
            self.error = f"invalid 16-bit FLAGS at {site}: {flags}"
        elif len(self.events) >= self.limit:
            self.error = f"observation limit {self.limit} exceeded"
        else:
            self.events.append(Event(site, flags))

    def checked(self):
        if self.error:
            raise FlagObservationError(self.error)
        return tuple(self.events)


def hook_unicorn(uc, trace, points):
    """Record FLAGS before instructions at {(CS, IP): site}; return hook handle.

    Match logical CS:IP as well as the physical address. A physical alias in a
    different segment must not impersonate a reviewed callback/watchpoint.
    """
    from unicorn import UC_HOOK_CODE
    from unicorn.x86_const import UC_X86_REG_CS, UC_X86_REG_IP, UC_X86_REG_EFLAGS

    points = dict(points)
    if set(points.values()) != trace.sites:
        raise FlagObservationError("every declared site needs a Unicorn watchpoint")
    for cs, ip in points:
        if not 0 <= cs <= 0xFFFF or not 0 <= ip <= 0xFFFF:
            raise FlagObservationError("watchpoint outside 16-bit CS:IP")

    def observe(emulator, address, _size, _context):
        cs = emulator.reg_read(UC_X86_REG_CS) & 0xFFFF
        ip = emulator.reg_read(UC_X86_REG_IP) & 0xFFFF
        site = points.get((cs, ip))
        if site is not None and address == ((cs << 4) + ip) & 0xFFFFF:
            trace.record(site, emulator.reg_read(UC_X86_REG_EFLAGS) & 0xFFFF)

    return uc.hook_add(UC_HOOK_CODE, observe)


def compare(original, candidate, label="case"):
    want, got = original.checked(), candidate.checked()
    if original.sites != candidate.sites:
        raise FlagObservationError(f"{label}: observation declarations differ")
    if len(want) != len(got):
        raise FlagObservationError(f"{label}: observation count {len(want)} != {len(got)}")
    for index, (left, right) in enumerate(zip(want, got)):
        if left != right:
            raise FlagObservationError(
                f"{label}: event {index}: {left.site} FLAGS={left.flags:04X} "
                f"!= {right.site} FLAGS={right.flags:04X}")
    return len(want)


class Contract:
    """Require a passing corpus and a real wrong-FLAGS witness per declared site.

    Corpus values are (original_trace, candidate_trace). A mutation must use the
    same cases and original observations, preserve event order/count, and flip
    exactly ARITH at the requested site. Missing callbacks, compile failures,
    crashes and unrelated final-state differences cannot count as a flag kill.
    """

    def __init__(self, sites, baseline):
        self.sites = frozenset(sites)
        if not self.sites or not baseline:
            raise FlagObservationError("nonempty sites and baseline corpus required")
        self.baseline = {}
        coverage = {site: 0 for site in self.sites}
        for label, (original, candidate) in baseline.items():
            if original.sites != self.sites or candidate.sites != self.sites:
                raise FlagObservationError(f"{label}: baseline site declarations differ")
            compare(original, candidate, label)
            self.baseline[label] = original.checked()
            for event in original.checked():
                coverage[event.site] += 1
        missing = [site for site, count in coverage.items() if not count]
        if missing:
            raise FlagObservationError(f"unobserved required sites: {sorted(missing)}")
        self.coverage = coverage
        self.controls = {}

    def require_wrong_flags(self, site, mutant):
        self.controls.pop(site, None)
        if site not in self.sites:
            raise FlagObservationError(f"undeclared mutation site: {site}")
        if set(mutant) != set(self.baseline):
            raise FlagObservationError("mutant must run the complete baseline corpus")
        killed, changed = [], 0
        for label, (original, candidate) in mutant.items():
            want, got = original.checked(), candidate.checked()
            if original.sites != self.sites or candidate.sites != self.sites:
                raise FlagObservationError(f"{label}: mutant site declarations differ")
            if want != self.baseline[label]:
                raise FlagObservationError(f"{label}: mutant changed the original oracle")
            if len(want) != len(got) or any(a.site != b.site for a, b in zip(want, got)):
                raise FlagObservationError(f"{label}: missing/reordered events are not a flag kill")
            differences = [(a, b) for a, b in zip(want, got) if a != b]
            for left, right in differences:
                if left.site != site or left.flags ^ right.flags != ARITH:
                    raise FlagObservationError(f"{label}: wrong-FLAGS control has unrelated differences")
            if differences:
                killed.append(label)
                changed += len(differences)
        if not killed:
            raise FlagObservationError(f"{site}: wrong-FLAGS mutant survived every case")
        self.controls[site] = {"cases": len(mutant), "killed": killed, "events": changed}
        return self.controls[site]

    def receipt(self):
        missing = self.sites - self.controls.keys()
        if missing:
            raise FlagObservationError(f"missing wrong-FLAGS controls: {sorted(missing)}")
        return {"cases": len(self.baseline), "coverage": self.coverage,
                "controls": self.controls, "flag_masks": 0}
