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
import sys

from function_queue import DIGESTS, check_inputs, number, overlay_rows

ROOT = Path(__file__).resolve().parents[2]
DEFAULT_LEDGER = ROOT / "docs/re/overlay_function_ledger.csv"
DEFAULT_CLASSIFICATION = ROOT / "docs/re/overlay_classification.csv"
DEFAULT_STRICT = ROOT / "docs/re/strict_full_c_addresses.csv"
DEFAULT_PARTIAL = ROOT / "docs/re/partial_callee_models.csv"
DEFAULT_DRIVER_INTERNAL = ROOT / "docs/re/driver_internal_models.csv"
DEFAULT_RESIDENT_RUNTIME = ROOT / "docs/re/resident_runtime_models.csv"
DEFAULT_IDENTITIES = ROOT / "docs/re/reviewed_function_identities.csv"
# Committed inputs an export records by hash. A consumer must refuse an export whose
# recorded hash differs from the tree: a file that exists is not a file that is current.
FRESHNESS_INPUTS = {
    "ledger": "docs/re/overlay_function_ledger.csv",
    "classification": "docs/re/overlay_classification.csv",
    "strict_map": "docs/re/strict_full_c_addresses.csv",
    "partial_models": "docs/re/partial_callee_models.csv",
    "driver_internal": "docs/re/driver_internal_models.csv",
    "resident_runtime": "docs/re/resident_runtime_models.csv",
    "identities": "docs/re/reviewed_function_identities.csv",
    "ready_helper_rules": "tools/overlay/strict_candidate_queue.py",
    "resident_ledger": "docs/re/resident_callable_ledger.csv",
}
STALE_EXIT = 2
REGENERATE = ("  python3 tools/overlay/strict_candidate_queue.py \\\n"
              "      --inventory resources/function-accounting-current/inventory.json \\\n"
              "      --callgraph resources/function-accounting-current/callgraph.json \\\n"
              "      --near-calls resources/function-accounting-current/near.json \\\n"
              "      --ovr resources/DOS/POOLRAD/GAME.OVR --repo . --json {json}")
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
    "RES_0AF8_00E9": "docs/re/DECLARED_BOUNDARY_0AF8_00E9.md",
    "RES_0AF8_00E2": "docs/re/DECLARED_BOUNDARY_0AF8_00E2.md",
    "RES_0AF8_00DF": "docs/re/DECLARED_BOUNDARY_0AF8_00DF.md",
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
    (0x0AF8, 0x1254): "system decimal core (system_str_1366_cpu)",
    (0x0AF8, 0x1366): "system Str longint (system_str_1366_cpu)",
    (0x0AF8, 0x11D6): "system random bit (system_random_bit_fullstate_11d6)",
    (0x0AF8, 0x13B1): "system Val longint (system_val_13b1_cpu, #1303)",
    (0x0AF8, 0x1186): "system Real multiply (system_real_cpu, #1215)",
    (0x0AF8, 0x119A): "system Real from longint (system_real_cpu, #1215)",
    (0x0AF8, 0x119E): "system Real Trunc (system_real_cpu, #1215)",
    (0x0AF8, 0x120F): "system seed step (system_seed_fullstate_120f)",
    (0x0AF8, 0x16A8): "system FillChar (system_runtime_leaves_cpu)",
    (0x0AF8, 0x182A): "system UpCase (system_runtime_leaves_cpu)",
    (0x0A32, 0x00A2): "Dos.GetTime (dos_gettime_00a2_cpu, #1245)",
    (0x0A32, 0x000B): "Dos.Intr (dos_intr_000b_cpu, #1259)",
    (0x0AF8, 0x04A9): "system IOCheck (system_iocheck_04a9_cpu, #1306)",
    (0x0AF8, 0x1491): "system Close (system_close_1491_cpu, #1309)",
    (0x0AF8, 0x1410): "system Reset (system_reset_1410_cpu, #1311)",
    (0x0AF8, 0x1563): "system Seek (system_seek_1563_cpu, #1314/#1317)",
    (0x0AF8, 0x1593): "system FilePos (system_filepos_1593_cpu, #1316)",
    (0x0AF8, 0x14FB): "system BlockRead (system_blockread_14fb_cpu, #1315)",
}

# A covered endpoint whose model stops by transferring into a TERMINAL must not be
# ready before that terminal is declared: otherwise every caller composes into an
# undeclared terminal, laundering by order of merging (VP i20256). load_queue refuses it.
RUNTIME_COVERED_TERMINALS = {
    (0x0AF8, 0x023F): "RES_0AF8_00E2",  # GetMem RUNERROR stop: AX=203, JMP 00E2
    (0x0AF8, 0x0254): "RES_0AF8_00E2",  # FreeMem RUNERROR stop: AX=204, JMP 00E2
    (0x0AF8, 0x04A9): "RES_0AF8_00E2",  # IOCheck RUNERROR stop: JMP 00E2 (RunError)
    (0x0AF8, 0x0A76): "RES_0AF8_00E2",  # LongDiv DIVIDE_ERROR stop: AX=200, JMP 00E2
    (0x0AF8, 0x1186): "RES_0AF8_00E2",  # Real multiply RUNERROR stop: AX=205, JMP 00E2
    (0x0AF8, 0x119E): "RES_0AF8_00E2",  # Real Trunc RUNERROR stop: AX=207, JMP 00E2
    (0x0AF8, 0x1593): "RES_0AF8_00E2",  # FilePos DIVIDE stop, through LongDiv's JMP 00E2
    (0x0AF8, 0x14FB): "RES_0AF8_00DF",  # BlockRead DIVIDE stop: INT 0, vector 0AF8:00DF (#1318)
}

# A covered endpoint whose model STOPS at a nested call (and resumes after it) is usable only
# while that callee is itself covered; the composed run is in the endpoint's evidence document.
RUNTIME_COVERED_STOP_CALLEES = {
    (0x0AF8, 0x1410): (0x0AF8, 0x1491),  # Reset CLOSE stop: PUSH CS; CALL 1491, resume at 1445
}

