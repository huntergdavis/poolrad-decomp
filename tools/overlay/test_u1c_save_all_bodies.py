#!/usr/bin/env python3
"""One command must execute every registered body before either row can be AGREE."""
import json
import sys
import unittest
from pathlib import Path
from unittest import mock

ROOT = Path(__file__).resolve().parents[2]
PROFILES = ROOT / "tools/overlay/checker_execution_profiles.json"
MEDIA = ROOT / "resources/DOS/POOLRAD/GAME.OVR"


class ProfileCoverageTest(unittest.TestCase):
    def test_profile_runs_all_bodies(self):
        profiles = json.loads(PROFILES.read_text())
        self.assertEqual(profiles["tools/overlay/u1c_save_cpu_audit.py"]["args"], [],
                         "a single-body arg would let the other registered row report AGREE unexecuted")

    def test_one_broken_body_fails_the_command(self):
        sys.path.insert(0, str(ROOT / "tools/overlay"))
        import u1c_save_cpu_audit as audit

        def fake_audit(body, _ovr, _units):
            if body == "0ebc":
                raise ValueError("broken 0EBC body")

        with mock.patch.object(audit, "media", return_value=(None, None)), \
                mock.patch.object(audit, "audit", side_effect=fake_audit):
            self.assertEqual(audit.main([]), 1)

    @unittest.skipUnless(MEDIA.exists(), "owner GAME.OVR not under ignored resources/")
    def test_all_bodies_runs_both(self):
        sys.path.insert(0, str(ROOT / "tools/overlay"))
        import u1c_save_cpu_audit as audit
        self.assertEqual(audit.main([]), 0)


if __name__ == "__main__":
    unittest.main()
