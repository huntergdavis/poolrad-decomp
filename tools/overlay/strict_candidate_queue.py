#!/usr/bin/env python3
"""Rank non-strict overlay originals as strict full-C candidates.

Consumes the regenerated, hash-pinned overlay products (inventory, callgraph,
decoded near-call audit) plus the checked-in function ledger, classification
and strict full-C CSVs, and a read-only view of the git history. Emits one
deterministic JSON queue in which every non-strict overlay original reports:

  * identity, size (bounded/decoded bytes, instruction count) and class,
  * callee readiness (strict-credited, modeled in main, verified or untranslated),
  * caller evidence: fixup-backed far callers and near callers, with the
    aligned PUSH CS call-site convention checked against the pinned GAME.OVR,
  * prior-translation evidence found in the main tree (own files and mentions,
    including bodies inlined/composed by other routines), in file names across
    remote branches, and in commit messages.

The queue ranks work candidates for strict full-C translation. It is not a
game-function denominator and makes no completion claim. Addresses, file
names, counts and short quoted repository lines only; no game bytes.
"""

import argparse
from collections import Counter, defaultdict
import csv
from functools import lru_cache
import hashlib
import json
from pathlib import Path
import re
import subprocess

from function_queue import DIGESTS, check_inputs, number, overlay_rows

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_LEDGER = ROOT / "docs/re/overlay_function_ledger.csv"
DEFAULT_CLASSIFICATION = ROOT / "docs/re/overlay_classification.csv"
DEFAULT_STRICT = ROOT / "docs/re/strict_full_c_addresses.csv"
DEFAULT_PARTIAL = ROOT / "docs/re/partial_callee_models.csv"
SOURCE_DIRS = ("src/recovered", "include/poolrad/recovered", "tests/recovered",
               "tools/overlay", "docs/re")
BRANCH_DIRS = ("src/recovered", "include/poolrad/recovered", "tests/recovered",
               "docs/re", "tools/overlay")
MODEL_DIRS = ("src/recovered", "include/poolrad/recovered", "tests/recovered")
MENTION_LIMIT = 8
BRANCH_LIMIT = 10
COMMIT_LIMIT = 6
OWNERSHIP_POINTS = {"game_verified": 40, "game_evidenced": 25,
                    "helper_candidate": 5, "game_or_other_library_unresolved": 0}
GRADE_POINTS = {"A": 20, "B": 10, "C": 5, "D": 0, "U": 0}
PRIOR_PENALTY = {"main_model": 45, "inline_in_strict": 40,
                 "unmerged_branch": 25, "inline_in_model": 18,
                 "commit_mention": 8, "source_mention": 5, "none": 0}
PRIOR_ORDER = ("main_model", "inline_in_strict", "unmerged_branch",
               "inline_in_model", "commit_mention", "source_mention", "none")

# Reviewed overlay originals and strict-credited resident targets whose only
# published C entry point is not a raw-CPU callee: a caller cannot be composed
# by handing it the live CPU and RAM image. Such a target keeps its strict
# credit, but a body whose direct or transitive closure reaches one is not
# claimed composable until a CPU-state interface exists. Keep this list small
# and evidence-backed; each entry names the exposing header.
RAW_CPU_INTERFACE_CAVEATS = {
    "RES_00BA_23DB": "resident_party_count.h exposes memory callbacks, not a "
                     "CPU-state callee",
    "OVL_0025_1815": "ecl_helpers.h exposes the string compare as a semantic "
                     "API, not a CPU-state callee; its runtime dependency "
                     "0AF8:0C10 has the merged pr_system_strcmp_0c10 raw model",
    "OVL_0025_18EB": "ecl_helpers.h exposes the word compare as a semantic API, "
                     "not a CPU-state callee",
}

# Endpoints DECLARED as boundary contracts: a caller may compose them without
# the endpoint becoming credited C. Each declaration names what is modelled and
# what is not, and the differential supplies the behaviour. This is an extension
# of the raw-CPU interface-caveat mechanism, not a new readiness rule: it marks
# a dependency USABLE, never CREDITED. A row in this set can never appear in
# strict_full_c_addresses.csv or reviewed_function_identities.csv - load_queue
# refuses that, and test_strict_candidate_queue proves the refusal.
DECLARED_BOUNDARY_ENDPOINTS = {
    "RES_0A40_02FD": "docs/re/DECLARED_BOUNDARY_0A40_02FD.md",
    "RES_0A40_030F": "docs/re/DECLARED_BOUNDARY_0A40_030F.md",
    "RES_0AF8_16BC": "docs/re/DECLARED_BOUNDARY_0AF8_16BC.md",
}

# Reviewed unresolved dispatches omitted by the direct-call graph. Keep the
# actual indirect site visible, and propagate its blocker through modeled
# callers. A filename/strict/full label cannot close a known dispatch boundary.
UNRESOLVED_INDIRECT_DISPATCH = {
    "OVL_009E_0000": dict(
        site=0x001E,
        note="LCALL through DS:6828+4*type; the raw prefix stops before the "
             "indirect call and does not execute its effect handler",
        evidence_doc="docs/re/U9E_LIST_RELEASE_0026_CPU.md"),
}

