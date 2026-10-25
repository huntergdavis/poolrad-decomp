#!/usr/bin/env python3
"""Synthetic and media-gated checks for the candidate-scout span digest checker."""
import re
import sys
import tempfile
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_candidate_proposal as C

ROOT = Path(__file__).resolve().parents[2]
MEDIA = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
PROPOSAL = ROOT / "docs/re/CANDIDATE_SCOUT_DS41B_BATCH1.md"


class ParserTest(unittest.TestCase):
    def test_blocks_reads_fields(self):
        text = ("prose\nBODY: OVL_0001_0002\nUNIT: 0x1\nREL_OFFSET: 0x2\n"
                "LENGTH: 4\nFILE_OFFSET: 0x20002\nSHA256: " + "0" * 64 + "\n")
        found = list(C.blocks(text))
        self.assertEqual(len(found), 1)
        self.assertEqual(found[0]["BODY"], "OVL_0001_0002")
        self.assertEqual(found[0]["FILE_OFFSET"], "0x20002")

    def test_blocks_requires_body(self):
        self.assertEqual(list(C.blocks("UNIT: 0x1\nLENGTH: 4\n")), [])

    def test_digest_matches_requires_a_claim(self):
        self.assertFalse(C.digest_matches(None, "abc"))
        self.assertTrue(C.digest_matches("ABC", "abc"))
        self.assertFalse(C.digest_matches("0" * 64, "abc"))


class ProposalTest(unittest.TestCase):
    @unittest.skipUnless(MEDIA.exists(), "owner GAME.OVR not under ignored resources/")
    def test_committed_proposal_verifies(self):
        self.assertEqual(C.main([str(PROPOSAL)]), 0)

    def _tampered(self, pattern, replacement):
        text = PROPOSAL.read_text()
        tampered, count = re.subn(pattern, replacement, text, count=1, flags=re.M)
        self.assertEqual(count, 1, f"no line matched {pattern!r}")
        handle = tempfile.NamedTemporaryFile("w", suffix=".md", delete=False)
        handle.write(tampered)
        handle.close()
        return Path(handle.name)

    @unittest.skipUnless(MEDIA.exists(), "owner GAME.OVR not under ignored resources/")
    def test_wrong_digest_exits_nonzero_even_when_bytes_match(self):
        path = self._tampered(r"^SHA256: [0-9a-fA-F]{64}$", "SHA256: " + "0" * 64)
        try:
            self.assertEqual(C.main([str(path)]), 1)
        finally:
            path.unlink()

    @unittest.skipUnless(MEDIA.exists(), "owner GAME.OVR not under ignored resources/")
    def test_wrong_length_exits_nonzero(self):
        path = self._tampered(r"^LENGTH: 78$", "LENGTH: 77")
        try:
            self.assertEqual(C.main([str(path)]), 1)
        finally:
            path.unlink()

    @unittest.skipUnless(MEDIA.exists(), "owner GAME.OVR not under ignored resources/")
    def test_wrong_file_offset_exits_nonzero(self):
        path = self._tampered(r"^FILE_OFFSET: 4056$", "FILE_OFFSET: 4057")
        try:
            self.assertEqual(C.main([str(path)]), 1)
        finally:
            path.unlink()

    @unittest.skipUnless(MEDIA.exists(), "owner GAME.OVR not under ignored resources/")
    def test_missing_digest_exits_nonzero(self):
        path = self._tampered(r"^SHA256: [0-9a-fA-F]{64}$", "SHA256_MISSING: x")
        try:
            self.assertEqual(C.main([str(path)]), 1)
        finally:
            path.unlink()


if __name__ == "__main__":
    unittest.main()
