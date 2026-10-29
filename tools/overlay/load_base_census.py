#!/usr/bin/env python3
"""Static census of constant segments assigned to CS in the recovered C models (the load-base defect class).

A far call in the original is `9A offset segment`, and the loader relocates the segment word, so a model that sets `CS := 00BAh`
(or any literal, or a macro defined as one) is right only at the unrelocated base. No differential sees it: the constant
equals the word at the default base, and a leaf callee's own model never looks at its entry CS. The segment must come from the
live word in the caller's code (or a base-relative parameter such as `load_segment`, or a difference of two live words).

POLICY (VP 2026-10-27): a constant `cs` assignment is allowed only as an EXACT SITE. The allowlist entry binds
(file, enclosing function, normalised statement) to a resolved VALUE, an occurrence count and a justification, and
`ALLOWED` records which of three classes it is:

  1. captured-runtime native model  - the segment is the runtime value of one captured run, so the constant is the contract;
  2. explicit load-zero composition - the body composes a model whose documented contract is load-zero only, and a guard
     pins the operand word before the assignment; the constant is the documented composition segment;
  3. unresolvable macro             - NEVER allowed implicitly: an uppercase identifier, or an object-like macro declared
     in an included project header, is reported by name when not resolved in-file. Header values are not evaluated.

A changed value, an extra assignment in an already-allowed file, a duplicated site, a site moved to another function, and a
missing expected site are all findings: the key is the site and the value, never the file alone, and silence is not a pass.
It reads no media and judges only the assignment; it is the cheap complement to the per-model spy tests, which prove a fixed
model really follows the word. See docs/re/LOAD_BASE_CENSUS.md.
"""
# NOT_A_ROW_CHECKER: static scan of C sources for constant CS assignments; it checks no registered row
import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ASSIGN = re.compile(
    r"((?:[A-Za-z_][A-Za-z0-9_]*)(?:\[[^\]]*\])?"
    r"(?:(?:->|\.)[A-Za-z_][A-Za-z0-9_]*(?:\[[^\]]*\])?)*(?:->|\.)cs)"
    r"\s*=\s*(0x[0-9A-Fa-f]+[uU]?|[A-Za-z_][A-Za-z0-9_]*)\s*[;,)]")
DEFINE = re.compile(r"^\s*#define\s+([A-Z][A-Z0-9_]*)\s+(0x[0-9A-Fa-f]+)[uU]?\b", re.M)

# Per-SITE allowlist: (file, enclosing function, normalised statement) -> (resolved value, occurrences, justification).
ALLOWED = {
    ("resident_464_2632_run014.c", "runtime_move", "cpu->cs = 0x131Cu;"):
        (0x131C, 1, "captured-runtime native model: 0x131C is that run's SYSTEM segment"),
    ("resident_464_2632_run014.c", "runtime_fill", "cpu->cs = 0x131Cu;"):
        (0x131C, 1, "captured-runtime native model: 0x131C is that run's SYSTEM segment"),
    ("slot_a_directory_fullstate.c", "pr_slot_a_directory_step", "c->cs = SYSTEM_SEG;"):
        (0x131C, 1, "captured-runtime saved-game Slot A model: SYSTEM_SEG is 0x131C in that file"),
    ("overlay_u6c_2033_cpu.c", "pr_ovl_006c_2033_cpu", "composed.cs = 0x00BAu;"):
        (0x00BA, 1, "explicit LOAD-ZERO composition of the accepted RES_00BA:0973 model: five-byte LCALL operand guard "
                    "9A 73 09 BA 00 at 2065h before this assignment; model introduced d7709238, merged #1610 49d14524"),
}


def site_text(line):
    """The allowlist key for one statement: whitespace collapsed."""
    return re.sub(r"\s+", " ", line).strip()


COMMENT = re.compile(r"/\*.*?\*/|//[^\n]*", re.S)
STRING = re.compile(r'"(?:\\.|[^"\\\n])*"|\'(?:\\.|[^\'\\\n])*\'')
HEADER = re.compile(r"^\s*(?:(?:static|inline|extern|const|unsigned|signed|void|int|char|long|short|float|double|"
                    r"size_t|ptrdiff_t|uint\d+_t|int\d+_t|pr_[A-Za-z0-9_]+|[A-Z][A-Za-z0-9_]*)[ \t\*]+)+"
                    r"([A-Za-z_][A-Za-z0-9_]*)\s*\(")
NON_FUNCTION = {"if", "for", "while", "switch", "do", "return", "sizeof", "else"}


def strip_noise(text):
    """Blank out comments and string/char literals while keeping newlines.

    Braces, semicolons and `cs =` text inside a comment or a string are not code: counting them would either close a
    function early or invent a site."""
    def blank(match):
        return "\n" * match.group(0).count("\n")
    return STRING.sub(blank, COMMENT.sub(blank, text))


INCLUDE = re.compile(r'^[ \t]*#[ \t]*include[ \t]*([<"])([^>"\n]+)[>"]', re.M)
OBJECT_MACRO = re.compile(r"^[ \t]*#[ \t]*define[ \t]+([A-Za-z_][A-Za-z0-9_]*)\b(?!\()", re.M)