TOKEN = re.compile(
    r"ovl_([0-9a-f]{4})_([0-9a-f]{4})"
    r"|0x([0-9a-f]{2,3}):0x([0-9a-f]{4})"
    r"|(?<![0-9a-f])u([0-9a-f]{2,3})_([0-9a-f]{4})(?![0-9a-f])"
    r"|\+0x([0-9a-f]{4})(?![0-9a-f])"
    r"|\+([0-9a-f]{4})(?![0-9a-f])"
    r"|(?<![0-9a-f])0x([0-9a-f]{4})(?![0-9a-f])"
    # SEG:OFF with no '+' or '0x' ('007F:0A88', or an overlay stub '0041:00CA'), as sources write far calls (par-b, PM i22086)
    r"|(?<![0-9a-f:])([0-9a-f]{4}):([0-9a-f]{4})(?![0-9a-f:])")
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


def resolve_match(match, line_low, path_units, candidates, by_offset, stubs=None):
    """Return (candidate id, kind) for a TOKEN match, or (None, None)."""
    if match.group(10) is not None:
        seg, off = int(match.group(10), 16), int(match.group(11), 16)
        if (seg, off) in candidates:                      # unit:offset
            return candidates[(seg, off)], "seg_off"
        cid = (stubs or {}).get((seg, off))               # an overlay stub paragraph:entry offset
        return (cid, "stub_seg_off") if cid else (None, None)
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


def stub_index(state):
    """(stub paragraph, entry offset) -> candidate id, from the rows' STUB_pppp_oooo aliases."""
    candidate_ids = set(state["candidates"].values())
    index = {}
    for alias, row in state["aliases"].items():
        m = re.fullmatch(r"STUB_([0-9A-F]{4})_([0-9A-F]{4})", alias)
        if m and row["id"] in candidate_ids:
            index[(int(m.group(1), 16), int(m.group(2), 16))] = row["id"]
    return index


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


def scan_mentions(root, candidates, stubs=None):
    """Find address mentions in the main tree, bucketed source/tooling/docs. `stubs` maps an overlay
    stub (paragraph, entry offset) to its candidate id, for the SEG:OFF form."""
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
                    cid, kind = resolve_match(match, low, path_units, candidates, by_offset, stubs)
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


# DRIVER-INTERNAL bodies (VP i19914/i19925, design par-a i19920): fully translated
# and evidenced bodies of the documented 08AE timer/audio driver, whose authorship is
# the driver's, not the game's. A member composes like a ready dependency and is NEVER
# a credit. It is a sibling of DECLARED_BOUNDARY_ENDPOINTS (an environment contract),
# not a reuse of it: RES_08AE_IDENTITY_POLICY.md rules that out. Membership is narrow
# and enforced here: only ids in docs/re/driver_internal_models.csv, only in the 08AE
# segment, disjoint from every credit table, declared boundaries and
# GAME_SPECIFIC_TARGETS, each with an evidence document that carries its body hash.
DRIVER_INTERNAL_FIELDS = ("id", "evidence_doc", "review_pr", "reviewed_commit", "body_sha256", "body_bytes")
DRIVER_INTERNAL_ID = re.compile(r"RES_08AE_[0-9A-F]{4}")

# RESIDENT-RUNTIME bodies (VP i20291/i20300, t30): fully translated and evidenced NEAR-called resident bodies whose
# subject is a runtime structure (the heap) or a service called only with constant arguments, so no game state enters
# them and they can never carry a game identity (the 06FB / RES_0464_2A0D line). A member composes like a ready
# dependency and is NEVER a credit: the same contract as driver_internal, for residents outside the 08AE driver.
# Membership is narrow: only ids in docs/re/resident_runtime_models.csv, never in 08AE (that is driver_internal),
# never an overlay-called entry (an id with a row in resident_callable_ledger.csv must earn identity instead),
# disjoint from every credit table, declared boundaries and GAME_SPECIFIC_TARGETS, each closure-complete, each with
# an evidence document that carries its body hash.
RESIDENT_RUNTIME_FIELDS = DRIVER_INTERNAL_FIELDS
RESIDENT_RUNTIME_ID = re.compile(r"RES_(?!08AE_)[0-9A-F]{4}_[0-9A-F]{4}")
RESIDENT_LEDGER = "docs/re/resident_callable_ledger.csv"

# PM i23493(2), clarified by i23747 and VP i23751: ALL callers, including
# resident near callers, must have game evidence. A candidate is not a member.
# Membership is an explicit reviewed act, never an automatic promotion. On
# 6d369fe3 both resident callers below are unresolved, so the exact set is empty.
READY_HELPER_MEMBERS = frozenset()
# Reviewed resident caller identities not yet strict. Entries use the same
# provenance fields as reviewed_function_identities.csv (which is overlay-only).
# Add only after the identity review lands on main; #1551 is not on 6d369fe3.
READY_HELPER_RESIDENT_IDENTITIES = ()
READY_HELPER_CANDIDATES = {
    "RES_03F1_0000": dict(
        evidence_doc="docs/re/RES_03F1_BOX_0000_CPU.md",
        audit="tools/overlay/res03f1_box_0000_cpu_audit.py",
        body_sha256="fae6cb41423bcd48237e75a08f5eda70642254eac8449373291d57f34120a8a4",
        body_bytes=311, refused_factors=(2,),
        # (owner, call instruction offset, evidence kind). 0329 is PUSH CS;
        # the CALL at 032A belongs to 02E8, not to the strict clear at 0137.
        callers=(("OVL_006A_04A1", 0x04F7, "overlay_far_fixup"),
                 ("OVL_0077_004A", 0x0073, "overlay_far_fixup"),
                 ("OVL_0077_0EF8", 0x11B8, "overlay_far_fixup"),
                 ("RES_00BA_16EC", 0x177B, "resident_far_fixup"),
                 ("RES_03F1_02E8", 0x032A, "resident_near_push_cs"))),
}
for _helper_id, _helper in READY_HELPER_CANDIDATES.items():
    for _field in ("evidence_doc", "audit"):
        FRESHNESS_INPUTS[f"ready_helper_{_helper_id}_{_field}"] = _helper[_field]
