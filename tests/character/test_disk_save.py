"""Synthetic tests: no original media, character names, or save contents."""
import hashlib
import importlib.util
import json
from pathlib import Path
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('disk_save', ROOT / 'tools/character/disk_save.py')
save = importlib.util.module_from_spec(spec)
sys.modules[spec.name] = save
spec.loader.exec_module(save)


class ExportTests(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        self.source = self.root / 'source'
        self.source.mkdir()
        self.output = self.root / 'output'
        # Every byte is patterned, including stale pointers and unknown spans.
        self.character = bytes((index * 37 + 19) % 256 for index in range(285))
        self.world = bytearray((index * 13 + 7) % 256 for index in range(13137))
        self.world[0x3208] = 1
        stem = b'CHRDATA1'
        self.world[0x3209:0x3209 + 1 + len(stem)] = bytes([len(stem)]) + stem
        self.original = {
            'CHRDATA1.SAV': self.character,
            'CHRDATA1.ITM': bytes(range(63)),
            'CHRDATA1.SPC': bytes(range(9)),
            'SAVGAMA.DAT': bytes(self.world),
        }
        for name, data in self.original.items():
            (self.source / name).write_bytes(data)

    def test_all_bytes_preserved_and_receipted(self):
        report = save.export_directory(self.source, self.output)
        self.assertEqual(report['byte_identical_files'], 4)
        receipts = json.loads((self.output / 'write-receipts.json').read_text())['files']
        self.assertEqual(len(receipts), 4)
        for receipt in receipts:
            name = Path(receipt['file']).name
            self.assertEqual((self.output / name).read_bytes(), self.original[name])
            self.assertEqual((self.source / name).read_bytes(), self.original[name])
            expected = hashlib.sha256(self.original[name]).hexdigest()
            self.assertEqual(receipt['before_sha256'], expected)
            self.assertEqual(receipt['after_sha256'], expected)
            self.assertEqual(receipt['changed_offsets'], [])
            self.assertEqual(receipt['before_after_bytes'], [])
            self.assertIsNone(receipt['game_build_loaded'])
            self.assertIn('UNVERIFIED', receipt['game_validation'])

    def test_existing_copy_is_never_overwritten(self):
        self.output.mkdir()
        sentinel = self.output / 'CHRDATA1.SAV'
        sentinel.write_bytes(b'keep')
        with self.assertRaises(ValueError):
            save.export_directory(self.source, self.output)
        self.assertEqual(sentinel.read_bytes(), b'keep')

    def test_source_and_descendant_destinations_refused(self):
        for destination in (self.source, self.source / 'new-copy'):
            with self.assertRaises(ValueError):
                save.export_directory(self.source, destination)
        self.assertEqual((self.source / 'CHRDATA1.SAV').read_bytes(), self.character)
        self.assertFalse((self.source / 'new-copy').exists())

    def test_symlink_destinations_refused(self):
        alias = self.root / 'alias'
        alias.symlink_to(self.source, target_is_directory=True)
        for destination in (alias, alias / 'new-copy'):
            with self.assertRaises(ValueError):
                save.export_directory(self.source, destination)
        broken = self.root / 'broken'
        broken.symlink_to(self.root / 'absent')
        with self.assertRaises(ValueError):
            save.export_directory(self.source, broken)

    def test_partial_frame_refused_before_writing(self):
        for name in ('CHRDATA1.SAV', 'CHRDATA1.ITM', 'CHRDATA1.SPC', 'SAVGAMA.DAT'):
            path = self.source / name
            path.write_bytes(self.original[name][:-1])
            with self.assertRaises(ValueError):
                save.export_directory(self.source, self.output)
            self.assertFalse(self.output.exists())
            path.write_bytes(self.original[name])

    def test_missing_world_member_refused_before_writing(self):
        (self.source / 'CHRDATA1.ITM').unlink()
        (self.source / 'CHRDATA1.SPC').unlink()
        (self.source / 'CHRDATA1.SAV').rename(self.source / 'CHRDATA2.SAV')
        with self.assertRaisesRegex(ValueError, 'absent character'):
            save.export_directory(self.source, self.output)
        self.assertFalse(self.output.exists())

    def test_opaque_name_and_pointers_are_not_normalized(self):
        # The raw exporter has no semantic name-length edit policy. Even an
        # uninterpreted length byte survives; framing is not game validation.
        raw = bytearray(self.character)
        raw[0] = 255
        for offset in (0x7f, 0xc8, 0x104, 0x108):
            raw[offset:offset + 4] = b'\xff\x12\x34\x56'
        (self.source / 'CHRDATA1.SAV').write_bytes(raw)
        save.export_directory(self.source, self.output)
        self.assertEqual((self.output / 'CHRDATA1.SAV').read_bytes(), raw)

    def test_failed_write_rolls_back_created_copy_only(self):
        original_open = Path.open
        def failing_open(path, mode='r', *args, **kwargs):
            if path == self.output / 'CHRDATA1.SPC' and mode == 'xb':
                raise OSError('synthetic write failure')
            return original_open(path, mode, *args, **kwargs)
        with patch.object(Path, 'open', failing_open):
            with self.assertRaisesRegex(OSError, 'synthetic write failure'):
                save.export_directory(self.source, self.output)
        self.assertFalse(self.output.exists())
        for name, data in self.original.items():
            self.assertEqual((self.source / name).read_bytes(), data)

    def test_absent_optional_sidecars_are_not_invented(self):
        (self.source / 'CHRDATA1.ITM').unlink()
        (self.source / 'CHRDATA1.SPC').unlink()
        report = save.export_directory(self.source, self.output)
        self.assertEqual(report['files_written'], 2)
        self.assertFalse((self.output / 'CHRDATA1.SPC').exists())
        self.assertFalse((self.output / 'CHRDATA1.ITM').exists())


if __name__ == '__main__':
    unittest.main()