# SYSTEM/runtime endpoints with merged raw-CPU full-state models. A runtime
# edge to one of these is covered, so a body whose only unready callees were
# those calls can compose. Each names the merged model that proves it.
RUNTIME_COVERED_ENDPOINTS = {
    (0x0AF8, 0x0D12): "system set expand (system_set_expand_0d12_cpu)",
    (0x0A40, 0x029C): "CRT Delay (resident_a40_delay_029c_cpu)",
    (0x0AF8, 0x0254): "system FreeMem (system_freemem_0254_cpu)",
    (0x0AF8, 0x0A3F): "system Move (system_runtime_leaves_cpu)",
    (0x0AF8, 0x04F7): "system Text Assign (system_textassign_insert_cpu)",
    (0x0AF8, 0x0C68): "system Insert (system_textassign_insert_cpu)",
    (0x0AF8, 0x0A5B): "system LongMul (system_longmul_0a5b_cpu)",
    (0x0AF8, 0x0A76): "system LongDiv (system_longdiv_0a76_cpu)",
    (0x0AF8, 0x04A2): "system IOResult (system_ioresult_assign_cpu)",
    (0x0AF8, 0x13E2): "system Assign (system_ioresult_assign_cpu)",
    (0x0AF8, 0x0B0B): "system string load (system_string_leaves_cpu)",
    (0x0AF8, 0x0B57): "system string copy (system_string_move_leaves_cpu)",
    (0x0AF8, 0x0B25): "system StrStore (system_strstore_fullstate_0b25)",
    (0x0AF8, 0x0C10): "system string compare (system_string_move_leaves_cpu)",
    (0x0AF8, 0x0C3B): "system char to string (system_string_move_leaves_cpu)",
    (0x0AF8, 0x1685): "system Move backward (system_string_move_leaves_cpu)",
    (0x0AF8, 0x0B98): "system string concat (system_string_leaves_cpu)",
    (0x0AF8, 0x0DC0): "system set IN (system_runtime_leaves_cpu)",
    (0x0AF8, 0x023F): "system GetMem (system_getmem_023f_cpu)",
    (0x0AF8, 0x11C0): "system RANDOM (system_random_fullstate_11c0)",
    (0x0AF8, 0x1254): "system decimal core (system_str_res1037_cpu)",
    (0x0AF8, 0x1366): "system Str longint (system_str_res1037_cpu)",
    (0x0AF8, 0x11D6): "system random bit (system_random_bit_fullstate_11d6)",
    (0x0AF8, 0x1186): "system Real multiply (system_real_cpu, #1215)",
    (0x0AF8, 0x119A): "system Real from longint (system_real_cpu, #1215)",
    (0x0AF8, 0x119E): "system Real Trunc (system_real_cpu, #1215)",
    (0x0AF8, 0x120F): "system seed step (system_seed_fullstate_120f)",
    (0x0AF8, 0x16A8): "system FillChar (system_runtime_leaves_cpu)",
    (0x0AF8, 0x182A): "system UpCase (system_runtime_leaves_cpu)",
}

TOKEN = re.compile(
    r"ovl_([0-9a-f]{4})_([0-9a-f]{4})"
    r"|0x([0-9a-f]{2,3}):0x([0-9a-f]{4})"
    r"|(?<![0-9a-f])u([0-9a-f]{2,3})_([0-9a-f]{4})(?![0-9a-f])"
    r"|\+0x([0-9a-f]{4})(?![0-9a-f])"
    r"|\+([0-9a-f]{4})(?![0-9a-f])"
    r"|(?<![0-9a-f])0x([0-9a-f]{4})(?![0-9a-f])")
UNIT_TOKEN = re.compile(r"(?<![0-9a-f])u([0-9a-f]{2,3})(?![0-9a-f])")
HEX4 = re.compile(r"(?<![0-9a-f])([0-9a-f]{4})(?![0-9a-f])")


def require(condition, message):
    if not condition:
        raise ValueError(message)


@lru_cache(maxsize=None)
def unit_ctx(unit):
    return re.compile(rf"(?<![0-9a-f])(?:u|0x){unit:02x}(?![0-9a-f])")


def ovl_id(unit, offset):
    return f"OVL_{unit:04X}_{offset:04X}"


def key_of_id(identifier):
    unit, offset = identifier.split("_")[1:]
    return int(unit, 16), int(offset, 16)


def file_routine(path, known_ids):
    """Overlay IDs a file name names, mixing the two repository conventions
    (`u<unit>_<offset>_words` and `u<unit>_words_<offset>`)."""
    name = Path(path).name.lower()
    units = {int(match.group(1), 16) for match in UNIT_TOKEN.finditer(name)}
    hits = {ovl_id(unit, int(match.group(1), 16))
            for unit in units for match in HEX4.finditer(name)
            if ovl_id(unit, int(match.group(1), 16)) in known_ids}
    return hits


def routine_of(path, known_ids):
    hits = file_routine(path, known_ids)
    return hits.pop() if len(hits) == 1 else None


