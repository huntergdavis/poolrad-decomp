#!/usr/bin/env python3
"""Synthetic tests for the pure parts of classify_overlay.py (no game data, no Capstone)."""
import os
import sys
import unittest

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import classify_overlay as co  # noqa: E402


def sig(**kw):
    base = dict(strings=0, doc_globals="", ver_calls=0, ds_globals=0, calls=0, far_runtime=0,
                far_overlay=0, far_resident=0, near=0, indirect=0, is_verified=0, decoded=0, bound=0)
    base.update(kw)
    return base


class PascalStringTest(unittest.TestCase):
    def test_length_prefixed_printable(self):
        code = bytes([3]) + b"ABC" + bytes([0])
        self.assertTrue(co.is_pascal_string(code, 0))

    def test_rejects_unprintable_and_truncated(self):
        self.assertFalse(co.is_pascal_string(bytes([2, 0x41, 0x01]), 0))
        self.assertFalse(co.is_pascal_string(bytes([5, 0x41, 0x42]), 0))
        self.assertFalse(co.is_pascal_string(bytes([0, 0x41]), 0))
        self.assertFalse(co.is_pascal_string(b"", 0))
        self.assertFalse(co.is_pascal_string(bytes([1, 0x41]), 5))


class ClassifyTest(unittest.TestCase):
    def test_verified_wins(self):
        self.assertEqual(co.classify(sig(is_verified=1, strings=0, calls=0)), ("game_verified", "verified"))

    def test_string_or_global_or_verified_call_is_evidenced(self):
        self.assertEqual(co.classify(sig(strings=2))[0], "game_evidenced")
        self.assertEqual(co.classify(sig(doc_globals="ecl_pc", ds_globals=1))[0], "game_evidenced")
        self.assertEqual(co.classify(sig(ver_calls=1, calls=1, far_overlay=1))[0], "game_evidenced")

    def test_helper_candidate_rules(self):
        self.assertEqual(co.classify(sig())[0], "helper_candidate")                       # pure leaf
        self.assertEqual(co.classify(sig(calls=3, far_runtime=3))[0], "helper_candidate")  # runtime-only
        self.assertEqual(co.classify(sig(calls=3, far_runtime=2, far_resident=1))[0],
                         "game_or_other_library_unresolved")                             # mixed calls
        self.assertEqual(co.classify(sig(ds_globals=1))[0], "game_or_other_library_unresolved")  # undocumented global

    def test_confidence_values(self):
        self.assertEqual(co.classify(sig(strings=1))[1], "provisional")
        self.assertEqual(co.classify(sig())[1], "provisional")
        self.assertEqual(co.classify(sig(ds_globals=2, calls=1, near=1))[1], "unresolved")


if __name__ == "__main__":
    unittest.main()
