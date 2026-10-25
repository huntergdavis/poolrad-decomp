#!/usr/bin/env python3
"""Synthetic negative controls for R2 new-route arrival attribution."""
import unittest

from r2_report import attribute


class AttributionTests(unittest.TestCase):
    def setUp(self):
        self.sequence = [dict(id=name, instruction=i * 10)
                         for i, name in enumerate(['shared', 'camp', 'rest', 'shared', 'new'])]
        self.counters = dict(shared=2, camp=1, rest=1, new=1, absent=0)
        self.markers = dict(camp='camp', rest='rest')

    def measure(self, sequence=None, counters=None):
        return attribute(self.sequence if sequence is None else sequence,
                         self.counters if counters is None else counters,
                         ['shared', 'new', 'absent'], self.markers, {'shared', 'absent'})

    def test_startup_arrival_is_excluded(self):
        result = self.measure()
        self.assertEqual(result['arrived_after_camp'], 2)
        self.assertEqual(result['shared_arrived_after_camp'], 1)
        row = result['candidates_by_path'][0]
        self.assertEqual((row['whole_run'], row['before_camp'], row['after_camp']), (2, 1, 1))

    def test_stale_counter_snapshot_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'sequence/count mismatch'):
            self.measure(counters=dict(self.counters, shared=1))

    def test_truncated_sequence_is_rejected(self):
        with self.assertRaisesRegex(ValueError, 'sequence/count mismatch'):
            self.measure(sequence=self.sequence[:-1])

    def test_missing_marker_is_rejected(self):
        sequence = [r for r in self.sequence if r['id'] != 'camp']
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            self.measure(sequence=sequence, counters=dict(self.counters, camp=0))

    def test_repeated_camp_is_not_silently_combined(self):
        sequence = self.sequence + [dict(id='camp', instruction=60)]
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            self.measure(sequence=sequence, counters=dict(self.counters, camp=2))

    def test_missing_zero_candidate_is_rejected(self):
        counters = dict(self.counters)
        del counters['absent']
        with self.assertRaisesRegex(ValueError, 'missing'):
            self.measure(counters=counters)

    def test_out_of_order_instructions_are_rejected(self):
        with self.assertRaisesRegex(ValueError, 'monotonic'):
            self.measure(sequence=list(reversed(self.sequence)))


if __name__ == '__main__':
    unittest.main()
