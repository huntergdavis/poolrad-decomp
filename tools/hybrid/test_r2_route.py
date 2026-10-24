#!/usr/bin/env python3
"""Synthetic controls for R2 menu transitions and the route-owner schema."""
import json
from pathlib import Path
import unittest

from r1_driver import Route


class R2RouteTests(unittest.TestCase):
    def setUp(self):
        self.data = json.loads(Path(__file__).with_name('routes').joinpath('r2.json').read_text())
        self.route = Route(self.data)

    def test_state_records_keep_the_owner_schema(self):
        fields = {'name', 'gate', 'key', 'expect_next', 'timeout_s', 'max_visits'}
        self.assertTrue(all(set(state) == fields for state in self.data['states']))

    def test_page_up_requires_the_previous_page_control(self):
        self.route.allowed = ['spell_prior']
        self.assertIsNone(self.route.choose({'prompt': 'CHOOSE NEXT'}))
        state = self.route.choose({'prompt': 'CHOOSE PREV'})
        self.assertEqual(self.route.advance(state), 'Prior')

    def test_reorder_key_requires_place_mode(self):
        self.route.allowed = ['order_before']
        self.assertIsNone(self.route.choose({'prompt': 'SELECT'}))
        state = self.route.choose({'prompt': 'PLACE'})
        self.assertEqual(self.route.advance(state), 'Home')

    def test_display_exit_does_not_accept_a_larger_menu(self):
        self.route.allowed = ['display_exit']
        self.assertIsNone(self.route.choose({'prompt': 'MENU EXIT'}))
        state = self.route.choose({'prompt': 'EXT.'})
        self.assertEqual(self.route.advance(state), 'e')


if __name__ == '__main__':
    unittest.main()
