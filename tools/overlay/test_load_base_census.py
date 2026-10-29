#!/usr/bin/env python3
"""The load-base census: detection on synthetic sources, and the committed tree under the site policy.

Every policy control starts from a FULL VALID FIXTURE (the real `src/recovered` tree, symlinked) and applies exactly one
mutation, so a red result is caused by the mutation and not by an incomplete fixture.
"""
import sys
import subprocess
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import load_base_census as census


class CensusTest(unittest.TestCase):
    def tree(self, **files):
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        (Path(folder.name) / "src/recovered").mkdir(parents=True)
        for name, text in files.items():
            (Path(folder.name) / "src/recovered" / name).write_text(text)
        return folder.name

    def full_fixture(self):
        """A complete, valid tree: every committed source symlinked, so only the mutation can make it red."""
        folder = tempfile.TemporaryDirectory()
        self.addCleanup(folder.cleanup)
        target = Path(folder.name) / "src/recovered"
        target.mkdir(parents=True)
        for path in (census.ROOT / "src/recovered").glob("*.c"):
            (target / path.name).symlink_to(path)
        return Path(folder.name)

    def mutate(self, root, name, old, new, count=1):
        """Replace the symlink with a real file carrying one mutation."""
        path = Path(root) / "src/recovered" / name
        text = path.read_text()
        self.assertIn(old, text)
        path.unlink()
        path.write_text(text.replace(old, new, count))

    def status(self, root):
        _, bad, _ = census.check(root)
        return 1 if bad else 0

    def test_a_literal_and_a_macro_are_found_but_a_live_read_and_a_parameter_are_not(self):
        root = self.tree(**{
            "a.c": "void f(void){ cpu->cs = 0x00BAu; }\n",
            "b.c": "#define SYSTEM_CS 0x0AF8u\nvoid f(void){ c->cs=SYSTEM_CS;c->ip=1; }\n",
            "c.c": "void f(void, uint16_t seg){ c->cs = seg; c->cs = rd(ram, cs, 0x0E11u); }\n",
            "d.c": "#define OFFSET 0x0BB8u\nvoid f(void){ c->ip = OFFSET; /* c->cs = 0x00BAu; */ }\n",
        })
        self.assertEqual(sorted((name, number) for name, number, _, _, _ in census.census(root)),
                         [("a.c", 1), ("b.c", 2)])

    def test_the_committed_tree_has_only_the_four_allowed_sites(self):
        _, bad, total = census.check()
        self.assertEqual(bad, set())
        self.assertEqual(total, 4)
        keys = {(name, function, statement) for name, _, statement, _, function in census.census()}
        self.assertEqual(keys, set(census.ALLOWED))

    def test_the_full_fixture_baseline_is_green(self):
        self.assertEqual(self.status(self.full_fixture()), 0)

    # --- the four controls the policy names (each on a full valid fixture) ---

    def test_changed_value_is_red(self):
        root = self.full_fixture()
        self.mutate(root, "overlay_u6c_2033_cpu.c", "composed.cs = 0x00BAu;", "composed.cs = 0x0AF8u;")
        self.assertEqual(self.status(root), 1)

    def test_extra_cs_assignment_in_an_allowed_file_is_red(self):
        root = self.full_fixture()
        self.mutate(root, "overlay_u6c_2033_cpu.c", "    cpu->ip = 0x204Cu;",
                    "    cpu->ip = 0x204Cu;\n    if (0) cpus[0].cs = 0x0AF8u;")
        self.assertEqual(self.status(root), 1)

    def test_duplicate_expected_statement_is_red(self):
        root = self.full_fixture()
        self.mutate(root, "overlay_u6c_2033_cpu.c", "    composed.cs = 0x00BAu;",
                    "    composed.cs = 0x00BAu;\n    composed.cs = 0x00BAu;")
        self.assertEqual(self.status(root), 1)

    def test_missing_site_is_red(self):
        """Silence is not a pass: an expected site that disappears from the tree fails the census."""
        root = self.full_fixture()
        self.mutate(root, "overlay_u6c_2033_cpu.c", "    composed.cs = 0x00BAu;", "    /* removed */")
        self.assertEqual(self.status(root), 1)

    # --- the two review findings that the policy has to enforce ---

    def test_a_changed_macro_definition_is_red(self):
        """F1: the resolved VALUE of an allowed macro site is enforced, not just its spelling."""
        root = self.full_fixture()
        self.mutate(root, "slot_a_directory_fullstate.c", "#define SYSTEM_SEG 0x131cu", "#define SYSTEM_SEG 0x0AF8u")
        rows, bad, _ = census.check(root)
        self.assertEqual(1 if bad else 0, 1)
        self.assertTrue(any("0x0AF8" in row and "0x131C" in row for row in rows), rows)

    def test_a_site_moved_to_another_function_is_red(self):
        """F2: the key binds the enclosing function, so appending the same statement elsewhere is not the same site."""
        root = self.full_fixture()
        self.mutate(root, "overlay_u6c_2033_cpu.c", "    composed.cs = 0x00BAu;", "    /* moved */")
        self.mutate(root, "overlay_u6c_2033_cpu.c", "static void note(pr_u6c_2033_diagnostic *diagnostic, uint16_t site, int callee, int status)\n{",
                    "static void note(pr_u6c_2033_diagnostic *diagnostic, uint16_t site, int callee, int status)\n{\n    composed.cs = 0x00BAu;")
        self.assertEqual(self.status(root), 1)

    def test_an_array_subscript_assignment_is_detected(self):
        """F3: the scanner must not lose a form the old one caught (`cpus[0].cs = ...`)."""
        root = self.tree(**{"e.c": "void f(void){ cpus[0].cs = 0x0AF8u; }\n"})
        self.assertEqual([(n, v) for n, _, _, v, _ in census.census(root)], [("e.c", 0x0AF8)])

    def test_a_one_line_definition_does_not_inherit_the_previous_function(self):
        """F2, count-preserving form: delete the allowed line, append a legal one-line function that assigns the same
        statement. A tolerant head parser plus a cleared name at the closing brace must make this a finding."""
        root = self.full_fixture()
        self.mutate(root, "overlay_u6c_2033_cpu.c", "    composed.cs = 0x00BAu;", "    /* moved out */")
        path = Path(root) / "src/recovered/overlay_u6c_2033_cpu.c"
        text = path.read_text() + ("\nvoid reviewer_moved(pr_res_0973_cpu composed) "
                                   "{ composed.cs = 0x00BAu; (void)composed; }\n")
        path.write_text(text)
        rows, bad, _ = census.check(root)
        self.assertEqual(1 if bad else 0, 1, rows)
        self.assertTrue(any("reviewer_moved" in row for row in rows), rows)

    def test_a_nested_block_keeps_the_function_identity(self):
        """Clearing happens at the real function boundary, not at every nested closing brace."""
        root = self.full_fixture()
        self.mutate(root, "overlay_u6c_2033_cpu.c", "    composed.cs = 0x00BAu;",
                    "    if (first < 6u) {\n        composed.cs = 0x00BAu;\n    }")
        self.assertEqual(self.status(root), 0)

    def test_comments_and_strings_do_not_change_context(self):
        """A brace or a `cs =` inside a comment or a string literal is not code and must not shift the parser."""
        root = self.full_fixture()
        self.mutate(root, "overlay_u6c_2033_cpu.c", "    composed.cs = 0x00BAu;",
                    '    const char *probe = "} composed.cs = 0x1234u; {";\n'
                    "    /* } composed.cs = 0x0AF8u; { */\n"
                    "    // } c->cs = 0x0AF8u;\n"
                    "    (void)probe;\n"
                    "    composed.cs = 0x00BAu;")
        self.assertEqual(self.status(root), 0)

    def test_a_same_line_boundary_does_not_inherit_the_previous_function(self):
        """F2, same-line form: '} void f(...) { composed.cs = ...; }' on ONE physical line must be attributed to f.

        The line closes the previous body and opens the next one, so a per-line map would still name the old function;
        the context has to be taken at the assignment's own position."""
        root = self.full_fixture()
        self.mutate(root, "overlay_u6c_2033_cpu.c", "    composed.cs = 0x00BAu;", "    /* moved out */")
        path = Path(root) / "src/recovered/overlay_u6c_2033_cpu.c"
        text = path.read_text().rstrip("\n") + (
            " void reviewer_moved(pr_res_0973_cpu composed) { composed.cs = 0x00BAu; (void)composed; }\n")
        path.write_text(text)
        rows, bad, _ = census.check(root)
        self.assertEqual(1 if bad else 0, 1, rows)
        self.assertTrue(any("reviewer_moved" in row for row in rows), rows)

    def test_a_file_scope_assignment_has_no_function_identity(self):
        """Fail closed: an assignment outside every body reports unknown function context and can never match."""
        root = self.full_fixture()
        path = Path(root) / "src/recovered/zz_scope.c"
        path.write_text("int x = 0;\nvoid g(void)\n{\n    comps[0].cs = 0x0AF8u;\n}\n")
        rows, bad, _ = census.check(root)
        self.assertEqual(1 if bad else 0, 1)
        self.assertTrue(any("zz_scope.c" in row for row in rows), rows)

    def test_an_unresolvable_macro_cannot_hide_the_expected_site(self):
        """A header-defined name is reported by name; it never silently disappears."""
        root = self.full_fixture()
        self.mutate(root, "overlay_u6c_2033_cpu.c", "composed.cs = 0x00BAu;", "composed.cs = RES_0973_SEG;")
        rows, bad, _ = census.check(root)
        self.assertEqual(1 if bad else 0, 1)
        self.assertTrue(any("unresolved macro" in row for row in rows), rows)

    def test_classify_reads_the_models_it_describes(self):
        root = self.tree(**{"e.c": "void f(void, uint16_t seg){ c->cs = seg; }\nvoid g(void){ uint16_t s = rd(ram, cs, 0x10u); c->cs = s; }\n"})
        counts = census.classify(root)
        self.assertEqual({k: len(v) for k, v in counts.items()}, {"LIVE": 1, "PARAM": 1, "OTHER": 0})

    def test_lowercase_header_macro_adds_one_finding_to_a_valid_tree(self):
        root = self.full_fixture()
        self.assertEqual(self.status(root), 0)
        folder = root / "src/recovered"
        (folder / "hidden.h").write_text("#define hidden_cs 0x1234u\n")
        (folder / "header_probe.c").write_text('#include "hidden.h"\nvoid f(void){cpu->cs=hidden_cs;}\n')
        rows, bad, total = census.check(root)
        key = ("header_probe.c", "f", "cpu->cs = hidden_cs;")
        self.assertEqual(bad, {key}, rows)
        self.assertEqual(total, 5)
        self.assertTrue(any("unresolved macro hidden_cs" in row for row in rows))

    def test_transitive_angle_header_and_cycle_find_mixed_case_macro(self):
        root = Path(self.tree(**{"a.c": '#include <poolrad/outer.h>\nvoid f(void){c->cs=HiddenCs;}\n'}))
        folder = root / "include/poolrad"
        folder.mkdir(parents=True)
        (folder / "outer.h").write_text('#include "inner.h"\n')
        (folder / "inner.h").write_text('#include "outer.h"\n#define HiddenCs (0x1234u)\n')
        self.assertEqual(census.census(root), [("a.c", 2, "c->cs = HiddenCs;", None, "f")])

    def test_unincluded_header_does_not_turn_parameter_into_macro(self):
        root = self.tree(**{"a.c": "void f(uint16_t seg){c->cs=seg;}\n",
                            "unrelated.h": "#define seg 0x1234u\n"})
        self.assertEqual(census.census(root), [])

    def test_commented_includes_and_defines_do_not_invent_macros(self):
        root = self.tree(**{"a.c": '/* #include "fake.h" */\n#include "real.h"\nvoid f(uint16_t seg){c->cs=seg;}\n',
                            "fake.h": "#define seg 0x1234u\n",
                            "real.h": "/* #define seg 0x1234u */\n// #define seg 0x1234u\n"})
        self.assertEqual(census.census(root), [])

    def test_function_like_header_macro_does_not_hide_parameter(self):
        root = self.tree(**{"a.c": '#include "helpers.h"\nvoid f(uint16_t seg){c->cs=seg;}\n',
                            "helpers.h": "#define seg(x) (x)\n"})
        self.assertEqual(census.census(root), [])

    def test_header_lexing_agrees_with_compiler_macro_kind(self):
        # VP15 i32460: comment boundaries and phase ordering affect macro kind.
        cases = [
            ("comment-before-value", "#define hidden_cs/**/(0x1234u)\n", True),
            ("comment-after-directive", "#define/**/hidden_cs 0x1234u\n", True),
            ("continued-line-comment", "// ignored \\\n#define hidden_cs 0x1234u\n", False),
            ("plain-object", "#define hidden_cs (0x1234u)\n", True),
            ("plain-function", "#define hidden_cs(x) (x)\n", False),
            ("spliced-directive", "#def\\\nine hidden_cs 0x1234u\n", True),
            ("spliced-function", "#define hidden_cs\\\n(x) (x)\n", False),
            ("comment-in-quoted-text", 'static char *s="/*";\n#define hidden_cs 0x1234u\n', True),
            ("multiline-comment", "#define hidden_cs/* first\nsecond */(0x1234u)\n", True),
        ]
        for label, header, expected in cases:
            with self.subTest(label=label):
                root = Path(self.tree(**{
                    "a.c": '#include "probe.h"\nvoid f(void){c->cs=hidden_cs;}\n',
                    "probe.h": header,
                }))
                result = subprocess.run(["cc", "-E", "-dM", "-x", "c", "-"], input=header,
                                        text=True, capture_output=True, check=True)
                declarations = [line for line in result.stdout.splitlines()
                                if line.startswith("#define hidden_cs ")]
                self.assertEqual(bool(declarations), expected, result.stdout)
                sites = census.census(root)
                self.assertEqual(len(sites), int(expected), (header, sites))
                if expected:
                    self.assertEqual(sites[0], ("a.c", 2, "c->cs = hidden_cs;", None, "f"))

    def test_header_discovery_does_not_resolve_or_allow_a_protected_site(self):
        root = self.full_fixture()
        self.assertEqual(self.status(root), 0)
        self.mutate(root, "slot_a_directory_fullstate.c", "#define SYSTEM_SEG 0x131cu",
                    '#include "system_seg.h"')
        (root / "src/recovered/system_seg.h").write_text("#define SYSTEM_SEG 0x131cu\n")
        rows, bad, total = census.check(root)
        self.assertEqual(total, 4)
        self.assertEqual(bad, {("slot_a_directory_fullstate.c", "pr_slot_a_directory_step", "c->cs = SYSTEM_SEG;")})
        self.assertTrue(any("unresolved macro SYSTEM_SEG" in row for row in rows))


if __name__ == "__main__":
    unittest.main()
