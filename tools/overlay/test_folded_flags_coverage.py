"""Reference coverage must not silently drop a historical folded-flag claim."""
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import folded_flags_coverage as coverage


class CoverageTest(unittest.TestCase):
    def fixture(self, directory):
        root = Path(directory)
        docs = root / 'docs/re'
        docs.mkdir(parents=True)
        ledger = coverage.ROOT / 'docs/re/FLAG_FOLDED_SITES.tsv'
        (docs / ledger.name).write_text(ledger.read_text())
        for name in coverage.DOCUMENTS.values():
            (docs / name).touch()
        return root, docs / ledger.name

    def test_an_uncovered_claim_is_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            root, ledger = self.fixture(directory)
            with ledger.open('a') as stream:
                stream.write('unproved_model.c\t1\tADD\tunproved flags\n')
            with patch.object(coverage, 'ROOT', root):
                with self.assertRaisesRegex(ValueError, 'coverage mismatch: missing=.*unproved_model'):
                    coverage.render()

    def test_two_proof_owners_are_refused(self):
        presets = dict(coverage.shared.PRESETS)
        presets['u77'] = [*presets['u77'], presets['u31'][0]]
        with patch.object(coverage.shared, 'PRESETS', presets):
            with self.assertRaisesRegex(ValueError, 'multiple proof-suite owners'):
                coverage.render()

    def test_missing_document_is_refused(self):
        with tempfile.TemporaryDirectory() as directory:
            root, _ = self.fixture(directory)
            (root / 'docs/re' / coverage.DOCUMENTS['u31']).unlink()
            with patch.object(coverage, 'ROOT', root):
                with self.assertRaisesRegex(ValueError, 'missing evidence document'):
                    coverage.render()


if __name__ == '__main__':
    unittest.main()
