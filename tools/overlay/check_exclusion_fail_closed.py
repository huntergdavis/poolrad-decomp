#!/usr/bin/env python3
"""Prove that an audit's infeasible-coverage exclusion fails closed.

Gate-side verification tool, NOT a row checker. Its name is outside the
`*_audit.py` / `*_native.py` checker pattern and it declares no CHECKER_ROWS,
so neither check_body_doc_pairing.py nor check_checker_builds.py treats it as
any row's checker, and it needs no NOT_A_ROW_CHECKER marker.

An audit may argue that some caller starts or conditional outcomes can never
execute. It can then report feasible coverage (for example 171/171) next to raw
coverage (171/172). That is a proof only if three things hold: the excluded
items stay in the reported totals, the run FAILS if any excluded item is ever
observed, and the exclusion is what closes the gap. Otherwise it is a filter
that hides a gap. This tool tests the claim by breaking it.

The audit's expensive compare() runs once per case and is memoised. The audit's
own differential() coverage logic is then re-run with the claim deliberately
broken, five ways:

  1 observed-edge     add each infeasible edge to one case's observed edges
  2 observed-start    add each infeasible start to one case's observed starts
  3 relabel-feasible  declare an observed edge (a sibling outcome of the same
                      branch where one exists) infeasible
  4 nonexistent       declare an edge whose target is not a decoded instruction
  5 empty             remove the exclusion; coverage must then be incomplete

Every attack must make differential() raise ValueError, which is the audits'
check() type. Any other exception is reported as a crash, not a detection. The
unbroken baseline must pass first; if it fails, the audit itself is red, and
the result says nothing about fail-closed behaviour.

Audit contract (the U3B0 +01DA convention, #1216):
  INFEASIBLE_EDGES  {(src_key, dst_key): reason}   module global
  INFEASIBLE_STARTS {key: reason}                  module global
  source() -> (bodies, decoded, meta);  library(directory) -> lib
  compare(name, ...) -> (status, seen_starts, seen_edges, ...)
  differential(bodies, decoded, lib) -> report, calling compare() through the
  module global, so it can be memoised.

Usage (needs the owner's media and the pinned checker dependencies):
  python3 tools/overlay/check_exclusion_fail_closed.py u3b0_entry_01da_cpu_audit
Exit 0: every attack detected. 1: an attack passed or crashed. 2: the baseline
failed. 3: environment missing (not a tree failure).
"""
import argparse
import importlib
import sys
import tempfile
from pathlib import Path

NONEXISTENT_TARGET = 0x10000  # outside any 16-bit offset, so never decoded


def memoise(audit):
    """Replace audit.compare with a per-case cache; return (cache, call counter)."""
    real = audit.compare
    cache = {}
    calls = {"n": 0}

    def compare(name, *args, **kwargs):
        if name not in cache:
            calls["n"] += 1
            cache[name] = real(name, *args, **kwargs)
        status, seen, edges, *rest = cache[name]
        return (status, set(seen), set(edges), *rest)

    audit.compare = compare
    return cache, calls


def observed(cache):
    seen, edges = set(), set()
    for _, s, e, *_ in cache.values():
        seen |= set(s)
        edges |= set(e)
    return seen, edges


def attacks(audit, cache):
    """Yield (label, edges, starts, inject_seen, inject_edges) for the five attacks."""
    edges0, starts0 = dict(audit.INFEASIBLE_EDGES), dict(audit.INFEASIBLE_STARTS)
    seen, edges = observed(cache)
    for edge in edges0:
        yield f"1 observed-edge {fmt_edge(edge)}", edges0, starts0, set(), {edge}
    for start in starts0:
        yield f"2 observed-start {fmt_key(start)}", edges0, starts0, {start}, set()
    # Attacks 3 and 4 ADD to the real exclusion rather than replace it. Replacing it
    # would re-require the real infeasible edge, so coverage would fail whether or not
    # the check under attack exists. Adding isolates the one check each attack targets.
    for src, dst in edges0:
        siblings = sorted(e for e in edges if e[0] == src and e != (src, dst)) or sorted(edges)
        if siblings:
            yield (f"3 relabel-feasible {fmt_edge(siblings[0])}", {**edges0, siblings[0]: "attack"},
                   starts0, set(), set())
        bogus = (src, (src[0], NONEXISTENT_TARGET))
        yield f"4 nonexistent {fmt_edge(bogus)}", {**edges0, bogus: "attack"}, starts0, set(), set()
    yield "5 empty (no exclusion)", {}, {}, set(), set()


