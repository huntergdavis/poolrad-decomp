#!/usr/bin/env python3
"""Focused synthetic checks for strict-candidate ranking and detection.

No media, git or decoder needed: the queue join reuses function_queue's
overlay rows, and the prior-translation evidence is injected as structured
records the real scanners produce. The checks cover the two behaviours the
queue exists for: an untranslated, game-owned, ready-callee original ranks
above a prior-translated one, and inline/branch evidence is reported.
"""

import copy
import sys
from pathlib import Path
import unittest

sys.path.insert(0, str(Path(__file__).resolve().parent))
import strict_candidate_queue as scq


def fixture() -> tuple[dict, list, list]:
    inventory = dict(inputs=scq.DIGESTS.copy(), units=[dict(
        stub_para=0x19, memory_segment=0x1C, ovr_offset=100, code_size=0x200,
        entries=[0x10], functions=[dict(rel=0x10), dict(rel=0x20), dict(rel=0x30)])])
    ledger, classifications = [], []
    for rel, grade, entry, near, classification in (
            (0x10, "A", 1, 0, "game_verified"),
            (0x20, "B", 0, 1, "game_evidenced"),
            (0x30, "U", 0, 0, "game_or_other_library_unresolved")):
        common = dict(id=f"0x19:{rel:#06x}", unit="0x19", offset=hex(rel),
                      ovr_offset=str(100 + rel), grade=grade)
        ledger.append(dict(common, entry=str(entry), entry_alias="0", near_called=str(near),
                           far_called="0", procptr="0", bound="40", decoded="40", insns="12",
                           cls="leaf"))
        classifications.append(dict(common, far_in="0", classification=classification,
                                    confidence="provisional"))
    return inventory, ledger, classifications


def evidence():
    mentions = {
        "OVL_0019_0020": {"source": [], "tooling": [], "docs": []},
        "OVL_0019_0030": {"source": [dict(file="src/recovered/overlay_u19_alpha_0010.c",
                                          line=9, kind="offset", text="call +0x0030")],
                          "tooling": [], "docs": []}}
    main_files = {"OVL_0019_0010": ["src/recovered/overlay_u19_alpha_0010.c"]}
    branches = {"OVL_0019_0020": [dict(sha="0" * 40, refs=["origin/codex/u19-0020"],
                                       path="src/recovered/overlay_u19_beta_0020.c")]}
    return mentions, main_files, branches


def callgraph():
    return dict(edges=[
        dict(kind="far_fixup_confirmed", unit=0x19, site=0x25, src_func=0x20, seg=0x1C, off=0x20,
             resolution="overlay", target_unit=0x19, target_entry=0, target_rel=0x10,
             target_func=0x10, exact_prologue=True),
        dict(kind="near_candidate", unit=0x19, site=0x35, src_func=0x30, target=0x20,
             target_func=0x20, exact_prologue=True),
        dict(kind="near_candidate", unit=0x19, site=0x1A, src_func=0x20, target=0x10,
             target_func=0x10, exact_prologue=True)])


def queue(data=None, graph=None, prior=None) -> dict:
    inventory, ledger, classifications = data or fixture()
    state = scq.load_queue(inventory, ledger, classifications,
                           [dict(id="OVL_0019_0010", evidence_doc="docs/re/synthetic.md")], None)
    mentions, main_files, branches = prior or evidence()
    overlay = bytearray(200)
    overlay[100 + 0x35 - 1] = 0x0E
    return scq.build_queue(state, graph or callgraph(), inventory["units"], ledger,
                           classifications, mentions, main_files, branches, {},
                           dict(ovr=bytes(overlay)))