def candidate_key(row):
    unit, offset = row["original_address"].split(":")
    return int(unit, 16), int(offset, 16)


def resolve_match(match, line_low, path_units, candidates, by_offset):
    """Return (candidate id, kind) for a TOKEN match, or (None, None)."""
    if match.group(1) is not None:
        key = (int(match.group(1), 16), int(match.group(2), 16))
    elif match.group(3) is not None:
        key = (int(match.group(3), 16), int(match.group(4), 16))
    elif match.group(5) is not None:
        key = (int(match.group(5), 16), int(match.group(6), 16))
    else:
        offset = int(match.group(7) or match.group(8) or match.group(9), 16)
        for unit in by_offset.get(offset, ()):
            if unit in path_units or unit_ctx(unit).search(line_low):
                return candidates[(unit, offset)], "offset"
        return None, None
    return candidates.get(key), "id"


def offset_index(candidates):
    index = defaultdict(list)
    for unit, offset in candidates:
        index[offset].append(unit)
    for offset in index:
        index[offset].sort()
    return index


def scan_main_files(root, known_ids):
    """Map overlay IDs to their merged per-routine files under model directories."""
    result = defaultdict(list)
    for base in MODEL_DIRS:
        for path in sorted((root / base).rglob("*")):
            if not path.is_file():
                continue
            rel = path.relative_to(root).as_posix()
            for identifier in file_routine(rel, known_ids):
                result[identifier].append(rel)
    return {key: sorted(set(value)) for key, value in result.items()}


def scan_mentions(root, candidates):
    """Find address mentions in the main tree, bucketed source/tooling/docs."""
    by_offset = offset_index(candidates)
    result = {cid: {"source": [], "tooling": [], "docs": []} for cid in candidates.values()}
    for base in SOURCE_DIRS:
        bucket = ("source" if base.startswith(("src/", "include/", "tests/"))
                  else "tooling" if base == "tools/overlay" else "docs")
        for path in sorted((root / base).rglob("*")):
            if not path.is_file() or path.stat().st_size > 2_000_000:
                continue
            try:
                text = path.read_text(errors="replace")
            except OSError:
                continue
            rel = path.relative_to(root).as_posix()
            low_path = rel.lower()
            path_units = {unit for unit in {u for u, _ in candidates}
                          if unit_ctx(unit).search(low_path)}
            seen = set()
            for lineno, line in enumerate(text.splitlines(), 1):
                low = line.lower()
                for match in TOKEN.finditer(low):
                    cid, kind = resolve_match(match, low, path_units, candidates, by_offset)
                    if cid is None or (cid, lineno, kind) in seen:
                        continue
                    seen.add((cid, lineno, kind))
                    result[cid][bucket].append(dict(file=rel, line=lineno, kind=kind,
                                                    text=line.strip()[:160]))
    for cid in result:
        for bucket in result[cid]:
            result[cid][bucket] = result[cid][bucket][:MENTION_LIMIT]
    return result


def git(repo, *arguments):
    result = subprocess.run(["git", "-C", str(repo), *arguments],
                            capture_output=True, text=True, check=True)
    return result.stdout


def scan_branches(repo, candidates, main_commits, known_ids):
    """File names per unmerged branch tip; merged tips are main-tree state."""
    refs = {}
    for line in git(repo, "for-each-ref", "--format=%(refname)%09%(objectname)",
                    "refs/remotes/origin", "refs/heads").splitlines():
        name, _, sha = line.partition("\t")
        if name.endswith("/HEAD"):
            continue
        refs[name] = sha
    tips = defaultdict(list)
    for name in sorted(refs):
        tips[refs[name]].append(name)
    unmerged = [sha for sha in sorted(tips) if sha not in main_commits]
    hits = {cid: [] for cid in candidates.values()}
    for sha in unmerged:
        files = git(repo, "ls-tree", "-r", "--name-only", sha, "--", *BRANCH_DIRS).splitlines()
        for path in files:
            for identifier in file_routine(path, known_ids):
                key = key_of_id(identifier)
                if key in candidates:
                    hits[candidates[key]].append(dict(sha=sha, refs=sorted(tips[sha]), path=path))
    for cid in hits:
        hits[cid] = sorted(hits[cid], key=lambda hit: (hit["path"], hit["sha"]))[:BRANCH_LIMIT]
    pins = dict(branch_refs=len(refs), branch_tips=len(tips), unmerged_tips=len(unmerged),
                tips_sha256=hashlib.sha256("\n".join(
                    f"{name} {refs[name]}" for name in sorted(refs)).encode()).hexdigest())
    return hits, pins


