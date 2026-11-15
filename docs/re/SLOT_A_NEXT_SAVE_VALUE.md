# Next SAVE destination and source value

The saved-script audit `SLOT_A_UP_ECL_GATE.md` identifies the next SAVE's
destination as 49FD. This synthetic shared-state chain reuses the existing
word-argument, raw word-helper, tag-branch and immediate slot-value adapters
after the loader. It supplies source value 5 as a fixture, not as a claim
about the saved script's actual source value.

The fixture checks actual helper statuses and return points, destination
49FD, the synthetic source value and preservation of prior effects. It checks
a mismatched helper stub. The component contracts' authenticated references,
immutable code, bindings, alias restrictions and disjoint-host requirements
remain in force. The store and return are not executed; same-invocation
provenance, a complete Up route and native 345B parity remain unverified.
