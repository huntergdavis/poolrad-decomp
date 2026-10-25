"""Executable selection and honest pin status, without game media or DOSBox."""
from hashlib import sha256
from pathlib import Path
import subprocess
import tempfile
from types import SimpleNamespace
import unittest
from unittest.mock import patch

import native_emulator_provenance as P


class ProvenanceTest(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.binary = Path(self.temp.name) / 'dosbox-x'
        self.binary.write_bytes(b'synthetic emulator build A')
        self.probe = patch.object(P.subprocess, 'run', return_value=SimpleNamespace(
            stdout='startup banner\nDOSBox-X version 2026.01.02, test build\n', stderr='', returncode=0))
        self.run = self.probe.start()
        self.addCleanup(self.probe.stop)

    def test_hash_and_version_describe_the_resolved_executable(self):
        alias = self.binary.with_name('alias')
        alias.symlink_to(self.binary)
        result = P.describe_dosbox(alias)
        self.assertEqual(result['path'], str(self.binary))
        self.assertEqual(result['sha256'], sha256(self.binary.read_bytes()).hexdigest())
        self.assertEqual(result['version'], 'DOSBox-X version 2026.01.02')
        self.assertFalse(result['pinned_build_used'])
        self.assertEqual(result['pinned_source'], P.PINNED_SOURCE)
        self.assertEqual(self.run.call_args.args[0], [str(self.binary), '--version'])
        self.assertFalse(self.run.call_args.kwargs['check'])
        self.assertEqual(self.run.call_args.kwargs['timeout'], 10)

    def test_path_selected_binary_is_the_one_hashed(self):
        other = self.binary.with_name('selected-by-path')
        other.write_bytes(b'different executable, same release string')
        with patch.object(P.shutil, 'which', return_value=str(other)):
            result = P.describe_dosbox('dosbox-x')
        self.assertEqual(result['path'], str(other))
        self.assertEqual(result['sha256'], sha256(other.read_bytes()).hexdigest())
        self.assertNotEqual(result['sha256'], sha256(self.binary.read_bytes()).hexdigest())
        self.assertFalse(result['pinned_build_used'])
        self.assertEqual(self.run.call_args.args[0][0], str(other))

    def test_version_on_stderr_is_recorded(self):
        self.run.return_value = SimpleNamespace(stdout='', stderr='DOSBox-X version 2026.01.02\n', returncode=0)
        self.assertEqual(P.describe_dosbox(self.binary)['version'], 'DOSBox-X version 2026.01.02')

    def test_unrecognized_version_refuses_a_provenance_claim(self):
        self.run.return_value = SimpleNamespace(stdout='another emulator 2026.01.02', stderr='', returncode=1)
        with self.assertRaisesRegex(ValueError, 'No DOSBox-X version'):
            P.describe_dosbox(self.binary)

    def test_failed_version_probe_is_not_a_successful_identity(self):
        self.run.return_value = SimpleNamespace(stdout='DOSBox-X version 2026.01.02\n',
                                               stderr='', returncode=2, args=['dosbox-x', '--version'])
        with self.assertRaises(subprocess.CalledProcessError):
            P.describe_dosbox(self.binary)

    def test_version_banner_with_exit_one_is_recorded_honestly(self):
        self.run.return_value.returncode = 1
        result = P.describe_dosbox(self.binary)
        self.assertEqual(result['version_exit_code'], 1)
        self.assertFalse(result['pinned_build_used'])

    def test_missing_path_command_fails_before_launch(self):
        with patch.object(P.shutil, 'which', return_value=None):
            with self.assertRaises(FileNotFoundError):
                P.describe_dosbox('dosbox-x')
        self.run.assert_not_called()


if __name__ == '__main__':
    unittest.main()
