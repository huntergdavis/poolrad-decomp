#!/usr/bin/env python3
"""Synthetic launch-boundary controls; no emulator is run by these tests."""
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


class ArithmeticProvenanceTest(unittest.TestCase):
    def test_selected_executable_and_existing_invocation(self):
        for name, family in (("system_longmul_0a5b_audit", "longmul-0a5b"),
                             ("system_longdiv_0a76_audit", "longdiv-0a76")):
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
                        module.native_audit(dict(body=b"\xcb"), None)
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

    def test_incomplete_provenance_refuses_before_emulator_launch(self):
        for name in ("system_longmul_0a5b_audit", "system_longdiv_0a76_audit"):
            with self.subTest(name=name), tempfile.TemporaryDirectory() as tmp:
                module = importlib.import_module(name)
                with patch.object(module, "ROOT", Path(tmp)), \
                     patch.object(module, "describe_dosbox", side_effect=ValueError("missing provenance")), \
                     patch.object(module.subprocess, "run") as run:
                    with self.assertRaisesRegex(ValueError, "missing provenance"):
                        module.native_audit(dict(body=b"\xcb"), None)
                self.assertEqual([call.args[0][0] for call in run.call_args_list], ["as", "ld"])


if __name__ == "__main__":
    unittest.main()
