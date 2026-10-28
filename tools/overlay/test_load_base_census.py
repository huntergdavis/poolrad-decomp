#!/usr/bin/env python3
"""The load-base census: its own detection on synthetic sources, and the exact committed result."""
import sys
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

    def test_a_literal_and_a_macro_are_found_but_a_live_read_and_a_parameter_are_not(self):
        root = self.tree(**{
            "a.c": "void f(void){ cpu->cs = 0x00BAu; }\n",
            "b.c": "#define SYSTEM_CS 0x0AF8u\nvoid f(void){ c->cs=SYSTEM_CS;c->ip=1; }\n",
            "c.c": "void f(uint16_t seg){ c->cs = seg; c->cs = rd(ram, cs, 0x0E11u); }\n",
            "d.c": "#define OFFSET 0x0BB8u\nvoid f(void){ c->ip = OFFSET; /* c->cs = 0x00BAu; */ }\n",
        })
        self.assertEqual([(name, number) for name, number, _, _ in census.census(root)], [("a.c", 1), ("b.c", 2)])

    def test_the_committed_sources_hold_only_the_allowed_sites(self):
        sites = census.census()
        keys = [(name, census.site_text(statement)) for name, _, _, statement in sites]
        self.assertEqual(set(keys), set(census.ALLOWED), sites)
        self.assertEqual(len(sites), 4)       # run014 twice, the Slot A capture once, the 006C composition once
        counts = {key: keys.count(key) for key in set(keys)}
        self.assertEqual({key: want for key, (want, _) in census.ALLOWED.items()}, counts)

    def coded_finding(self, **files):
        """Run the module's own policy over a synthetic tree and return its exit status."""
        root = self.tree(**files)
        sites = census.census(root)
        counts = {}
        for name, _, _, statement in sites:
            key = (name, census.site_text(statement))
            counts[key] = counts.get(key, 0) + 1
        bad = set()
        for name, _, _, statement in sites:
            key = (name, census.site_text(statement))
            want = census.ALLOWED.get(key, (None, None))[0]
            if want is None or want != counts[key]:
                bad.add(key)
        for key, (want, _) in census.ALLOWED.items():
            if counts.get(key, 0) != want:
                bad.add(key)
        return 0 if not bad else 1

    def test_the_four_required_controls_are_red(self):
        # (a) changed value at the expected site
        self.assertEqual(self.coded_finding(**{"overlay_u6c_2033_cpu.c": "void f(void)\n{\n    composed.cs = 0x0AF8u;\n}\n"}), 1)
        # (b) extra constant in the same file, different value
        self.assertEqual(self.coded_finding(**{
            "overlay_u6c_2033_cpu.c": "void f(void)\n{\n    composed.cs = 0x00BAu;\n    if (0) cpu->cs = 0x1234u;\n}\n"}), 1)
        # (c) duplicate occurrence of the expected statement
        self.assertEqual(self.coded_finding(**{
            "overlay_u6c_2033_cpu.c": "void f(void)\n{\n    composed.cs = 0x00BAu;\n    composed.cs = 0x00BAu;\n}\n"}), 1)
        # (d) the expected site missing (shifted away): silence is not a pass
        self.assertEqual(self.coded_finding(**{"unrelated.c": "void f(void)\n{\n    c->cs = 0x00BAu;\n}\n"}), 1)

    def test_a_header_defined_macro_cannot_hide_the_expected_site(self):
        # The scanner cannot resolve a header #define, so the uppercase name is reported as a constant site.
        self.assertEqual(self.coded_finding(**{"overlay_u6c_2033_cpu.c": "void f(void)\n{\n    composed.cs = RESIDENT_SEG;\n}\n"}), 1)

    def test_a_second_or_changed_constant_in_an_allowed_file_is_still_a_finding(self):
        # The allowlist is keyed by SITE, not filename: an added occurrence or a changed value must not ride along.
        allowed = "composed.cs = 0x00BAu;\n"
        twice = self.tree(**{"overlay_u6c_2033_cpu.c": "void f(void)\n{\n" + allowed + allowed + "}\n"})
        self.assertIn(("overlay_u6c_2033_cpu.c", census.site_text("composed.cs = 0x00BAu;")),
                      [(n, census.site_text(st)) for n, _, _, st in census.census(twice)])
        changed = self.tree(**{"overlay_u6c_2033_cpu.c": "void f(void)\n{\ncomposed.cs = 0x00BBu;\n}\n"})
        self.assertNotIn(("overlay_u6c_2033_cpu.c", census.site_text("composed.cs = 0x00BBu;")), set(census.ALLOWED))
        self.assertEqual([(n, census.site_text(st)) for n, _, _, st in census.census(changed)],
                         [("overlay_u6c_2033_cpu.c", census.site_text("composed.cs = 0x00BBu;"))])

    def test_a_header_defined_macro_name_is_a_candidate(self):
        # The scanner cannot resolve #defines from headers, so any uppercase identifier counts as a constant.
        root = self.tree(**{"h.c": "void f(void)\n{\n    cpu->cs = RESIDENT_CS;\n}\n"})
        self.assertEqual([(n, census.site_text(st)) for n, _, _, st in census.census(root)],
                         [("h.c", census.site_text("cpu->cs = RESIDENT_CS;"))])

    def test_classify_reads_the_models_it_describes(self):
        root = self.tree(**{"e.c": "void f(uint16_t seg){ c->cs = seg; }\nvoid g(void){ uint16_t s = rd(ram, cs, 0x10u); c->cs = s; }\n"})
        counts = census.classify(root)
        self.assertEqual({k: len(v) for k, v in counts.items()}, {"LIVE": 1, "PARAM": 1, "OTHER": 0})


if __name__ == "__main__":
    unittest.main()
