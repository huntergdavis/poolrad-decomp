#!/usr/bin/env python3
"""Refuse a change that adds a body or checker without its evidence and registration.

A body model under `src/recovered/` and a checker under `tools/overlay/` are
claims about original code. They are only reviewable if the same change carries
the evidence document and, for a checker, a `CHECKER_ROWS` registration — the
contract the handoff gate relies on. Relying on a human to notice a stray body in
a PR is how an untested model reached a reviewable branch once already; this
turns "someone noticed" into "the branch cannot land".

Rules, deliberately narrow:

* a changed `src/recovered/*.c` requires at least one changed `docs/re/*.md`;
* a changed `tools/overlay/*_audit.py` or `*_native.py` (a checker by house
  convention) requires a changed `docs/re/*.md` **and** a `CHECKER_ROWS` line in
  one of the changed checker files;
* gate scripts, generators and tests are not checkers and are not covered.

A file that matches the checker pattern but is not a row checker (for example
the queue generator `function_queue_audit.py`, which produces the accounting
pin) is exempt only through an explicit, greppable line in the file itself:

    # NOT_A_ROW_CHECKER: <reason, at least ten characters>

The exemption is recorded rather than hidden, so a rename is never needed. A
marker without a reason exempts nothing. A file that carries the marker and
also declares `CHECKER_ROWS` is refused as a contradiction, and an exempt file
never satisfies the registration requirement for a real checker.

Tests are exempt in shape only: `tests/` is not scanned, because a body without
its model is not a body.

An edit to a body or checker that is ALREADY PAIRED on the base may land without
a new document when a commit message in the change carries one line per file:

    Evidence-note: <id>: <what changed and why the evidence stands, ten+ chars>

`<id>` names the file, matched as a whole id, never a substring: its path,
basename or stem; for a checker, one of its CHECKER_ROWS; for a body, its own
row id (the stem carries the row's unit and offset, and a base `docs/re/*.md`
names both as whole tokens) or that row's four-digit offset. Each changed file
needs its own note; a note whose id names no changed file is refused (a typo or
a blanket note), and so is one id covering two files unless it is a full row id. "Already paired on the base" is measured, not assumed: the
file exists at the merge-base, a base `docs/re/*.md` names its stem (or, for a
checker, its basename or one of its rows), and a checker declares CHECKER_ROWS
there. The note path is closed - the change is refused even with notes - when it
adds a checker or a body, deletes one, changes a checker's CHECKER_ROWS set, or
touches a credit table (CREDIT_TABLES). A new body or checker therefore still
needs its document and CHECKER_ROWS in the same change.

Exit status: 0 when every changed body/checker is paired, 1 otherwise.
"""
import argparse
import ast
import re
import subprocess
import sys

CHECKER_SUFFIXES = ("_audit.py", "_native.py")
EXEMPTION = re.compile(r"^# NOT_A_ROW_CHECKER: (?P<reason>\S.{9,})$", re.MULTILINE)
NOTE = re.compile(r"^Evidence-note: (?P<id>[^\s:]+): (?P<why>\S.{9,})$", re.MULTILINE)
ROWS = re.compile(r"^CHECKER_ROWS\s*=\s*(?P<value>.+)$", re.MULTILINE)
CREDIT_TABLES = ("docs/re/credit_evidence_kind.csv", "docs/re/strict_full_c_addresses.csv",
                 "docs/re/reviewed_function_identities.csv")


def exemption(text):
    """The stated reason if `text` carries a valid not-a-row-checker marker, else None."""
    match = EXEMPTION.search(text)
    return match.group("reason").strip() if match else None


def changed_paths(base, root="."):
    """Files this branch changes relative to `base` (three-dot, so merge-base)."""
    done = subprocess.run(["git", "-C", root, "diff", "--name-only", f"{base}...HEAD"],
                          capture_output=True, text=True)
    if done.returncode:
        raise SystemExit(f"cannot diff against {base}: {done.stderr.strip()}")
    return [line for line in done.stdout.splitlines() if line]


def git(root, *args):
    done = subprocess.run(["git", "-C", root, *args], capture_output=True, text=True)
    return done.stdout if done.returncode == 0 else None


