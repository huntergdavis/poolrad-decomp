"""t133's dormant contract: no member and no media/readiness assertion."""
import csv
import hashlib
import io
import subprocess
import unittest
from pathlib import Path


from conditional_dependency_contract import (
    FIELDS, MEMBERS, READINESS_RULE_TESTS, RECHECK_TRIGGERS,
    validate_empty_table, validate_condition_wording, validate_candidate_control,
    validate_credit_rows, validate_change_scope,
)


ROOT = Path(__file__).resolve().parents[2]
DOC = ROOT / "docs/re/CONDITIONALLY_EXCLUDED_DEPENDENCIES.md"
TABLE = ROOT / "docs/re/conditionally_excluded_dependencies.csv"


class ConditionalDependencyContractTest(unittest.TestCase):
    def test_committed_table_has_exact_header_and_zero_members(self):
        self.assertEqual(MEMBERS, frozenset())
        self.assertEqual(READINESS_RULE_TESTS, {})
        validate_empty_table(TABLE.read_text())

    def test_missing_renamed_duplicate_and_extra_headers_fail(self):
        for fields in ((), FIELDS[:-1], ("wrong",) + FIELDS[1:],
                       FIELDS + (FIELDS[0],), FIELDS + ("extra",)):
            with self.subTest(fields=fields), self.assertRaisesRegex(ValueError, "header"):
                validate_empty_table(",".join(fields) + "\n")

    def test_even_a_fully_populated_synthetic_member_is_refused(self):
        output = io.StringIO()
        writer = csv.writer(output)
        writer.writerow(FIELDS)
        writer.writerow(["synthetic" for _ in FIELDS])
        with self.assertRaisesRegex(ValueError, "no members admitted"):
            validate_empty_table(output.getvalue())

    def test_short_extra_and_malformed_rows_are_refused(self):
        header = ",".join(FIELDS) + "\n"
        for row in ("synthetic\n", ",".join(["x"] * (len(FIELDS) + 1)), '"unterminated'):
            with self.subTest(row=row), self.assertRaises(ValueError):
                validate_empty_table(header + row)

    def test_doc_and_table_keep_conditions_with_claims(self):
        validate_condition_wording(DOC.read_text())
        validate_condition_wording(TABLE.read_text())

    def test_condition_cannot_be_moved_to_another_sentence(self):
        for text in ("D is excluded.", "D is excluded. While frame P holds.",
                     "D is excluded while. Frame P holds.",
                     "D is excluded while\n\nframe P holds."):
            with self.subTest(text=text), self.assertRaises(ValueError):
                validate_condition_wording(text)
        validate_condition_wording("D is excluded while frame P holds, for rule R only.")

    def test_candidate_without_executed_failing_control_is_refused(self):
        row = {"body_id": "SYNTHETIC_BODY", "control_receipt": "synthetic-receipt",
               "readiness_rule": "synthetic-rule"}
        for receipt in (None, {}, {"baseline_passed": True},
                        {"baseline_passed": True, "control_executed": True,
                         "expected_assertion_failed": False}):
            with self.subTest(receipt=receipt), self.assertRaisesRegex(
                    ValueError, "executed failing control required"):
                validate_candidate_control(row, {"synthetic-receipt": receipt})

    def test_even_a_reported_control_cannot_supply_default_readiness(self):
        row = {"control_receipt": "synthetic-receipt", "readiness_rule": "synthetic-rule"}
        receipt = dict(baseline_passed=True, control_executed=True,
                       expected_assertion_failed=True, assertion="synthetic-guard",
                       artifact_sha256="0" * 64)
        with self.assertRaisesRegex(ValueError, "no accepted readiness rule/test"):
            validate_candidate_control(row, {"synthetic-receipt": receipt})

    def test_credit_row_cannot_name_member_or_class(self):
        for row in ({"id": "SYNTHETIC_MEMBER"},
                    {"id": "OTHER", "evidence_doc": str(DOC.relative_to(ROOT))},
                    {"id": "OTHER", "readiness": "conditionally_excluded"}):
            with self.subTest(row=row), self.assertRaisesRegex(ValueError, "cannot support a credit row"):
                validate_credit_rows([row], members={"SYNTHETIC_MEMBER"})
        validate_credit_rows([{"id": "OTHER", "evidence_doc": "independent.md"}])

    def test_committed_credit_rows_do_not_cite_class_and_are_unchanged(self):
        paths = [ROOT / "docs/re" / name for name in
                 ("strict_full_c_addresses.csv", "reviewed_function_identities.csv")]
        before = {path: hashlib.sha256(path.read_bytes()).digest() for path in paths}
        for path in paths:
            with path.open(newline="") as stream:
                validate_credit_rows(csv.DictReader(stream))
        self.assertEqual(before, {path: hashlib.sha256(path.read_bytes()).digest() for path in paths})

    def test_no_accounting_or_unrelated_path_in_contract_change(self):
        changed = subprocess.run(
            ["git", "diff", "--name-only", "origin/main"], cwd=ROOT,
            check=True, capture_output=True, text=True).stdout.splitlines()
        validate_change_scope(changed)
        with self.assertRaisesRegex(ValueError, "unrelated/accounting paths"):
            validate_change_scope([str(DOC.relative_to(ROOT)),
                                   "docs/re/function_accounting_checkpoint.json"])

    def test_recheck_triggers_and_empty_rule_registry_are_documented(self):
        text = DOC.read_text()
        for trigger in RECHECK_TRIGGERS:
            self.assertIn(trigger, text)
        self.assertIn("<!-- CONDITIONAL_RULES:EMPTY -->", text)


if __name__ == "__main__":
    unittest.main()