for _identity in READY_HELPER_RESIDENT_IDENTITIES:
    FRESHNESS_INPUTS[f"ready_helper_identity_{_identity['id']}"] = _identity["evidence_doc"]


def verify_ready_helper_callers(exe, ovr, ledger_rows, resident_rows, candidates=None):
    """Re-derive all direct callers from original bytes; refuse a stale census.

    Overlay sites require loader fixups. Resident far sites use the expanded
    image's relocation stream; resident near sites use the existing decoded
    candidate-seeded path walk, including its near-call closure. Unknown
    owners or extra sites cannot disappear into an overlay-only summary.
    """
    from far_call_sites import sites
    from resident_loader_map import unpack, relocations, word
    from resident_callable_coverage import near_closure
    candidates = READY_HELPER_CANDIDATES if candidates is None else candidates
    image = bytes(unpack(exe)[0])
    fixups, _ = relocations(exe, image)
    starts = {number(row["image_offset"]): row["id"] for row in resident_rows}
    seeds, walk = near_closure(image, set(starts))
    require(not walk["invalid"] and not walk["truncated"], "ready_helper caller decode incomplete")
    for identifier, candidate in candidates.items():
        seg, off = key_of_id(identifier)
        found = set()
        for unit, site, _ in sites(exe, ovr, seg, off):
            owners = [row for row in ledger_rows if number(row["unit"]) == unit
                      and number(row["offset"]) <= site < number(row["offset"]) + number(row["bound"])]
            require(len(owners) == 1, f"ready_helper caller has no unique overlay owner: {unit:04X}:{site:04X}")
            found.add((ovl_id(unit, number(owners[0]["offset"])), site, "overlay_far_fixup"))
        target = 16 * seg + off
        far = {fixup - 3 for fixup in fixups if fixup >= 3 and image[fixup - 3] == 0x9A
               and 16 * word(image, fixup) + word(image, fixup - 2) == target}
        for kind, callsites in (("resident_far_fixup", far),
                                ("resident_near_push_cs", walk["near"].get(target, set()))):
            for site in callsites:
                start = max((start for start in seeds if start <= site), default=-1)
                owner = starts.get(start, "unknown")
                require(owner.startswith("RES_"), f"ready_helper resident caller owner unresolved at {site:05X}")
                owner_seg, _ = key_of_id(owner)
                if kind == "resident_near_push_cs":
                    require(image[site - 1] == 0x0E, f"ready_helper near caller lacks PUSH CS at {site:05X}")
                found.add((owner, site - 16 * owner_seg, kind))
        require(found == set(candidate["callers"]),
                f"ready_helper {identifier} caller census changed: "
                f"missing={sorted(set(candidate['callers']) - found)}, extra={sorted(found - set(candidate['callers']))}")
    return image


def assess_ready_helper(identifier, candidate, caller_classes, root=ROOT, image=None):
    """Narrow admission predicate; return every refusal, never partial readiness.

    Original-byte audit execution is an acceptance obligation, recorded in the
    evidence document; exports check that document, its registered audit and
    the original span. Caller sites are pinned to the audited executable and
    independently re-derived by measure_ready_helpers.py.
    """
    reasons = []
    document = Path(root) / candidate["evidence_doc"]
    audit = Path(root) / candidate["audit"]
    if not (candidate["evidence_doc"].startswith("docs/re/")
            and ".." not in candidate["evidence_doc"].split("/")
            and document.is_file() and identifier in document.read_text()
            and candidate["body_sha256"] in document.read_text()
            and audit.is_file() and re.search(
                rf"CHECKER_ROWS\s*=\s*\([^)]*['\"]{re.escape(identifier)}['\"]",
                audit.read_text())):
        reasons.append("missing full evidence or registered original-byte audit")
    if image is not None:
        seg, off = key_of_id(identifier)
        body = image[16 * seg + off:16 * seg + off + candidate["body_bytes"]]
        if hashlib.sha256(body).hexdigest() != candidate["body_sha256"]:
            reasons.append("original body hash differs from full evidence")
    if tuple(candidate["refused_factors"]) != (2,):
        reasons.append("not refused on factor 2 alone")
    callers = [dict(id=owner, site=f"{site:04X}", kind=kind,
                    classification=caller_classes.get(owner, "unresolved"))
               for owner, site, kind in candidate["callers"]]
    if not callers:
        reasons.append("no evidenced callers")
    for caller in callers:
        if caller["classification"] not in ("game_evidenced", "game_verified", "strict", "reviewed_identity"):
            reasons.append(f"caller {caller['id']} is {caller['classification']}")
    return dict(id=identifier, eligible=not reasons, reasons=reasons, callers=callers)