class Base:
    """The merge-base side of the change: file texts, the docs/re/*.md corpus, and the
    commit messages of the change. Tests build one directly instead of from git."""

    def __init__(self, files=None, messages=""):
        self.files = dict(files or {})
        self.messages = messages
        self._docs = None

    @classmethod
    def from_git(cls, base, root="."):
        merge = (git(root, "merge-base", base, "HEAD") or "").strip()
        if not merge:
            raise SystemExit(f"cannot find the merge-base of {base} and HEAD")
        base_obj = cls(messages=git(root, "log", "--format=%B", f"{merge}..HEAD") or "")
        base_obj.rev, base_obj.root = merge, root
        return base_obj

    def text(self, path):
        """The file at the merge-base, or None when it does not exist there."""
        if path in self.files:
            return self.files[path]
        rev = getattr(self, "rev", None)
        return git(self.root, "show", f"{rev}:{path}") if rev else None

    def doc_texts(self):
        """path -> text of every docs/re/*.md at the merge-base (one `git cat-file --batch`)."""
        if self._docs is None:
            rev = getattr(self, "rev", None)
            if rev:
                names = [n for n in (git(self.root, "ls-tree", "--name-only", rev, "docs/re/") or "").split()
                         if n.endswith(".md")]
                done = subprocess.run(["git", "-C", self.root, "cat-file", "--batch"], capture_output=True,
                                      input="".join(f"{rev}:{n}\n" for n in names).encode())
                out, at, self._docs = done.stdout, 0, {}
                for n in names:
                    end = out.index(b"\n", at)
                    size = int(out[at:end].split()[2])
                    self._docs[n] = out[end + 1:end + 1 + size].decode(errors="ignore")
                    at = end + 1 + size + 1
            else:
                self._docs = {p: t for p, t in self.files.items() if p.startswith("docs/re/") and p.endswith(".md")}
        return self._docs

    def docs(self):
        return "\n".join(self.doc_texts().values())


def checker_rows(text):
    """The CHECKER_ROWS ids a file declares (empty when it declares none or it is not a literal)."""
    match = ROWS.search(text or "")
    if not match:
        return set()
    try:
        value = ast.literal_eval(match.group("value").split("#")[0].strip())
    except (ValueError, SyntaxError):
        return {match.group("value").strip()}
    return {value} if isinstance(value, str) else {str(v) for v in value}


def notes(messages):
    """id -> reason for every Evidence-note line in the change's commit messages."""
    return {m.group("id"): m.group("why").strip() for m in NOTE.finditer(messages or "")}


ROW_ID = re.compile(r"^(?P<kind>OVL|RES)_(?P<unit>[0-9A-F]{4})_(?P<off>[0-9A-F]{4})$")
ROW_TOKEN = re.compile(r"\b(?:OVL|RES)_[0-9A-F]{4}_[0-9A-F]{4}\b")


def token(word, text):
    """True when `word` occurs in `text` as a whole token (never inside a longer identifier)."""
    return re.search(r"(?<![A-Za-z0-9_])" + re.escape(word) + r"(?![A-Za-z0-9_])", text) is not None


def stem_of(path):
    return path.rsplit("/", 1)[-1].rsplit(".", 1)[0]


def own_row(row, stem):
    """True when a row id is this body's by name: the stem carries its offset as an underscore token and its
    unit as `u<unit>` or the four-digit unit (overlay_u6f_entry6_0ec4_cpu <- OVL_006F_0EC4)."""
    m = ROW_ID.match(row)
    parts = stem.lower().split("_")
    if not m or m.group("off").lower() not in parts:
        return False
    unit = m.group("unit").lower()
    return "u" + (unit.lstrip("0") or "0") in parts or unit in parts


def ids_of(path, kind, base):
    """Every id an Evidence-note may use for one changed file: its path, basename and stem; a checker's base
    CHECKER_ROWS; a body's own row ids that a base docs/re/*.md names (whole tokens) together with its stem, and
    the four-digit offset of such a row. Matching is exact on whole ids, never a substring."""
    stem = stem_of(path)
    ids = {path, path.rsplit("/", 1)[-1], stem}
    if kind == "checker":
        return ids | checker_rows(base.text(path))
    for text in base.doc_texts().values():
        if token(stem, text):
            for row in ROW_TOKEN.findall(text):
                if own_row(row, stem):
                    ids |= {row, row[-4:]}
    return ids


