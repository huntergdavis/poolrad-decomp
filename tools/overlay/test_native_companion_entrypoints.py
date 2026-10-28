"""Native companions must refuse direct execution instead of reporting zero-case success."""
from pathlib import Path
import subprocess
import sys
import unittest

ROOT = Path(__file__).resolve().parents[2]
COMPANIONS = (
    "res0464_bitmap_release_2595",
    "res03f1_clear_0137_cpu",
    "res00ba_clear_17e3_cpu",
    "res00ba_display_1693_cpu",
    "res00ba_display_1816_cpu",
    "res0709_clear_0dd3_cpu",
    "res0709_glyph_01df_cpu",
    "res0709_printer_0626_cpu",
    "res0709_rect_fill_00b7_cpu",
    "res0709_screen_0cab_cpu",
    "res0709_space_05eb_cpu",
    "u3c6_slot_getters_cpu",
    "u9e_sidecar_0160_cpu",
    "u9e_sidecar_groups_02fa_cpu",
)


class NativeCompanionEntrypoints(unittest.TestCase):
    def test_direct_execution_refuses_with_actual_audit_command(self):
        for name in COMPANIONS:
            for mode, flags in (("normal", []), ("isolated", ["-I", "-S"])):
                with self.subTest(companion=name, mode=mode):
                    # Normal invocation reproduces the original false-green
                    # path. Isolated mode also requires refusal without optional
                    # audit imports, site packages or PYTHONPATH.
                    result = subprocess.run(
                        [sys.executable, *flags,
                         str(ROOT / "tools/overlay" / (name + "_native.py"))],
                        capture_output=True, text=True, timeout=10,
                    )
                    self.assertEqual(result.returncode, 1)
                    self.assertEqual(result.stdout, "")
                    self.assertEqual(
                        result.stderr.strip(),
                        "No cases executed: invoke tools/overlay/" + name +
                        "_audit.py --native through poolrad-heavy.sh.",
                    )


if __name__ == "__main__":
    unittest.main()
