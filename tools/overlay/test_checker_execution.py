"""Execution must prove a differential ran; source selection must not lose dependencies."""
from pathlib import Path
import tempfile
import unittest

import checker_execution as execution
import check_checker_builds as gate


class ExecutionTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.path = Path('tests/recovered/check_synthetic.py')
        (self.root / self.path).parent.mkdir(parents=True)
        for p in execution.MEDIA:
            target = self.root / p
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(b'synthetic')
        self.wrapper = self.root / 'heavy'
        self.wrapper.write_text('#!/bin/sh\nexec "$@"\n')
        self.wrapper.chmod(0o755)
        self.registry = {'OVL_0038_0400': [self.path]}
        self.profiles = {str(self.path): {'args': ['--compare']}}

    def run_case(self, source):
        (self.root / self.path).write_text(source)
        return execution.execute(self.root, self.registry, [self.path], self.profiles, heavy=self.wrapper)

    def test_explicit_differential_arguments_and_receipt(self):
        status, report = self.run_case('import sys\nassert sys.argv[1:] == ["--compare"]\nprint("1 case agrees")\n')
        self.assertEqual(status, 0)
        self.assertEqual(report['checkers'][0]['result'], 'AGREE')
        self.assertEqual(report['checkers'][0]['rows'], ['OVL_0038_0400'])

    def test_build_marker_is_not_agreement(self):
        status, _ = self.run_case('print("CHECKER_BUILD_OK")\n')
        self.assertEqual(status, 4)

    def test_empty_success_is_not_agreement(self):
        status, _ = self.run_case('pass\n')
        self.assertEqual(status, 4)

    def test_failure_names_the_row(self):
        status, report = self.run_case('raise AssertionError("RAM differs")\n')
        self.assertEqual(status, 1)
        self.assertEqual(report['checkers'][0]['result'], 'DIFFERENTIAL_FAILED')
        self.assertIn('OVL_0038_0400', report['checkers'][0]['rows'])

    def test_missing_import_is_cannot_run(self):
        status, _ = self.run_case('import nonexistent_poolrad_module\n')
        self.assertEqual(status, 4)

    def test_missing_media_does_not_launch(self):
        (self.root / execution.MEDIA[0]).unlink()
        with self.assertRaisesRegex(ValueError, 'missing owner media'):
            self.run_case('raise AssertionError("must not run")\n')

    def test_missing_profile_does_not_launch(self):
        self.profiles.clear()
        with self.assertRaisesRegex(ValueError, 'no differential execution profile'):
            self.run_case('raise AssertionError("must not run")\n')

    def test_sequential_order_and_persisted_partial_receipt(self):
        second = self.path.with_name('check_second.py')
        (self.root / self.path).write_text('from pathlib import Path\nPath("first-done").write_text("yes")\nprint("first agrees")\n')
        (self.root / second).write_text('from pathlib import Path\nassert Path("first-done").read_text()=="yes"\nraise AssertionError("second differs")\n')
        self.registry['OVL_0038_133F'] = [second]
        self.profiles[str(second)] = {'args': []}
        output = self.root / 'receipt.json'
        status, report = execution.execute(self.root, self.registry, [self.path, second], self.profiles, output, heavy=self.wrapper)
        self.assertEqual(status, 1)
        self.assertEqual([r['result'] for r in report['checkers']], ['AGREE', 'DIFFERENTIAL_FAILED'])
        self.assertTrue(output.is_file())


    def test_timeout_is_cannot_run(self):
        (self.root / self.path).write_text('import time\ntime.sleep(30)\n')
        status, report = execution.execute(self.root, self.registry, [self.path], self.profiles,
                                           heavy=self.wrapper, timeout=0.05)
        self.assertEqual(status, 4)
        self.assertEqual(report['checkers'][0]['result'], 'CANNOT_RUN')

    def test_reduced_profile_is_rejected(self):
        import json
        profile = self.root / execution.PROFILES
        profile.parent.mkdir(parents=True, exist_ok=True)
        for option in ('--compile-only', '--quick', '--allow-incomplete'):
            profile.write_text(json.dumps({str(self.path): {'args': [option]}}))
            with self.assertRaisesRegex(ValueError, 'reduced execution profile'):
                execution.load_profiles(self.root)


class RegisteredProfilesTest(unittest.TestCase):
    def test_every_registered_checker_has_explicit_arguments(self):
        registry = gate.discover(gate.ROOT)
        self.assertEqual(set(execution.load_profiles(gate.ROOT)),
                         {str(p) for paths in registry.values() for p in paths})


