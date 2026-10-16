# U6F +02AE complete caller composition and explicit UI dependency

`pr_u6f_02ae_fullstate_step` composes all 129 bytes / 51 instructions of
the U6F caller. The unchanged entry adapter covers 75 bytes / 30 starts;
the new body covers the other 54 bytes / 21 starts at +02E9..031E.
SYSTEM StrLoad B0B, Chr C3B and StrCat B98 reuse existing instruction
adapters. DOS services and the entire resident display/input callee
remain explicit full-state dependencies. This is bounded caller coverage,
not a whole-function/native/identity/strict-count promotion.

Before edits, PM cleared this scope after p5/p9/census ownership screens.
Two isolated branches keep this implementation separate from the +1446
cold-return dependency map; the [merged composition](U6F_1446_FULLSTATE.md)
is unchanged.
The bounded recall query found no indexed matches. Reused evidence is
cited below; the original spans and both caller operands were independently
checked by static disassembly, without original-byte execution.

GAME.OVR is 206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`;
START.EXE is 65,231 bytes, SHA-256
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`.

| Span | Bound | SHA-256 |
| --- | --- | --- |
| U6F +02AE..032E, GAME 1E294..1E314 | 129 bytes / 51 instructions | `ef005d8298feecebd4c1a466d86c8006533fc43073c47b0f33ef263ffd9c6539` |
| Added +02E9..031E, GAME 1E2CF..1E304 | 54 bytes / 21 instructions | `7975be64c15f8771caa46217d7242d4c001d79a2c61845864eeeef43be8b8c02` |
| Resident 0709:+0BF5..0C3E, expanded START 7C85..7CCE | 74 bytes, supplied dependency | `4a3f940caae95794f257a25f5241bcf3a04c2749b212f5b8d9def5befd63a4cc` |

Two direct source callers are now recorded: U6F +0023 PUSH CS / +0024
near CALL returns to +0027; +19E6 PUSH CS / +19E7 near CALL returns to
+19EA. The earlier caller lies in the +0010 body after its initial
output-pointer clears. This supersedes the older note's single known
caller statement; it does not establish a new reached error route.
The outer RETF consumes the actual live frame, with no fixed caller PC.
Retained run-040 +02D0 RAM (655,360 bytes, SHA-256
`93ba55cde6ece70feaa9f85b158ac9ca76e4468d3b138807ee4be44d4738d52a`)
also confirms the loaded +031A operand is 0F2D:0BF5. Reading that operand
does not establish execution of the skipped error path.

| Error-body boundary | Ordered behavior and frame |
| --- | --- |
| +02E9/+02EB | MOV AL,0E retains AH; push the full AX color word |
| +02EC..02F7 | Stack message destination SS:[BP-0202]; CS:029A prefix to StrLoad; RETF4 leaves destination pointer |
| +02FC..0306 | Stack character destination SS:[BP-0102]; read live DS:537B even if path length is zero; Chr RETF2 leaves that pointer |
| +030B/+0315 | Concatenate character, then CS:02AC suffix; each StrCat RETF4 retains message destination |
| +031A | Far CALL resident 0709:0BF5, loaded 0F2D:0BF5, pushing actual 2A8E:031F frame |
| +031F..0329 | Reread SS:[BP-2] for both comparisons; zero/five exit, other words retry at +02B5 |

For P=U6F frame BP and S=P-0202, SP before +031A is S-6: message
offset/segment then color word. Resident entry SP=S-10; return IP/CS
occupy the first four bytes and arguments the next six. Its original
body saves BP, allocates 42 bytes, uses StrStore with capacity 40, calls
row clear +05EB, printer +0626 and key routine 0899:0059, stores the key
byte and executes RETF6. Those effects are owned by the explicit UI event,
not inferred from the message bytes or a renderer callback.

The UI callback receives read-only full 16-field CPU and RAM at actual
callee entry and supplies complete post-callee CPU plus an ordered byte
journal, including stack/local/helper/display/key effects. Missing or
blocked dependency preserves state. Count/address/old-byte chains and
projected return frame are validated before commit; normal return must
be 2A8E:031F with original SS and SP advanced by ten. Arbitrary UI
diversions are unsupported. Other registers, FLAGS and live caller locals
come from the event without fabricated restoration. A callback can change
SS:[P-2] or the live path; subsequent comparisons/copies observe it.
Validation cannot establish event completeness; the callback contract
requires it. The synthetic fixture is explicitly not original UI execution.

RAM is bounded to at most 1MiB, with the reused helpers' linear spans:
each operand must fit a segment and RAM without A20 wrap. Unsupported
PC/span/DF blocks; individual 16-bit pointer updates wrap. Logic AF is
normalized to zero. Loaded U6F=2A8E, SYSTEM=131C and resident=0F2D require
host-audited code identity. There is no host retry cap. Strings/literals
come from live RAM; no original text or media is compiled into the model.

Focused strict and ASan/UBSan gates cover two source-level prefix
comparisons against the unchanged adapter (all 16 CPU fields and every
byte of 1MiB), four synthetic complete invocations, both recorded return
PCs and an arbitrary caller, retained AH, message assembly, changed path
on retry, and callback changes to the live result zero/five. Six invalid
UI events plus a blocked callback preserve the entire CPU/RAM image;
repeated journal addresses, generic cleanup, string limit and atomic
span/argument blockers are checked. These comparisons reuse source APIs,
not an independent original-byte executor. The static audit hashes private
originals, checks both caller operands and all 51 caller start addresses.

```sh
bash tests/replay/check_u6f_02ae_fullstate.sh
bash tests/replay/check_u6f_02ae_fullstate.sh --sanitize
python3 tests/replay/audit_u6f_02ae_fullstate.py \
  resources/DOS/POOLRAD/GAME.OVR resources/DOS/POOLRAD/START.EXE
```

The [saved native directory path](SLOT_A_NATIVE_U6F_POST_U9B_DIRECTORY.md)
returned IOResult=5 and skipped this error body. No natural nonzero/nonfive
error, complete native UI return, independent branch/alias oracle or
whole-invocation parity is claimed here. A future natural error gate must
reach the original caller through UI/path selection and an actual DOS
result, recording +02DA/+02E9/+031A/+031F and retry/return in one run.
Do not force error registers, patch guest RAM or treat synthetic service
events as a reached route. No new native run is started under the host hold.

Reused source evidence: [entry ABI](U6F_02AE_ENTRY_IORESULT_ABI.md),
[original body/UI contract](U6F_POSTINSERT_02AE.md),
[StrLoad](SLOT_A_SAVED_COPY_1965_JOIN.md),
[Chr](SLOT_A_STRING_0425_ABI.md), and [StrCat](SYSTEM_B98_NATIVE_ABI.md).
Parity review: `target_function=OVL_006F_02AE`;
`source_candidates=u6f_02ae_fullstate.c with existing entry/string adapters`;
`signals=hash/extent, both callers, 51 source starts, call/frame graph,
synthetic state/negative-event checks`; `score=five evidence categories,
not a probability`; `verdict=unclear` for whole invocation. Supplied DOS/UI
effects and absent natural-error/native execution prevent strict promotion.
