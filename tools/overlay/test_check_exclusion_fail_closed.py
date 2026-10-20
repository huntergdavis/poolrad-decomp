"""The fail-closed exclusion harness must catch each of its five attacks, and must
flag an audit that is missing the check that attack targets.

The tests use a synthetic audit with the U3B0 +01DA contract. No media, Capstone or
Unicorn is needed. The audit has one conditional at root:0002 with outcomes to 0003
and 0004. It argues 0002->0003 infeasible, and start 0003 with it. Each "broken"
variant drops exactly one of the three checks a sound audit needs, so each attack
proves that it can fail.
"""
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
        with self.assertRaises(SystemExit):
            tool.run(audit, {}, DECODED, None)


if __name__ == "__main__":
    unittest.main()