class SourceSelectionTest(unittest.TestCase):
    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        (self.root / 'tools/overlay').mkdir(parents=True)
        (self.root / 'CMakeLists.txt').write_text('add_library(model src/recovered/a.c src/recovered/b.c)\n# src/recovered/not_real.c\n')
        self.a, self.b = Path('tools/overlay/a.py'), Path('tools/overlay/b.py')

    def test_declared_lists_and_cyclic_imports_keep_both_sources(self):
        (self.root / self.a).write_text('import b\nSOURCES=["a"]\n')
        (self.root / self.b).write_text('import a\nSOURCES=["b"]\n')
        result = execution.source_map(self.root, [self.a, self.b], gate._cc_commands)
        for receipt in result.values():
            self.assertEqual(receipt['sources'], ['src/recovered/a.c', 'src/recovered/b.c'])

    def test_declared_sources_cannot_hide_unrelated_opaque_inputs(self):
        (self.root / self.a).write_text('import b as B\nSOURCES=sorted(set(B.SOURCES) | {"a"})\nsubprocess.run(["cc", *unknown_sources])\n')
        (self.root / self.b).write_text('SOURCES=["b"]\n')
        result = execution.source_map(self.root, [self.a], gate._cc_commands)[self.a]
        self.assertEqual(result['sources'], ['src/recovered/a.c', 'src/recovered/b.c'])
        self.assertTrue(result['conservative'])

    def test_opaque_compiler_uses_labelled_cmake_superset(self):
        (self.root / self.a).write_text('import subprocess\nsubprocess.run(["cc", *unknown_sources])\n')
        receipt = execution.source_map(self.root, [self.a], gate._cc_commands)[self.a]
        self.assertEqual(receipt['sources'], ['src/recovered/a.c', 'src/recovered/b.c'])
        self.assertTrue(receipt['conservative'])

    def test_cli_all_touching_uses_derived_selection(self):
        import json
        from unittest.mock import patch
        (self.root / self.a).write_text('CHECKER_ROWS=("OVL_0038_0400",)\nSOURCES=["a"]\n')
        (self.root / self.b).write_text('CHECKER_ROWS=("OVL_0038_133F",)\nSOURCES=["b"]\n')
        source = self.root / 'src/recovered/a.c'
        source.parent.mkdir(parents=True)
        source.write_text('/* synthetic */\n')
        (self.root / execution.PROFILES).write_text(json.dumps({str(self.a): {'args': []}, str(self.b): {'args': []}}))
        with patch.object(gate, 'ROOT', self.root), patch.object(gate, 'missing_dependencies', return_value=[]), \
             patch.object(gate, 'precondition_error', return_value=None), patch.object(gate, 'git', return_value='synthetic'), \
             patch.object(execution, 'execute', return_value=(0, {})) as run:
            # Receipt provenance reads the real gate source paths in this synthetic tree.
            for name in ('check_checker_builds.py', 'checker_execution.py'):
                (self.root / 'tools/overlay' / name).write_text('# synthetic gate\n')
            self.assertEqual(gate.main(['--all-touching', 'src/recovered/a.c']), 0)
            self.assertEqual(run.call_args.args[2], [self.a])
            run.reset_mock()
            self.assertEqual(gate.main(['--execute', 'OVL_9999_0000']), 4)
            run.assert_not_called()

    def test_unresolved_command_keeps_best_effort_sweep_nonfatal(self):
        import ast
        self.assertEqual(gate._cc_commands(ast.parse('subprocess.run(unknown_command)'), self.root), [])

    def test_dynamic_output_does_not_hide_following_sources(self):
        import ast
        tree = ast.parse('subprocess.run(["cc", "-o", str(output)] + [str(ROOT/"src/recovered/a.c")])')
        command = gate._cc_commands(tree, self.root, strict=True)[0]
        self.assertIn(str(self.root / 'src/recovered/a.c'), command)

    def test_source_list_expression_reaches_compiler_inputs(self):
        import ast
        tree = ast.parse('SOURCES=["a", "b"]\nsubprocess.run(["cc", *[str(ROOT/"src/recovered"/(s+".c")) for s in SOURCES], "-o", unknown])')
        commands = gate._cc_commands(tree, self.root)
        self.assertIn(str(self.root / 'src/recovered/a.c'), commands[0])
        self.assertIn(str(self.root / 'src/recovered/b.c'), commands[0])


