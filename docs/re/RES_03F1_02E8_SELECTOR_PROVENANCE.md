# 03F1:02E8: exceptional scalar provenance (t24)

This is a static investigation of the variable scalar at `0086:0173`, not a
new game-identity verdict, a C/original comparison, readiness membership, or a
keystone unlock. It extends the enumerated direct-caller evidence in
[the t100 identity census](RES_03F1_02E8_IDENTITY.md), which found that the text
passed at all 27 overlay and seven resident far sites is copied from CS
literals. It does not change t100's NO factor-2 finding.

## Argument chain

The 0086 caller `OVL_0086_008A` has one relocation-backed overlay call to
its entry stub `0096:0025`, at `0077:2AF6` in `OVL_0077_299C`. Its own
`[BP+0A]` byte selects `0Fh` when equal to 2 and `16h` otherwise; that
value is pushed as a scalar for `03F1:02E8` at `0086:0173`. The wrapper
passes the corresponding low byte at `[BP+0E]` into the box call at `032A`.
The 299C call loads the selector from its `[BP+0E]`, and
[its provisional model](U77_SPELL_299C.md) records that argument forwarding.

The checker pins the nine relocation-backed overlay calls to `0085:005C`
(299C) and the two raw near-call candidates in unit 77. Both near candidates
are instruction-aligned calls preceded by `PUSH CS`; each has one ledger
owner. All 11 adjacent argument setups load an immediate low byte into the
selector slot: values 0, 1, 2, 3, or 4. The check does not claim the full
pushed word is constant: `MOV AL` leaves AH unchanged. Nor does it claim the
selector is itself a documented game object; no such witness is established
by these setups.

| Direct call kind | Sites | Selector low bytes |
| --- | --- | --- |
| Relocated overlay far | `005E:2453`, `006A:04D6`, `0937`, `09D9`, `0A6F`, `0B88`, `0BF1`, `0DED`, `006C:2F81` | 1, 1, 0, 2, 0, 0, 3, 0, 4 |
| Unit 77 near, preceded by PUSH CS | `0077:0CCB`, `1ACB` | 0, 1 |

The site lists are static byte-census results, not execution observations.
The checker pins the original executable and overlay through the project's
media digests, confirms each owner and instruction alignment, verifies the
adjacent stack setup, the selection, the 299C forwarding and the 02E8 box
call. Its raw near scan can find false positives; the two found candidates
are additionally decoded from their ledger owner entries. A new or changed
site fails the check. It does not prove the absence of indirect calls,
overlapping code, alternate entry points, or runtime writes that alias the
argument frame. It does not establish a natural game arrival or a full
control-flow dominance proof. The known direct-entry observations do not
supply a qualifying argument-borne game object for 02E8 under t100's rule.

## Reproduction

From this tree with the pinned original media under ignored `resources/`:

```sh
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh python3 tools/overlay/res03f1_selector_provenance.py
PYTHONPATH=tools/overlay PYTHONDONTWRITEBYTECODE=1 /home/hunter/workspace/poolrad-heavy.sh python3 -m unittest tools/overlay/test_res03f1_selector_provenance.py
```

The synthetic tests cover the expected far and near setup and reject a
register/frame-sourced or changed selector, wrong pointer segment and missing
`PUSH CS`. They do not execute original instructions or prove a game-state
identity. The result adds no accounting row; independent review is required.
