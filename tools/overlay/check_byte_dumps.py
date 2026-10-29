"""Fail when tracked documentation carries verbatim original-byte dumps.

The owner's rule is that public tracked files carry addresses, counts and hashes
only - never original game bytes. A document that pastes a span's bytes hands out
the media it claims to describe, and no other check notices: the emitter gate
compares generated sources, the provenance guard checks commit ids, and the
record guard checks links.

What counts as a dump

  * a fenced block whose info string names bytes (` ```bytes `);
  * an xxd/objdump column: an offset prefix followed by eight or more byte pairs;
  * a run of eight or more **space- or tab-separated** byte pairs that looks like
    bytes (at least one pair carries a hex letter or has the high bit set).

What does not, and why

  * Runs of two-digit decimal numbers below 0x80 (`17 18 19 20 ...`):
    indistinguishable from hex pairs, and this repository writes function-number
    lists that way. The high-bit/hex-letter rule keeps them out of the failure set.
  * `0x02, 0x0A, 0x0E, ...` enumerations are NOT exempt: syntax cannot tell a
    semantic opcode set from original bytes reformatted with `0x` and commas, so
    they fail by default and each one needs a row in the allowlist whose reason
    names that file's actual subject.
  * Scope is `*.md`. C sources legitimately initialise byte arrays, so they are
    out of scope by path, not silently skipped - a dump hiding in a .c file is a
    different review finding, not this check's.
  * Byte pairs must be separated. A contiguous hex blob is indistinguishable from
    a hash or a git id, so it is not flagged; hashes are what the rule asks for.
  * It proves no dump-shaped text is present, not that no original content is: a
    hex table without separators, a base64 paste or an image are outside it.

Allowlist: `docs/re/BYTE_DUMP_ALLOWLIST.tsv`, one `path<TAB>task<TAB>justification`
line per file, reviewed like code. The row must name the task that removes it
(`t139`, for example), the justification must carry an expiry marker, and an entry
that no longer hits only warns - so a scrub PR and this check can land in either
order. When the naming task is done while rows remain, the checker warns: a
permission whose reason has expired is a permission nobody is watching. The task
lookup uses the Dunking Sheep CLI when it is present and is skipped in CI, which
has no daemon; `--task-status` overrides it for tests.

Exit status: 0 clean (or --report), 1 unallowlisted hits or allowlist error.
"""
from __future__ import annotations
def verify_evidence(base="main", head="HEAD"):
    import subprocess, hashlib
    out = subprocess.check_output(["git","diff","--name-only",f"{base}..{head}"], text=True)
    files = [f for f in out.split() if f.startswith("docs/re/") and f.endswith(".md")]
    ok = True
    for f in files:
        try:
            pre = subprocess.check_output(["git","show",f"{base}:{f}"], text=True, stderr=subprocess.DEVNULL)
        except subprocess.CalledProcessError:
            continue
        post = subprocess.check_output(["git","show",f"{head}:{f}"], text=True, stderr=subprocess.DEVNULL)
        runs = []
        for m in BARE.finditer(pre):
            run = m.group(1)
            if looks_like_bytes(run):
                runs.append(bytes.fromhex(run.replace(" ","").replace("\t","")))
        markers = re.findall(r"\[scrubbed: span=\S+ sha256=([0-9a-f]{64}) bytes=(\d+)", post)
        if not markers:
            if runs:
                print(f"MISSING MARKERS {f}: {len(runs)} pre-image run(s) but no scrubbed markers")
                ok = False
            continue
        if len(markers) != len(runs):
            print(f"MISMATCH count {f}: markers={len(markers)} runs={len(runs)}")
            ok = False
            continue
        for (h, n), run in zip(markers, runs):
            if int(n) != len(run) or h != hashlib.sha256(run).hexdigest():
                print(f"MISMATCH {f}: marker bytes={n}/hash={h[:12]} vs run bytes={len(run)}/hash={hashlib.sha256(run).hexdigest()[:12]}")
                ok = False
    if ok:
        print("verify-evidence: all markers match pre-image runs")
    return 0 if ok else 1

#!/usr/bin/env python3


import argparse
import re
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ALLOWLIST = ROOT / "docs/re/BYTE_DUMP_ALLOWLIST.tsv"
SCOPE = "*.md"