def note_problems(bodies, checkers, paths, root, base):
    """Why an edit without a new document may not land on Evidence-notes, or an empty list.
    Every changed body and checker must be already paired on the base and named by its own note;
    every note must name exactly one changed file (a full row id may name that row's body and checker)."""
    found, given, docs = [], notes(base.messages), base.docs()
    for table in CREDIT_TABLES:
        if table in paths:
            found.append(f"{table} is a credit table; an Evidence-note never covers it")
    vocab = {}
    for path in bodies + checkers:
        stem = stem_of(path)
        before, after = base.text(path), read(path, root)
        kind = "checker" if path in checkers else "body"
        if before is None:
            found.append(f"{path} is a new {kind}: it needs its evidence document"
                         + (" and CHECKER_ROWS" if kind == "checker" else "") + " in this change")
            continue
        if not after:
            found.append(f"{path} is deleted: a deletion needs its evidence document")
            continue
        rows_before, rows_after = checker_rows(before), checker_rows(after)
        if kind == "checker" and rows_before != rows_after:
            found.append(f"{path} changes CHECKER_ROWS ({sorted(rows_before)} -> {sorted(rows_after)}):"
                         " a registration change needs its evidence document")
            continue
        if kind == "checker" and not rows_before:
            found.append(f"{path} is not paired on the base: it declares no CHECKER_ROWS there")
            continue
        names = [stem, path.rsplit("/", 1)[-1]] + (sorted(rows_before) if kind == "checker" else [])
        if not any(token(n, docs) for n in names):
            found.append(f"{path} is not paired on the base: no docs/re/*.md there names it")
            continue
        vocab[path] = ids_of(path, kind, base)
        if not any(i in vocab[path] for i in given):
            found.append(f"{path} is paired on the base but no 'Evidence-note: <id>: <why>' line in "
                         f"this change's commit messages names it (accepted ids: {', '.join(sorted(vocab[path] - {path}))})")
    for ident in sorted(given):
        covered = sorted(p for p, ids in vocab.items() if ident in ids)
        if not covered and vocab:
            found.append(f"Evidence-note id '{ident}' names no changed body or checker: a typo or a blanket note is refused")
        elif len(covered) > 1 and not ROW_ID.match(ident):
            found.append(f"Evidence-note id '{ident}' covers {len(covered)} files ({', '.join(covered)}): write one note per file")
    return found


def accepted_notes(bodies, checkers, base):
    """(path, id, reason) for each note that covers a changed file, for the report."""
    out, given = [], notes(base.messages)
    for path in bodies + checkers:
        ids = ids_of(path, "checker" if path in checkers else "body", base)
        for i, why in sorted(given.items()):
            if i in ids:
                out.append((path, i, why))
                break
    return out


def read(path, root="."):
    from pathlib import Path
    target = Path(root) / path
    return target.read_text(errors="ignore") if target.is_file() else ""


def exempt_generators(paths, root="."):
    """Checker-pattern files that declare themselves not row checkers, with reasons."""
    exempt = {}
    for p in paths:
        if p.startswith("tools/overlay/") and p.endswith(CHECKER_SUFFIXES):
            reason = exemption(read(p, root))
            if reason is not None:
                exempt[p] = reason
    return exempt


def problems(paths, root=".", base=None):
    """Why this change set is not reviewable, or an empty list. With `base` (the
    merge-base side), an edit without a new document may pass on Evidence-notes."""
    bodies = [p for p in paths if p.startswith("src/recovered/") and p.endswith(".c")]
    exempt = exempt_generators(paths, root)
    checkers = [p for p in paths
                if p.startswith("tools/overlay/") and p.endswith(CHECKER_SUFFIXES)
                and p not in exempt]
    docs = [p for p in paths if p.startswith("docs/re/") and p.endswith(".md")]
    found = []
    for p in sorted(exempt):
        if "CHECKER_ROWS" in read(p, root):
            found.append(f"{p} declares NOT_A_ROW_CHECKER and CHECKER_ROWS; "
                         "a file is a row checker or it is not")
    if (bodies or checkers) and not docs:
        refused = note_problems(bodies, checkers, paths, root, base) if base is not None else ["no base given"]
        if refused:
            found.append("no evidence document under docs/re/ is part of this change"
                         f" (bodies: {len(bodies)}, checkers: {len(checkers)})")
            if base is not None:
                found.extend(f"Evidence-note path closed: {line}" for line in refused)
    if checkers:
        registered = [p for p in checkers if "CHECKER_ROWS" in read(p, root)]
        if not registered:
            found.append("no changed checker declares CHECKER_ROWS "
                         f"(checkers: {', '.join(checkers)})")
    return found


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--base", default="origin/main")
    parser.add_argument("--root", default=".")
    parser.add_argument("--changed", nargs="*", default=None,
                        help="explicit path list (tests); default is the git diff")
    args = parser.parse_args(argv)
    try:
        paths = args.changed if args.changed is not None else changed_paths(args.base, args.root)
        base = Base.from_git(args.base, args.root) if args.changed is None else None
    except SystemExit as error:
        print(error)
        print("PATH ORIGIN: unknown (comparison history unavailable); PR=unknown")
        return 1
    return report(paths, args.root, base)


