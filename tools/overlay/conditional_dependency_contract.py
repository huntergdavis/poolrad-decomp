"""Read-only t133 refusal scaffold: no membership or readiness consumer."""
import csv
import io
import re

MEMBERS = frozenset()
READINESS_RULE_TESTS = {}
RECHECK_TRIGGERS = ("new caller", "changed frame", "changed stub", "changed body pin")
CONTRACT_FILES = frozenset((
    "docs/re/CONDITIONALLY_EXCLUDED_DEPENDENCIES.md",
    "docs/re/conditionally_excluded_dependencies.csv",
    "tools/overlay/conditional_dependency_contract.py",
    "tools/overlay/test_conditional_dependency_contract.py",
))
ALLOWED_CHANGE_FILES = CONTRACT_FILES | {".github/workflows/repository-hygiene.yml"}


FIELDS = (
    "body_id", "dependency_id", "call_site", "entry_condition",
    "readiness_rule", "exclusion_statement", "ceiling", "evidence_doc",
    "control_command", "control_receipt", "body_sha256", "dependency_sha256",
    "frame_pin", "stub_pin", "caller_census_pin", "checker_pin", "review_pr",
    "reviewed_commit",
)


def validate_empty_table(text):
    """Fail closed until a later independent review admits a pinned member set."""
    try:
        rows = list(csv.reader(io.StringIO(text), strict=True))
    except csv.Error as exc:
        raise ValueError("malformed member CSV") from exc
    if not rows or tuple(rows[0]) != FIELDS:
        raise ValueError("member table header differs from the reviewed schema")
    if MEMBERS or any(row for row in rows[1:]):
        raise ValueError("no members admitted; dependency remains uncovered")


def validate_condition_wording(text):
    """Lexical guard only: an independent reviewer must assess the predicate."""
    for match in re.finditer(r"\bexcluded\b", text, re.IGNORECASE):
        clause = re.match(r"\s+while\s+([^.!?]+)[.!?]", text[match.end():])
        if clause is None or not clause[1].strip() or re.search(r"\n\s*\n", clause[0]):
            raise ValueError("each exclusion claim needs its condition in the same sentence")


def validate_candidate_control(row, receipts):
    """Refuse unsupported evidence; these flags cannot authenticate real media."""
    receipt = receipts.get(row.get("control_receipt"))
    if not isinstance(receipt, dict) or any(receipt.get(key) is not True for key in
            ("baseline_passed", "control_executed", "expected_assertion_failed")):
        raise ValueError("executed failing control required; dependency remains uncovered")
    if not receipt.get("assertion") or not receipt.get("artifact_sha256"):
        raise ValueError("named failure and pinned receipt required")
    if row.get("readiness_rule") not in READINESS_RULE_TESTS:
        raise ValueError("no accepted readiness rule/test; class satisfies nothing by default")
    raise ValueError("no members admitted; independent member implementation required")


def validate_credit_rows(rows, members=MEMBERS):
    for row in rows:
        if row.get("id") in members or any(
                "conditionally_excluded" in str(value).lower() for value in row.values()):
            raise ValueError("conditional dependency class cannot support a credit row")


def validate_change_scope(changed_paths):
    paths = set(changed_paths)
    if paths & CONTRACT_FILES and paths - ALLOWED_CHANGE_FILES:
        raise ValueError("contract change touches unrelated/accounting paths: " +
                         ", ".join(sorted(paths - ALLOWED_CHANGE_FILES)))
