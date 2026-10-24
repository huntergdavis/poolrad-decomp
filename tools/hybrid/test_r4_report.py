#!/usr/bin/env python3
"""Synthetic counterexamples for service attribution."""
from collections import Counter
import unittest
from r4_report import attribute


class AttributionTest(unittest.TestCase):
    def check(self, ids, counters=None, phases=None):
        seq = [dict(id=name, instruction=i) for i, name in enumerate(ids)]
        return attribute(seq, counters if counters is not None else dict(Counter(ids), ZERO=0),
                         ['A', 'ZERO'], phases or {'shop': 'SHOP', 'temple': 'TEMPLE'},
                         {'shop': 'BUY', 'temple': 'HEAL'})

    def test_duplicate_candidates_cannot_inflate_arrivals(self):
        with self.assertRaisesRegex(ValueError, 'duplicate candidate'):
            attribute([], {'A': 0}, ['A', 'A'], {'shop': 'S'}, {})

    def test_shared_body_is_split_by_guest_markers(self):
        result = self.check(['A', 'SHOP', 'BUY', 'A', 'TEMPLE', 'HEAL', 'A', 'A'])
        self.assertEqual(result['candidates_by_path'][0],
                         dict(id='A', whole_run=4, before_services=1, shop=1, temple=2))
        self.assertEqual(result['candidates_by_path'][1]['whole_run'], 0)

    def test_incomplete_trace_rejected(self):
        with self.assertRaisesRegex(ValueError, 'sequence/count'):
            self.check(['SHOP', 'BUY', 'A', 'TEMPLE', 'HEAL'],
                       dict(SHOP=1, BUY=1, A=2, TEMPLE=1, HEAL=1, ZERO=0))

    def test_missing_zero_counter_rejected(self):
        ids = ['SHOP', 'BUY', 'A', 'TEMPLE', 'HEAL']
        with self.assertRaisesRegex(ValueError, 'missing'):
            self.check(ids, dict(Counter(ids)))

    def test_repeated_entry_rejected(self):
        with self.assertRaisesRegex(ValueError, 'exactly one'):
            self.check(['SHOP', 'BUY', 'A', 'SHOP', 'TEMPLE', 'HEAL'])

    def test_reversed_service_order_rejected(self):
        with self.assertRaisesRegex(ValueError, 'order'):
            self.check(['TEMPLE', 'HEAL', 'A', 'SHOP', 'BUY'])

    def test_action_before_service_does_not_count(self):
        with self.assertRaisesRegex(ValueError, 'action marker'):
            self.check(['BUY', 'SHOP', 'A', 'TEMPLE', 'HEAL'])

    def test_instruction_order_rejected(self):
        with self.assertRaisesRegex(ValueError, 'monotonic'):
            attribute([dict(id='S', instruction=2), dict(id='A', instruction=1)],
                      {'S': 1, 'A': 1}, ['A'], {'shop': 'S'}, {})

    def test_action_both_inside_and_outside_phase_rejected(self):
        with self.assertRaisesRegex(ValueError, 'outside service phase'):
            self.check(['BUY', 'SHOP', 'BUY', 'A', 'TEMPLE', 'HEAL'])

    def test_repeated_buy_inside_shop_phase_is_valid(self):
        result = self.check(['SHOP', 'BUY', 'A', 'BUY', 'TEMPLE', 'HEAL'])
        self.assertEqual(result['marker_arrivals']['BUY'], 2)


if __name__ == '__main__':
    unittest.main()
