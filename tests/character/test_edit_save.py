"""Synthetic contract and rollback tests; no copyrighted fixtures or game claims."""
from copy import deepcopy
import json
from pathlib import Path
import subprocess
import sys
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'tools/character'))
import edit_save as E


class EditCopyTests(unittest.TestCase):
    def setUp(self):
        self.temp = tempfile.TemporaryDirectory()
        self.addCleanup(self.temp.cleanup)
        self.root = Path(self.temp.name)
        self.source = self.root / 'source'
        self.source.mkdir()
        self.before = bytearray(285)
        self.before[0x10], self.before[0x98] = 17, 1
        self.middle = bytearray(self.before)
        self.middle[0x10] = 16
        self.after = bytearray(self.middle)
        self.after[0x98] = 2
        self.name = 'CHRDATA1.SAV'
        (self.source / self.name).write_bytes(self.before)
        (self.source / 'CHRDATA1.ITM').write_bytes(bytes(63))
        self.originals = {f.name: f.read_bytes() for f in self.source.iterdir()}
        self.profiles = deepcopy(E.PROFILES)
        for name, a, b in [('reference_strength_17_to_16', self.before, self.middle),
                           ('reference_fighter_level_1_to_2', self.middle, self.after)]:
            self.profiles[name]['before_sha256'] = E.digest(a)
            self.profiles[name]['after_sha256'] = E.digest(b)
        self.changes = [dict(file=self.name, field=p['field'], expected_before=p['before'],
                             value=p['after'], validation_profile=name)
                        for name, p in self.profiles.items()]
        self.request = dict(schema_version=1, operation='edit_copy', source=str(self.source),
                            destination=str(self.root / 'copy'),
                            expected_source_hashes={n: E.digest(b) for n, b in self.originals.items()},
                            changes=self.changes)

    def run_edit(self, request=None):
        with patch.object(E, 'PROFILES', self.profiles):
            return E.process(self.request if request is None else request)

    def assert_source_unchanged(self):
        self.assertEqual({f.name: f.read_bytes() for f in self.source.iterdir()}, self.originals)

    def refuse(self, code, request=None):
        with self.assertRaises(E.Refused) as error:
            self.run_edit(request)
        self.assertEqual(error.exception.code, code)
        self.assertFalse((self.root / 'copy').exists())
        self.assert_source_unchanged()

    def test_inspect_eligibility_and_no_writes(self):
        with patch.object(E, 'PROFILES', self.profiles):
            result = E.process(dict(schema_version=1, operation='inspect', source=str(self.source)))
        self.assertEqual(result['writes'], 0)
        self.assertEqual(result['expected_source_hashes'], self.request['expected_source_hashes'])
        profiles = result['characters'][0]['profiles']
        self.assertTrue(profiles[0]['eligible'])
        self.assertFalse(profiles[1]['eligible'])
        self.assertEqual(profiles[0]['eligible_after_application'], ['reference_fighter_level_1_to_2'])
        self.assert_source_unchanged()
        self.assertEqual(set(p.name for p in self.root.iterdir()), {'source'})

    def test_ordered_edits_receipt_and_untouched_sidecar(self):
        result = self.run_edit()
        copy = Path(result['output_copy'])
        self.assertEqual((copy / self.name).read_bytes(), self.after)
        self.assertEqual((copy / 'CHRDATA1.ITM').read_bytes(), bytes(63))
        receipt = json.loads(Path(result['receipt']).read_text())
        char = next(r for r in receipt['files'] if r['file'].endswith('.SAV'))
        self.assertEqual(char['changed_offsets'], [0x10, 0x98])
        self.assertEqual(char['before_after_bytes'], [{'offset': 0x10, 'before': 17, 'after': 16}, {'offset': 0x98, 'before': 1, 'after': 2}])
        self.assertEqual(result['byte_identical_files'], 1)
        self.assertEqual(result['applied_changes'][0]['after_sha256'], result['applied_changes'][1]['before_sha256'])
        self.assertEqual(result['game_validation'], 'UNVERIFIED')
        self.assertIsNone(receipt['game_build_loaded'])
        self.assertEqual(receipt['rejected_changes'], [])
        self.assert_source_unchanged()

    def test_noop_interface_preserved(self):
        from disk_save import export_directory
        result = export_directory(self.source, self.root / 'noop')
        for name, data in self.originals.items():
            self.assertEqual((Path(result['output_copy']) / name).read_bytes(), data)
        self.assertEqual(result['byte_identical_files'], 2)

    def test_real_allowlist_rejects_synthetic_record_with_correct_values(self):
        with self.assertRaises(E.Refused) as error:
            E.process(self.request)
        self.assertEqual(error.exception.code, 'profile_mismatch')
        self.assertFalse((self.root / 'copy').exists())

    def test_reversed_order_rejected_before_writing(self):
        self.request['changes'] = list(reversed(self.changes))
        self.refuse('profile_mismatch')

    def test_bad_second_edit_leaves_no_partial_copy(self):
        self.request['changes'][1]['value'] = 3
        self.refuse('unsupported_value')

    def test_duplicate_field_rejected(self):
        self.request['changes'] = [self.changes[0], self.changes[0]]
        self.refuse('duplicate_change')

    def test_unknown_field_profile_keys_and_boolean_rejected(self):
        for update, code in [({'field': 'hit_points'}, 'unsupported_field'),
                             ({'validation_profile': 'anything'}, 'unsupported_profile'),
                             ({'value': True}, 'invalid_request'),
                             ({'offset': 16}, 'invalid_request')]:
            with self.subTest(update=update):
                request = deepcopy(self.request)
                request['changes'][0].update(update)
                self.refuse(code, request)
        for update in ({'schema_version': True}, {'arbitrary': 1}):
            request = deepcopy(self.request)
            request.update(update)
            self.refuse('invalid_request', request)

    def test_complete_source_hash_set_required(self):
        for mode in ('missing', 'extra', 'wrong'):
            with self.subTest(mode=mode):
                request = deepcopy(self.request)
                hashes = request['expected_source_hashes']
                if mode == 'missing': hashes.pop('CHRDATA1.ITM')
                if mode == 'extra': hashes['CHRDATX.SAV'] = 'a' * 64
                if mode == 'wrong': hashes[self.name] = 'b' * 64
                self.refuse('source_mismatch', request)

    def test_existing_and_inside_source_destinations_are_preserved(self):
        for target in (self.source, self.source / 'nested', self.root / 'occupied'):
            with self.subTest(target=target):
                if target.name == 'occupied':
                    target.mkdir()
                    (target / 'sentinel').write_text('keep')
                request = deepcopy(self.request)
                request['destination'] = str(target)
                self.refuse('invalid_destination', request)
                if target.name == 'occupied': self.assertEqual((target / 'sentinel').read_text(), 'keep')

    def test_broken_destination_symlink_and_source_symlink_refused(self):
        target = self.root / 'copy'
        target.symlink_to(self.root / 'absent')
        with self.assertRaises(E.Refused) as error: self.run_edit()
        self.assertEqual(error.exception.code, 'invalid_destination')
        self.assertTrue(target.is_symlink())
        target.unlink()
        real = self.root / 'actual.sav'
        real.write_bytes(self.before)
        (self.source / self.name).unlink()
        (self.source / self.name).symlink_to(real)
        with self.assertRaises(E.Refused) as error: self.run_edit()
        self.assertEqual(error.exception.code, 'invalid_source')
        self.assertEqual(real.read_bytes(), self.before)

    def test_source_growth_during_write_rolls_back_only_output(self):
        original_snapshot = E.source_snapshot
        calls = 0
        def changed(source):
            nonlocal calls
            calls += 1
            if calls == 2: (source / 'CHRDATNEW.SAV').write_bytes(bytes(285))
            return original_snapshot(source)
        with patch.object(E, 'source_snapshot', side_effect=changed):
            with self.assertRaises(E.Refused) as error: self.run_edit()
        self.assertEqual(error.exception.code, 'source_changed')
        self.assertFalse((self.root / 'copy').exists())
        self.assertTrue((self.source / 'CHRDATNEW.SAV').exists())
        for name, data in self.originals.items(): self.assertEqual((self.source / name).read_bytes(), data)

    def test_write_failure_rolls_back_created_files(self):
        original_open = Path.open
        def fail_receipt(path, *args, **kwargs):
            if path.name == 'write-receipts.json': raise OSError('injected receipt failure')
            return original_open(path, *args, **kwargs)
        with patch.object(Path, 'open', fail_receipt):
            with self.assertRaisesRegex(OSError, 'injected'): self.run_edit()
        self.assertFalse((self.root / 'copy').exists())
        self.assert_source_unchanged()

    def test_cli_inspect_json_and_structured_errors(self):
        cli = [sys.executable, str(ROOT / 'tools/character/edit_save.py')]
        r = subprocess.run(cli, input=json.dumps(dict(schema_version=1, operation='inspect', source=str(self.source))), text=True, capture_output=True)
        self.assertEqual(r.returncode, 0)
        self.assertEqual(r.stderr, '')
        self.assertEqual(json.loads(r.stdout)['writes'], 0)
        self.assertFalse(any(p['eligible'] for p in json.loads(r.stdout)['characters'][0]['profiles']))
        for payload in (json.dumps(self.request), '{"schema_version":1,"schema_version":1}', 'not json'):
            with self.subTest(payload=payload):
                r = subprocess.run(cli, input=payload, text=True, capture_output=True)
                self.assertEqual(r.returncode, 1)
                self.assertEqual(r.stdout, '')
                self.assertEqual(json.loads(r.stderr)['applied_changes'], [])
                self.assertIn('code', json.loads(r.stderr)['error'])
        self.assertFalse((self.root / 'copy').exists())
        self.assert_source_unchanged()


if __name__ == '__main__':
    unittest.main()
