"""Synthetic, media-free U19 backend outcome classification tests."""

from dataclasses import replace
import unittest

from tools.replay.u19_row14_backend_outcome_source import (
    Attempt, Backend, CompletionWitness, Outcome, classify_backend_outcome,
)


def attempt(backend: Backend, site: int, **changes: object) -> Attempt:
    base = Attempt("run_u19", 7, "backend_u19", backend, site,
                   True, True, False,
                   1 if (backend, site) in ((Backend.SDL2_SURFACE, 771),
                                            (Backend.SDL1_SURFACE, 773)) else None)
    return replace(base, **changes)


def witness(value: Attempt, **changes: object) -> CompletionWitness:
    base = CompletionWitness(value.run_token, value.frame_id,
                             value.backend_event_id, value.backend,
                             value.site, "independent_backend_completion", True)
    return replace(base, **changes)


class BackendOutcomeSourceTests(unittest.TestCase):
    def test_reached_calls_and_success_codes_remain_attempts(self) -> None:
        for backend, sites in (
                (Backend.SDL2_SURFACE, (687, 716, 737, 744, 771)),
                (Backend.SDL1_SURFACE, (689, 718, 739, 746, 773)),
                (Backend.OPENGL, (1026, 1132))):
            for site in sites:
                with self.subTest(backend=backend, site=site):
                    value = attempt(backend, site)
                    self.assertEqual(classify_backend_outcome(value),
                                     Outcome.ATTEMPT_UNVERIFIED)
                    if not ((backend, site) in ((Backend.SDL1_SURFACE, 773),
                                                (Backend.OPENGL, 1026),
                                                (Backend.OPENGL, 1132))):
                        self.assertEqual(
                            classify_backend_outcome(replace(value, api_result=0)),
                            Outcome.ATTEMPT_UNVERIFIED)

    def test_no_output_gates_reject_verification(self) -> None:
        value = attempt(Backend.SDL2_SURFACE, 771)
        for changed in (
                {"call_reached": False, "backend_event_id": None},
                {"call_reached": False, "rect_count": 0,
                 "backend_event_id": None},
                {"render_aborted": True, "backend_event_id": None},
                {"selected_new_frame": False, "backend_event_id": None},
        ):
            with self.subTest(changed=changed):
                gated = replace(value, **changed)
                self.assertEqual(classify_backend_outcome(gated), Outcome.NO_OUTPUT)
                with self.assertRaisesRegex(ValueError, "no output"):
                    classify_backend_outcome(gated, witness(gated))
        gl = attempt(Backend.OPENGL, 1132, call_reached=False,
                     backend_event_id=None)
        self.assertEqual(classify_backend_outcome(gl), Outcome.NO_OUTPUT)
        with self.assertRaisesRegex(ValueError, "minted"):
            classify_backend_outcome(replace(gl, backend_event_id="forged"))

    def test_supplied_independent_witness_is_structural_only(self) -> None:
        for backend, site in ((Backend.SDL2_SURFACE, 687),
                              (Backend.SDL1_SURFACE, 773),
                              (Backend.OPENGL, 1132)):
            with self.subTest(backend=backend):
                value = attempt(backend, site)
                self.assertEqual(classify_backend_outcome(value, witness(value)),
                                 Outcome.COMPLETION_VERIFIED)
                self.assertEqual(classify_backend_outcome(
                    value, witness(value, verified=False)),
                    Outcome.ATTEMPT_UNVERIFIED)

    def test_wrong_frame_event_backend_site_and_source_reject(self) -> None:
        value = attempt(Backend.SDL2_SURFACE, 687)
        for changed in ({"run_token": "other"}, {"frame_id": 8},
                        {"backend_event_id": "other"},
                        {"frame_id": 7.0},
                        {"backend": Backend.SDL1_SURFACE}, {"site": 716},
                        {"backend": "sdl2_surface"},
                        {"source_domain": "call_return"},
                        {"verified": 1}):
            with self.subTest(changed=changed):
                with self.assertRaises(ValueError):
                    classify_backend_outcome(value, witness(value, **changed))

    def test_void_results_failed_calls_and_impossible_rects_reject(self) -> None:
        for backend, site in ((Backend.SDL1_SURFACE, 773),
                              (Backend.OPENGL, 1026),
                              (Backend.OPENGL, 1132)):
            with self.subTest(backend=backend, site=site):
                with self.assertRaisesRegex(ValueError, "void"):
                    classify_backend_outcome(
                        attempt(backend, site, api_result=0))
        failed = attempt(Backend.SDL2_SURFACE, 687, api_result=-1)
        self.assertEqual(classify_backend_outcome(failed),
                         Outcome.ATTEMPT_UNVERIFIED)
        with self.assertRaisesRegex(ValueError, "contradicts"):
            classify_backend_outcome(failed, witness(failed))
        with self.assertRaisesRegex(ValueError, "zero rectangles"):
            classify_backend_outcome(
                attempt(Backend.SDL2_SURFACE, 771, rect_count=0))
        with self.assertRaisesRegex(ValueError, "call site"):
            classify_backend_outcome(attempt(Backend.SDL2_SURFACE, 1132))
        with self.assertRaisesRegex(ValueError, "lacks backend event ID"):
            classify_backend_outcome(
                attempt(Backend.SDL2_SURFACE, 687, backend_event_id=None))
        with self.assertRaises(ValueError):
            classify_backend_outcome(attempt(Backend.SDL2_SURFACE, 687,
                                             frame_id=True))


if __name__ == "__main__":
    unittest.main()
