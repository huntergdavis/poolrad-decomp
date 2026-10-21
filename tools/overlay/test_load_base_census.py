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
        self.assertEqual([(name, number) for name, number, _ in census.census(root)], [("a.c", 1), ("b.c", 2)])

    def test_the_committed_sources_hold_only_the_allowed_native_capture_sites(self):
        sites = census.census()
        self.assertEqual({name for name, _, _ in sites}, set(census.ALLOWED), sites)
        self.assertEqual(len(sites), 3)       # run014 twice and the Slot A directory once

    def test_classify_reads_the_models_it_describes(self):
        root = self.tree(**{"e.c": "void f(uint16_t seg){ c->cs = seg; }\nvoid g(void){ uint16_t s = rd(ram, cs, 0x10u); c->cs = s; }\n"})
        counts = census.classify(root)
        self.assertEqual({k: len(v) for k, v in counts.items()}, {"LIVE": 1, "PARAM": 1, "OTHER": 0})


if __name__ == "__main__":
    unittest.main()
