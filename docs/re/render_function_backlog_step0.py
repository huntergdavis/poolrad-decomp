#!/usr/bin/env python3
"""Render FUNCTION_BACKLOG_STEP0_<sha>.md from its authoritative CSV.

The CSV is the artefact; this file only renders it so every update is a
regeneration plus a diff, never a hand edit (review ruling pi-review-2, t144).

Usage: python3 docs/re/render_function_backlog_step0.py <measuring-sha>
Writes docs/re/FUNCTION_BACKLOG_STEP0_<short-sha>.md next to the CSV.
"""
import csv
import sys
from collections import Counter
from pathlib import Path

here = Path(__file__).resolve().parent


def main() -> None:
    measuring_sha = sys.argv[1]
    short = measuring_sha[:8]
    csv_path = here / f"function_backlog_step0_{short}.csv"
    rows = list(csv.DictReader(csv_path.open()))
    mech = [r for r in rows if r["checkability"] == "mechanically-checkable"]
    interp = [r for r in rows if r["checkability"] != "mechanically-checkable"]
    assert len(mech) + len(interp) == len(rows), "unknown checkability value"
    by_domain = Counter(r["domain"] for r in rows)
    blocks = [rows[i : i + 10] for i in range(0, len(rows), 10)]

    lines = []
    ap = lines.append
    ap(f"# Function conversion backlog, step 0 (measured at main `{measuring_sha}`)")
    ap("")
    ap(
        f"Exact list of identified-but-uncredited game function bodies: "
        f"**{len(rows)} identities** ({len(mech)} mechanically-checkable, "
        f"{len(interp)} interpretation-heavy), published as blocks of ten, "
        "mechanically-checkable first. Owner/checker slots are filled by the "
        "leader when a block is assigned; a pre-filled slot is an assignment "
        "nobody made."
    )
    ap("")
    ap(
        "**Authoritative artefact:** "
        f"`function_backlog_step0_{short}.csv` (this document is GENERATED "
        "from it by `render_function_backlog_step0.py`; edit the CSV and "
        "regenerate, never hand-edit this page)."
    )
    ap("")
    ap("## Derivation (measured)")
    ap("")
    ap(
        "1. `function_backlog_reconciliation.csv` rows with "
        "`game_ownership == game_verified` = 335 (287 overlay + 48 resident), "
        "matching `function_accounting_checkpoint.json` "
        "(`verified_game_identities: 335`)."
    )
    ap(
        "2. Rows with `strict_full_c == 1` = 290, matching "
        "`strict_full_c_originals: 290` and the 290 ids in "
        "`strict_full_c_addresses.csv`."
    )
    ap(f"3. Gap = **{len(rows)}**: rows with `game_ownership == game_verified`")
    ap(
        f"   AND `strict_full_c == 0` "
        f"({by_domain['overlay']} overlay + {by_domain['resident']} resident), "
        "equal to 335 - 290."
    )
    ap("")
    ap(
        "**Do NOT derive this list by string-subtracting the two ID files.** "
        "Naive subtraction gives 92, not 45: the 47 credited resident ids in "
        "`strict_full_c_addresses.csv` use `RES_<seg>_<off>` naming "
        "(`RES_00BA_0000`) while the reconciliation uses `RES_IMAGE_<imgoff>` "
        "(`RES_IMAGE_000F6`), so they never string-match. The per-row "
        "`strict_full_c` flag is the only consistent join without a "
        "segment:offset to image-offset mapping."
    )
    ap("")
    ap(
        "**Ledger join key** (for joining `OVL_` ids to "
        "`overlay_function_ledger.csv`): the ledger's `id` is "
        "`0x<unit unpadded hex>:0x<offset padded to 4>` (e.g. `OVL_001C_0001` "
        "is `0x1c:0x0001`). The `OVL_` form does not string-match the ledger "
        "id; join via the reconciliation's `original_address` column."
    )
    ap("")
    ap("## Classification rule (proxy, not a conversion claim)")
    ap("")
    ap(
        "- **mechanically-checkable**: body fully decoded in "
        "`overlay_function_ledger.csv` (`decoded == bound`), grade A or B "
        "(own caller contract: entry site or a specific near-call site), no "
        "alias entry."
    )
    ap(
        "- **interpretation-heavy**: partial decode (`decoded < bound`), "
        "grade D (alias entry, semantics inferred), or contract is fallthrough "
        "rather than a call site."
    )
    ap("")
    ap(
        "This is a classification from decode/ledger metadata only. "
        "`recorded_body_bytes` is empty on all 45 rows and no compiler was "
        "run: \"mechanically-checkable\" means the byte-identical-body + "
        "caller-contract check is *well-posed from recorded evidence*, not "
        "that any body is verified convertible. Reviewer verification note: "
        "the resident row's fallthrough characterisation is the author's read "
        "of the resident ledger, not independently reviewed."
    )
    ap("")
    ap("## Blocks of ten")
    ap("")
    for n, block in enumerate(blocks, 1):
        cls = block[0]["checkability"]
        ap(
            f"### Block {n} ({len(block)} identities, {cls}) — owner: — | checker: —"
        )
        ap("")
        ap("| id | addr | grade | ledger class | decoded/bound | insns | ret | callers | checkability |")
        ap("|---|---|---|---|---|---|---|---|---|")
        for r in block:
            db = f"{r['decoded']}/{r['bound']}" if r["bound"] else "n/a"
            ap(
                f"| {r['id']} | {r['addr']} | {r['grade']} | {r['cls_ledger']} "
                f"| {db} | {r['insns']} | {r['ret']} | {r['callers']} "
                f"| {r['checkability']} |"
            )
        ap("")
    ap("## Limits")
    ap("")
    ap(
        f"- The count ({len(rows)}) is a fact measured at main "
        f"`{measuring_sha}`; it moves as PRs merge. Re-run the derivation "
        "before quoting it against a different head."
    )
    ap(
        "- Classification is proxy metadata; no body conversion or compilation "
        "was attempted (step 0 is list-only)."
    )
    ap(
        "- The 92-vs-45 discrepancy is a naming artifact, not a finding of 92 "
        "uncredited bodies; the 47 mismatched ids are all credited residents."
    )
    ap(
        "- Owner/checker slots are deliberately empty; the leader fills them "
        "at assignment time."
    )

    out_path = here / f"FUNCTION_BACKLOG_STEP0_{short}.md"
    out_path.write_text("\n".join(lines) + "\n")
    print(f"wrote {out_path} ({len(rows)} identities, {len(blocks)} blocks)")


if __name__ == "__main__":
    main()
