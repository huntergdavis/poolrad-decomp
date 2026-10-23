#!/usr/bin/env python3
"""Generated sources must stay reproducible by their emitters (the #1433 generator hazard).

#1433 hand-fixed the pinned MUL/AF flag rules in five generated files without changing
their emit_* generators, so re-running any emitter silently dropped the fixes. Two checks:

- media-free (runs in CI): every flag helper in a generated file appears verbatim in its
  emitter's template, so a hand edit to a helper without the template fails here;
- with the owner START.EXE under resources/: each emitter's own --check reproduces its
  .c and .h byte for byte. Without the media it is skipped and says so; it never passes.
"""
# NOT_A_ROW_CHECKER: checks that emit_* generators reproduce their committed outputs; it checks no registered row
import re
import subprocess
import sys
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OVERLAY = ROOT / "tools/overlay"
MEDIA = ROOT / "resources/DOS/POOLRAD/START.EXE"
EMITTERS = {
    "emit_ovl040c_015d": "src/recovered/overlay_u40c_tile_015d_cpu.c",
    "emit_res03f1_box_0000": "src/recovered/resident_03f1_box_0000_cpu.c",
    "emit_res0464_blit_1951": "src/recovered/resident_0464_blit_1951_cpu.c",
    "emit_res0464_compose_08d4": "src/recovered/resident_0464_compose_08d4_cpu.c",
    "emit_res08ae_time_0000": "src/recovered/resident_08ae_time_0000_cpu.c",
}
FLAG_HELPERS = ("mul16", "shift", "logic", "incdec", "szp")
HELPER = re.compile(r"^static [a-z0-9_ ]+\*?(" + "|".join(FLAG_HELPERS) + r")\(.*?^}\n", re.S | re.M)


class EmitterTest(unittest.TestCase):
    def test_emitter_set_is_pinned(self):
        found = {p.stem for p in OVERLAY.glob("emit_*.py") if p.stem != "emit_raw_cpu_c"}
        self.assertEqual(found, set(EMITTERS),
                         "a new emit_* generator needs an entry here (its output file)")

    def test_flag_helpers_in_outputs_come_from_templates(self):
        for emitter, output in EMITTERS.items():
            template = (OVERLAY / f"{emitter}.py").read_text()
            helpers = [m.group(0) for m in HELPER.finditer((ROOT / output).read_text())]
            self.assertTrue(helpers, f"{output}: no flag helper found, the pattern is stale")
            for body in helpers:
                with self.subTest(emitter=emitter, helper=body.split("(")[0].split()[-1]):
                    self.assertIn(body, template,
                                  f"{output} has a flag helper its emitter does not generate")

    def test_emitters_reproduce_outputs_with_media(self):
        if not MEDIA.exists():
            self.skipTest("owner START.EXE not under resources/: emitter --check NOT run")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter):
                run = subprocess.run([sys.executable, f"{emitter}.py", "--check"], cwd=OVERLAY,
                                     capture_output=True, text=True, timeout=600)
                self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
                self.assertIn("EMIT_CHECK OK", run.stdout)


if __name__ == "__main__":
    unittest.main()
