"""Falsify stale NOT-CLOSABLE reasons; synthetic dispositions are not proofs."""
import copy
import json
import unittest

import res16ec_closure_status as S


class ClosureStatusTests(unittest.TestCase):
    def setUp(self):
        self.state = json.loads(S.STATE.read_text())
        self.document = S.DOCUMENT.read_text()

    def test_current_verdict_matches_recorded_dispositions(self):
        expected = sorted(row['id'] for row in self.state['obligations'] if row['state'] == 'open')
        self.assertEqual(S.check(self.state, self.document), expected)

    def test_each_discharge_rejects_its_stale_open_reason(self):
        for ident in sorted(S.IDS):
            with self.subTest(reason=ident):
                state = copy.deepcopy(self.state)
                row = next(row for row in state['obligations'] if row['id'] == ident)
                row.update(state='discharged', discharge=dict(
                    reviewer='synthetic reviewer fixture', review_ref='synthetic fixture only',
                    proof_ref='not an actual proof', scope='fixture scope',
                    reviewed_head='a' * 40, proof_sha256='b' * 64))
                # Ensure the stale claim is present even after future real discharges.
                stale = self.document
                if not any(line.startswith(f'| {ident} |') for line in stale.splitlines()):
                    stale += f'\n| {ident} | synthetic stale reason |\n'
                with self.assertRaisesRegex(ValueError, f'{ident}: stale open reason'):
                    S.check(state, stale)
                corrected = '\n'.join(line for line in stale.splitlines()
                                      if not line.startswith(f'| {ident} |'))
                self.assertNotIn(ident, S.check(state, corrected))

    def test_discharge_without_review_reference_is_rejected(self):
        self.state['obligations'][0].update(state='discharged', discharge=None)
        with self.assertRaisesRegex(ValueError, 'discharge missing reviewer'):
            S.check(self.state, self.document)

    def test_open_reason_cannot_be_silently_removed(self):
        self.state['obligations'][0].update(state='open', discharge=None)
        document = '\n'.join(line for line in self.document.splitlines()
                             if not line.startswith('| NC1 |'))
        with self.assertRaisesRegex(ValueError, 'NC1: open reason missing'):
            S.check(self.state, document)

    def test_unknown_reason_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'unknown documented open reason'):
            S.check(self.state, self.document + '\n| NC6 | invented reason |\n')


if __name__ == '__main__':
    unittest.main()