def scan_commits(repo, candidates):
    raw = git(repo, "log", "--all", "--date=short",
              "--pretty=format:%H%x1f%ad%x1f%s%x1f%b%x1e")
    by_offset = offset_index(candidates)
    hits = {cid: [] for cid in candidates.values()}
    for record in raw.split("\x1e"):
        record = record.strip("\n")
        fields = record.split("\x1f")
        if len(fields) < 4:
            continue
        sha, date, subject, body = fields[0], fields[1], fields[2], fields[3]
        seen = set()
        for line in [subject, *body.splitlines()]:
            low = line.lower()
            for match in TOKEN.finditer(low):
                cid, _ = resolve_match(match, low, frozenset(), candidates, by_offset)
                if cid is not None and cid not in seen:
                    seen.add(cid)
                    hits[cid].append(dict(sha=sha, date=date, subject=subject[:100]))
    for cid in hits:
        hits[cid] = sorted(hits[cid], key=lambda hit: (hit["sha"],))[:COMMIT_LIMIT]
    pins = dict(commits_sha256=hashlib.sha256(raw.encode()).hexdigest(),
                commits_scanned=raw.count("\x1e"))
    return hits, pins


def require_not_declared_boundary(row_id):
    """Fail closed when a credit row names a declared boundary endpoint.

    A declared boundary is a dependency that may be COMPOSED; it is never a
    credit. Keeping this in one place means the loader and the tests enforce the
    same sentence.
    """
    require(row_id not in DECLARED_BOUNDARY_ENDPOINTS,
            f"declared boundary endpoints can never be strict-credited: {row_id}")


def load_queue(inventory, ledger_rows, classification_rows, strict_rows, near,
               partial_rows=None):
    """Validated join: reuse function_queue's overlay row construction."""
    near_validated = False
    if near is not None:
        check_inputs(near)
        require(all(near.get(key) == 0 for key in
                    ("invalid_seed_paths", "truncated_seed_paths",
                     "raw_exact_not_decoded", "decoded_exact_not_raw")),
                "near-call validation failed")
        total = sum(number(row["near_called"]) for row in ledger_rows)
        require(near.get("decoded_near_calls_to_prologues") ==
                near.get("raw_near_call_candidates_to_prologues") == total,
                "near-call validation site count differs from the ledger")
        near_validated = True
    rows, _ = overlay_rows(inventory, ledger_rows, classification_rows, near_validated)
    aliases = {}
    for row in rows:
        for alias in [row["id"], *row["aliases"]]:
            require(alias not in aliases, "alias maps to more than one original")
            aliases[alias] = row
    strict_overlay, strict_resident_ids = set(), set()
    for credit in strict_rows:
        target = aliases.get(credit["id"])
        require_not_declared_boundary(credit["id"])
        if target is None:
            require(credit["id"].startswith("RES_"),
                    "strict full-C address is absent from the overlay queue")
            strict_resident_ids.add(credit["id"])
            continue
        require(target["game_or_library"] == "game_verified",
                "strict full-C credit lacks verified game identity")
        require(credit["evidence_doc"].startswith("docs/re/")
                and ".." not in credit["evidence_doc"].split("/"),
                "missing strict full-C provenance")
        strict_overlay.add(target["id"])
    full_models, partial_models = {}, set()
    resident_full, resident_callback = set(), set()
    for entry in partial_rows or []:
        require(entry.get("status") in ("full", "partial", "callback"),
                "callee model manifest status must be full, partial or callback")
        require(entry["evidence_doc"].startswith("docs/re/")
                and ".." not in entry["evidence_doc"].split("/"),
                "missing callee model manifest provenance")
        target = aliases.get(entry["id"])
        if target is None:
            require(entry["id"].startswith("RES_")
                    and entry["id"] in strict_resident_ids,
                    "callee model manifest address is neither an overlay row nor a "
                    "strict-credited resident id")
            require(entry["status"] in ("full", "callback"),
                    "resident manifest entries must be full or callback")
            if entry["status"] == "full":
                resident_full.add(entry["id"])
            else:
                resident_callback.add(entry["id"])
            continue
        require(entry["status"] in ("full", "partial"),
                "overlay manifest entries must be full or partial")
        if entry["status"] == "full":
            full_models[target["id"]] = entry["evidence_doc"]
        else:
            partial_models.add(target["id"])
    candidates = {candidate_key(row): row["id"] for row in rows
                  if row["id"] not in strict_overlay}
    return dict(rows=rows, aliases=aliases, candidates=candidates,
                all_ids={row["id"] for row in rows},
                strict_overlay=strict_overlay,
                strict_resident_ids=strict_resident_ids,
                full_models=full_models, partial_models=partial_models,
                resident_full=resident_full, resident_callback=resident_callback,
                near_validated=near_validated)


# Evidence shapes that mention an address without modelling it: a test assertion
# about where the PREVIOUS body ends, a call-site map entry (`0x0829: 0x047A`),
# a span hash of bytes seen en route, or a comment naming the next prologue. A
# *call* is the opposite of an inlined model, so none of these is inline-model
# evidence. Require a call/entry/patch/model keyword to treat a text as such.
BOUNDARY_ASSERTION = re.compile(
    r"address\s*\+\s*[^=]{1,40}?==\s*0x[0-9a-fA-F]{3,5}"
    r"|\bsize\s*\+\s*[^=]{1,40}?==\s*0x[0-9a-fA-F]{3,5}")
CALLSITE_MAP = re.compile(r"^0x[0-9a-fA-F]{3,5}\s*:\s*0x[0-9a-fA-F]{3,5}\s*,?$")
SPAN_HASH = re.compile(
    r"^\(0x[0-9a-fA-F]{3,5},\s*0x[0-9a-fA-F]{3,5}\)\s*:\s*\"?[0-9a-fA-F]{16,}")
