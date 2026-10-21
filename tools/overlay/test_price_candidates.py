"""Failure paths for the candidate pricer's input handling.

The pricer reads a gitignored, derived queue file, so a fresh checkout does not
have it. These tests pin the two acceptable behaviours: generate it when the
prerequisites exist, and otherwise stop with an actionable message and a
distinct exit code — never a bare traceback.
"""
import collections
import hashlib
import io
import json
import contextlib
import tempfile
import unittest
from pathlib import Path

import price_candidates as pricer


class InputHandlingTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.queue = self.root / "queue.json"

    def test_fresh_checkout_reports_actionably(self):
        message = pricer.input_problem(self.queue, self.root)
        self.assertIsNotNone(message)
        self.assertIn("QUEUE INPUT MISSING", message)
        self.assertIn("NOT a tree failure", message)
        # It must name the exact commands, not send the reader to the source.
        self.assertIn("function_queue_audit.py", message)
        self.assertIn("strict_candidate_queue.py", message)
        self.assertIn("resources/DOS/POOLRAD", message)

    def test_present_input_is_not_a_problem(self):
        self.queue.write_text('{"rows": []}')
        self.assertIsNone(pricer.input_problem(self.queue, self.root))

    def test_prerequisites_detected_as_generatable(self):
        accounting = self.root / "resources/function-accounting-current"
        media = self.root / "resources/DOS/POOLRAD"
        accounting.mkdir(parents=True)
        media.mkdir(parents=True)
        for name in ("inventory.json", "callgraph.json", "near.json"):
            (accounting / name).write_text("{}")
        (media / "START.EXE").write_bytes(b"MZ")
        (media / "GAME.OVR").write_bytes(b"OVR")
        self.assertEqual(pricer.input_problem(self.queue, self.root), "generate")

    def test_main_exits_with_input_code_not_a_traceback(self):
        real = pricer.gate.missing_dependencies
        pricer.gate.missing_dependencies = lambda: []   # this test is about the INPUT
        buffer = io.StringIO()
        try:
            with contextlib.redirect_stdout(buffer):
                code = pricer.main(["--root", str(self.root), "--queue", str(self.queue)])
        finally:
            pricer.gate.missing_dependencies = real
        self.assertEqual(code, pricer.INPUT_EXIT)
        self.assertIn("QUEUE INPUT MISSING", buffer.getvalue())

    def test_missing_environment_is_distinct_from_missing_input(self):
        real = pricer.gate.missing_dependencies
        pricer.gate.missing_dependencies = lambda: ["capstone==5.0.7"]
        buffer = io.StringIO()
        try:
            with contextlib.redirect_stdout(buffer):
                code = pricer.main(["--root", str(self.root), "--queue", str(self.queue)])
        finally:
            pricer.gate.missing_dependencies = real
        self.assertEqual(code, pricer.gate.ENVIRONMENT_EXIT)
        self.assertIn("ENVIRONMENT MISSING", buffer.getvalue())


class ReadinessVocabularyTest(unittest.TestCase):
    """The unready set is the definition of an unusable dependency.

    A hand-written subset of this set omitted `indirect_unresolved`, so six rows
    carrying an indirect dispatch were counted as having zero unready callees.
    The pricer's own menu hid the bug by skipping indirect rows before reading
    the set; the first new consumer inherited it and priced a dispatcher as free.
    """

    def test_indirect_dispatch_is_unready(self):
        # Pinned directly: an indirect callee's target set is unbounded, so no
        # one-hop figure is a bound for a row that contains one.
        self.assertIn("indirect_unresolved", pricer.UNREADY)

    def test_ready_and_unready_partition_the_vocabulary(self):
        self.assertEqual(pricer.UNREADY,
                         pricer.READINESS_STATES - pricer.USABLE_DEPENDENCIES)
        self.assertEqual(pricer.UNREADY | pricer.USABLE_DEPENDENCIES,
                         pricer.READINESS_STATES)
        self.assertEqual(pricer.UNREADY & pricer.USABLE_DEPENDENCIES, set())

    def test_declared_boundary_stays_usable_but_is_not_a_credit(self):
        # Usable for composition; the credit prohibition is enforced elsewhere.
        self.assertNotIn("declared_boundary", pricer.UNREADY)

    def test_queue_emits_only_known_states(self):
        # Drift guard: a new readiness state in the queue must be classified
        # here, because an unclassified state defaults to "ready".
        queue = pricer.DEFAULT_QUEUE
        if not Path(queue).exists():
            self.skipTest("derived queue not present in this checkout")
        rows = json.loads(Path(queue).read_text())["rows"]
        emitted = {t["readiness"] for row in rows
                   for t in row["callee_readiness"]["targets"]}
        self.assertEqual(emitted - pricer.READINESS_STATES, set())

    def test_indirect_only_row_is_reported_blocked(self):
        # Behavioural, not a restatement of the constant: a row whose only
        # unready callee class is an indirect dispatch must be reported as
        # blocked, never as carrying zero unready dependencies.
        row = {"id": "OVL_0001_0001",
               "callee_readiness": {"targets": [{"kind": "runtime",
                                                 "readiness": "indirect_unresolved"}],
                                    "counts": {"indirect_unresolved": 1},
                                    "raw_cpu_caveats": []}}
        self.assertEqual(pricer.sole_blockers([row]),
                         collections.Counter({"INDIRECT": 1}))

    def test_indirect_callee_with_an_id_is_counted_unready(self):
        # The omission showed up exactly here: consumers build their unready set
        # as `readiness in UNREADY and has an id`, so an indirect callee that
        # carries an id was invisible and its row looked actionable.
        targets = [{"id": "OVL_009E_0000", "readiness": "indirect_unresolved"}]
        unready = {t.get("id") for t in targets
                   if t["readiness"] in pricer.UNREADY and t.get("id")}
        self.assertEqual(unready, {"OVL_009E_0000"})
        # And the row is reported as blocked by the dispatch as well, so a
        # reviewer sees two blockers rather than zero.
        row = {"id": "OVL_0001_0001",
               "callee_readiness": {"targets": targets,
                                    "counts": {"indirect_unresolved": 1},
                                    "raw_cpu_caveats": []}}
        self.assertEqual(pricer.sole_blockers([row]), collections.Counter())



