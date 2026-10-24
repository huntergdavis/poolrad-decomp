#!/usr/bin/env python3
"""Synthetic safety checks: no original text, media or captured observations."""
import json
from pathlib import Path
import unittest
import tempfile
from r1_report import report
from r1_driver import Route, matches, single_exit_prompt


class RouteTest(unittest.TestCase):
    def test_single_exit_fallback_rejects_other_controls(self):
        for text in ('EXIT', 'EXT', 'EXT.'):
            self.assertEqual(single_exit_prompt(text), text)
        for text in ('NEXT', 'TEXT', 'EXTRA', 'EXIT NEXT', '', 'A'):
            self.assertEqual(single_exit_prompt(text), '')

    def test_exact_gate_rejects_an_earlier_menu_with_same_word(self):
        self.route.allowed=['view_items']
        self.route.exact_prompt_states={'view_items'}
        self.assertIsNone(self.route.choose({'prompt':'ITEMS TRADE'}))
        self.assertIsNotNone(self.route.choose({'prompt':'ITEMS'}))

    def test_unknown_exact_gate_is_rejected(self):
        data=dict(self.route.data, exact_prompt_states=['missing'])
        with self.assertRaisesRegex(ValueError, 'unknown exact'):
            Route(data)

    def setUp(self):
        self.route=Route(json.loads(Path(__file__).with_name('routes').joinpath('r1.json').read_text()))

    def test_unknown_screen_never_selects_key(self):
        self.assertIsNone(self.route.choose({'prompt':'UNRECOGNIZED'}))

    def test_slot_gate_rejects_unrelated_stable_text(self):
        self.route.allowed=['slots']
        self.assertIsNone(self.route.choose({'prompt':'UNRECOGNIZED'}))
        self.assertIsNotNone(self.route.choose({'prompt':'LOAD GAME'}))
        self.assertIsNotNone(self.route.choose({'prompt':'LODB WHICH X'}))

    def test_gate_cannot_jump_to_later_menu(self):
        self.assertIsNone(self.route.choose({'prompt':'ITEMS TRADE'}))

    def test_visit_limit_blocks_repeated_input(self):
        record=self.route.records['code']
        self.assertEqual(self.route.advance(record),'Return')
        with self.assertRaises(RuntimeError):
            self.route.advance(record)

    def test_animation_does_not_trigger_quick(self):
        self.route.allowed=['combat_quick']
        self.assertIsNone(self.route.choose({'prompt':'MOVE LEFT'}))
        self.assertIsNotNone(self.route.choose({'prompt':'MOVE GUARD QUICK'}))

    def test_pool_share_exit_order(self):
        self.route.allowed=['treasure_pool']
        for expected in ['p','s','e']:
            record=self.route.choose({'prompt':'POOL SHARE TAKE'})
            self.assertEqual(self.route.advance(record),expected)
        self.assertNotIn('treasure_pool',self.route.allowed)

    def test_digit_gate_uses_digit_observation(self):
        gate=self.route.records['at15_4']
        self.assertFalse(matches(gate,{'prompt':'15,4'}))
        self.assertTrue(matches(gate,{'digits':'15,4 W'}))

    def test_exit_confirmation_precedes_remaining_question(self):
        self.route.allowed=['leave_confirm','leave_remaining','area_view']
        r=self.route.choose({'prompt':'SURE YES NO'})
        self.assertEqual(r['name'],'leave_confirm')
        self.assertEqual(self.route.advance(r),'y')
        r=self.route.choose({'prompt':'YES NO'})
        self.assertEqual(self.route.advance(r),'n')

    def test_report_distinguishes_zero_from_missing(self):
        with tempfile.TemporaryDirectory() as directory:
            run=Path(directory)
            (run/'r1-summary.json').write_text(json.dumps(dict(success=True,error=None,
                shutdown='SIGTERM',gates=[])))
            (run/'r1-mode.txt').write_text('observer')
            (run/'observer.tsv').write_text('# id\tarrivals\nA\t3\nB\t0\n')
            result=report(run,{'candidates':['A','B','C']})
            self.assertEqual(result['arrived_bodies'],1)
            self.assertEqual(result['missing_candidates'],1)
            self.assertEqual([r['status'] for r in result['candidates']],
                             ['arrived','not observed','missing'])

    def test_route_states_have_owner_schema(self):
        for state in self.route.data['states']:
            self.assertEqual(set(state), {'name','gate','key','expect_next','timeout_s','max_visits'})
            self.assertIsInstance(state['gate'],list)

    def test_seek_alternates_and_accepts_ambush(self):
        self.route.allowed=['seek_down']
        self.assertEqual(self.route.advance(self.route.choose({'prompt':'AREA'})),'Down')
        self.assertEqual(self.route.advance(self.route.choose({'prompt':'AREA'})),'Up')
        self.assertEqual(self.route.advance(self.route.choose({'prompt':'QUICK GUARD'})),'q')

    def test_unknown_recognizable_menu_is_not_idle(self):
        observation={'prompt':'AREA'}
        self.assertIsNone(self.route.choose(observation))
        self.assertFalse(self.route.idle(observation))

    def test_battle_animation_waits_for_choice(self):
        self.route.allowed=['battle_end']
        self.assertIsNone(self.route.choose({'prompt':'BATTLE'}))
        self.assertTrue(self.route.idle({'prompt':'BATTLE'}))
        self.assertEqual(self.route.advance(self.route.choose({'prompt':'BATTLE YES NO'})),'n')


if __name__=='__main__':
    unittest.main()
