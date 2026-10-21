#!/usr/bin/env python3
"""Static census of constant segments assigned to CS in the recovered C models (the load-base defect class).

A far call in the original is `9A offset segment`, and the loader relocates the segment word, so a model that sets `CS := 00BAh`
(or any literal, or a macro defined as one) is right only at the unrelocated base. No differential sees it: the constant
equals the word at the default base, and a leaf callee's own model never looks at its entry CS. The segment must come from the
live word in the caller's code (or a base-relative parameter such as `load_segment`, or a difference of two live words).

This scans every `src/recovered/*.c` for an assignment to a `cs` field whose right side is a hex literal or a macro defined as
one, and fails closed on any site not in the allowlist below. It reads no media and judges only the assignment; it is the
cheap complement to the per-model spy tests, which prove a fixed model really follows the word. See
docs/re/LOAD_BASE_CENSUS.md.
"""
# NOT_A_ROW_CHECKER: static scan of C sources for constant CS assignments; it checks no registered row
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ASSIGN = re.compile(r"(?:->|\.)cs\s*=\s*(0x[0-9A-Fa-f]+[uU]?|[A-Z][A-Z0-9_]*)\s*[;,)]")
DEFINE = re.compile(r"^\s*#define\s+([A-Z][A-Z0-9_]*)\s+(0x[0-9A-Fa-f]+)[uU]?\b", re.M)

# Native-capture models: their segments are the runtime values of ONE captured run (a saved game's loaded segments), not
# loader-relative words, so a constant is the contract there. Anything else must read the word.
ALLOWED = {
    "resident_464_2632_run014.c": "native capture of one runtime (0x131C is that run's SYSTEM segment)",
    "slot_a_directory_fullstate.c": "saved-game Slot A capture: SYSTEM_SEG 0x131C is the captured run's segment",
}


def census(root=ROOT):
    sites = []
    for path in sorted((Path(root) / "src/recovered").glob("*.c")):
        text = path.read_text(errors="replace")
        macros = {name for name, _ in DEFINE.findall(text)}
        for number, line in enumerate(text.splitlines(), 1):
            code = re.sub(r"/\*.*?\*/", "", line.split("//")[0])
            for match in ASSIGN.finditer(code):
                value = match.group(1)
                if value.lower().startswith("0x") or value in macros:
                    sites.append((path.name, number, line.strip()))
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
    bad = [s for s in sites if s[0] not in ALLOWED]
    for name, number, line in sites:
        print(f"{'ALLOWED' if name in ALLOWED else 'CONSTANT-CS'} {name}:{number}: {line[:110]}")
    print(f"{len(sites)} constant-CS assignments, {len(bad)} not allowed")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
