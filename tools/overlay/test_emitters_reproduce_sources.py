#!/usr/bin/env python3
"""Generated sources must stay reproducible by their emitters (the #1433 generator hazard).

#1433 hand-fixed the pinned MUL/AF flag rules in five generated files without changing
their emit_* generators, so re-running any emitter silently dropped the fixes. Two checks:

- media-free (runs in CI): every flag helper in a generated file appears verbatim in its
  emitter's template, so a hand edit to a helper without the template fails here;
- with the owner START.EXE under resources/: each emitter's own --check reproduces its
  .c and .h byte for byte. Without the media this suite FAILS with an explicit REFUSED
  diagnostic. Media-free CI explicitly reports NOT RUN and runs only the media-free
  cases below. tools/run_media_checks.sh is the required local reproduction gate.
"""
# NOT_A_ROW_CHECKER: checks that emit_* generators reproduce their committed outputs; it checks no registered row
import re
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
OVERLAY = ROOT / "tools/overlay"
MEDIA = ROOT / "resources/DOS/POOLRAD/START.EXE"
EMITTERS = {
    "emit_ovl038_0400": "src/recovered/overlay_u38_handler_0400_cpu.c",
    "emit_ovl038_0b1b": "src/recovered/overlay_u38_0b1b_cpu.c",
    "emit_ovl03b6_0000": "src/recovered/overlay_u3b6_window_0000_cpu.c",
    "emit_ovl03ce_00f1": "src/recovered/overlay_u3ce_tile_00f1_cpu.c",
    "emit_ovl040c_015d": "src/recovered/overlay_u40c_tile_015d_cpu.c",
    "emit_u1c_save_0292": "src/recovered/overlay_u1c_save_0292_cpu.c",
    "emit_u1c_save_table_0ebc": "src/recovered/overlay_u1c_save_table_0ebc_cpu.c",
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
            # a generator may keep its shared template in candidate_ecl_template.py
            generator = OVERLAY / f"{emitter}.py"
            template = generator.read_text()
            shared = OVERLAY / "candidate_ecl_template.py"
            if shared.exists():
                template += "\n" + shared.read_text()
            helpers = [m.group(0) for m in HELPER.finditer((ROOT / output).read_text())]
            self.assertTrue(helpers, f"{output}: no flag helper found, the pattern is stale")
            for body in helpers:
                with self.subTest(emitter=emitter, helper=body.split("(")[0].split()[-1]):
                    self.assertIn(body, template,
                                  f"{output} has a flag helper its emitter does not generate")

    def test_emitters_reproduce_outputs_with_media(self):
        if not MEDIA.is_file():
            self.fail("REFUSED: owner START.EXE missing under resources/DOS/POOLRAD/: "
                      "emitter --check NOT run; supply owner media to run this gate")
        for emitter in EMITTERS:
            with self.subTest(emitter=emitter):
                run = subprocess.run([sys.executable, f"{emitter}.py", "--check"], cwd=OVERLAY,
                                     capture_output=True, text=True, timeout=600)
                self.assertEqual(run.returncode, 0, run.stdout + run.stderr)
                self.assertIn("EMIT_CHECK OK", run.stdout)


class MissingMediaTest(unittest.TestCase):
    def test_missing_media_fails_before_any_emitter_runs(self):
        # An isolated absent path exercises the real test without moving shared media.
        with tempfile.TemporaryDirectory() as directory:
            with patch.object(sys.modules[__name__], "MEDIA", Path(directory) / "START.EXE"):
                with patch.object(subprocess, "run") as run:
                    case = EmitterTest("test_emitters_reproduce_outputs_with_media")
                    with self.assertRaisesRegex(AssertionError, "REFUSED:.*emitter --check NOT run"):
                        case.test_emitters_reproduce_outputs_with_media()
                    run.assert_not_called()


if __name__ == "__main__":
    unittest.main()