def included_macro_names(path, text, root, cache):
    """Object-like macro names in literal project includes, regardless of case.

    This is conservative discovery, not preprocessing: do not evaluate header
    values, #if branches, #undef or include ordering. Follow quoted includes
    beside their includer, then project include/; angle includes use include/.
    Missing/external headers and computed includes are outside this scan.
    """
    root = Path(root).resolve()
    pending = [(path, text)]
    visited, names = set(), set()
    while pending:
        origin, contents = pending.pop()
        # Keep include filename strings; remove comment-only directives.
        clean = COMMENT.sub(lambda m: "\n" * m.group(0).count("\n"), contents)
        clean = clean.replace("\\\n", "")
        for delimiter, name in INCLUDE.findall(clean):
            candidates = ([origin.parent / name] if delimiter == '"' else []) + [root / "include" / name]
            for candidate in candidates:
                header = candidate.resolve()
                if not header.is_relative_to(root) or not header.is_file():
                    continue
                if header not in visited:
                    visited.add(header)
                    if header not in cache:
                        cache[header] = header.read_text(errors="replace")
                    contents = cache[header]
                    declarations = COMMENT.sub(lambda m: "\n" * m.group(0).count("\n"), contents)
                    names.update(OBJECT_MACRO.findall(declarations.replace("\\\n", "")))
                    pending.append((header, contents))
                break  # the first existing include candidate owns this name
    return names


HEADER_ANY = re.compile(
    r"(?:(?:static|inline|extern|const|unsigned|signed|void|int|char|long|short|float|double|size_t|ptrdiff_t|"
    r"uint\d+_t|int\d+_t|pr_[A-Za-z0-9_]+|[A-Z][A-Za-z0-9_]*)[ \t\*]+)+([A-Za-z_][A-Za-z0-9_]*)\s*\(")


def context_map(text):
    """(line -> incoming function, line -> [(char index, function after that char)]), position aware and fail-closed.

    The map is built per character, not per line, because a definition can start on the same physical line that closes
    the previous body (`} void f(...) { ... }`). An assignment is attributed to the context at its own position, so a
    mid-line boundary is respected; a name is cleared the moment a top-level body closes, and a site with no context is
    reported as unknown."""
    cleaned = strip_noise(text)
    incoming = {}
    marks = {}
    depth = 0
    current = None
    pending = None
    for number, line in enumerate(cleaned.splitlines(), 1):
        incoming[number] = current
        marks[number] = []
        segment = 0
        for index, char in enumerate(line):
            if char == "{":
                if depth == 0:
                    header = HEADER_ANY.search(line[segment:index])
                    current = header.group(1) if header else pending
                    pending = None
                depth += 1
                marks[number].append((index, current))
                segment = index + 1
            elif char == "}":
                depth = max(0, depth - 1)
                if depth == 0:
                    current = None
                marks[number].append((index, current))
                segment = index + 1
        if depth == 0 and "{" not in line:
            header = HEADER_ANY.search(line)
            if header:
                pending = header.group(1)
    return incoming, marks


def context_at(incoming, marks, number, position):
    context = incoming.get(number)
    for index, after in marks.get(number, ()):
        if index < position:
            context = after
        else:
            break
    return context


def census(root=ROOT):
    """(file, line, statement, resolved value or None, enclosing function) for every constant-cs assignment."""
    sites = []
    header_cache = {}
    for path in sorted((Path(root) / "src/recovered").glob("*.c")):
        text = path.read_text(errors="replace")
        cleaned = strip_noise(text)
        macros = {name: int(value, 16) for name, value in DEFINE.findall(cleaned)}
        header_macros = None
        incoming, marks = context_map(text)
        for number, code in enumerate(cleaned.splitlines(), 1):
            for match in ASSIGN.finditer(code):
                raw = match.group(2)
                if raw.lower().startswith("0x"):
                    value = int(raw.rstrip("uU"), 16)
                elif re.fullmatch(r"[A-Z][A-Z0-9_]*", raw):
                    value = macros.get(raw)          # None == unresolvable (header-defined) macro: still a finding
                else:
                    if header_macros is None:
                        header_macros = included_macro_names(path, text, root, header_cache)
                    if raw not in header_macros:
                        continue                      # no evidence this identifier is a macro
                    value = None                      # header macro: visible, never implicitly allowlisted
                sites.append((path.name, number, site_text(match.group(1) + " = " + raw + ";"), value,
                              context_at(incoming, marks, number, match.start())))
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


def check(root=ROOT):
    """The policy over one tree: (printed rows, set of offending site keys, site count)."""
    sites = census(root)
    counts = {}
    for name, _, statement, value, function in sites:
        key = (name, function, statement)
        counts[key] = counts.get(key, 0) + 1
    bad = set()
    rows = []
    for name, number, statement, value, function in sites:
        key = (name, function, statement)
        entry = ALLOWED.get(key)
        allowed = entry is not None and entry[0] == value and entry[1] == counts[key]
        reason = ""
        if function is None:
            reason = " [unknown function context]"
        elif value is None:
            reason = f" [unresolved macro {statement.split('=')[-1].strip().rstrip(';')}]"
        elif entry is None:
            reason = " [not an allowed site]"
        elif entry[0] != value:
            reason = f" [value 0x{value:04X} != allowed 0x{entry[0]:04X}]"
        elif entry[1] != counts[key]:
            reason = f" [occurrences {counts[key]} != allowed {entry[1]}]"
        if not allowed:
            bad.add(key)
        rows.append(f"{'ALLOWED' if allowed else 'CONSTANT-CS'} {name}:{number} ({function}): {statement[:90]}{reason}")
    for key, (value, want, why) in sorted(ALLOWED.items()):
        if counts.get(key, 0) != want:
            bad.add(key)
            rows.append(f"COUNT {key[0]} ({key[1]}): expected {want} of '{key[2][:70]}', found {counts.get(key, 0)}")
    return rows, bad, len(sites)


def main():
    if "--classify" in sys.argv:
        counts = classify()
        for kind, rows in counts.items():
            print(kind, len(rows), "sites in", len({r[0] for r in rows}), "files")
        return 0
    rows, bad, total = check()
    print("\n".join(rows))
    print(f"{total} constant-CS assignments, {len(bad)} not allowed")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