NEXT_PROLOGUE = re.compile(r"next\s+prologue|prologue\s+is\s+\+?0x", re.IGNORECASE)
# A comment describing an address RANGE (e.g. "+2A87..+2AE6 (95 original bytes)")
# names this routine only as the end of its neighbour's extent.
# An IP REDIRECT (`c->ip = 0x07BCu`) is the loader handing control to an
# address, and a bare comment mention only names it; neither models the body.
IP_REDIRECT = re.compile(r"->\s*ip\s*=\s*(?:0x[0-9a-fA-F]+|\d+)|\bip\s*=\s*0x[0-9a-fA-F]+u?")
COMMENT_ONLY = re.compile(r"^\s*(?:/\*|\*|//)")
RANGE_COMMENT = re.compile(
    r"\+?(?:0x)?[0-9a-fA-F]{3,5}\s*\.\.\s*\+?(?:0x)?[0-9a-fA-F]{3,5}")
INLINE_MODEL_WORD = re.compile(r"\bcalls?\b|\bcalled\b|invoke|inline|\bmodel\b|patched",
                               re.IGNORECASE)
INLINE_EVIDENCE_KEYWORD = re.compile(r"call|invoke|entry|patch|model", re.IGNORECASE)


def is_boundary_mention(mention):
    """True when a mention of this address is not evidence of an inline model."""
    text = (mention.get("text") or "").strip()
    structural = bool(CALLSITE_MAP.match(text)
                      or SPAN_HASH.match(text)
                      or BOUNDARY_ASSERTION.search(text)
                      or RANGE_COMMENT.search(text)
                      or IP_REDIRECT.search(text)
                      or COMMENT_ONLY.match(text))
    if structural:
        return not INLINE_MODEL_WORD.search(text)
    return bool(NEXT_PROLOGUE.search(text))


def prior_evidence(cid, main_files, mentions, branches, commits, strict_overlay, known_ids):
    own = main_files.get(cid, [])
    flags, inline = set(), []
    if own:
        flags.add("main_model")
    for mention in mentions["source"]:
        covered_by = routine_of(mention["file"], known_ids)
        if covered_by is None or covered_by == cid:
            continue
        if is_boundary_mention(mention):
            continue
        if covered_by in strict_overlay:
            inline.append(dict(mention, covered_by=covered_by))
            flags.add("inline_in_strict")
        elif covered_by in main_files:
            inline.append(dict(mention, covered_by=covered_by))
            flags.add("inline_in_model")
    if branches:
        flags.add("unmerged_branch")
    if commits:
        flags.add("commit_mention")
    if mentions["source"] or mentions["tooling"] or mentions["docs"]:
        flags.add("source_mention")
    status = next((state for state in PRIOR_ORDER if state in flags), "none")
    strong = [state for state in PRIOR_ORDER[:4] if state in flags]
    penalty = 0
    if strong:
        penalty = max(PRIOR_PENALTY[state] for state in strong) + 5 * (len(strong) - 1)
    else:
        penalty = PRIOR_PENALTY[status]
    return dict(status=status, penalty=min(penalty, 60), main_files=own,
                inline=inline[:MENTION_LIMIT], branches=branches, commits=commits,
                mentions=mentions)