FENCE = re.compile(r"^\s*```+\s*bytes\b.*?^\s*```+\s*$", re.I | re.M | re.S)
XXD = re.compile(r"^\s*[0-9a-fA-F]{4,8}:\s*(.*)$", re.M)
PAIR_TOKEN = re.compile(r"\s*([0-9a-fA-F]{2})(?![0-9A-Za-z])")
BARE = re.compile(r"(?<![0-9A-Za-z])([0-9a-fA-F]{2}(?:[ \t]+[0-9a-fA-F]{2}){7,})(?![0-9A-Za-z])")
VALUE_LIST = re.compile(r"(?:\b0x[0-9a-fA-F]{1,3}\s*,\s*){7,}\b0x[0-9a-fA-F]{1,3}")
MIN_JUSTIFICATION = 20
DISCLOSURE = ("byte-dump checker: heuristic; known margins: decimal-looking low-value pairs "
              "outside addressed columns, non-.md files, contiguous hex blobs, encoded content")
TASK_RE = re.compile(r"^t\d+$")
EXPIRY_RE = re.compile(r"expires\b", re.I)


def looks_like_bytes(run: str) -> bool:
    """True when a space-separated run is byte-like, not a decimal number list."""
    for pair in run.split():
        value = int(pair, 16)
        if value >= 0x80 or any(c in "abcdefABCDEF" for c in pair):
            return True
    return False


def tracked(root: Path, scope: str = SCOPE):
    """Paths git tracks that match the scope; empty when this is not a checkout."""
    try:
        out = subprocess.run(["git", "-C", str(root), "ls-files", scope],
                             capture_output=True, text=True, check=True).stdout
    except (OSError, subprocess.CalledProcessError):
        return []
    return [root / line for line in out.splitlines() if line.strip()]


def task_status(task: str):
    """Status of a Dunking Sheep task, or None when the CLI/daemon is unavailable.

    The CLI's own output puts the state on the first line (`t119 [high] done ...`),
    so parse both the structured JSON form and that line rather than looking for a
    separate `status` record.
    """
    try:
        run = subprocess.run(["dunkingsheep", "--json", "task", "read", task],
                             capture_output=True, text=True, timeout=30)
        import json
        payload = json.loads(run.stdout or "{}")
        status = (payload.get("status") or payload.get("task", {}).get("status"))
        if status:
            return status
        out = subprocess.run(["dunkingsheep", "task", "read", task],
                             capture_output=True, text=True, timeout=30).stdout
    except (OSError, subprocess.SubprocessError, ValueError):
        return None
    for line in out.splitlines():
        fields = line.replace("[", " ").replace("]", " ").split()
        if not fields:
            continue
        if fields[0].startswith("t") and fields[0][1:].isdigit():
            for word in fields[1:]:
                if word in ("open", "in_progress", "blocked", "done", "cancelled"):
                    return word
            return None
        if line.startswith("status"):
            return fields[-1]
    return None


def load_allowlist(path: Path):
    """{relative path: (task, justification)}; raises ValueError for a malformed entry."""
    if not path.is_file():
        return {}
    allowed = {}
    for lineno, line in enumerate(path.read_text().splitlines(), 1):
        if not line.strip() or lineno == 1 and line.startswith("path\t"):
            continue
        parts = [part.strip() for part in line.split("\t")]
        if len(parts) != 3:
            raise ValueError(f"{path}: line {lineno} is not path<TAB>task<TAB>justification")
        name, task, justification = parts
        if not TASK_RE.match(task):
            raise ValueError(f"{path}: line {lineno} names no removal task (e.g. t139)")
        if len(justification) < MIN_JUSTIFICATION:
            raise ValueError(f"{path}: line {lineno} has no real justification")
        if not EXPIRY_RE.search(justification):
            raise ValueError(f"{path}: line {lineno} justification carries no expiry marker")
        allowed[name] = (task, justification)
    return allowed


