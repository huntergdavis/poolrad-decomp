# Pairing-gate attribution and workflow context

A stacked change may fail `check_body_doc_pairing.py` because an earlier commit
added a checker without `CHECKER_ROWS`. The unrelated tip commit is not evidence
of who added that path. On refusal, the gate now prints `PATH ORIGIN` lines after
the existing reasons. A proven, unique, single-parent path addition in the
merge-base-to-HEAD range is identified by its full commit SHA.

For example, if commit A adds an unregistered checker and commit B edits only
an unrelated note, the refusal names A as `added-in`, not B. Adding registration
alone still makes the same synthetic change pass. The gate's `problems()`
decisions, evidence-note rules, exemptions and exit status are unchanged.

`PATH ORIGIN` is context for changed body/checker paths, not a claim that each
path caused each reason, or that the addition introduced every defect. Existing
base paths, uncommitted changes, deleted paths, renames that Git recognizes,
multiple additions, merge additions, shallow history and unavailable history
are reported as unknown rather than attributed to the tip. Git rename detection
is heuristic; heavily rewritten moves may appear as additions. This reports
path history, not original authorship or semantic defect introduction.

PR mapping is explicitly `unknown`: local Git history does not establish GitHub
PR membership, and a `#123` in a commit message is not proof. Use the named
commit to investigate the stack; do not assign its fault to the PR being tested.

Both Repository hygiene jobs record event and revision context immediately
after checkout, before their gates. The summary distinguishes a `push` revision
from the synthetic merge revision checked by `pull_request`. It includes the
workflow SHA, ref, head/base refs and run ID, with absent values marked unknown.
Thus a push stale-base failure and a successful PR merge check retain separate
identities. The existing stale-base check and all other gates remain in place.

## Verification scope

Synthetic tests exercise the stacked refusal and registration-only positive
control, conservative history fallbacks, event-specific summaries, safe ref
rendering, and summary placement before gates in both jobs. They need no game
media. Local summary rendering does not prove GitHub's hosted presentation;
report hosted observations separately and pin them to their run/event/head.