class CoverageTest(unittest.TestCase):
    """A registered row is AGREE only when the declared command executed its differential."""

    def setUp(self):
        temp = tempfile.TemporaryDirectory()
        self.addCleanup(temp.cleanup)
        self.root = Path(temp.name)
        self.path = Path('tests/recovered/check_synthetic.py')
        (self.root / self.path).parent.mkdir(parents=True)
        for p in execution.MEDIA:
            target = self.root / p
            target.parent.mkdir(parents=True, exist_ok=True)
            target.write_bytes(b'synthetic')
        self.wrapper = self.root / 'heavy'
        self.wrapper.write_text('#!/bin/sh\nexec "$@"\n')
        self.wrapper.chmod(0o755)
        self.registry = {'OVL_0038_0400': [self.path], 'OVL_0038_133F': [self.path]}
        (self.root / self.path).write_text('print("1 case agrees")\n')

    def run_profile(self, profile):
        self.profiles = {str(self.path): profile}
        return execution.execute(self.root, self.registry, [self.path],
                                 self.profiles, heavy=self.wrapper)

    def test_declared_rows_emit_not_covered(self):
        status, report = self.run_profile({'args': ['--compare'], 'rows': ['OVL_0038_0400']})
        self.assertEqual(status, execution.NOT_COVERED)
        entry = report['checkers'][0]
        self.assertEqual(entry['row_results'],
                         {'OVL_0038_0400': 'AGREE', 'OVL_0038_133F': 'NOT_COVERED'})
        self.assertEqual(entry['not_covered'], ['OVL_0038_133F'])

    def test_waived_not_covered_does_not_fail(self):
        status, report = self.run_profile({'args': ['--compare'], 'rows': ['OVL_0038_0400'],
                                           'waive_not_covered': ['OVL_0038_133F']})
        self.assertEqual(status, 0)
        self.assertEqual(report['checkers'][0]['row_results']['OVL_0038_133F'], 'NOT_COVERED')
        self.assertEqual(report['checkers'][0]['result'], 'AGREE')

    def run_script(self, script, profile):
        (self.root / self.path).write_text(script)
        self.profiles = {str(self.path): profile}
        return execution.execute(self.root, self.registry, [self.path],
                                 self.profiles, heavy=self.wrapper)

    def test_observed_rows_agree(self):
        status, report = self.run_script(
            'print("ROW_EXECUTED OVL_0038_0400 PASS")\n'
            'print("ROW_EXECUTED OVL_0038_133F PASS")\n',
            {'args': ['--compare'], 'rows': ['OVL_0038_0400', 'OVL_0038_133F'], 'observe': True})
        self.assertEqual(status, 0)
        self.assertEqual(report['checkers'][0]['row_results'],
                         {'OVL_0038_0400': 'AGREE', 'OVL_0038_133F': 'AGREE'})

    def test_declared_but_not_observed_row_is_not_covered(self):
        status, report = self.run_script(
            'print("ROW_EXECUTED OVL_0038_0400 PASS")\n',
            {'args': ['--compare'], 'rows': ['OVL_0038_0400', 'OVL_0038_133F'], 'observe': True})
        self.assertEqual(status, execution.NOT_COVERED)
        entry = report['checkers'][0]
        self.assertEqual(entry['row_results'],
                         {'OVL_0038_0400': 'AGREE', 'OVL_0038_133F': 'NOT_COVERED'})
        self.assertEqual(entry['coverage_source'], 'stdout')

    def test_marker_for_undeclared_row_is_flagged(self):
        status, report = self.run_script(
            'print("ROW_EXECUTED OVL_0038_0400 PASS")\n'
            'print("ROW_EXECUTED OVL_0038_133F PASS")\n',
            {'args': ['--compare'], 'rows': ['OVL_0038_0400'], 'observe': True})
        self.assertEqual(status, execution.NOT_COVERED)
        self.assertEqual(report['checkers'][0]['undeclared_markers'], ['OVL_0038_133F'])

    def test_failing_command_has_no_agree_rows(self):
        status, report = self.run_script(
            'print("ROW_EXECUTED OVL_0038_0400 PASS")\nraise SystemExit(1)\n',
            {'args': ['--compare'], 'rows': ['OVL_0038_0400', 'OVL_0038_133F'], 'observe': True})
        self.assertEqual(status, 1)
        entry = report['checkers'][0]
        self.assertEqual(entry['result'], 'DIFFERENTIAL_FAILED')
        self.assertNotIn('AGREE', entry['row_results'].values())

    def test_multi_row_profile_without_rows_is_refused(self):
        self.profiles = {str(self.path): {'args': ['--compare']}}
        with self.assertRaisesRegex(ValueError, "needs an explicit 'rows'"):
            execution.execute(self.root, self.registry, [self.path], self.profiles,
                              heavy=self.wrapper)

    def test_profile_row_not_registered_is_refused(self):
        self.profiles = {str(self.path): {'args': ['--compare'], 'rows': ['OVL_9999_9999']}}
        with self.assertRaisesRegex(ValueError, 'not registered'):
            execution.execute(self.root, self.registry, [self.path], self.profiles,
                              heavy=self.wrapper)


if __name__ == '__main__':
    unittest.main()