def load_ready_helpers(classification_rows, strict_rows, identity_rows, root=ROOT,
                       image=None, members=None, candidates=None, resident_identities=None):
    """Validate the explicit never-credit set and report deferred candidates."""
    members = READY_HELPER_MEMBERS if members is None else frozenset(members)
    candidates = READY_HELPER_CANDIDATES if candidates is None else candidates
    require(members <= candidates.keys(), "ready_helper member has no candidate evidence")
    classes = {ovl_id(number(row["unit"]), number(row["offset"])): row["classification"]
               for row in classification_rows}
    # Reviewed identities are stronger than game_evidenced. Unmerged PRs and
    # identity arguments without a reviewed declaration do not qualify.
    for row in identity_rows:
        document = Path(root) / row.get("evidence_doc", "")
        if document.is_file() and row["id"] in document.read_text():
            classes[row["id"]] = "reviewed_identity"
    resident_identities = (READY_HELPER_RESIDENT_IDENTITIES if resident_identities is None
                           else resident_identities)
    for row in resident_identities:
        document = row["evidence_doc"]
        require(re.fullmatch(r"RES_[0-9A-F]{4}_[0-9A-F]{4}", row["id"])
                and re.fullmatch(r"[0-9a-f]{40}", row["reviewed_commit"])
                and str(row["review_pr"]).isdigit() and int(row["review_pr"]) > 0
                and re.fullmatch(r"[0-9a-f]{64}", row["body_sha256"])
                and str(row["body_bytes"]).isdigit() and int(row["body_bytes"]) > 0
                and document.startswith("docs/re/") and ".." not in document.split("/")
                and (Path(root) / document).is_file(), "ready_helper caller lacks reviewed identity provenance")
        text = (Path(root) / document).read_text()
        require(row["id"] in text and row["body_sha256"] in text,
                "ready_helper caller identity document lacks its address/body hash")
        classes[row["id"]] = "reviewed_identity"
    for row in strict_rows:
        require_not_driver_internal(row["id"], members, "strict_full_c_addresses.csv", "ready_helper")
        classes[row["id"]] = "strict"
    for row in identity_rows:
        require_not_driver_internal(row["id"], members, "reviewed_function_identities.csv", "ready_helper")
    for row in resident_identities:
        require_not_driver_internal(row["id"], members, "ready-helper resident identities", "ready_helper")
    assessments = [assess_ready_helper(identifier, candidate, classes, root, image)
                   for identifier, candidate in sorted(candidates.items())]
    for assessment in assessments:
        assessment["member"] = assessment["id"] in members
        require(not assessment["member"] or assessment["eligible"],
                f"ready_helper {assessment['id']} refused: {'; '.join(assessment['reasons'])}")
    return members, assessments


def _direct_near_target(ins):
    """Target of a direct near branch or call: the instruction's single immediate operand; None if indirect."""
    from capstone.x86 import X86_OP_IMM
    operands = ins.operands
    return operands[0].imm & 0xFFFF if len(operands) == 1 and operands[0].type == X86_OP_IMM else None


def _direct_far_target(ins):
    """(segment, offset) of a direct far call or jump: two immediate operands; None if indirect."""
    from capstone.x86 import X86_OP_IMM
    operands = ins.operands
    if len(operands) == 2 and all(operand.type == X86_OP_IMM for operand in operands):
        return operands[0].imm & 0xFFFF, operands[1].imm & 0xFFFF
    return None


# Environment services a driver_internal member may invoke by INT (VP ruling i20129): explicit and minimal. Each
# entry is (interrupt, AH). Adding one is a reviewed change with its own reason, never a widening. The member's
# evidence document must declare each service it uses with the exact marker "INT 21h AH=25h" (etc.).
DECLARED_DOS_SERVICES = {
    (0x21, 0x25): "DOS set interrupt vector",
    (0x21, 0x35): "DOS get interrupt vector",
}


def _static_ah(body, index, block_starts):
    """AH at body[index] when a literal MOV in the same basic block sets it, with no intervening AH write, call or
    branch; None otherwise. Walks backwards and stops at the block start (a branch target or the entry)."""
    from capstone.x86 import X86_OP_IMM
    for position in range(index - 1, -1, -1):
        ins = body[position]
        if ins.mnemonic in ("call", "lcall", "jmp", "ljmp", "ret", "retf", "iret", "int", "int3", "into") \
                or ins.mnemonic.startswith("j") or ins.mnemonic.startswith("loop"):
            return None
        written = {ins.reg_name(register) for register in ins.regs_access()[1]}
        if written & {"ah", "ax", "eax"}:
            operands = ins.operands
            if ins.mnemonic != "mov" or len(operands) != 2 or operands[1].type != X86_OP_IMM:
                return None
            destination = ins.reg_name(operands[0].reg)
            return operands[1].imm & 0xFF if destination == "ah" else (operands[1].imm >> 8) & 0xFF
        if ins.address in block_starts or position == 0:
            return None
    return None


def driver_internal_closure_gaps(member, image, members, strict_ids=frozenset(), services=None):
    """Transfers out of a member body that are not ready; empty means closure-complete.

    The body runs from its entry to the first return that no earlier direct forward branch jumps past, decoded
    from the loader-expanded image. A far transfer must reach a runtime-covered endpoint, a declared boundary, a
    member or a strict resident; a near call, or a near jump leaving the body, must reach a member or a strict
    resident in the same segment. Indirect transfers are gaps. An interrupt is a gap unless it is INT 21h with AH
    set by a literal MOV in the same basic block to a DECLARED_DOS_SERVICES entry; such services are collected in
    `services` for the caller to check against the evidence document. Directness comes from the decoded operand
    types (an immediate target), never from how the disassembler prints them.
    """
    from capstone import CS_ARCH_X86, CS_MODE_16, Cs
    seg, entry = int(member[4:8], 16), int(member[9:13], 16)
    code = bytes(image[seg * 16 + entry:seg * 16 + entry + 0x1200])
    decoder = Cs(CS_ARCH_X86, CS_MODE_16)
    decoder.detail = True
    farthest, body, end = entry, [], None
    for ins in decoder.disasm(code, entry):
        body.append(ins)
        if ins.mnemonic.startswith("j") or ins.mnemonic.startswith("loop"):
            target = _direct_near_target(ins)
            if target is not None:
                farthest = max(farthest, target)
        if ins.mnemonic in ("ret", "retf", "iret") and ins.address + ins.size > farthest:
            end = ins.address + ins.size
            break
    if end is None:
        return [f"no return found from +{entry:04X}"]

    def ready(resident_id):
        return resident_id in members or resident_id in strict_ids or resident_id in DECLARED_BOUNDARY_ENDPOINTS

    block_starts = {entry} | {target for ins in body
                              if (ins.mnemonic.startswith("j") or ins.mnemonic.startswith("loop"))
                              and (target := _direct_near_target(ins)) is not None}
    gaps = []
    for index, ins in enumerate(body):
        where, mnemonic = f"+{ins.address:04X} {ins.mnemonic} {ins.op_str}", ins.mnemonic
        if mnemonic in ("int3", "into"):
            gaps.append(f"{where}: interrupt (unresolved environment edge)")
        elif mnemonic == "int":
            number = ins.operands[0].imm & 0xFF if ins.operands else None
            ah = _static_ah(body, index, block_starts) if number == 0x21 else None
            if number != 0x21:
                gaps.append(f"{where}: INT {number:02X}h is not a declared environment service")
            elif ah is None:
                gaps.append(f"{where}: INT 21h with AH not statically resolved in its basic block")
            elif (0x21, ah) not in DECLARED_DOS_SERVICES:
                gaps.append(f"{where}: INT 21h AH={ah:02X}h is not a declared environment service")
            elif services is not None:
                services.add((0x21, ah))
        elif mnemonic in ("lcall", "ljmp"):
            target = _direct_far_target(ins)
            if target is None:
                gaps.append(f"{where}: indirect far transfer")
                continue
            far_seg, far_off = target
            if (far_seg, far_off) not in RUNTIME_COVERED_ENDPOINTS and not ready(f"RES_{far_seg:04X}_{far_off:04X}"):
                gaps.append(f"{where}: {far_seg:04X}:{far_off:04X} is not runtime-covered, declared, strict or a member")
        elif mnemonic == "call" or mnemonic == "jmp" or mnemonic.startswith("j") or mnemonic.startswith("loop"):
            target = _direct_near_target(ins)
            if target is None:
                if mnemonic in ("call", "jmp"):
                    gaps.append(f"{where}: indirect near transfer")
                continue
            if mnemonic == "call" or not entry <= target < end:
                if not ready(f"RES_{seg:04X}_{target:04X}"):
                    gaps.append(f"{where}: RES_{seg:04X}_{target:04X} is not a member or strict")
    return gaps


