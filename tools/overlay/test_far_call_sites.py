"""far_call_sites.py: the 063A identity sites (PR #1507) reproduce in one command. Skipped without the media."""
import unittest
from pathlib import Path

import far_call_sites as F

MEDIA = F.ROOT / 'resources/DOS/POOLRAD'


@unittest.skipUnless((MEDIA / 'GAME.OVR').exists() and (MEDIA / 'START.EXE').exists(), 'game media not present')
class FarCallSitesTest(unittest.TestCase):
    def setUp(self):
        self.exe = (MEDIA / 'START.EXE').read_bytes()
        self.ovr = (MEDIA / 'GAME.OVR').read_bytes()

    def test_063a_stub_has_exactly_the_three_documented_sites(self):
        found = {(para, at) for para, at, _ in F.sites(self.exe, self.ovr, 0x008F, 0x002F)}
        self.assertEqual(found, {(0x23, 0x0663), (0x1E, 0x0DF4), (0x20, 0x106A)})

    def test_unrelocated_bytes_do_not_count(self):
        # Every reported site has its segment word in the fixup table; a target nobody calls gives nothing.
        self.assertEqual(F.sites(self.exe, self.ovr, 0x008F, 0xFFF0), [])


if __name__ == '__main__':
    unittest.main()
