"""Synthetic party assembly checks; all game-like bytes are invented."""
import json
from pathlib import Path
import sys
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/character'))
from assemble_party import assemble_directory
from disk_save import import_directory


class AssemblyTests(unittest.TestCase):
    def setUp(self):
        self.tmp = tempfile.TemporaryDirectory()
        self.addCleanup(self.tmp.cleanup)
        self.root = Path(self.tmp.name)
        self.source = self.root / 'source'
        self.source.mkdir()
        self.output = self.root / 'party'
        self.inputs = {}
        world = bytearray((i * 13 + 7) % 256 for i in range(13137))
        world[0x3208] = 3
        for index in range(1, 4):
            stem = f'CHRDATA{index}'
            self.inputs[stem + '.SAV'] = bytes((i * 37 + index) % 256 for i in range(285))
            self.inputs[stem + '.ITM'] = bytes([index]) * 63
            if index != 3:
                self.inputs[stem + '.SPC'] = bytes([index + 10]) * 9
            position = 0x3209 + 41 * (index - 1)
            world[position:position + 9] = bytes([8]) + stem.encode()
        self.inputs['SAVGAMA.DAT'] = bytes(world)
        for name, data in self.inputs.items():
            (self.source / name).write_bytes(data)

    def test_reordered_subset_preserves_records_sidecars_and_unknown_world_bytes(self):
        report = assemble_directory(self.source, self.output,
                                    ['CHRDATA3.SAV', 'CHRDATA1.SAV'])
        self.assertEqual(report['member_count'], 2)
        self.assertEqual((self.output / 'CHRDATB1.SAV').read_bytes(), self.inputs['CHRDATA3.SAV'])
        self.assertEqual((self.output / 'CHRDATB2.SAV').read_bytes(), self.inputs['CHRDATA1.SAV'])
        self.assertEqual((self.output / 'CHRDATB1.ITM').read_bytes(), self.inputs['CHRDATA3.ITM'])
        self.assertFalse((self.output / 'CHRDATB1.SPC').exists())
        self.assertEqual((self.output / 'CHRDATB2.SPC').read_bytes(), self.inputs['CHRDATA1.SPC'])
        world = (self.output / 'SAVGAMB.DAT').read_bytes()
        self.assertEqual(world[0x3208], 2)
        allowed = {0x3208}
        for index in range(2):
            position = 0x3209 + 41 * index
            self.assertEqual(world[position:position + 9], bytes([8]) + f'CHRDATB{index + 1}'.encode())
            allowed.update(range(position, position + 9))
        for offset, (before, after) in enumerate(zip(self.inputs['SAVGAMA.DAT'], world)):
            if offset not in allowed:
                self.assertEqual(before, after)
        import_directory(self.output)  # All new occupied references resolve.
        for name, data in self.inputs.items():
            self.assertEqual((self.source / name).read_bytes(), data)
        receipts = json.loads((self.output / 'write-receipts.json').read_text())['files']
        for receipt in receipts:
            original = self.inputs[Path(receipt['source']).name]
            output = Path(receipt['file']).read_bytes()
            changes = [i for i, (a, b) in enumerate(zip(original, output)) if a != b]
            self.assertEqual(receipt['changed_offsets'], changes)
            for change in receipt['before_after_bytes']:
                i = change['offset']
                self.assertEqual(change['before'], f'{original[i]:02x}')
                self.assertEqual(change['after'], f'{output[i]:02x}')
                self.assertTrue(change['reason'])
            self.assertIsNone(receipt['game_build_loaded'])

    def test_same_order_same_slot_is_byte_identical(self):
        members = [f'CHRDATA{i}.SAV' for i in range(1, 4)]
        report = assemble_directory(self.source, self.output, members, slot='A')
        self.assertEqual(report['byte_identical_files'], len(self.inputs))
        for name, data in self.inputs.items():
            self.assertEqual((self.output / name).read_bytes(), data)

    def test_invalid_selection_refused_before_output(self):
        for members in ([], ['CHRDATA1.SAV'] * 2, ['MISSING.SAV'],
                        [f'CHRDATZ{i}.SAV' for i in range(9)], ['SAVGAMA.DAT']):
            with self.assertRaises(ValueError):
                assemble_directory(self.source, self.output, members)
            self.assertFalse(self.output.exists())

    def test_invalid_slot_or_world_refused_before_output(self):
        for slot in ('../A', 'b', 'Z', 'AB', ''):
            with self.assertRaises(ValueError):
                assemble_directory(self.source, self.output, ['CHRDATA1.SAV'], slot=slot)
        for world in ('MISSING.DAT', 'CHRDATA1.SAV', '../SAVGAMA.DAT'):
            with self.assertRaises(ValueError):
                assemble_directory(self.source, self.output, ['CHRDATA1.SAV'], world=world)
        self.assertFalse(self.output.exists())

    def test_existing_output_refused_without_mutation(self):
        self.output.mkdir()
        sentinel = self.output / 'keep'
        sentinel.write_bytes(b'unchanged')
        with self.assertRaises(ValueError):
            assemble_directory(self.source, self.output, ['CHRDATA1.SAV'])
        self.assertEqual(sentinel.read_bytes(), b'unchanged')


if __name__ == '__main__':
    unittest.main()