def _load_noncredit_class(rows, name, pattern, outside, root, fields, image, strict_ids, ready_elsewhere,
                          refuse=None):
    """Shared validation for the never-credit readiness classes (driver_internal, resident_runtime). Fail closed."""
    from resident_inventory import GAME_SPECIFIC_TARGETS
    require(fields is None or tuple(fields) == DRIVER_INTERNAL_FIELDS,
            f"{name} table columns must be exactly {DRIVER_INTERNAL_FIELDS}")
    members, texts = set(), {}
    for row in rows:
        member = row["id"]
        require(pattern.fullmatch(member) is not None, f"{name} member {outside}: {member}")
        require(member not in members, f"duplicate {name} member: {member}")
        require(member not in DECLARED_BOUNDARY_ENDPOINTS,
                f"{name} member is a declared boundary: {member}")
        require((int(member[4:8], 16), int(member[9:13], 16)) not in GAME_SPECIFIC_TARGETS,
                f"{name} member is a game-specific resident target: {member}")
        if refuse is not None:
            refuse(member)
        require(re.fullmatch(r"[0-9a-f]{64}", row["body_sha256"] or "") is not None
                and str(row["body_bytes"]).isdigit() and int(row["body_bytes"]) > 0
                and re.fullmatch(r"[0-9a-f]{40}", row["reviewed_commit"] or "") is not None
                and str(row["review_pr"]).isdigit() and int(row["review_pr"]) > 0,
                f"{name} member lacks body/approval provenance: {member}")
        document = row["evidence_doc"]
        require(document.startswith("docs/re/") and ".." not in document.split("/")
                and (Path(root) / document).is_file(),
                f"{name} member names a missing evidence document: {member}")
        text = (Path(root) / document).read_text(errors="ignore")
        require(member in text and row["body_sha256"] in text,
                f"{name} evidence document does not name {member} and its body hash")
        members.add(member)
        texts[member] = text
    if image is not None:
        for member in sorted(members):
            services = set()
            gaps = driver_internal_closure_gaps(member, image, members | set(ready_elsewhere), strict_ids, services)
            require(not gaps, f"{name} member {member} has an incomplete closure: {'; '.join(gaps)}")
            for number, ah in sorted(services):
                require(f"INT {number:02X}h AH={ah:02X}h" in texts[member],
                        f"{name} evidence document for {member} does not declare INT {number:02X}h AH={ah:02X}h "
                        f"as an environment service")
    return frozenset(members)


def load_driver_internal(rows, root=ROOT, fields=None, image=None, strict_ids=frozenset(),
                         resident_runtime=frozenset()):
    """Validate the driver-internal table and return its member ids. Fail closed.

    With the loader-expanded START image, every member must also be closure-complete: a member whose callee is
    unready would read as ready and launder that callee, so it is refused here rather than left to review.
    """
    return _load_noncredit_class(rows, "driver_internal", DRIVER_INTERNAL_ID, "outside the 08AE driver segment",
                                 root, fields, image, strict_ids, resident_runtime)


def resident_ledger_ids(root=ROOT):
    """Ids of overlay-called resident entries (rows of resident_callable_ledger.csv)."""
    path = Path(root) / RESIDENT_LEDGER
    if not path.is_file():
        return frozenset()
    # Only overlay-called rows: a reviewed near-only body also carries a RES_ name but is not an overlay-called entry.
    return frozenset(row["id"] for row in csv.DictReader(path.read_text().splitlines())
                     if row["kind"] == "overlay_called")


def load_resident_runtime(rows, root=ROOT, fields=None, image=None, strict_ids=frozenset(), driver_internal=frozenset(),
                          ledger_ids=None):
    """Validate the resident-runtime table and return its member ids. Fail closed.

    Same rules as driver_internal, outside 08AE, plus: a member must be NEAR-called only. An id with a row in the
    resident callable ledger is an overlay-called entry, which must earn identity rather than be declared non-credit.
    """
    entries = resident_ledger_ids(root) if ledger_ids is None else frozenset(ledger_ids)

    def near_only(member):
        require(member not in entries,
                f"resident_runtime member is an overlay-called entry (resident_callable_ledger.csv): {member}")
    return _load_noncredit_class(rows, "resident_runtime", RESIDENT_RUNTIME_ID,
                                 "is outside the class (08AE bodies are driver_internal)",
                                 root, fields, image, strict_ids, driver_internal, refuse=near_only)


def require_not_driver_internal(row_id, members, table, name="driver_internal"):
    """Fail closed when a credit or manifest row names a never-credit class member."""
    require(row_id not in members,
            f"{name} members can never appear in {table}: {row_id}")


