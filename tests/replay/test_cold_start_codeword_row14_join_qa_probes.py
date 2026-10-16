"""Disjoint synthetic U19 join QA probes; no guest or host capture."""

from dataclasses import replace
import unittest

from tests.replay.test_cold_start_codeword_row14_same_run_join_contract import fixture
from tools.replay.cold_start_codeword_row14_phase_clock_qa import (
    PhaseClockSample, check_latch_and_store,
)
from tools.replay.cold_start_codeword_row14_same_run_join_contract import (
    join_source_contract,
)


class JoinQaProbes(unittest.TestCase):
    def test_a_wrong_audited_route_token_rejected(self):
        route, host, plane, expected = fixture()
        self.assertEqual(join_source_contract(route, host, plane, expected).run_token,
                         "run_42")
        with self.assertRaisesRegex(ValueError, "same-run selected-store key"):
            join_source_contract(replace(route, run_token="other_run"),
                                 host, plane, expected)

    def test_b_cross_phase_substitution_rejected_with_same_phase_controls(self):
        key = ("run_42", "store_7", "arm_7", 7)
        host_latch = PhaseClockSample(*key, "latch_enter", 100)
        passive_latch = PhaseClockSample(*key, "latch_enter", 100)
        host_store = PhaseClockSample(*key, "selected_store", 200)
        passive_store = PhaseClockSample(*key, "selected_store", 200)
        self.assertEqual(check_latch_and_store(host_latch, passive_latch,
                                               host_store, passive_store),
                         "SYNTHETIC_PHASE_CONTRACT_NO_LIVE_HOOK")
        self.assertEqual(check_latch_and_store(
            replace(host_latch, host_ns=140),
            replace(passive_latch, host_ns=140),
            replace(host_store, host_ns=110),
            replace(passive_store, host_ns=110)),
            "SYNTHETIC_PHASE_CONTRACT_NO_LIVE_HOOK")
        with self.assertRaisesRegex(ValueError, "cross-phase"):
            check_latch_and_store(host_latch, passive_latch, host_store,
                                  replace(passive_latch, host_ns=200))
        with self.assertRaisesRegex(ValueError, "same-phase common clock"):
            check_latch_and_store(host_latch, passive_latch, host_store,
                                  replace(passive_store, host_ns=100))


if __name__ == "__main__":
    unittest.main()
