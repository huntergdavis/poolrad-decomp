"""Synthetic controls for binding a private build receipt to a hybrid run."""
import json
import io
from contextlib import redirect_stderr
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import emulator_provenance as provenance


class HybridProvenanceTest(unittest.TestCase):
    def setUp(self):
        self.identity = dict(path='/synthetic/dosbox-x', sha256='a' * 64,
                             version='DOSBox-X synthetic', pinned_build_used=False)

    def test_source_revision_does_not_certify_a_binary(self):
        build = dict(binary_sha256='a' * 64, source_commit='b' * 40,
                     commands=[['make', '-j2']], compiler_version='synthetic C++ 1')
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / 'build.json'
            path.write_text(json.dumps(build))
            with patch.object(provenance, 'describe_dosbox', return_value=dict(self.identity)):
                actual = provenance.describe('/synthetic/dosbox-x', path)
            self.assertFalse(actual['pinned_build_used'])
            self.assertEqual(actual['private_build'], build)

    def test_mismatched_or_incomplete_build_receipt_is_refused(self):
        for build in [dict(binary_sha256='c' * 64, source_commit='b' * 40, commands=['make']),
                      dict(binary_sha256='a' * 64, source_commit='b' * 40),
                      dict(binary_sha256='a' * 64, commands=['make'])]:
            with tempfile.TemporaryDirectory() as directory:
                path = Path(directory) / 'build.json'
                path.write_text(json.dumps(build))
                with patch.object(provenance, 'describe_dosbox', return_value=dict(self.identity)):
                    with self.assertRaises(ValueError):
                        provenance.describe('/synthetic/dosbox-x', path)

    def test_missing_executable_returns_cannot_run(self):
        with patch('sys.argv', ['provenance', 'missing', '--out', 'unused.json']):
            with patch.object(provenance, 'describe_dosbox', side_effect=FileNotFoundError('missing')):
                with redirect_stderr(io.StringIO()):
                    self.assertEqual(provenance.main(), 4)


if __name__ == '__main__':
    unittest.main()