def callee_evidence(key, edges_by_source, strict_overlay, main_files, class_by_id,
                    partial_models, transitively_unready, resident_full,
                    resident_callback, endpoints):
    source_id = ovl_id(*key)
    targets, seen = [], set()
    for edge in edges_by_source.get(key, []):
        entry = None
        if edge["kind"] == "near_candidate" and edge.get("target_func") is not None \
                and edge.get("exact_prologue"):
            entry = ("overlay", edge["unit"], edge["target_func"])
        elif edge["kind"] == "far_fixup_confirmed":
            if edge.get("resolution") == "overlay" and edge.get("target_func") is not None:
                entry = ("overlay", edge["target_unit"], edge["target_func"])
            elif edge.get("resolution") in ("resident", "runtime"):
                entry = (edge["resolution"], edge.get("seg"), edge.get("off"))
        if entry is None or entry in seen:
            continue
        seen.add(entry)
        if entry[0] == "overlay":
            target_id = ovl_id(entry[1], entry[2])
            readiness = ("indirect_unresolved" if target_id in UNRESOLVED_INDIRECT_DISPATCH
                         else "interface_caveat" if target_id in RAW_CPU_INTERFACE_CAVEATS
                         else "strict" if target_id in strict_overlay
                         else "partial" if target_id in partial_models
                         else "modeled" if target_id in main_files
                         and target_id not in transitively_unready
                         else "modeled_blocked" if target_id in main_files
                         else "verified" if class_by_id.get(target_id) == "game_verified"
                         else "untranslated")
            targets.append(dict(kind="overlay", id=target_id, unit=entry[1],
                                offset=entry[2], readiness=readiness))
            if target_id in UNRESOLVED_INDIRECT_DISPATCH:
                targets[-1]["unresolved_dispatch"] = UNRESOLVED_INDIRECT_DISPATCH[target_id].copy()
        elif entry[0] == "resident":
            res_id = (f"RES_{entry[1]:04X}_{entry[2]:04X}"
                      if entry[1] is not None and entry[2] is not None else None)
            readiness = ("resident_interface_caveat" if res_id in RAW_CPU_INTERFACE_CAVEATS
                         else "declared_boundary" if res_id in DECLARED_BOUNDARY_ENDPOINTS
                         else "resident_covered" if res_id in resident_full
                         else "resident_callback" if res_id in resident_callback
                         else "resident_unmapped")
            targets.append(dict(kind="resident", id=res_id, seg=entry[1],
                                offset=entry[2], readiness=readiness))
        else:
            readiness = ("runtime_covered"
                         if (entry[1], entry[2]) in RUNTIME_COVERED_ENDPOINTS
                         else "runtime_unmapped")
            targets.append(dict(kind="runtime", seg=entry[1], offset=entry[2],
                                readiness=readiness))
    if source_id in UNRESOLVED_INDIRECT_DISPATCH:
        targets.append(dict(kind="indirect", seg=None, offset=None,
                            readiness="indirect_unresolved",
                            **UNRESOLVED_INDIRECT_DISPATCH[source_id]))
    counts = Counter(target["readiness"] for target in targets)
    caveats = sorted((dict(id=target["id"], note=RAW_CPU_INTERFACE_CAVEATS[target["id"]])
                      for target in targets if target.get("id") in RAW_CPU_INTERFACE_CAVEATS),
                     key=lambda item: item["id"])
    # declared_boundary composes like a ready dependency, but is never a credit.
    ready = (counts["strict"] + counts["modeled"] + counts["resident_covered"]
             + counts["runtime_covered"] + counts["declared_boundary"])
    unready = (counts["untranslated"] + counts["resident_unmapped"]
               + counts["resident_callback"] + counts["runtime_unmapped"]
               + counts["partial"] + counts["modeled_blocked"] + counts["verified"]
               + counts["interface_caveat"] + counts["resident_interface_caveat"]
               + counts["indirect_unresolved"])
    base = min(30, 6 * counts["strict"] + 4 * counts["modeled"]
               + 2 * counts["verified"] + counts["partial"]
               + counts["modeled_blocked"] + 2 * counts["resident_covered"]
               + counts["runtime_covered"])
    points = 12 if not targets else round(base * ready / len(targets))
    return dict(targets=sorted(targets, key=lambda t: (t["kind"], t.get("unit", t.get("seg")),
                                                       t["offset"])),
                counts=dict(sorted(counts.items())), raw_cpu_caveats=caveats,
                ready=ready, unready=unready, total=len(targets),
                composable=not targets or unready == 0, points=points)


def caller_evidence(key, far_edges_by_target, near_edges_by_target, ovr, unit_ovr):
    def site(edge):
        return dict(src_unit=edge["unit"], src_func=edge["src_func"], site=edge["site"],
                    exact_prologue=edge.get("exact_prologue"))

    fixup = sorted({tuple(sorted(site(edge).items())) for edge in far_edges_by_target.get(key, [])})
    fixup = [dict(item) for item in fixup]
    near = sorted({tuple(sorted(site(edge).items())) for edge in near_edges_by_target.get(key, [])})
    near = [dict(item) for item in near]
    aligned = []
    if ovr is not None:
        for caller in fixup + near:
            base = unit_ovr.get(caller["src_unit"])
            if base is None or caller["site"] < 1:
                continue
            position = base + caller["site"] - 1
            if position < len(ovr) and ovr[position] == 0x0E:
                aligned.append(caller)
    points = (min(15, 5 * len(fixup)) + min(10, 2 * len(aligned)) +
              (2 if near else 0))
    return dict(fixup_backed=fixup, near_exact=near,
                aligned_push_cs=None if ovr is None else aligned, points=points)


