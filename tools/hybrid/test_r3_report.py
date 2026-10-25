"""Synthetic refusal controls for native R3 evidence; no game media."""
import json
from pathlib import Path
import tempfile
import unittest

from r3_report import dispatch, pair, snapshot


class R3ReportTest(unittest.TestCase):
    def test_matching_pixels_do_not_hide_different_inputs_actions_or_rng(self):
        good = dict(mode='original', hashes={'binary': 'A'}, actions=[('gate', 'x')],
                    final_pixels='pixels', rng=['12345678'])
        self.assertTrue(all(pair(good, dict(good)).values()))
        for field, wrong in [('hashes', {'binary': 'B'}), ('actions', [('gate', 'y')]),
                             ('rng', ['87654321']), ('mode', 'dispatch')]:
            with self.assertRaises(ValueError):
                pair(good, dict(good, **{field: wrong}))

    def test_dispatch_differences_and_duplicate_summaries_are_refused(self):
        fields = dict(arrivals=2, returned_by_c=1, c_equal=1, c_differ=0,
                      midbody_handback_differ=0, argument_rerun_differ=0)
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            def write(values):
                line = '# OVL_0038_010D ' + ' '.join(f'{k}={v}' for k, v in values.items()) + '\n'
                (root / 'observer.dispatch').write_text(line)
                return line
            write(fields)
            self.assertEqual(dispatch(root, False)['OVL_0038_010D']['c_equal'], 1)
            for field in ('c_differ', 'midbody_handback_differ', 'argument_rerun_differ'):
                write(dict(fields, **{field: 1}))
                with self.assertRaises(ValueError):
                    dispatch(root, False)
            line = write(fields)
            (root / 'observer.dispatch').write_text(line + line)
            with self.assertRaisesRegex(ValueError, 'duplicate'):
                dispatch(root, False)

    def test_partial_observer_trace_cannot_establish_equality(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'r1-summary.json').write_text(json.dumps(dict(success=True, error=None, shutdown='SIGTERM')))
            (root / 'r1-seed.txt').write_text('12345678\n')
            (root / 'observer.tsv').write_text('A\t2\n')
            (root / 'observer.seq').write_text('1\tA\t0000:0000\t10\n')
            with self.assertRaisesRegex(ValueError, 'sequence/count'):
                snapshot(root)


if __name__ == '__main__':
    unittest.main()
