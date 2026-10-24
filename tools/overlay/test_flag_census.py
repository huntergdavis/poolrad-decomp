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

    def test_a_comment_or_newline_between_the_signature_and_the_brace_is_still_a_helper(self):
        # par-d2 i23004: "static T f(args)   /* comment */" with "{" on the next line was never hashed.
        root = self.tree(**{"a.c": "static void logic(cpu_t *c)   /* AND/OR/XOR */\n{\n    c->flags &= ~0x08D5u;\n}\n"
                                   "static void inc8(cpu_t *c) // CF kept\n{ c->flags |= 1u; }\n"
                                   "static void spaced(cpu_t *c)\n\n{ c->flags = 0u; }\n"
                                   "void pr_body(cpu_t *c) /* public,\n   two-line note */\n{ c->flags ^= 2u; }\n"
                                   "static void proto(cpu_t *c); /* a prototype */\nstatic int plain(int a)\n{ return a; }\n"})
        self.assertEqual([(f, fn) for f, fn, _, _ in census.helpers(root)],
                         [("a.c", "logic"), ("a.c", "inc8"), ("a.c", "spaced"), ("a.c", "pr_body")])

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

    def test_commented_signature_helpers_in_the_tree_are_found(self):
        found = {(name, function) for name, function, _, _ in census.helpers()}
        self.assertIn(("overlay_u25_entry4_0341_cpu.c", "dec16_flags"), found)   # par-d2 i23004: '{' after a comment
        self.assertIn(("resident_0802_0105_cpu.c", "cmp_flags"), found)


class MacroHashTest(unittest.TestCase):
    """A flag rule that lives in a #define must move the hash (OVL_0025_0756's fix was a #define edit)."""

    def digests(self, mask, header_mask="0x0010u"):
        import hashlib
        with tempfile.TemporaryDirectory() as tmp:
            src, inc = Path(tmp) / "src/recovered", Path(tmp) / "include"
            src.mkdir(parents=True); inc.mkdir()
            (inc / "flags.h").write_text(f"#define AF_BIT {header_mask}\n")
            (src / "a.c").write_text(f'#include "flags.h"\n#define KEEP {mask}\n'
                                     "static void f(unsigned *flags) { *flags &= ~KEEP; }\n"
                                     "static void g(unsigned *flags) { *flags |= AF_BIT; }\n"
                                     "static void h(unsigned *flags) { *flags = 0u; }\n")
            found = {fn: (body, d) for _, fn, body, d in census.helpers(tmp)}
            plain = hashlib.md5(census.normalize(found["h"][0]).encode()).hexdigest()
            return {fn: d for fn, (_, d) in found.items()}, plain

    def test_define_change_moves_the_hash(self):
        a, _ = self.digests("0x08C5u")
        b, _ = self.digests("0x08D5u")
        self.assertNotEqual(a["f"], b["f"])          # the #define in the same file
        self.assertEqual(a["h"], b["h"])             # a macro-free body is untouched

    def test_included_define_counts_one_level_deep(self):
        a, _ = self.digests("0x08C5u", "0x0010u")
        b, _ = self.digests("0x08C5u", "0x0000u")
        self.assertNotEqual(a["g"], b["g"])

    def test_macro_free_body_keeps_its_old_hash(self):
        found, plain = self.digests("0x08C5u")
        self.assertEqual(found["h"], plain)


class CarryFromTest(unittest.TestCase):
    """--carry-from computes the OLD tree's hashes; they must match the committed rows for bodies that name header macros."""

    HAND_CARRIED = {   # the four rows --carry-from missed before it archived include/ (par-d2, 2026-10-23)
        ("overlay_u33_grid_diagonal_0bde.c", "pr_u33_grid_diagonal_0bde"),
        ("resident_0709_delay_wrapper_0dbe_cpu.c", "pr_r0709_delay_wrapper_0dbe_cpu"),
        ("overlay_u7d_list_timer_0000_cpu.c", "pr_u7d_list_timer_0000_cpu"),
        ("overlay_u9e_entry8_fullstate_loop_0e05.c", "pr_u9e_entry8_fullstate_loop_0e05"),
    }

    def test_rev_digests_match_committed_rows_for_the_hand_carried_four(self):
        old = census.rev_digests("HEAD")
        rows = census.read_rows()
        self.assertTrue(self.HAND_CARRIED <= set(old), sorted(self.HAND_CARRIED - set(old)))
        unmatched = {key for key in self.HAND_CARRIED if old[key] not in rows}
        self.assertEqual(unmatched, set())

if __name__ == "__main__":
    unittest.main()