def require_not_declared_boundary(row_id):
    """Fail closed when a credit row names a declared boundary endpoint.

    A declared boundary is a dependency that may be COMPOSED; it is never a
    credit. Keeping this in one place means the loader and the tests enforce the
    same sentence.
    """
    require(row_id not in DECLARED_BOUNDARY_ENDPOINTS,
            f"declared boundary endpoints can never be strict-credited: {row_id}")


def load_queue(inventory, ledger_rows, classification_rows, strict_rows, near,
               partial_rows=None, driver_internal=frozenset(), identity_rows=(), resident_runtime=frozenset(),
               ready_helpers=frozenset()):
    """Validated join: reuse function_queue's overlay row construction."""
    for endpoint, terminal in RUNTIME_COVERED_TERMINALS.items():
        require(endpoint not in RUNTIME_COVERED_ENDPOINTS or terminal in DECLARED_BOUNDARY_ENDPOINTS,
                f"runtime-covered {endpoint[0]:04X}:{endpoint[1]:04X} stops into {terminal}, "
                "which is not a declared boundary")
    for endpoint, callee in RUNTIME_COVERED_STOP_CALLEES.items():
        require(endpoint not in RUNTIME_COVERED_ENDPOINTS or callee in RUNTIME_COVERED_ENDPOINTS,
                f"runtime-covered {endpoint[0]:04X}:{endpoint[1]:04X} stops at a call to "
                f"{callee[0]:04X}:{callee[1]:04X}, which is not runtime-covered")
    # Refuse any credit or manifest row naming a driver-internal member BEFORE anything
    # else, so an attempt to credit one fails on the class, not on some later check.
    for table, rows_ in (("reviewed_function_identities.csv", identity_rows),
                         ("strict_full_c_addresses.csv", strict_rows),
                         ("partial_callee_models.csv", partial_rows or [])):
        for row_ in rows_:
            require_not_driver_internal(row_["id"], driver_internal, table)
            require_not_driver_internal(row_["id"], resident_runtime, table, "resident_runtime")
            require_not_driver_internal(row_["id"], ready_helpers, table, "ready_helper")
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
                driver_internal=frozenset(driver_internal), resident_runtime=frozenset(resident_runtime),
                ready_helpers=frozenset(ready_helpers), near_validated=near_validated)


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
# An IP REDIRECT (`c->ip = 0x....u`) hands control to an address and a bare
# comment only names it; by themselves neither models the body. The exception is
# a span the covering routine's own registered checker pins by hash (SPAN_PIN
# below): +0E0C's `c->ip = 0x07BCu` is followed by a full model of +07BC, which
# u25_load_word_0e0c_audit.py executes against the original bytes.
IP_REDIRECT = re.compile(r"->\s*ip\s*=\s*(?:0x[0-9a-fA-F]+|\d+)|\bip\s*=\s*0x[0-9a-fA-F]+u?")
COMMENT_ONLY = re.compile(r"^\s*(?:/\*|\*|//)")
RANGE_COMMENT = re.compile(
    r"\+?(?:0x)?[0-9a-fA-F]{3,5}\s*\.\.\s*\+?(?:0x)?[0-9a-fA-F]{3,5}")
INLINE_MODEL_WORD = re.compile(r"\bcalls?\b|\bcalled\b|invoke|inline|\bmodel\b|patched",
                               re.IGNORECASE)
INLINE_EVIDENCE_KEYWORD = re.compile(r"call|invoke|entry|patch|model", re.IGNORECASE)


def boundary_rule(mention):
    """The name of the rule that makes a mention boundary-only (not inline-model evidence), or None."""
    text = (mention.get("text") or "").strip()
    for name, hit in (("CALLSITE_MAP", CALLSITE_MAP.match(text)), ("SPAN_HASH", SPAN_HASH.match(text)),
                      ("BOUNDARY_ASSERTION", BOUNDARY_ASSERTION.search(text)), ("RANGE_COMMENT", RANGE_COMMENT.search(text)),
                      ("IP_REDIRECT", IP_REDIRECT.search(text)), ("COMMENT_ONLY", COMMENT_ONLY.match(text))):
        if hit:
            return None if INLINE_MODEL_WORD.search(text) else name
    return "NEXT_PROLOGUE" if NEXT_PROLOGUE.search(text) else None


def is_boundary_mention(mention):
    """True when a mention of this address is not evidence of an inline model."""
    return boundary_rule(mention) is not None


# A span the covering routine's registered checker pins by SHA-256 is translated
# there: `(0x07BC, 0x0A74, "6ec8...")` in u25_load_word_0e0c_audit.py is +0E0C's
# checker executing +07BC's original bytes against its C. A redirect or a bare
# comment in that routine's model then names a body it models, not a boundary.
SPAN_PIN = re.compile(r"\(\s*0x(?P<start>[0-9a-fA-F]{3,5})\s*,\s*0x[0-9a-fA-F]{3,5}\s*,\s*[\"'][0-9a-f]{64}[\"']\s*\)")


def pinned_spans(repo):
    """Row id -> span start offsets that one of its registered checkers pins by hash (the CI registry, both folders)."""
    from check_checker_builds import discover
    pinned = {}
    for row, paths in discover(Path(repo)).items():
        for path in paths:
            text = (Path(repo) / path).read_text(errors="ignore")
            pinned.setdefault(row, set()).update(int(m.group("start"), 16) for m in SPAN_PIN.finditer(text))
    return pinned


def pinned_by(cid, covered_by, pinned):
    """True when covered_by's checker pins the span that starts at cid's offset in the same unit."""
    if not pinned or covered_by is None or cid[:8] != covered_by[:8]:
        return False
    return int(cid[-4:], 16) in pinned.get(covered_by, ())