class StrictCandidateQueueTest(unittest.TestCase):
    def test_ranking_and_prior_detection(self):
        result = queue()
        rows = {row["id"]: row for row in result["rows"]}
        self.assertEqual([row["id"] for row in result["rows"]],
                         ["OVL_0019_0020", "OVL_0019_0030"])
        self.assertEqual(result["summary"]["strict_overlay_excluded"], 1)
        self.assertNotIn("OVL_0019_0010", rows)
        untranslated, prior = rows["OVL_0019_0020"], rows["OVL_0019_0030"]
        self.assertEqual(untranslated["prior_translation"]["status"], "unmerged_branch")
        self.assertEqual(untranslated["callee_readiness"]["counts"], {"strict": 1})
        self.assertTrue(untranslated["callee_readiness"]["composable"])
        self.assertEqual(untranslated["score_terms"]["composable"], 6)
        self.assertEqual(untranslated["callers_aligned_push_cs"], 1)
        self.assertGreater(untranslated["score"], prior["score"])
        self.assertEqual(prior["prior_translation"]["status"], "inline_in_strict")
        self.assertEqual(prior["prior_translation"]["inline"][0]["covered_by"], "OVL_0019_0010")
        self.assertEqual(prior["callee_readiness"]["counts"], {"untranslated": 1})
        self.assertFalse(prior["callee_readiness"]["composable"])
        self.assertEqual(prior["score_terms"]["composable"], 0)
        self.assertEqual(result["summary"]["composable_bodies"], 1)

    def test_partial_callees_are_unready(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None,
                               [dict(id="OVL_0019_0010", status="partial",
                                     evidence_doc="docs/re/synthetic.md")])
        mentions, main_files, branches = evidence()
        mentions = {**mentions, "OVL_0019_0010": {"source": [], "tooling": [], "docs": []}}
        result: dict = scq.build_queue(state, callgraph(), inventory["units"], ledger,
                                       classifications, mentions, main_files, branches, {},
                                       dict(ovr=bytes(200)))
        row = next(r for r in result["rows"] if r["id"] == "OVL_0019_0020")
        self.assertEqual(row["callee_readiness"]["counts"], {"partial": 1})
        self.assertFalse(row["callee_readiness"]["composable"])
        self.assertEqual(row["callee_readiness"]["unready"], 1)

    def test_closure_chain_and_cycle(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None)
        mentions = {cid: {"source": [], "tooling": [], "docs": []}
                    for cid in state["candidates"].values()}
        result: dict = scq.build_queue(
            state, callgraph(), inventory["units"], ledger, classifications, mentions,
            {"OVL_0019_0020": ["src/recovered/overlay_u19_x_0020.c"]}, {}, {},
            dict(ovr=bytes(200)))
        rows = {row["id"]: row for row in result["rows"]}
        self.assertEqual(rows["OVL_0019_0020"]["callee_readiness"]["counts"], {"verified": 1})
        self.assertFalse(rows["OVL_0019_0020"]["callee_readiness"]["composable"])
        self.assertEqual(rows["OVL_0019_0030"]["callee_readiness"]["counts"],
                         {"modeled_blocked": 1})
        cycle = dict(edges=[
            dict(kind="near_candidate", unit=0x19, site=0x1A, src_func=0x20, target=0x10,
                 target_func=0x10, exact_prologue=True),
            dict(kind="near_candidate", unit=0x19, site=0x35, src_func=0x30, target=0x20,
                 target_func=0x20, exact_prologue=True),
            dict(kind="near_candidate", unit=0x19, site=0x40, src_func=0x10, target=0x20,
                 target_func=0x20, exact_prologue=True)])
        result2: dict = scq.build_queue(
            state, cycle, inventory["units"], ledger, classifications, mentions,
            {"OVL_0019_0010": ["src/recovered/overlay_u19_x_0010.c"],
             "OVL_0019_0020": ["src/recovered/overlay_u19_x_0020.c"]}, {}, {},
            dict(ovr=bytes(200)))
        rows2 = {row["id"]: row for row in result2["rows"]}
        self.assertTrue(rows2["OVL_0019_0020"]["callee_readiness"]["composable"])
        self.assertEqual(rows2["OVL_0019_0030"]["callee_readiness"]["counts"], {"modeled": 1})

    def test_resident_callee_requires_strict_credit(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None)
        mentions = {cid: {"source": [], "tooling": [], "docs": []}
                    for cid in state["candidates"].values()}
        graph = dict(edges=[
            dict(kind="near_candidate", unit=0x19, site=0x35, src_func=0x30, target=0x20,
                 target_func=0x20, exact_prologue=True),
            dict(kind="far_fixup_confirmed", unit=0x19, site=0x45, src_func=0x20, seg=0x00BA,
                 off=0x2E8B, resolution="resident")])
        main_files = {"OVL_0019_0020": ["src/recovered/overlay_u19_x_0020.c"]}
        result: dict = scq.build_queue(state, graph, inventory["units"], ledger,
                                       classifications, mentions, main_files, {}, {},
                                       dict(ovr=bytes(200)))
        rows = {row["id"]: row for row in result["rows"]}
        self.assertEqual(rows["OVL_0019_0020"]["callee_readiness"]["counts"],
                         {"resident_unmapped": 1})
        self.assertEqual(rows["OVL_0019_0030"]["callee_readiness"]["counts"],
                         {"modeled_blocked": 1})
        state2 = scq.load_queue(inventory, ledger, classifications,
                                [dict(id="RES_00BA_2E8B",
                                      evidence_doc="docs/re/synthetic.md")], None)
        result2: dict = scq.build_queue(state2, graph, inventory["units"], ledger,
                                        classifications, mentions, main_files, {}, {},
                                        dict(ovr=bytes(200)))
        rows2 = {row["id"]: row for row in result2["rows"]}
        self.assertEqual(rows2["OVL_0019_0020"]["callee_readiness"]["counts"],
                         {"resident_covered": 1})
        self.assertTrue(rows2["OVL_0019_0020"]["callee_readiness"]["composable"])
        self.assertEqual(rows2["OVL_0019_0030"]["callee_readiness"]["counts"], {"modeled": 1})

    def test_prior_status_none_when_no_evidence(self):
        inventory, ledger, classifications = fixture()
        state = scq.load_queue(inventory, ledger, classifications, [], None)
        empty = {cid: {"source": [], "tooling": [], "docs": []}
                 for cid in state["candidates"].values()}
        result: dict = scq.build_queue(state, callgraph(), inventory["units"], ledger,
                                       classifications, empty, {}, {}, {},
                                       dict(ovr=bytes(200)))
        self.assertTrue(all(row["prior_translation"]["status"] == "none"
                            for row in result["rows"]))

    def test_export_is_order_independent(self):
        original = queue()
        data, graph = copy.deepcopy(fixture()), copy.deepcopy(callgraph())
        data[0]["units"][0]["functions"].reverse()
        data[1].reverse()
        data[2].reverse()
        graph["edges"].reverse()
        self.assertEqual(original, queue(data=data, graph=graph))


if __name__ == "__main__":
    unittest.main()
