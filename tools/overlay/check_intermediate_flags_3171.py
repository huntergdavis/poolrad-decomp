#!/usr/bin/env python3
"""Companion reproduction for t135; does not modify the candidate author's files.

Supply a candidate checkout containing u1c_run_3171_cpu_audit.py and its C/header.
Original media comes from this checkout's ignored resources/. Run through the
shared heavy wrapper. This is a boundary-observation check, not row acceptance.
"""
# NOT_A_ROW_CHECKER: instrumentation proof against an explicitly supplied draft.
import argparse
import hashlib
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

from intermediate_flags import ARITH, Contract, FlagObservationError, Trace, compare, hook_unicorn

ROOT = Path(__file__).resolve().parents[2]
AUDIT = Path("tools/overlay/u1c_run_3171_cpu_audit.py")
SOURCE = Path("src/recovered/overlay_u1c_run_3171_cpu.c")
HEADER = Path("include/poolrad/recovered/overlay_u1c_run_3171_cpu.h")
SITE = "3171:add-di-ax-before-dispatch"
ANCHOR = "        add16_flags(cpu, before, pc);"
ASSIGNMENT = "cpu->flags = (uint16_t)((cpu->flags & (uint16_t)~FLAG_MASK) | flags);"


def require(condition, reason):
    if not condition:
        raise ValueError(reason)


def load_candidate(root):
    spec = importlib.util.spec_from_file_location("t135_candidate_3171", root / AUDIT)
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    module.ROOT = root
    module.EXE = ROOT / "resources/DOS/POOLRAD/START.EXE"
    module.OVR = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
    _, _, module.source_bytes = module.source()
    # Body hash is checked by source(). Establish that this callback sees the ADD
    # result: no flag writer lies between ADD DI,AX and the dispatcher CALL.
    decoded = list(module.Cs(module.CS_ARCH_X86, module.CS_MODE_16).disasm(
        module.source_bytes, module.ENTRY))
    producer = next(i for i in decoded if i.address == 0x319A)
    require((producer.mnemonic, producer.op_str) == ("add", "di, ax"), "ADD site changed")
    between = [i for i in decoded if 0x319A < i.address < 0x31A8]
    require([i.mnemonic for i in between] == ["mov", "mov", "push", "call"],
            "callback no longer preserves ADD FLAGS")
    return module


def observed_compare(audit, label, ram, lib, dispatch_type):
    """Keep the author's full final-state comparison and add both entry traces."""
    original, candidate = Trace((SITE,)), Trace((SITE,))
    real_uc = audit.Uc

    def traced_uc(*args, **kwargs):
        emu = real_uc(*args, **kwargs)
        # Installed before the author's callback substitution hook.
        hook_unicorn(emu, original, {(audit.UNIT_CS, audit.DISPATCH_REL): SITE})
        return emu

    def traced_dispatch(callback):
        @dispatch_type
        def observe(ptr, size, cpu_ptr, context):
            candidate.record(SITE, cpu_ptr.contents.flags)
            return callback(ptr, size, cpu_ptr, context)
        return observe

    audit.Uc = traced_uc
    try:
        # This still checks full RAM, CPU14, status and dispatch counts. A crash
        # or unrelated failure is not caught and cannot masquerade as a flag kill.
        calls = audit.compare(label, ram, lib, traced_dispatch)
    finally:
        audit.Uc = real_uc
    original.checked()
    candidate.checked()
    return (original, candidate), calls