class ExportFreshnessTest(unittest.TestCase):
    """A consumer must refuse an export written before a committed input changed.

    On 2026-10-20 three figures were true of a file and false of the tree (453 rows,
    455 rows, and a 578 B price for a row whose callee was credited at 12:47): each
    came from an export that existed, so nothing regenerated it.
    """
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        for name, relative in pricer.queue_rules.FRESHNESS_INPUTS.items():
            path = self.root / relative
            path.parent.mkdir(parents=True, exist_ok=True)
            path.write_text("id,evidence_doc\n")
        strict = self.root / pricer.queue_rules.FRESHNESS_INPUTS["strict_map"]
        strict.write_text("id,evidence_doc\nOVL_0001_0000,docs/re/A.md\n")
        self.queue = self.root / "resources/strict-candidate-queue/queue.json"
        self.queue.parent.mkdir(parents=True)
        recorded = {name: hashlib.sha256((self.root / relative).read_bytes()).hexdigest()
                    for name, relative in pricer.queue_rules.FRESHNESS_INPUTS.items()}
        self.queue.write_text(json.dumps({"rows": [], "artifacts_sha256": recorded}))

    def credit_one_more(self):
        strict = self.root / pricer.queue_rules.FRESHNESS_INPUTS["strict_map"]
        strict.write_text(strict.read_text() + "OVL_0002_0000,docs/re/B.md\n")

    def refusal(self, call):
        err = io.StringIO()
        with contextlib.redirect_stderr(err), self.assertRaises(SystemExit) as stop:
            call()
        return stop.exception.code, err.getvalue()

    def test_current_export_loads(self):
        export = pricer.queue_rules.load_fresh_export(self.queue, self.root)
        self.assertEqual(export["rows"], [])

    def test_export_one_credit_behind_is_refused_naming_the_input(self):
        self.credit_one_more()
        code, message = self.refusal(lambda: pricer.queue_rules.load_fresh_export(self.queue, self.root))
        self.assertEqual(code, pricer.queue_rules.STALE_EXIT)
        self.assertIn("strict_map (docs/re/strict_full_c_addresses.csv)", message)
        for other in ("identities", "driver_internal", "partial_models", "ledger", "classification"):
            self.assertNotIn(f"  {other} (", message)   # only the input that changed is named
        self.assertIn("strict_candidate_queue.py", message)   # and how to regenerate it

    def test_export_without_recorded_hashes_is_refused(self):
        self.queue.write_text(json.dumps({"rows": []}))
        code, message = self.refusal(lambda: pricer.queue_rules.load_fresh_export(self.queue, self.root))
        self.assertEqual(code, pricer.queue_rules.STALE_EXIT)
        self.assertIn("not recorded", message)

    def test_consumer_refuses_a_present_but_stale_export(self):
        # The pricer regenerated only a MISSING file, so a stale one was silently preferred.
        self.credit_one_more()
        real = pricer.gate.missing_dependencies
        pricer.gate.missing_dependencies = lambda: []
        try:
            code, message = self.refusal(lambda: pricer.main(["--root", str(self.root), "--queue", str(self.queue)]))
        finally:
            pricer.gate.missing_dependencies = real
        self.assertEqual(code, pricer.queue_rules.STALE_EXIT)
        self.assertIn("strict_map", message)

if __name__ == "__main__":
    unittest.main()