def path_origin(path, root, base):
    """Diagnostic only: prove a unique path addition, never blame the tip commit.

    This identifies when the *path* was added in the comparison range, not when
    every reported defect arose. Existing paths, renames, merges and incomplete
    history may need human investigation. PR membership is not a Git fact.
    """
    unknown = "added-in=unknown"
    if base is None or not getattr(base, "rev", None):
        return unknown + " (no Git comparison history)"
    if git(root, "rev-parse", "--is-shallow-repository") != "false\n":
        return unknown + " (history unavailable or shallow)"
    if base.text(path) is not None:
        return unknown + " (path already exists at comparison base)"
    if git(root, "diff", "--name-only", "HEAD", "--", path) != "":
        return unknown + " (uncommitted change or history unavailable)"
    if git(root, "cat-file", "-e", f"HEAD:{path}") is None:
        return unknown + " (path absent at HEAD or history unavailable)"
    # Keep rename detection enabled: a rename must not masquerade as an addition.
    history = git(root, "log", "--full-history", "--find-renames", "--diff-filter=A",
                  "--format=%H", f"{base.rev}..HEAD", "--", path)
    commits = history.splitlines() if history is not None else []
    if len(commits) != 1:
        return unknown + " (no unique addition in comparison range)"
    commit = commits[0]
    parents = (git(root, "rev-list", "--parents", "-n", "1", commit) or "").split()
    if len(parents) != 2:
        return unknown + " (addition is not a single-parent commit)"
    # Inspect the full tree diff before filtering the path; a path-limited log
    # may otherwise classify a rename as an addition when its source is omitted.
    added = git(root, "diff-tree", "--no-commit-id", "--name-only", "-z",
                "--find-renames", "--diff-filter=A", "-r", parents[1], commit)
    if added is None or path not in added.split("\0"):
        return unknown + " (rename or addition could not be established)"
    return f"added-in={commit}"


def report_origins(paths, root, base):
    """Context for refused changes; does not influence any gate decision."""
    for path in sorted(set(paths)):
        if not ((path.startswith("src/recovered/") and path.endswith(".c")) or
                (path.startswith("tools/overlay/") and path.endswith(CHECKER_SUFFIXES))):
            continue
        try:
            origin = path_origin(path, root, base)
        except OSError:
            origin = "added-in=unknown (Git unavailable)"
        print(f"PATH ORIGIN: {path}; {origin}; PR=unknown (not resolved from local Git)")


def report(paths, root=".", base=None):
    found = problems(paths, root, base)
    if found:
        for line in found:
            print(f"UNPAIRED BODY: {line}")
        report_origins(paths, root, base)
        print(f"REFUSED: {len(found)} reason(s) - a body or checker lands with its "
              f"evidence document and CHECKER_ROWS, or it does not land")
        return 1
    for path, reason in sorted(exempt_generators(paths, root).items()):
        print(f"EXEMPT (not a row checker): {path} - {reason}")
    if base is not None and not [p for p in paths if p.startswith("docs/re/") and p.endswith(".md")]:
        exempt = exempt_generators(paths, root)
        bodies = [p for p in paths if p.startswith("src/recovered/") and p.endswith(".c")]
        checkers = [p for p in paths if p.startswith("tools/overlay/") and p.endswith(CHECKER_SUFFIXES) and p not in exempt]
        for path, ident, why in accepted_notes(bodies, checkers, base):
            print(f"EVIDENCE-NOTE: {path} - {ident}: {why}")
    print(f"OK: {len(paths)} changed path(s); every body/checker is paired "
          f"with evidence and registration")
    return 0


if __name__ == "__main__":
    sys.exit(main())