def run(candidate_root):
    root = candidate_root.resolve()
    hashes = {str(path): hashlib.sha256((root / path).read_bytes()).hexdigest()
              for path in (AUDIT, SOURCE, HEADER)}
    audit = load_candidate(root)
    cases = list(audit.cases())
    require(len({label for label, _ in cases}) == len(cases) and cases, "empty/duplicate cases")
    text = (root / SOURCE).read_text()
    require(text.count(ANCHOR) == 1, "wrong-ADD mutation needs exactly one source anchor")
    baseline, mutant = {}, {}
    with tempfile.TemporaryDirectory(prefix="poolrad-flag-observations-") as tmp:
        directory = Path(tmp)
        base_dir, mutant_dir = directory / "baseline", directory / "mutant"
        base_dir.mkdir()
        mutant_dir.mkdir()
        lib, dispatch_type = audit.build(base_dir)
        dispatches = 0
        for label, case in cases:
            pair, count = observed_compare(audit, label, audit.setup_ram(**case), lib, dispatch_type)
            baseline[label] = pair
            dispatches += count
        contract = Contract((SITE,), baseline)

        # Only a temporary copy changes; the candidate checkout is read-only.
        mutant_root = directory / "candidate"
        for path in (SOURCE, HEADER):
            (mutant_root / path).parent.mkdir(parents=True, exist_ok=True)
            shutil.copyfile(root / path, mutant_root / path)
        (mutant_root / SOURCE).write_text(text.replace(
            ANCHOR, ANCHOR + f"\n        cpu->flags ^= 0x{ARITH:04X}u; /* wrong ADD FLAGS */"))
        audit.ROOT = mutant_root
        mutated, mutated_type = audit.build(mutant_dir)
        audit.ROOT = root
        for label, case in cases:
            pair, _ = observed_compare(audit, label, audit.setup_ram(**case), mutated, mutated_type)
            mutant[label] = pair
        control = contract.require_wrong_flags(SITE, mutant)
        receipt = contract.receipt()

        # Independent review i25007 used a different mutant: replace the ADD
        # assignment with XOR of incoming flags, leaving CMP intact. Reproduce
        # it separately; its delta need not be exactly ARITH versus correct ADD.
        require(text.count(ASSIGNMENT) == 2, "reviewer mutant needs the two known flag assignments")
        (mutant_root / SOURCE).write_text(text.replace(
            ASSIGNMENT, "(void)flags; cpu->flags ^= FLAG_MASK; /* reviewer ADD mutant */", 1))
        reviewer_dir = directory / "reviewer-mutant"
        reviewer_dir.mkdir()
        audit.ROOT = mutant_root
        reviewed_mutant, reviewed_type = audit.build(reviewer_dir)
        audit.ROOT = root
        reviewer_killed = []
        for label, case in cases:
            pair, _ = observed_compare(audit, label, audit.setup_ram(**case), reviewed_mutant, reviewed_type)
            want, got = pair[0].checked(), pair[1].checked()
            require(want == baseline[label][0].checked(), "reviewer mutant changed original observations")
            require([e.site for e in want] == [e.site for e in got],
                    "reviewer mutant changed event order/count, not only flags")
            try:
                compare(*pair, label)
            except FlagObservationError:
                reviewer_killed.append(label)
        require(reviewer_killed, "reviewer wrong-ADD mutant survived boundary comparison")
    require(hashes == {str(path): hashlib.sha256((root / path).read_bytes()).hexdigest()
                       for path in (AUDIT, SOURCE, HEADER)}, "candidate files changed during audit")
    return {"result": "PASS", "candidate_sha256": hashes,
            "media_sha256": {path.name: hashlib.sha256(path.read_bytes()).hexdigest()
                             for path in (audit.EXE, audit.OVR)},
            "body_sha256": audit.BODY_SHA, "baseline_final_state_cases": len(cases),
            "baseline_dispatches": dispatches, "legacy_final_state_mutant_kills": 0,
            "boundary_mutant_kills": len(control["killed"]), "flags": receipt,
            "reviewer_replacement_mutant": {
                "mutation": "replace ADD assignment with XOR of incoming arithmetic flags; CMP intact",
                "cases": len(cases), "legacy_final_state_mutant_kills": 0,
                "boundary_mutant_kills": len(reviewer_killed), "killed": reviewer_killed},
            "scope": "declared ADD-to-dispatch boundary; real dispatcher substituted",
            "not_verified": ["native execution", "real dispatcher semantics", "natural reachability",
                             "every arithmetic producer in this or other bodies", "row acceptance"]}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--candidate-root", required=True, type=Path)
    parser.add_argument("--output", required=True, type=Path)
    args = parser.parse_args()
    # Remove a stale success receipt before a failing/aborted rerun.
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps({"result": "INCOMPLETE"}) + "\n")
    try:
        receipt = run(args.candidate_root)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        args.output.write_text(json.dumps({"result": "FAIL", "error": str(error)}, indent=2) + "\n")
        raise SystemExit(f"intermediate FLAGS check failed: {error}")
    args.output.write_text(json.dumps(receipt, indent=2) + "\n")
    print(json.dumps(receipt, indent=2))


if __name__ == "__main__":
    main()
