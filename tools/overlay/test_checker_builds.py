"""Failure paths for the pre-handoff gate, using synthetic checker programs."""
from pathlib import Path
import shutil
import tempfile
import unittest

import check_checker_builds as gate


class CheckerBuildGateTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        (self.root / "tests/recovered").mkdir(parents=True)
        self.path = Path("tests/recovered/check_synthetic.py")

    def checker(self, program):
        (self.root / self.path).write_text(
            'CHECKER_ROWS = ("OVL_0038_12B2",)\n' + program)

    def test_unknown_row_blocks_instead_of_skipping(self):
        self.checker('print("CHECKER_BUILD_OK")\n')
        with self.assertRaisesRegex(ValueError, "unregistered.*handoff blocked"):
            gate.select(gate.discover(self.root), ["OVL_0038_2B20"])

    def test_empty_selection_blocks(self):
        with self.assertRaisesRegex(ValueError, "no registered rows"):
            gate.select({}, [])

    def test_new_or_changed_manifest_row_requires_registration(self):
        header = "id,status,evidence_doc\n"
        before = header + "OVL_0038_12B2,partial,docs/re/a.md\n"
        after = header + "OVL_0038_12B2,full,docs/re/a.md\n" + \
            "OVL_0038_2B20,full,docs/re/b.md\n"
        self.assertEqual(gate.changed_rows(before, after),
                         {"OVL_0038_12B2", "OVL_0038_2B20"})
        with self.assertRaisesRegex(ValueError, "unregistered"):
            gate.select({}, gate.changed_rows(before, after))

    def test_unchanged_legacy_rows_do_not_claim_build_coverage(self):
        data = "id,status,evidence_doc\nOVL_0038_12B2,full,docs/re/a.md\n"
        self.assertEqual(gate.changed_rows(data, data), set())

    def test_registration_is_in_actual_checker(self):
        self.checker('import sys\nassert sys.argv[1:] == ["--compile-only"]\n'
                     'print("CHECKER_BUILD_OK")\n')
        paths = gate.select(gate.discover(self.root), ["OVL_0038_12B2"])
        self.assertEqual(paths, [self.path])
        self.assertEqual(gate.run_checker(self.root, paths[0])["checker"], str(self.path))

    def test_success_without_completion_marker_blocks(self):
        self.checker('print("not a compile-only checker")\n')
        with self.assertRaisesRegex(ValueError, "checker build failed"):
            gate.run_checker(self.root, self.path)

    def test_compile_error_blocks(self):
        self.checker('import subprocess\nsubprocess.run(["cc", "missing.c"], check=True)\n')
        with self.assertRaisesRegex(ValueError, "checker build failed"):
            gate.run_checker(self.root, self.path)

    @unittest.skipUnless(shutil.which("cc"), "C compiler required")
    def test_shared_library_with_unresolved_symbol_blocks_at_load(self):
        (self.root / "missing.c").write_text(
            'extern int missing_dependency(void);\n'
            'int model(void) { return missing_dependency(); }\n')
        self.checker('import ctypes, subprocess\n'
                     'subprocess.run(["cc", "-shared", "-fPIC", "missing.c", '
                     '"-o", "model.so"], check=True)\n'
                     'ctypes.CDLL("./model.so").model\nprint("CHECKER_BUILD_OK")\n')
        with self.assertRaisesRegex(ValueError, "undefined symbol: missing_dependency"):
            gate.run_checker(self.root, self.path)

    def test_malformed_registration_blocks(self):
        (self.root / self.path).write_text('CHECKER_ROWS = ("not-a-row",)\n')
        with self.assertRaisesRegex(ValueError, "invalid CHECKER_ROWS"):
            gate.discover(self.root)


