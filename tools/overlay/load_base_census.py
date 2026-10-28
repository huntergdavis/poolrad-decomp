#!/usr/bin/env python3
"""Static census of constant segments assigned to CS in the recovered C models (the load-base defect class).

A far call in the original is `9A offset segment`, and the loader relocates the segment word, so a model that sets `CS := 00BAh`
(or any literal, or a macro defined as one) is right only at the unrelocated base. No differential sees it: the constant
equals the word at the default base, and a leaf callee's own model never looks at its entry CS. The segment must come from the
live word in the caller's code (or a base-relative parameter such as `load_segment`, or a difference of two live words).

POLICY (VP 2026-10-27): a constant `cs` assignment is allowed only as an EXACT SITE: the `(file, normalised statement)`
pair, its resolved value, and its occurrence count must all match an entry below. The allowlist is keyed by site, not by
filename, so in an already-allowed file a changed value, an extra assignment, a duplicate or a missing expected site is a
finding. A hex literal and an uppercase identifier are both candidates: a macro name defined in a header the scanner never
reads must be REPORTED (as an unresolvable constant), never silently ignored, so a named constant cannot hide an expected
site. It reads no media and judges only the assignment; it is the cheap complement to the per-model spy tests, which prove
a fixed model really follows the word. See docs/re/LOAD_BASE_CENSUS.md.
"""
# NOT_A_ROW_CHECKER: static scan of C sources for constant CS assignments; it checks no registered row
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ASSIGN = re.compile(r"([A-Za-z_][A-Za-z0-9_]*(?:->|\.)cs)\s*=\s*(0x[0-9A-Fa-f]+[uU]?|[A-Z][A-Z0-9_]*)\s*[;,)]")
DEFINE = re.compile(r"^\s*#define\s+([A-Z][A-Z0-9_]*)\s+(0x[0-9A-Fa-f]+)[uU]?\b", re.M)

# Per-SITE allowlist: (file name, normalised statement) -> (expected occurrences, justification).
# The key is the site, not the file, so an extra or changed constant in an allowed file is still reported.
#   resident_464_2632_run014.c / slot_a_directory_fullstate.c: capture models, whose segments are the runtime
#     values of ONE captured run (a saved game's loaded segments), not loader-relative words, so the constant is
#     the contract there.
#   overlay_u6c_2033_cpu.c: LOAD-ZERO COMPOSITION of the accepted RES_00BA:0973 model. The 006C model and the
#     0973 contract are load-zero only, and the C guard requires the five-byte LCALL operand 9A 73 09 BA 00 at
#     2065h before this assignment, so a relocated load is refused rather than silently composed. Arrived with
#     d7709238 ("Model OVL_006C_2033 ... (t182 step 2)"), merged as #1610 49d14524.
# Removing a site is better than listing it: an entry exists only where the constant is the documented contract.
ALLOWED = {
    ("resident_464_2632_run014.c", "cpu->cs = 0x131Cu;"):
        (2, "native capture of one runtime (0x131C is that run's SYSTEM segment)"),
    ("slot_a_directory_fullstate.c", "c->cs = SYSTEM_SEG;"):
        (1, "saved-game Slot A capture: SYSTEM_SEG 0x131C is the captured run's segment"),
    ("overlay_u6c_2033_cpu.c", "composed.cs = 0x00BAu;"):
        (1, "load-zero composition of the accepted RES_00BA:0973 model; five-byte LCALL operand guard at 2065h; "
            "lineage d7709238 / #1610 49d14524"),
}


def site_text(statement):
    """The allowlist key for one statement: whitespace collapsed."""
    return re.sub(r"\s+", " ", statement).strip()


def census(root=ROOT):
    sites = []
    for path in sorted((Path(root) / "src/recovered").glob("*.c")):
        text = path.read_text(errors="replace")
        for number, line in enumerate(text.splitlines(), 1):
            code = re.sub(r"/\*.*?\*/", "", line.split("//")[0])
            for match in ASSIGN.finditer(code):
                # A hex literal, or ANY uppercase identifier: a macro name defined in this file, or one defined
                # in a header the scanner never reads. Not resolving the latter is the header-macro blind spot.
                value = match.group(2)
                if value.lower().startswith("0x") or re.fullmatch(r"[A-Z][A-Z0-9_]*", value):
                    sites.append((path.name, number, line.strip(), re.sub(r"\s+", " ", match.group(0)).strip()))
    return sites


IDENT = re.compile(r"(?:->|\.)cs\s*=\s*([a-z_][a-z0-9_]*)\s*[;,)]")
READ = re.compile(r"\b(read|rd|word|get|at|peek|load)[a-z0-9_]*\s*\(", re.I)
SKIP = re.compile(r"(caller|source|return|ret_|saved|pop|stub|frame)")


def classify(root=ROOT):
    """HEURISTIC split of the non-literal assignments `cs = ident`: LIVE (ident was read from RAM a few lines up), PARAM (ident is a
    parameter of the enclosing function: the host chooses the segment and the model ignores the code word), OTHER (not decided).
    PARAM is the load-base class one step removed: it is right whenever the host passes what the loader put in the word."""
    counts = {"LIVE": [], "PARAM": [], "OTHER": []}
    for path in sorted((Path(root) / "src/recovered").glob("*.c")):
        lines = path.read_text(errors="replace").splitlines()
        for i, line in enumerate(lines):
            code = re.sub(r"/\*.*?\*/", "", line.split("//")[0])
            for match in IDENT.finditer(code):
                ident = match.group(1)
                if SKIP.search(ident) or ident in ("cs", "c"):
                    continue
                kind = "OTHER"
                for j in range(i, max(-1, i - 80), -1):
                    if re.search(rf"\b{ident}\b\s*=[^=]", lines[j]) and READ.search(lines[j]):
                        kind = "LIVE"
                        break
                    if re.match(r"^[A-Za-z_].*\(", lines[j]) and re.search(rf"\b{ident}\b", lines[j]) and not lines[j].strip().endswith(";"):
                        kind = "PARAM"
                        break
                if kind == "OTHER":
                    for j in range(i, max(-1, i - 120), -1):
                        if re.search(rf"\b(uint16_t|unsigned|int)\s+{ident}\b", lines[j]) and lines[j].rstrip().endswith((",", ")", "{")):
                            kind = "PARAM"
                            break
                counts[kind].append((path.name, i + 1, ident))
    return counts


def main():
    if "--classify" in sys.argv:
        counts = classify()
        for kind, rows in counts.items():
            print(kind, len(rows), "sites in", len({r[0] for r in rows}), "files")
        return 0
    sites = census()
    counts = {}
    for name, _, _, statement in sites:
        key = (name, site_text(statement))
        counts[key] = counts.get(key, 0) + 1
    bad = []
    for name, number, line, statement in sites:
        key = (name, site_text(statement))
        entry = ALLOWED.get(key)
        allowed = entry is not None and entry[0] == counts[key]
        if not allowed:
            bad.append(key)
        print(f"{'ALLOWED' if allowed else 'CONSTANT-CS'} {name}:{number}: {line[:110]}")
    for (name, text), (want, why) in sorted(ALLOWED.items()):
        if counts.get((name, text), 0) != want:
            # A missing or duplicated expected site is a finding too: silence is not a pass.
            bad.append((name, text))
            print(f"COUNT {name}: expected {want} of '{text[:70]}', found {counts.get((name, text), 0)}")
    bad = sorted(set(bad))
    print(f"{len(sites)} constant-CS assignments, {len(bad)} not allowed")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