def hits_in(text: str):
    """[(lineno, kind)] for dump-shaped content in one document."""
    found = []
    for match in FENCE.finditer(text):
        found.append((text[:match.start()].count("\n") + 1, "```bytes fence"))
    for match in XXD.finditer(text):
        # An addressed column is a dump by shape, whatever its byte values: consume
        # pair tokens separated by ANY whitespace (spaces or tabs) until the first
        # non-pair token, which starts the ASCII gutter. Counting tokens this way
        # neither needs a trailing separator nor mistakes internal spacing for one.
        pairs, rest = 0, match.group(1)
        token = PAIR_TOKEN.match(rest)
        while token:
            pairs += 1
            rest = rest[token.end():]
            token = PAIR_TOKEN.match(rest)
        if pairs >= 8:
            found.append((text[:match.start()].count("\n") + 1, "xxd/objdump column"))
    for lineno, line in enumerate(text.splitlines(), 1):
        bare = [m.group(1) for m in BARE.finditer(line) if looks_like_bytes(m.group(1))]
        if bare:
            found.append((lineno, "run of >=8 space-separated byte pairs"))
        if VALUE_LIST.search(line):
            found.append((lineno, "0x value list (enumerated values, not a span)"))
    return sorted(set(found))


def scan(root: Path, paths=None, allowlist_path: Path = ALLOWLIST):
    """{"hits": [(path, lineno, kind)], "stale": [path], "allowed": {path: why}}"""
    allowed = load_allowlist(allowlist_path)
    hits, seen = [], set()
    for path in (Path(p) for p in paths) if paths else tracked(root):
        try:
            text = path.read_text(errors="replace")
        except OSError:
            continue
        resolved, base = path.resolve(), root.resolve()
        relative = resolved.relative_to(base).as_posix() if resolved.is_relative_to(base) else path.name
        found = hits_in(text)
        if not found:
            continue
        seen.add(relative)
        for lineno, kind in found:
            hits.append((relative, lineno, kind))
    stale = sorted(set(allowed) - seen)
    return dict(hits=hits, stale=stale, allowed=allowed,
                tasks=sorted({task for task, _ in allowed.values()}))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("paths", nargs="*", help="files to scan; default: tracked *.md")
    parser.add_argument("--root", type=Path, default=ROOT)
    parser.add_argument("--allowlist", type=Path, default=ALLOWLIST)
    parser.add_argument("--report", action="store_true",
                        help="print every hit and exit 0; a report is not a pass")
    parser.add_argument("--quiet", action="store_true")
    parser.add_argument("--verify-evidence", action="store_true")
    parser.add_argument("--verify-base", default="main")
    parser.add_argument("--verify-head", default="HEAD")
    parser.add_argument("--task-status", type=Path,
                        help="read each task's status from a JSON map instead of the CLI")
    args = parser.parse_args(argv)
    print(DISCLOSURE)  # every run states its own margins; a silent heuristic reads as a guarantee
    try:
        result = scan(args.root, args.paths, args.allowlist)
    except ValueError as error:
        print(f"BYTE-DUMP ALLOWLIST ERROR: {error}", file=sys.stderr)
        return 1

    def failing(hit):
        return hit[0] not in result["allowed"]

    unlisted = [hit for hit in result["hits"] if failing(hit)]
    if result["tasks"] and not args.quiet:
        statuses = {}
        if args.task_status and args.task_status.is_file():
            import json
            statuses = json.loads(args.task_status.read_text())
        for task in result["tasks"]:
            state = statuses.get(task) if statuses else task_status(task)
            if state in ("done", "cancelled"):
                rows = sum(1 for t, _ in result["allowed"].values() if t == task)
                print(f"WARNING: allowlist names {task}, which is {state}, but {rows} row(s) remain")
    if not args.quiet:
        for path, lineno, kind in result["hits"]:
            state = "ALLOWED" if path in result["allowed"] else "FAIL"
            print(f"{state}: {path}:{lineno}: {kind}")
        for path in result["stale"]:
            print(f"WARNING: allowlist entry no longer hits: {path} "
                  f"(remove it in the scrub PR that cleaned it)")
    if args.report:
        files = len({hit[0] for hit in result["hits"]})
        print(f"report: {len(result['hits'])} hit line(s) in {files} file(s); "
              f"{len(unlisted)} would fail; {len(result['stale'])} stale allowlist entry(s)")
        return 0
    if unlisted:
        print(f"BYTE DUMPS: {len(unlisted)} unallowlisted dump line(s) in tracked "
              f"documentation - public files carry addresses, counts and hashes, never "
              f"original bytes; replace a dump with a span digest at the offset",
              file=sys.stderr)
        return 1
    print(f"OK: no unallowlisted byte-dump lines in {SCOPE}"
          + (f"; {len(result['allowed'])} allowlisted path(s)" if result["allowed"] else ""))
    if args.verify_evidence:
        return verify_evidence(args.verify_base, args.verify_head)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
