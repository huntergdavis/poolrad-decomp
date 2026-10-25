"""Synthetic negative controls; no original game bytes or captures."""
import hashlib
import json
from pathlib import Path
import tempfile
import unittest

from parity_report import report


class ReceiptTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.left, self.right = [Path(self.temp.name) / n for n in ('original', 'replacement')]
        for path in (self.left, self.right):
            path.mkdir()
            (path / 'parity-mode.txt').write_text(path.name)
            (path / '.oracle-input-sha256').write_text('synthetic  ./SAVGAMA.DAT\n')
            (path / 'parity-build-sha256').write_text('synthetic-build\n')
            (path / 'registry.tsv').write_text('RES_FAKE_0000\t0000\t1\t00\tff\tunique\n')
            (path / 'parity-command.txt').write_text('synthetic-test')
            (path / 'parity-dispatch.txt').write_text('RES_FAKE_0000')
            (path / 'observer.dispatch').write_text('RES_FAKE_0000\t1\t1234:0000\tROUTE_RETURNED\n')
            (path / 'checkpoint.ram').write_bytes(bytes(1 << 20))
            (path / 'checkpoint.vram').write_bytes(bytes(16))
            (path / 'checkpoint.rgb').write_bytes(bytes(3))
            regs = dict.fromkeys('eax ebx ecx edx esi edi ebp esp eip cs ds es ss fs gs flags'.split(), 0)
            data = dict(schema=1, id='RES_FAKE_0000', case='loaded', arrival=1,
                        routed_returns=int(path == self.right), routed_stops=0,
                        registers=regs, video=dict(vmemsize=16),
                        frame=dict(width=1, height=1, format='RGB24',
                                   sha256=hashlib.sha256(bytes(3)).hexdigest()))
            (path / 'checkpoint.json').write_text(json.dumps(data))

    def mutate_json(self, fn):
        path = self.right / 'checkpoint.json'
        data = json.loads(path.read_text())
        fn(data)
        path.write_text(json.dumps(data))

    def test_equal(self):
        self.assertEqual(report(self.left, self.right)['result'], 'PASS')

    def test_ram_corruption_fails(self):
        with (self.right / 'checkpoint.ram').open('r+b') as f:
            f.seek(54321)
            f.write(b'\x01')
        r = report(self.left, self.right)
        self.assertEqual(r['result'], 'FAIL')
        self.assertEqual(r['state']['ram']['first_offsets'], [54321])

    def test_vram_corruption_fails(self):
        (self.right / 'checkpoint.vram').write_bytes(b'\x01' + bytes(15))
        self.assertEqual(report(self.left, self.right)['result'], 'FAIL')

    def test_frame_corruption_rejected(self):
        (self.right / 'checkpoint.rgb').write_bytes(b'\x01\x00\x00')
        with self.assertRaisesRegex(ValueError, 'corrupt'):
            report(self.left, self.right)

    def test_valid_but_different_frame_fails(self):
        pixels = b'\x01\x00\x00'
        (self.right / 'checkpoint.rgb').write_bytes(pixels)
        self.mutate_json(lambda d: d['frame'].update(sha256=hashlib.sha256(pixels).hexdigest()))
        self.assertEqual(report(self.left, self.right)['result'], 'FAIL')

    def test_register_corruption_fails(self):
        self.mutate_json(lambda d: d['registers'].update(eax=1))
        self.assertEqual(report(self.left, self.right)['result'], 'FAIL')

    def test_missing_register_rejected(self):
        self.mutate_json(lambda d: d['registers'].pop('fs'))
        with self.assertRaisesRegex(ValueError, 'register'):
            report(self.left, self.right)

    def test_wrong_arrival_rejected(self):
        self.mutate_json(lambda d: d.update(id='RES_WRONG_0000'))
        with self.assertRaisesRegex(ValueError, 'arrivals'):
            report(self.left, self.right)

    def test_wrong_save_rejected(self):
        (self.right / '.oracle-input-sha256').write_text('different-save')
        with self.assertRaisesRegex(ValueError, 'save'):
            report(self.left, self.right)

    def test_no_c_execution_rejected(self):
        self.mutate_json(lambda d: d.update(routed_returns=0))
        with self.assertRaisesRegex(ValueError, 'route returns'):
            report(self.left, self.right)

    def test_different_registry_rejected(self):
        (self.right / 'registry.tsv').write_text('different')
        with self.assertRaisesRegex(ValueError, 'registries'):
            report(self.left, self.right)

    def test_original_with_c_execution_rejected(self):
        path = self.left / 'checkpoint.json'
        data = json.loads(path.read_text())
        data['routed_returns'] = 1
        path.write_text(json.dumps(data))
        with self.assertRaisesRegex(ValueError, 'original capture'):
            report(self.left, self.right)

    def test_ambiguous_signature_rejected(self):
        for run in (self.left, self.right):
            (run / 'registry.tsv').write_text('RES_FAKE_0000\t0000\t1\t00\tff\tambiguous\n')
        with self.assertRaisesRegex(ValueError, 'unique registry'):
            report(self.left, self.right)


if __name__ == '__main__':
    unittest.main()
