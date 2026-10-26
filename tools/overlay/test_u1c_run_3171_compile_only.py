#!/usr/bin/env python3
"""The 3171 audit must really build for the CI gate, and profiles must not name absent flags."""
import ast
import json
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
AUDIT = ROOT / "tools/overlay/u1c_run_3171_cpu_audit.py"
PROFILES = ROOT / "tools/overlay/checker_execution_profiles.json"


class CompileOnlyTest(unittest.TestCase):
    def test_real_compile_only_builds_and_binds(self):
        run = subprocess.run([sys.executable, str(AUDIT), "--compile-only"], cwd=ROOT,
                             capture_output=True, text=True, timeout=120)
        self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
        self.assertIn("CHECKER_BUILD_OK", run.stdout.splitlines())

    def test_broken_c_model_fails_compile_only(self):
        sys.path.insert(0, str(ROOT / "tools/overlay"))
        import u1c_run_3171_cpu_audit as audit

        with tempfile.TemporaryDirectory() as temp:
            fake = Path(temp)
            (fake / "src/recovered").mkdir(parents=True)
            (fake / "include/poolrad/recovered").mkdir(parents=True)
            (fake / "src/recovered/overlay_u1c_run_3171_cpu.c").write_text("this is not C\n")
            header = ROOT / "include/poolrad/recovered/overlay_u1c_run_3171_cpu.h"
            (fake / "include/poolrad/recovered/overlay_u1c_run_3171_cpu.h").write_bytes(
                header.read_bytes())
            original = audit.ROOT
            audit.ROOT = fake
            try:
                self.assertEqual(audit.main(["--compile-only"]), 1)
            finally:
                audit.ROOT = original


class ObserverControlTest(unittest.TestCase):
    """The OF-flip collision is a permanent control: a rolling-hash refactor must fail here."""

    @unittest.skipUnless((ROOT / "resources/DOS/POOLRAD/GAME.OVR").exists(),
                         "owner GAME.OVR not under ignored resources/")
    def test_observation_overflow_is_refused(self):
        """64 observations agree; 65 must be refused, not silently truncated."""
        sys.path.insert(0, str(ROOT / "tools/overlay"))
        import u1c_run_3171_cpu_audit as audit

        ovr, at, body = audit.source()
        audit.source_bytes = body
        with tempfile.TemporaryDirectory() as temp:
            lib, dispatch_t = audit.build(Path(temp))
            boundary = audit.setup_ram(script=[1] * 63 + [0], entry=0, stop=0,
                                       stop2=0, latch=0, buf_off=0)
            audit.compare("boundary-64", boundary, lib, dispatch_t)
            over = audit.setup_ram(script=[1] * 64 + [0], entry=0, stop=0,
                                   stop2=0, latch=0, buf_off=0)
            with self.assertRaises(ValueError):
                audit.compare("overflow-65", over, lib, dispatch_t)

    @unittest.skipUnless((ROOT / "resources/DOS/POOLRAD/GAME.OVR").exists(),
                         "owner GAME.OVR not under ignored resources/")
    def test_of_flip_collision_is_rejected(self):
        sys.path.insert(0, str(ROOT / "tools/overlay"))
        import u1c_run_3171_cpu_audit as audit

        ovr, at, body = audit.source()
        audit.source_bytes = body
        with tempfile.TemporaryDirectory() as temp:
            directory = Path(temp)
            lib, dispatch_t = audit.build(directory)
            mutant, mutant_t = audit.build(directory, "of_nonterminal")
            for offset in (0x7FFE, 0x7FFF, 0x8000):
                ram = audit.setup_ram(script=[1, 1, 0], entry=0, stop=0, stop2=0,
                                      latch=0, buf_off=offset)
                audit.compare(f"unmutated-{offset:04x}", ram, lib, dispatch_t)
                with self.assertRaises(ValueError):
                    audit.compare(f"of-collision-{offset:04x}", ram, mutant, mutant_t)


class MaskBoundaryTest(unittest.TestCase):
    """The external-mode scratch window must be documented by behaviour, not prose."""

    def test_external_mask_boundary_controls(self):
        sys.path.insert(0, str(ROOT / "tools/overlay"))
        import u1c_run_3171_cpu_audit as audit

        want = bytearray(1 << 20)
        obs_low = (audit.DATA_DS << 4) + 0x0F00
        inside = {"inside_low": obs_low, "inside_high": obs_low + 0x81}
        outside = {"outside_low": obs_low - 1, "outside_high": obs_low + 0x82}
        # Normal full-RAM acceptance rejects all four corruptions.
        for addr in list(inside.values()) + list(outside.values()):
            got = bytearray(want)
            got[addr] = 0xA5
            self.assertTrue(audit.ram_mismatch(want, got, False), f"normal must reject {addr:#x}")
        # External (diagnostic) mode rejects the two outside, accepts the two inside.
        for addr in outside.values():
            got = bytearray(want)
            got[addr] = 0xA5
            self.assertTrue(audit.ram_mismatch(want, got, True), f"external must reject {addr:#x}")
        for addr in inside.values():
            got = bytearray(want)
            got[addr] = 0xA5
            self.assertFalse(audit.ram_mismatch(want, got, True), f"external masks {addr:#x}")


class ProfileFlagTest(unittest.TestCase):
    def test_every_profile_flag_appears_in_its_checker(self):
        """A profile flag that the checker never mentions is the class of bug that broke CI."""
        profiles = json.loads(PROFILES.read_text())
        problems = []
        for checker, profile in profiles.items():
            flags = [a for a in profile.get("args", [])
                     if isinstance(a, str) and a.startswith("-")]
            if not flags:
                continue
            path = ROOT / checker
            if not path.is_file():
                problems.append(f"{checker}: missing file")
                continue
            tree = ast.parse(path.read_text())
            strings = {n.value for n in ast.walk(tree)
                       if isinstance(n, ast.Constant) and isinstance(n.value, str)}
            absent = [f for f in flags if f not in strings]
            if absent:
                problems.append(f"{checker}: {absent}")
        self.assertEqual(problems, [])


if __name__ == "__main__":
    unittest.main()
