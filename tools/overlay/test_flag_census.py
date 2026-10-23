#!/usr/bin/env python3
"""The flag census gate: detection on synthetic sources, and the committed table covers every helper in src/recovered."""
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import flag_census as census


class CensusTest(unittest.TestCase):
    def tree(self, **files):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        (Path(folder.name) / "src/recovered").mkdir(parents=True)
        for name, text in files.items():
            (Path(folder.name) / "src/recovered" / name).write_text(text)
        return Path(folder.name)

    def test_a_flag_writing_static_function_is_found_and_a_plain_one_is_not(self):
        root = self.tree(**{"a.c": "static void zero_ah(cpu_t *c){ c->flags &= 1; }\nstatic int add(int a){ return a; }\n"})
        self.assertEqual([(f, fn) for f, fn, _, _ in census.helpers(root)], [("a.c", "zero_ah")])

    def test_copies_modulo_names_share_a_row_and_a_changed_body_does_not(self):
        root = self.tree(**{"a.c": "static void s(pr_a *cpu){ cpu->flags |= 0x10u; }\n",
                            "b.c": "static void s(pr_b *cpu){ cpu->flags |= 0x10u; /* note */ }\n",
                            "c.c": "static void s(pr_c *cpu){ cpu->flags &= ~0x10u; }\n"})
        self.assertEqual(sorted(len(m) for m in census.groups(root).values()), [1, 2])

    def test_check_fails_for_a_helper_with_no_row_and_for_an_unclassified_row(self):
        root = self.tree(**{"a.c": "static void s(pr_a *c){ c->flags |= 0x10u; }\n"})
        table = root / "t.tsv"
        census.write_rows({}, table)
        self.assertEqual(census.gaps(root, table), ([("a.c", "s")], []))
        census.regen(root, table)
        self.assertEqual(census.gaps(root, table), ([], [("a.c", "s")]))
        rows = census.read_rows(table)
        for row in rows.values():
            row["agent_verdict"] = "agree"
        census.write_rows(rows, table)
        self.assertEqual(census.gaps(root, table), ([], []))

    def test_every_helper_in_the_tree_has_a_classified_row(self):
        missing, pending = census.gaps()
        self.assertEqual((missing, pending), ([], []), "run tools/overlay/flag_census.py --regen, classify the new rows, commit")



class InlineDetectorTest(unittest.TestCase):
    def test_non_static_body_with_inline_flags_is_found(self):
        found = {(name, function) for name, function, _, _ in census.helpers()}
        self.assertIn(("overlay_u5e_half_0e52.c", "pr_u5e_half_0e52"), found)   # the PM i21766 miss

if __name__ == "__main__":
    unittest.main()