def build_queue(state, callgraph, units, ledger_rows, classification_rows, mentions,
                main_files, branch_hits, commit_hits, media=None, git_view=None):
    ledger = {row["id"]: row for row in ledger_rows}
    classes = {row["id"]: row for row in classification_rows}
    class_by_id = {row["id"]: classes[row["original_address"]]["classification"]
                   for row in state["rows"]}
    unit_ovr = {unit["stub_para"]: unit["ovr_offset"] for unit in units}
    ovr = media.get("ovr") if media else None
    edges_by_source, far_by_target, near_by_target = defaultdict(list), defaultdict(list), defaultdict(list)
    for edge in callgraph["edges"]:
        if edge.get("src_func") is not None:
            edges_by_source[(edge["unit"], edge["src_func"])].append(edge)
        if edge["kind"] == "far_fixup_confirmed" and edge.get("resolution") == "overlay" \
                and edge.get("target_func") is not None:
            far_by_target[(edge["target_unit"], edge["target_func"])].append(edge)
        if edge["kind"] == "near_candidate" and edge.get("target") is not None:
            near_by_target[(edge["unit"], edge["target"])].append(edge)
    graph = defaultdict(set)
    resident_edges, runtime_edges = defaultdict(set), defaultdict(set)
    for edge in callgraph["edges"]:
        source = edge.get("src_func")
        if source is None:
            continue
        source_id = ovl_id(edge["unit"], source)
        if edge["kind"] == "near_candidate" and edge.get("target_func") is not None \
                and edge.get("exact_prologue"):
            graph[source_id].add(ovl_id(edge["unit"], edge["target_func"]))
        elif edge["kind"] == "far_fixup_confirmed":
            if edge.get("resolution") == "overlay" and edge.get("target_func") is not None:
                graph[source_id].add(ovl_id(edge["target_unit"], edge["target_func"]))
            elif edge.get("resolution") == "resident" \
                    and edge.get("seg") is not None and edge.get("off") is not None:
                resident_edges[source_id].add(f"RES_{edge['seg']:04X}_{edge['off']:04X}")
            elif edge.get("resolution") == "runtime" and edge.get("seg") is not None \
                    and edge.get("off") is not None:
                runtime_edges[source_id].add((edge["seg"], edge["off"]))
    nodes = set(graph) | set(resident_edges) | set(runtime_edges) | set(UNRESOLVED_INDIRECT_DISPATCH)
    for targets in graph.values():
        nodes |= targets

    def base_category(node):
        if node in state["strict_overlay"]:
            return "strict"
        if node in state["partial_models"]:
            return "partial"
        if node in main_files:
            return "modeled"
        if class_by_id.get(node) == "game_verified":
            return "verified"
        return "untranslated"

    # Transitive closure: strict bodies and manifest-full models are reviewed
    # endpoints that handle their own environment calls. Every other modeled
    # body is composable only when each overlay callee is ready, each resident
    # target is strict-credited, and no runtime call remains uncovered.
    endpoints = {node for node in nodes if node in state["full_models"]}
    ready = {node: (node not in RAW_CPU_INTERFACE_CAVEATS)
                    and (node not in UNRESOLVED_INDIRECT_DISPATCH)
                    and (base_category(node) in ("strict", "modeled") or node in endpoints)
             for node in nodes}
    changed = True
    while changed:
        changed = False
        for node in sorted(nodes):
            if not ready[node] or node in endpoints or base_category(node) == "strict":
                continue
            if any(not ready.get(target, False) for target in graph.get(node, ())):
                ready[node] = False
                changed = True
            elif any(res_id not in state["resident_full"]
                     or res_id in RAW_CPU_INTERFACE_CAVEATS
                     for res_id in resident_edges.get(node, ())):
                ready[node] = False
                changed = True
            elif any((seg, off) not in RUNTIME_COVERED_ENDPOINTS
                     for seg, off in runtime_edges.get(node, ())):
                ready[node] = False
                changed = True
    transitively_unready = {node for node in nodes
                            if not ready[node] and base_category(node) == "modeled"}
    ranked = []
    for key, cid in state["candidates"].items():
        row = state["aliases"][cid]
        entry = ledger[row["original_address"]]
        classification = classes[row["original_address"]]
        callees = callee_evidence(key, edges_by_source, state["strict_overlay"],
                                  main_files, class_by_id, state["partial_models"],
                                  transitively_unready, state["resident_full"],
                                  state["resident_callback"], endpoints)
        callers = caller_evidence(key, far_by_target, near_by_target, ovr, unit_ovr)
        prior = prior_evidence(cid, main_files, mentions[cid], branch_hits.get(cid, []),
                               commit_hits.get(cid, []), state["strict_overlay"],
                               state["all_ids"])
        size_bytes = number(entry["bound"])
        size_points = (12 if size_bytes <= 48 else 8 if size_bytes <= 128 else
                       4 if size_bytes <= 256 else 2 if size_bytes <= 512 else 0)
        terms = dict(ownership=OWNERSHIP_POINTS[row["classification"]],
                     grade=GRADE_POINTS[row["grade"]], callee=callees["points"],
                     callers=callers["points"], size=size_points,
                     composable=6 if callees["composable"] else 0,
                     prior_penalty=-prior["penalty"])
        ranked.append(dict(
            id=cid, original_address=row["original_address"], unit=key[0], offset=key[1],
            ovr_offset=number(entry["ovr_offset"]), aliases=row["aliases"],
            entry_slots=[slot["slot"] for slot in row["entry_slots"]],
            bytes=size_bytes, decoded_bytes=number(entry["decoded"]),
            instructions=number(entry["insns"]), grade=row["grade"],
            classification=row["classification"], confidence=classification["confidence"],
            behaviour=entry["cls"], callers_fixup_backed=len(callers["fixup_backed"]),
            callers_aligned_push_cs=None if callers["aligned_push_cs"] is None
            else len(callers["aligned_push_cs"]),
            callee_readiness=callees, callers=callers, prior_translation=prior,
            score=sum(terms.values()), score_terms=terms))
    ranked.sort(key=lambda item: (-item["score"], item["id"]))
    for index, item in enumerate(ranked, 1):
        item["rank"] = index
    counts = lambda values: dict(sorted(Counter(values).items()))
    summary = dict(candidates=len(ranked),
                   strict_overlay_excluded=len(state["strict_overlay"]),
                   strict_resident_ignored=len(state["strict_resident_ids"]),
                   composable_bodies=sum(item["callee_readiness"]["composable"]
                                         for item in ranked),
                   interface_caveat_bodies=sum(1 for item in ranked
                                               if item["callee_readiness"]["raw_cpu_caveats"]),
                   unresolved_indirect_bodies=sum(1 for item in ranked
                       if item["callee_readiness"]["counts"].get("indirect_unresolved")),
                   near_call_evidence=("decoded_validated" if state["near_validated"]
                                       else "raw_unvalidated"),
                   prior_status=counts(item["prior_translation"]["status"] for item in ranked),
                   classification=counts(item["classification"] for item in ranked),
                   grade=counts(item["grade"] for item in ranked),
                   behaviour=counts(item["behaviour"] for item in ranked),
                   fixup_backed_callers=sum(item["callers_fixup_backed"] for item in ranked),
                   aligned_push_cs_callers=sum(item["callers_aligned_push_cs"] or 0
                                               for item in ranked),
                   top_score=ranked[0]["score"] if ranked else None)
    return dict(schema_version=9, inputs=DIGESTS.copy(), git=git_view,
                summary=summary, rows=ranked)