def prior_evidence(cid, main_files, mentions, branches, commits, strict_overlay, known_ids, pinned=None):
    own = main_files.get(cid, [])
    flags, inline, discarded = set(), [], []
    if own:
        flags.add("main_model")
    for mention in mentions["source"]:
        covered_by = routine_of(mention["file"], known_ids)
        if covered_by is None or covered_by == cid:
            discarded.append(dict(mention, rule="NO_SINGLE_ROUTINE" if covered_by is None else "OWN_ROUTINE", covered_by=covered_by))
            continue
        rule = boundary_rule(mention)
        if rule is not None:
            if not pinned_by(cid, covered_by, pinned):
                discarded.append(dict(mention, rule=rule, covered_by=covered_by))
                continue
            mention = dict(mention, pinned_span=True, rescued_from=rule)
        if covered_by not in strict_overlay and covered_by not in main_files:
            discarded.append(dict(mention, rule="COVERING_ROUTINE_UNMODELLED", covered_by=covered_by))
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
                mentions=mentions, discarded=discarded)


def runtime_readiness(seg, off):
    """Readiness of a runtime-resolved far target.

    Every product that defines runtime readiness is consulted: the covered
    runtime models and the declared boundaries. The call graph resolves SYSTEM
    targets as runtime, so a RES_0AF8_* declaration must be found here too, or
    it never applies at overlay call sites (0AF8:16BC read runtime_unmapped).
    """
    if (seg, off) in RUNTIME_COVERED_ENDPOINTS:
        return "runtime_covered"
    if seg is not None and off is not None \
            and f"RES_{seg:04X}_{off:04X}" in DECLARED_BOUNDARY_ENDPOINTS:
        return "declared_boundary"
    return "runtime_unmapped"


def resident_target_ready(res_id, resident_full, driver_internal=frozenset(), resident_runtime=frozenset(),
                          ready_helpers=frozenset()):
    """Transitive-pass mirror of callee_evidence's resident readiness classes."""
    if res_id in RAW_CPU_INTERFACE_CAVEATS:
        return False
    return (res_id in DECLARED_BOUNDARY_ENDPOINTS or res_id in driver_internal
            or res_id in resident_runtime or res_id in ready_helpers or res_id in resident_full)


def callee_evidence(key, edges_by_source, strict_overlay, main_files, class_by_id,
                    partial_models, transitively_unready, resident_full,
                    resident_callback, endpoints, driver_internal=frozenset(), resident_runtime=frozenset(),
                    ready_helpers=frozenset()):
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
                         else "driver_internal" if res_id in driver_internal
                         else "resident_runtime" if res_id in resident_runtime
                         else "ready_helper" if res_id in ready_helpers
                         else "resident_covered" if res_id in resident_full
                         else "resident_callback" if res_id in resident_callback
                         else "resident_unmapped")
            targets.append(dict(kind="resident", id=res_id, seg=entry[1],
                                offset=entry[2], readiness=readiness))
        else:
            readiness = runtime_readiness(entry[1], entry[2])
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
    # Declared boundaries and the three noncredit classes compose without credit.
    ready = (counts["strict"] + counts["modeled"] + counts["resident_covered"]
             + counts["runtime_covered"] + counts["declared_boundary"] + counts["driver_internal"]
             + counts["resident_runtime"] + counts["ready_helper"])
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
    # target is ready by the same classes callee_evidence uses (strict-credited,
    # declared boundary or driver_internal), and no runtime call remains unmapped.
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
            elif any(not resident_target_ready(res_id, state["resident_full"],
                                               state.get("driver_internal", frozenset()),
                                               state.get("resident_runtime", frozenset()),
                                               state.get("ready_helpers", frozenset()))
                     for res_id in resident_edges.get(node, ())):
                ready[node] = False
                changed = True
            elif any(runtime_readiness(seg, off) == "runtime_unmapped"
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
                                  state["resident_callback"], endpoints,
                                  state.get("driver_internal", frozenset()),
                                  state.get("resident_runtime", frozenset()),
                                  state.get("ready_helpers", frozenset()))
        callers = caller_evidence(key, far_by_target, near_by_target, ovr, unit_ovr)
        prior = prior_evidence(cid, main_files, mentions[cid], branch_hits.get(cid, []),
                               commit_hits.get(cid, []), state["strict_overlay"],
                               state["all_ids"], state.get("pinned_spans"))
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
    discards = []
    for item in ranked:          # every rejected source mention goes to the discard report; the row keeps the count
        rejected = item["prior_translation"].pop("discarded", [])
        item["prior_translation"]["discarded_mentions"] = len(rejected)
        discards += [dict(row=item["id"], file=d.get("file"), line=d.get("line"), rule=d["rule"],
                          covered_by=d.get("covered_by"), text=d.get("text")) for d in rejected]
    discards.sort(key=lambda d: (d["row"], d["file"] or "", d["line"] or 0, d["rule"]))
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
                   top_score=ranked[0]["score"] if ranked else None,
                   discarded_mentions=len(discards),
                   discarded_by_rule=counts(d["rule"] for d in discards),
                   pinned_span_rescues=sum(1 for item in ranked for m in item["prior_translation"]["inline"]
                                           if m.get("pinned_span")))
    return dict(schema_version=9, inputs=DIGESTS.copy(), git=git_view,
                summary=summary, rows=ranked, discards=discards)


def stale_inputs(export, root=ROOT):
    """(name, path, recorded, current) for each committed input that differs from the export."""
    recorded = export.get("artifacts_sha256") or {}
    changed = []
    for name, relative in sorted(FRESHNESS_INPUTS.items()):
        path = Path(root) / relative
        current = hashlib.sha256(path.read_bytes()).hexdigest() if path.is_file() else None
        if recorded.get(name) != current:
            changed.append((name, relative, recorded.get(name), current))
    return changed


