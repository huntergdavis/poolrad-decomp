"""Synthetic negative controls; no original game bytes or captures."""
import hashlib
import inspect
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import parity_report

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

    def known_layout(self):
        for run in (self.left, self.right):
            (run / '.oracle-input-sha256').write_text(
                'd825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd  ./START.EXE\n'
                '872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614  ./GAME.OVR\n')
            (run / 'registry.tsv').write_text('RES_0A40_02FD\t02fd\t1\t00\tff\tunique\n')
            path = run / 'checkpoint.json'
            data = json.loads(path.read_text())
            data['id'] = 'RES_0A40_02FD'
            data['registers'].update(cs=0x1264, eip=0x02fd, ds=0x14a0, ss=0x1d16, esp=0x3e04)
            path.write_text(json.dumps(data))

    def test_localization_boundaries_keep_every_difference(self):
        self.known_layout()
        expected = dict(bios_timer=[0x46c, 0x46f], randseed=[0x18e28, 0x18e2b],
                        stack_below_sp=[0x1d160, 0x20f63],
                        stack_at_or_above_sp=[0x20f64, 0x2115f],
                        unclassified=[0x46b, 0x470, 0x18e27, 0x18e2c, 0x1d15f, 0x21160])
        offsets = sorted(i for group in expected.values() for i in group)
        with (self.right / 'checkpoint.ram').open('r+b') as stream:
            for offset in offsets:
                stream.seek(offset)
                stream.write(b'\x01')
        result = report(self.left, self.right)
        ram = result['state']['ram']
        detail = ram['localization']
        self.assertEqual(result['result'], 'FAIL')
        self.assertEqual(ram['first_offsets'], offsets)
        self.assertEqual(ram['differing_bytes'], len(offsets))
        self.assertEqual(detail['status'], 'classified')
        self.assertEqual(detail['masked_bytes'], 0)
        self.assertEqual(detail['total_differing_bytes'], len(offsets))
        for name, wanted in expected.items():
            actual = detail['categories'][name]
            self.assertEqual(actual['differing_bytes'], len(wanted))
            self.assertEqual([i for start, end in actual['ranges'] for i in range(start, end)], wanted)

    def test_dropped_byte_mutant_changes_masked_bytes(self):
        self.known_layout()
        offsets = (0x46c, 0x18e28, 0x1d160, 0x20f64, 0x21160)
        with (self.right / 'checkpoint.ram').open('r+b') as stream:
            for offset in offsets:
                stream.seek(offset)
                stream.write(b'\x01')
        baseline = report(self.left, self.right)
        self.assertEqual(baseline['state']['ram']['localization']['masked_bytes'], 0)
        # Reproduce review mutant M1: omit one differing BIOS byte from classification.
        source = inspect.getsource(parity_report.ram_localization)
        needle = '    for offset in offsets:\n'
        self.assertEqual(source.count(needle), 1)
        mutated = source.replace(needle, needle + '        if offset == 0x46c:\n            continue\n')
        namespace = vars(parity_report).copy()
        exec(compile(mutated, '<M1 dropped BIOS byte>', 'exec'), namespace)
        with patch.object(parity_report, 'ram_localization', namespace['ram_localization']):
            result = report(self.left, self.right)
        detail = result['state']['ram']['localization']
        self.assertEqual(detail['total_differing_bytes'], 5)
        self.assertEqual(sum(g['differing_bytes'] for g in detail['categories'].values()), 4)
        self.assertEqual(detail['masked_bytes'], 1)
        self.assertEqual(result['result'], 'FAIL')
        self.assertEqual(result['state']['ram']['differing_bytes'], 5)
        self.assertEqual(result['state']['ram']['first_offsets'], list(offsets))

    def test_overlapping_regions_refuse_report(self):
        self.known_layout()
        regions = parity_report._ram_regions(dict(ss=0x1d16, ds=0x14a0, esp=0x3e04))
        regions['randseed'] = [0x46f, 0x473]
        with patch.object(parity_report, '_ram_regions', return_value=regions):
            with self.assertRaisesRegex(ValueError, 'overlapping RAM regions: bios_timer and randseed'):
                report(self.left, self.right)

    def test_empty_stack_regions_are_disjoint(self):
        for sp in (0, 0x4000):
            with self.subTest(sp=sp):
                self.known_layout()
                for run in (self.left, self.right):
                    path = run / 'checkpoint.json'
                    data = json.loads(path.read_text())
                    data['registers']['esp'] = sp
                    path.write_text(json.dumps(data))
                result = report(self.left, self.right)
                self.assertEqual(result['result'], 'PASS')
                detail = result['state']['ram']['localization']
                self.assertEqual(detail['status'], 'classified')
                self.assertEqual(detail['masked_bytes'], 0)

    def test_unknown_image_stays_unclassified(self):
        self.known_layout()
        for run in (self.left, self.right):
            (run / '.oracle-input-sha256').write_text('synthetic unknown image\n')
        with (self.right / 'checkpoint.ram').open('r+b') as stream:
            stream.seek(0x46c)
            stream.write(b'\x01\x01')
        result = report(self.left, self.right)
        detail = result['state']['ram']['localization']
        self.assertEqual(result['result'], 'FAIL')
        self.assertEqual(detail['status'], 'unavailable')
        self.assertEqual(detail['categories']['unclassified'],
                         dict(differing_bytes=2, ranges=[[0x46c, 0x46e]]))

    def test_disagreeing_or_unsupported_layout_stays_unclassified(self):
        self.known_layout()
        for change in (dict(ds=0x14a1), dict(ss=0x1d17), dict(esp=0x4001), dict(esp=0x3e02)):
            with self.subTest(change=change):
                self.known_layout()
                self.mutate_json(lambda d: d['registers'].update(change))
                with (self.right / 'checkpoint.ram').open('r+b') as stream:
                    stream.seek(0x20f64)
                    stream.write(b'\x01')
                result = report(self.left, self.right)
                detail = result['state']['ram']['localization']
                self.assertEqual(result['result'], 'FAIL')
                self.assertEqual(detail['status'], 'unavailable')
                self.assertEqual(detail['categories']['unclassified']['differing_bytes'], 1)


if __name__ == '__main__':
    unittest.main()