class EnvironmentDetectionTest(unittest.TestCase):
    """A missing import must not look like a broken tree."""

    def test_message_and_exit_code_are_distinct(self):
        message = gate.environment_message(["capstone==5.0.7"])
        self.assertTrue(message.startswith("ENVIRONMENT MISSING:"))
        self.assertIn("NOT a tree failure", message)
        self.assertIn("capstone==5.0.7", message)

    def test_missing_dependencies_detects_absent_module(self):
        real = gate.importlib.util.find_spec
        gate.importlib.util.find_spec = lambda name: None if name == "capstone" else real(name)
        try:
            self.assertIn("capstone==5.0.7", gate.missing_dependencies())
        finally:
            gate.importlib.util.find_spec = real

    def test_main_returns_environment_exit_and_never_gates_the_tree(self):
        real = gate.missing_dependencies
        gate.missing_dependencies = lambda: ["capstone==5.0.7", "unicorn==2.1.4"]
        try:
            self.assertEqual(gate.main(["--all"]), gate.ENVIRONMENT_EXIT)
        finally:
            gate.missing_dependencies = real


@unittest.skipUnless(shutil.which("git"), "git required")
class PreconditionTest(unittest.TestCase):
    """The stale-tree check must be strict locally and relaxed in PR CI - but
    never so relaxed that an unrelated checkout passes."""

    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.git("init", "-q", "-b", "main")
        self.git("config", "user.email", "test@example.com")
        self.git("config", "user.name", "test")
        (self.root / "a").write_text("a")
        self.git("add", "-A")
        self.git("commit", "-qm", "one")
        self.git("checkout", "-qb", "feature")
        (self.root / "b").write_text("b")
        self.git("add", "-A")
        self.git("commit", "-qm", "two")
        self.git("checkout", "-q", "main")
        (self.root / "c").write_text("c")
        self.git("add", "-A")
        self.git("commit", "-qm", "three")
        self.git("checkout", "-q", "feature")
        self.git("update-ref", "refs/remotes/origin/main", self.rev("main"))

    def git(self, *args):
        gate.subprocess.run(["git", "-C", str(self.root), *args], check=True,
                            capture_output=True)

    def rev(self, ref):
        return gate.subprocess.run(["git", "-C", str(self.root), "rev-parse", ref],
                                   check=True, capture_output=True,
                                   text=True).stdout.strip()

    def test_local_run_requires_current_main(self):
        # Local means "no CI environment at all"; in CI the merge-base relation
        # is checked instead, whatever the event name.
        problem = gate.precondition_error(self.root, "origin/main", event=None, ci=False)
        self.assertIn("HEAD does not include fetched main", problem)

    def test_ci_without_an_event_is_relaxed(self):
        self.assertIsNone(gate.precondition_error(self.root, "origin/main",
                                                  event=None, ci=True))

    def test_pull_request_ci_tolerates_main_ahead_of_head(self):
        self.assertIsNone(gate.precondition_error(self.root, "origin/main",
                                                  event="pull_request"))
        self.assertIsNone(gate.precondition_error(self.root, "origin/main",
                                                  event="merge_group"))

    def test_push_event_ci_tolerates_main_ahead_of_head(self):
        """The hygiene workflow also runs on push, where main is legitimately ahead."""
        self.assertIsNone(gate.precondition_error(self.root, "origin/main",
                                                 event="push", ci=True))

    def test_local_run_stays_strict_without_ci_environment(self):
        problem = gate.precondition_error(self.root, "origin/main", event=None, ci=False)
        self.assertIn("HEAD does not include fetched main", problem)

    def test_require_current_main_flag_is_strict_even_in_ci(self):
        problem = gate.precondition_error(self.root, "origin/main",
                                          require_current_main=True,
                                          event="pull_request")
        self.assertIn("HEAD does not include fetched main", problem)

    def test_pr_mode_still_fails_closed_on_unrelated_history(self):
        self.git("checkout", "-q", "--orphan", "orphan")
        (self.root / "z").write_text("z")
        self.git("add", "-A")
        self.git("commit", "-qm", "orphan")
        problem = gate.precondition_error(self.root, "origin/main", event="pull_request")
        self.assertIn("no merge base", problem)

    def test_unresolvable_base_blocks(self):
        problem = gate.precondition_error(self.root, "origin/missing", event="pull_request")
        self.assertIn("fetch before handoff", problem)


if __name__ == "__main__":
    unittest.main()
