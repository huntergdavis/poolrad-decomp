"""The fail-closed exclusion harness must catch each of its five attacks, and must
flag an audit that is missing the check that attack targets.

The tests use a synthetic audit with the U3B0 +01DA contract. No media, Capstone or
Unicorn is needed. The audit has one conditional at root:0002 with outcomes to 0003
and 0004. It argues 0002->0003 infeasible, and start 0003 with it. Each "broken"
variant drops exactly one of the three checks a sound audit needs, so each attack
proves that it can fail.
"""
import contextlib
import io
import sys
import types
import unittest

import check_exclusion_fail_closed as tool


def key(offset):
    return ("root", offset)


EXPECTED = {(key(2), key(3)), (key(2), key(4))}
DECODED = {key(offset): None for offset in (1, 2, 3, 4, 5)}
OBSERVED = {"a": ({key(1), key(2), key(4)}, {(key(2), key(4))}),
            "b": ({key(2), key(4), key(5)}, {(key(2), key(4))})}


def check(condition, message):
    if not condition:
        raise ValueError(message)


def make_audit(observed_check=True, real_check=True, coverage_check=True,
               baseline_ok=True, crash_on_attack=False):
    audit = types.ModuleType("synthetic_audit")
    audit.INFEASIBLE_EDGES = {(key(2), key(3)): "same byte read twice"}
    audit.INFEASIBLE_STARTS = {key(3): "only reached by the impossible fall-through"}
    audit.calls = 0

    def compare(name, *args):
        audit.calls += 1
        seen, edges = OBSERVED[name]
        if not baseline_ok and name == "a":
            seen = set()
        return 0, set(seen), set(edges), 0

    def differential(bodies, decoded, lib):
        seen, edges = set(), set()
        for name in OBSERVED:
            _, s, e, _ = audit.compare(name)
            seen |= s
            edges |= e
        if crash_on_attack and len(audit.INFEASIBLE_EDGES) != 1:
            raise KeyError("unrelated bug")
        infeasible_edges, infeasible_starts = set(audit.INFEASIBLE_EDGES), set(audit.INFEASIBLE_STARTS)
        if real_check:
            check(infeasible_edges <= EXPECTED and infeasible_starts <= set(decoded),
                  "infeasible set names real caller edges/starts")
        if observed_check:
            check(not infeasible_edges & edges and not infeasible_starts & seen,
                  "an edge argued infeasible was observed")
        if coverage_check:
            check(set(decoded) - infeasible_starts <= seen, "caller feasible start coverage incomplete")
            check(EXPECTED - infeasible_edges <= edges, "caller feasible outcome coverage incomplete")
        return {"caller_feasible_starts": len((set(decoded) - infeasible_starts) & seen),
                "caller_total": len(decoded)}

    audit.compare = compare
    audit.differential = differential
    return audit


def outcomes(audit):
    baseline, results = tool.run(audit, {}, DECODED, None)
    return baseline, {label.split(" ", 1)[0]: outcome for label, outcome, _ in results
                      if label[0].isdigit()}, results


class FailClosedHarnessTest(unittest.TestCase):
    def test_sound_audit_detects_all_five_attacks(self):
        baseline, by_attack, results = outcomes(make_audit())
        self.assertEqual(baseline["caller_feasible_starts"], 4)
        self.assertEqual(by_attack, {str(n): "detected" for n in range(1, 6)})
        self.assertEqual(tool.verdict(results), 0)

    def test_attack1_and_2_flag_missing_observed_check(self):
        _, by_attack, results = outcomes(make_audit(observed_check=False))
        self.assertEqual(by_attack["1"], "passed")
        self.assertEqual(by_attack["2"], "passed")
        self.assertEqual(tool.verdict(results), 1)

    def test_attack3_flags_relabelled_feasible_edge(self):
        _, by_attack, results = outcomes(make_audit(observed_check=False))
        self.assertEqual(by_attack["3"], "passed")
        self.assertEqual(tool.verdict(results), 1)

    def test_attack4_flags_missing_real_edge_check(self):
        _, by_attack, results = outcomes(make_audit(real_check=False))
        self.assertEqual(by_attack["4"], "passed")
        self.assertEqual({n: by_attack[n] for n in "1235"}, {n: "detected" for n in "1235"})
        self.assertEqual(tool.verdict(results), 1)

    def test_attack5_flags_missing_coverage_enforcement(self):
        _, by_attack, results = outcomes(make_audit(coverage_check=False))
        self.assertEqual(by_attack["5"], "passed")
        self.assertEqual(tool.verdict(results), 1)

    def test_crash_is_not_a_detection(self):
        _, by_attack, results = outcomes(make_audit(crash_on_attack=True))
        self.assertEqual(by_attack["5"], "crashed")
        self.assertEqual(tool.verdict(results), 1)

    def test_failed_baseline_is_reported_not_judged(self):
        baseline, _, results = outcomes(make_audit(baseline_ok=False))
        self.assertIsNone(baseline)
        self.assertEqual(tool.verdict(results), 2)

    def test_compare_runs_once_per_case(self):
        audit = make_audit()
        tool.run(audit, {}, DECODED, None)
        self.assertEqual(audit.calls, len(OBSERVED))

    def test_module_state_restored_after_attacks(self):
        audit = make_audit()
        edges, starts = dict(audit.INFEASIBLE_EDGES), dict(audit.INFEASIBLE_STARTS)
        tool.run(audit, {}, DECODED, None)
        self.assertEqual((audit.INFEASIBLE_EDGES, audit.INFEASIBLE_STARTS), (edges, starts))

    def test_audit_without_exclusion_is_refused(self):
        audit = make_audit()
        audit.INFEASIBLE_EDGES, audit.INFEASIBLE_STARTS = {}, {}
        with self.assertRaises(tool.UnsupportedShape):
            tool.run(audit, {}, DECODED, None)