def fmt_key(key):
    return f"{key[0]}:{key[1]:04X}"


def fmt_edge(edge):
    return f"{fmt_key(edge[0])}->{fmt_key(edge[1])}"


def run(audit, bodies, decoded, lib):
    """Return (baseline_report, [(label, outcome, detail)]); outcome is detected/passed/crashed."""
    if not (getattr(audit, "INFEASIBLE_EDGES", None) or getattr(audit, "INFEASIBLE_STARTS", None)):
        raise SystemExit(f"{audit.__name__} declares no INFEASIBLE_EDGES/INFEASIBLE_STARTS")
    cache, calls = memoise(audit)
    edges0, starts0 = dict(audit.INFEASIBLE_EDGES), dict(audit.INFEASIBLE_STARTS)
    try:
        baseline = audit.differential(bodies, decoded, lib)
    except ValueError as error:
        return None, [("baseline", "failed", str(error))]
    first = next(iter(cache))
    results = []
    for label, edges, starts, add_seen, add_edges in list(attacks(audit, cache)):
        saved = cache[first]
        status, seen, observed_edges, *rest = saved
        cache[first] = (status, set(seen) | add_seen, set(observed_edges) | add_edges, *rest)
        audit.INFEASIBLE_EDGES, audit.INFEASIBLE_STARTS = dict(edges), dict(starts)
        try:
            audit.differential(bodies, decoded, lib)
            results.append((label, "passed", "claim break NOT detected"))
        except ValueError as error:
            results.append((label, "detected", str(error)))
        except Exception as error:  # a crash is not a detection
            results.append((label, "crashed", f"{type(error).__name__}: {error}"))
        finally:
            cache[first] = saved
            audit.INFEASIBLE_EDGES, audit.INFEASIBLE_STARTS = edges0, starts0
    results.append(("compare calls", "info", f"{calls['n']} for {len(cache)} cases"))
    return baseline, results


def verdict(results):
    if any(outcome == "failed" for _, outcome, _ in results):
        return 2
    return 1 if any(outcome in ("passed", "crashed") for _, outcome, _ in results) else 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    parser.add_argument("audit", help="audit module name under tools/overlay, e.g. u3b0_entry_01da_cpu_audit")
    args = parser.parse_args(argv)
    sys.path.insert(0, str(Path(__file__).resolve().parent))
    try:
        audit = importlib.import_module(args.audit)
    except ModuleNotFoundError as error:
        if error.name in ("capstone", "unicorn"):
            print(f"ENVIRONMENT MISSING: {error.name} not importable - this is NOT a tree failure. "
                  "pip install capstone==5.0.7 unicorn==2.1.4 in a venv; see "
                  "docs/re/REVIEW_LANE_HANDOFF.md.", file=sys.stderr)
            return 3
        raise
    bodies, decoded, _ = audit.source()
    with tempfile.TemporaryDirectory() as tmp:
        lib = audit.library(Path(tmp))
        baseline, results = run(audit, bodies, decoded, lib)
    if baseline is not None:
        keys = [k for k in baseline if not k.startswith("missing")]
        print("BASELINE", {k: baseline[k] for k in keys})
    for label, outcome, detail in results:
        print(f"{outcome.upper():9} {label}: {detail}")
    code = verdict(results)
    print({0: "FAIL-CLOSED: every attack detected", 1: "NOT FAIL-CLOSED", 2: "BASELINE FAILED"}[code])
    return code


if __name__ == "__main__":
    sys.exit(main())
