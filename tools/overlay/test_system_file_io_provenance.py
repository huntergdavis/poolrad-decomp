#!/usr/bin/env python3
"""Synthetic launch-boundary controls; no emulator is run by these tests."""
import contextlib
import io
import importlib
import os
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

sys.path.insert(0, str(Path(__file__).resolve().parent))


class LaunchReached(Exception):
    pass


class FileIoProvenanceTest(unittest.TestCase):
    def test_selected_executable_and_existing_invocation(self):
        for name, family in (("system_close_1491_cpu_audit", "close-1491"),
                             ("system_filecheck_14b6_cpu_audit", "filecheck-14b6")):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as tmp:
                module = importlib.import_module(name)
                metadata = dict(path="/synthetic/selected-dosbox", version="synthetic",
                                sha256="synthetic", pinned_build_used=False)
                calls = []

                def run(command, **kwargs):
                    calls.append((command, kwargs))
                    if command[0] == metadata["path"]:
                        raise LaunchReached
                    return subprocess.CompletedProcess(command, 0)

                with patch.object(module, "ROOT", Path(tmp)), \
                     patch.object(module, "describe_dosbox", return_value=metadata) as describe, \
                     patch.object(module.subprocess, "run", side_effect=run):
                    with self.assertRaises(LaunchReached):
                        module.native_audit(b"\xcb", None)
                describe.assert_called_once_with("dosbox-x")
                command, options = calls[-1]
                folder = Path(tmp) / ("resources/oracle-runs/system-" + family + "-synthetic")
                self.assertEqual(command, [metadata["path"], "-conf", str(folder / "dosbox.conf"),
                                           "-fastlaunch", "-exit", "-noconsole"])
                self.assertEqual(options["env"], dict(os.environ, SDL_VIDEODRIVER="dummy", SDL_AUDIODRIVER="dummy"))
                self.assertTrue(options["check"])
                self.assertEqual(options["timeout"], 300)
                self.assertEqual(options["stderr"], subprocess.STDOUT)
                self.assertEqual(Path(options["stdout"].name), folder / "dosbox.log")

    def test_compile_only_binds_real_c_without_media_or_emulator(self):
        for name in ("system_close_1491_cpu_audit", "system_filecheck_14b6_cpu_audit"):
            with self.subTest(name=name):
                module = importlib.import_module(name)
                with patch.object(sys, "argv", [name, "--compile-only"]), \
                     patch.object(module, "source", side_effect=AssertionError("media accessed")), \
                     patch.object(module, "describe_dosbox", side_effect=AssertionError("emulator probed")), \
                     contextlib.redirect_stdout(io.StringIO()) as output:
                    module.main()
                self.assertEqual(output.getvalue().splitlines(), ["CHECKER_BUILD_OK"])

    def test_incomplete_provenance_refuses_before_emulator_launch(self):
        for name in ("system_close_1491_cpu_audit", "system_filecheck_14b6_cpu_audit"):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as tmp:
                module = importlib.import_module(name)
                with patch.object(module, "ROOT", Path(tmp)), \
                     patch.object(module, "describe_dosbox", side_effect=ValueError("missing provenance")), \
                     patch.object(module.subprocess, "run") as run:
                    with self.assertRaisesRegex(ValueError, "missing provenance"):
                        module.native_audit(b"\xcb", None)
                self.assertEqual([call.args[0][0] for call in run.call_args_list], ["as", "ld"])


if __name__ == "__main__":
    unittest.main()