def load_fresh_export(path, root=ROOT):
    """Read an export, refusing (exit 2) when any committed input changed since it was written."""
    export = json.loads(Path(path).read_text())
    changed = stale_inputs(export, root)
    if changed:
        lines = [f"STALE EXPORT: {path} was generated from inputs that differ from this tree; refusing to read it."]
        lines += [f"  {name} ({relative}): export {(was or 'not recorded')[:12]}, tree {(now or 'missing')[:12]}"
                  for name, relative, was, now in changed]
        lines += ["Regenerate it, then rerun:", REGENERATE.format(json=path)]
        print("\n".join(lines), file=sys.stderr)
        raise SystemExit(STALE_EXIT)
    return export


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
    parser.add_argument("--driver-internal", default=str(DEFAULT_DRIVER_INTERNAL),
                        help="declared driver-internal bodies (ready, never credits)")
    parser.add_argument("--resident-runtime", default=str(DEFAULT_RESIDENT_RUNTIME),
                        help="declared near-called resident runtime bodies (ready, never credits)")
    parser.add_argument("--identities", default=str(DEFAULT_IDENTITIES),
                        help="reviewed identity table, checked disjoint from driver_internal")
    parser.add_argument("--ovr", help="hash-pinned GAME.OVR for PUSH CS call-site checks")
    parser.add_argument("--exe", default=str(ROOT / "resources/DOS/POOLRAD/START.EXE"),
                        help="hash-pinned START.EXE; required whenever driver_internal or resident_runtime has members "
                             "(closure check)")
    parser.add_argument("--repo", default=str(ROOT), help="repository for read-only history scans")
    parser.add_argument("--no-git", action="store_true", help="skip branch and commit scans")
    parser.add_argument("--json", required=True)
    parser.add_argument("--discards", help="write the detector's discard report (every rejected mention, its rule) here")
    parser.add_argument("--without-ready-helpers", action="store_true",
                        help="counterfactual export with ready-helper membership disabled")
    args = parser.parse_args()
    paths = {"inventory": Path(args.inventory), "callgraph": Path(args.callgraph),
             "ledger": Path(args.ledger), "classification": Path(args.classification),
             "strict_map": Path(args.strict_map), "partial_models": Path(args.partial_models),
             "driver_internal": Path(args.driver_internal), "resident_runtime": Path(args.resident_runtime),
             "identities": Path(args.identities),
             "ready_helper_rules": Path(__file__),
             "resident_ledger": Path(args.repo) / RESIDENT_LEDGER}
    for name, relative in FRESHNESS_INPUTS.items():
        if name.startswith("ready_helper_") and name != "ready_helper_rules":
            paths[name] = Path(args.repo) / relative
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
        driver_text = blobs["driver_internal"].decode()
        driver_rows = csv_rows(blobs["driver_internal"])
        runtime_text = blobs["resident_runtime"].decode()
        runtime_rows = csv_rows(blobs["resident_runtime"])
        image = None
        if driver_rows or runtime_rows or READY_HELPER_CANDIDATES:
            # A member must be closure-complete; that is decided from the bytes, so the check cannot be skipped.
            exe_path = Path(args.exe)
            require(exe_path.is_file(), f"readiness classes need the original-byte check and "
                                        f"START.EXE: {exe_path}")
            exe = exe_path.read_bytes()
            require(hashlib.sha256(exe).hexdigest() == DIGESTS["start_exe_sha256"],
                    "START.EXE hash differs from the audited build")
            from resident_loader_map import unpack
            image = bytes(unpack(exe)[0])
        strict_residents = frozenset(row["id"] for row in csv_rows(blobs["strict_map"]) if row["id"].startswith("RES_"))
        runtime_ids = frozenset(row["id"] for row in runtime_rows)
        driver_internal = load_driver_internal(driver_rows, repo,
                                               fields=csv.DictReader(driver_text.splitlines()).fieldnames,
                                               image=image, strict_ids=strict_residents, resident_runtime=runtime_ids)
        resident_runtime = load_resident_runtime(runtime_rows, repo,
                                                 fields=csv.DictReader(runtime_text.splitlines()).fieldnames,
                                                 image=image, strict_ids=strict_residents,
                                                 driver_internal=driver_internal)
        if READY_HELPER_CANDIDATES:
            require("ovr" in blobs, "ready_helper caller verification requires --ovr GAME.OVR")
            verify_ready_helper_callers(exe, blobs["ovr"], csv_rows(blobs["ledger"]),
                                        csv_rows(blobs["resident_ledger"]))
        ready_helpers, helper_assessments = load_ready_helpers(
            csv_rows(blobs["classification"]), csv_rows(blobs["strict_map"]),
            csv_rows(blobs["identities"]), repo, image)
        state = load_queue(inventory, csv_rows(blobs["ledger"]),
                           csv_rows(blobs["classification"]),
                           csv_rows(blobs["strict_map"]), near,
                           csv_rows(blobs["partial_models"]), driver_internal,
                           csv_rows(blobs["identities"]), resident_runtime,
                           frozenset() if args.without_ready_helpers else ready_helpers)
        main_files = scan_main_files(repo, state["all_ids"])
        state["pinned_spans"] = pinned_spans(repo)
        for identifier, evidence in sorted(state["full_models"].items()):
            main_files.setdefault(identifier, []).append(evidence)
        mentions = scan_mentions(repo, state["candidates"], stub_index(state))
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
    result["ready_helpers"] = dict(
        enabled=not args.without_ready_helpers, members=sorted(state["ready_helpers"]),
        candidates=helper_assessments)
    result["artifacts_sha256"] = {name: hashlib.sha256(blob).hexdigest()
                                  for name, blob in sorted(blobs.items())}
    discards = result.pop("discards")
    export = json.dumps(result, indent=2, sort_keys=True) + "\n"
    Path(args.json).write_text(export)
    if args.discards:          # a snapshot: it names the tree and the export it was measured at
        head = subprocess.run(["git", "-C", str(repo), "rev-parse", "HEAD"], capture_output=True, text=True).stdout.strip()
        Path(args.discards).write_text(json.dumps(dict(
            schema_version=2, kind="pinned measurement of the strict queue detector's discards, not a live list",
            pin=dict(tree_commit=head or None, export_sha256=hashlib.sha256(export.encode()).hexdigest()),
            artifacts_sha256=result["artifacts_sha256"], git=result["git"],
            count=len(discards), by_rule=result["summary"]["discarded_by_rule"], discards=discards),
            indent=1, sort_keys=True) + "\n")
    print(json.dumps(result["summary"], sort_keys=True))


if __name__ == "__main__":
    main()