def shaped_module(name, source_values, differential_params, exclusion=True):
    """A module with a given source() arity and differential() signature, as main() sees it."""
    audit = types.ModuleType(name)
    audit.source_calls = 0
    if exclusion:
        audit.INFEASIBLE_EDGES = {(key(2), key(3)): "x"}
        audit.INFEASIBLE_STARTS = {}

    def source():
        audit.source_calls += 1
        return tuple(range(source_values))

    namespace = {}
    exec(f"def differential({', '.join(differential_params)}): return {{}}", namespace)
    audit.source, audit.differential = source, namespace["differential"]
    audit.library = lambda directory: None
    audit.compare = lambda name, *a: (0, set(), set())
    return audit


class AuditShapeTest(unittest.TestCase):
    """The two shapes the ds4 lane hit must be refused with exit 4, never a traceback."""

    def main_with(self, audit):
        sys.modules[audit.__name__] = audit
        self.addCleanup(sys.modules.pop, audit.__name__, None)
        err = io.StringIO()
        with contextlib.redirect_stderr(err), contextlib.redirect_stdout(io.StringIO()):
            code = tool.main([audit.__name__])
        return code, err.getvalue()

    def test_five_value_source_is_unsupported_not_a_crash(self):
        # u3b0_dispose_11f3_cpu_audit shape: 5-value source(), 5-parameter differential().
        audit = shaped_module("shape_five", 5, ("image", "blob", "decoded", "conditional", "f"))
        code, err = self.main_with(audit)
        self.assertEqual(code, tool.UNSUPPORTED_EXIT)
        self.assertIn("UNSUPPORTED AUDIT SHAPE: shape_five", err)
        self.assertIn("differential() takes 5 positional parameters", err)
        self.assertEqual(audit.source_calls, 0, "static shape must be refused before source() reads media")

    def test_eight_value_source_without_declared_exclusion_is_unsupported(self):
        # u6c_record_unlink_3291_audit shape: 8-value source(), differential(ctx, f), no INFEASIBLE_* globals.
        audit = shaped_module("shape_eight", 8, ("ctx", "f"), exclusion=False)
        code, err = self.main_with(audit)
        self.assertEqual(code, tool.UNSUPPORTED_EXIT)
        self.assertIn("declares no INFEASIBLE_EDGES/INFEASIBLE_STARTS", err)
        self.assertEqual(audit.source_calls, 0)

    def test_wrong_source_arity_with_right_signature_is_caught_after_source(self):
        audit = shaped_module("shape_source_only", 5, ("bodies", "decoded", "lib"))
        code, err = self.main_with(audit)
        self.assertEqual(code, tool.UNSUPPORTED_EXIT)
        self.assertIn("source() returned 5 values, not 3", err)
        self.assertEqual(audit.source_calls, 1)

    def test_supported_shape_passes_the_guard(self):
        self.assertEqual(tool.static_shape(shaped_module("shape_ok", 3, ("bodies", "decoded", "lib"))), [])
        self.assertEqual(tool.source_shape((1, 2, 3)), [])


if __name__ == "__main__":
    unittest.main()