def csv_rows(blob):
    return list(csv.DictReader(blob.decode().splitlines()))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--inventory", required=True)
    parser.add_argument("--callgraph", required=True)
    parser.add_argument("--near-calls")
    parser.add_argument("--ledger", default=str(DEFAULT_LEDGER))
    parser.add_argument("--classification", default=str(DEFAULT_CLASSIFICATION))
    parser.add_argument("--strict-map", default=str(DEFAULT_STRICT))
    parser.add_argument("--partial-models", default=str(DEFAULT_PARTIAL),
                        help="checked-in bounded-model manifest for callee readiness")
    parser.add_argument("--ovr", help="hash-pinned GAME.OVR for PUSH CS call-site checks")
    parser.add_argument("--repo", default=str(ROOT), help="repository for read-only history scans")
    parser.add_argument("--no-git", action="store_true", help="skip branch and commit scans")
    parser.add_argument("--json", required=True)
    args = parser.parse_args()
    paths = {"inventory": Path(args.inventory), "callgraph": Path(args.callgraph),
             "ledger": Path(args.ledger), "classification": Path(args.classification),
             "strict_map": Path(args.strict_map), "partial_models": Path(args.partial_models)}
    if args.near_calls:
        paths["near_calls"] = Path(args.near_calls)
    if args.ovr:
        paths["ovr"] = Path(args.ovr)
    blobs = {name: path.read_bytes() for name, path in paths.items()}
    repo = Path(args.repo).resolve()
    try:
        inventory = json.loads(blobs["inventory"])
        callgraph = json.loads(blobs["callgraph"])
        check_inputs(inventory)
        require(callgraph["inputs"]["start_exe_sha256"] == inventory["inputs"]["start_exe_sha256"]
                and callgraph["inputs"]["game_ovr_sha256"] == inventory["inputs"]["game_ovr_sha256"],
                "callgraph build differs from the inventory")
        if "ovr" in blobs:
            require(hashlib.sha256(blobs["ovr"]).hexdigest() == DIGESTS["game_ovr_sha256"],
                    "GAME.OVR hash differs from the audited build")
        near = json.loads(blobs["near_calls"]) if "near_calls" in blobs else None
        state = load_queue(inventory, csv_rows(blobs["ledger"]),
                           csv_rows(blobs["classification"]),
                           csv_rows(blobs["strict_map"]), near,
                           csv_rows(blobs["partial_models"]))
        main_files = scan_main_files(repo, state["all_ids"])
        for identifier, evidence in sorted(state["full_models"].items()):
            main_files.setdefault(identifier, []).append(evidence)
        mentions = scan_mentions(repo, state["candidates"])
        if args.no_git:
            branch_hits, commit_hits, git_view = {}, {}, None
        else:
            main_commits = set(git(repo, "rev-list", "origin/main").split())
            branch_hits, branch_pins = scan_branches(repo, state["candidates"],
                                                     main_commits, state["all_ids"])
            commit_hits, commit_pins = scan_commits(repo, state["candidates"])
            git_view = dict(main_commits=len(main_commits), **branch_pins, **commit_pins)
        result = build_queue(state, callgraph, inventory["units"],
                             csv_rows(blobs["ledger"]), csv_rows(blobs["classification"]),
                             mentions, main_files, branch_hits, commit_hits,
                             dict(ovr=blobs["ovr"]) if "ovr" in blobs else None, git_view)
    except (ValueError, KeyError, TypeError) as error:
        parser.exit(2, f"strict candidate queue refused: {error}\n")
    result["artifacts_sha256"] = {name: hashlib.sha256(blob).hexdigest()
                                  for name, blob in sorted(blobs.items())}
    Path(args.json).write_text(json.dumps(result, indent=2, sort_keys=True) + "\n")
    print(json.dumps(result["summary"], sort_keys=True))


if __name__ == "__main__":
    main()
