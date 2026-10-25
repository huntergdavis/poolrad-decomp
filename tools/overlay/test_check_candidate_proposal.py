#!/usr/bin/env python3
"""Synthetic and media-gated checks for the candidate-scout byte compare."""
import sys
import unittest
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
import check_candidate_proposal as C

ROOT = Path(__file__).resolve().parents[2]
MEDIA = ROOT / "resources/DOS/POOLRAD/GAME.OVR"
PROPOSAL = ROOT / "docs/re/CANDIDATE_SCOUT_DS41B_BATCH1.md"


class ParserTest(unittest.TestCase):
    def test_hexbytes_tolerates_spacing_and_line_breaks(self):
        self.assertEqual(C.hexbytes("55 89 e5\n8b 46 06"), bytes.fromhex("5589e58b4606"))
        self.assertEqual(C.hexbytes("5589e5"), bytes.fromhex("5589e5"))
        self.assertIsNone(C.hexbytes(""))

    def test_blocks_reads_fields_and_dump(self):
        text = ("prose\nBODY: OVL_0001_0002\nUNIT: 0x1\nREL_OFFSET: 0x2\n"
                "LENGTH: 4\nSHA256: " + "0" * 64 + "\nBYTES:\n```bytes\n55 89 e5 cb\n```\n")
        found = list(C.blocks(text))
        self.assertEqual(len(found), 1)
        fields, dump = found[0]
        self.assertEqual(fields["BODY"], "OVL_0001_0002")
        self.assertEqual(C.hexbytes(dump), bytes.fromhex("5589e5cb"))

    def test_blocks_requires_bytes_fence(self):
        text = "BODY: OVL_0001_0002\nUNIT: 0x1\nREL_OFFSET: 0x2\nLENGTH: 4\n"
        fields, dump = list(C.blocks(text))[0]
        self.assertIsNone(dump)


class ProposalTest(unittest.TestCase):
    @unittest.skipUnless(MEDIA.exists(), "owner GAME.OVR not under ignored resources/")
    def test_committed_proposal_verifies(self):
        self.assertEqual(C.main([str(PROPOSAL)]), 0)


if __name__ == "__main__":
    unittest.main()
