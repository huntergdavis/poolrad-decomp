# Saved slot-A Up: caller gate after first script, before entry 1

`pr_slot_a_caller_to_entry1` models unit `0x1C:+0x345B..+0x348C` after the
`+0x3458` ECL call returns. It stops before the first reached callee: near
`+0x31B4` or unit `0x68` entry 1. The nonmovement branch stops at
`+0x34F5`. This is a bounded caller step, not a complete one-Up replay.

Before this work, `deja "slot A Up caller 345B 34E7 after ECL EXIT search
3171 coordinates time"` could not open its read-only index lock in the
sandbox; the access retry produced no result before it was stopped. Reused
and rechecked [the gameplay loop map](U1C_LOOP_3387.md),
[the live first ECL binding](SLOT_A_ECL_LIVE_BIND.md),
[the first EXIT boundary](SLOT_A_FIRST_EXIT_9974.md), and
[the native AL10 caller observation](SLOT_A_ONE_UP_NATIVE_AL10_BRANCH.md).
The first EXIT boundary landed in PR #341.

## Original bytes and native stops

The ignored `GAME.OVR` (206,105 bytes, SHA-256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`)
maps unit `0x1C` to file offset `3398`. Original `+0x345B..+0x3491`
(`0x41A1..0x41D7`) is 55 bytes, SHA-256
`3970e585476250421f70a482f2315571c16ca249bf4395293efbdc14e5475df8`.
The first 50 bytes, through `+0x348C`, are SHA-256
`2fd3cff4ce75d0d714af53fff8bc44caa2c331505414354a8245c8e73bdfe58e`.
The loaded 55 bytes at `3582:345B` in both same-run RAM stops are SHA-256
`785a11da0a7c4bf8328f5865751fe96597ab3a1dc13ff84100b1337d366daf1a`.
The first 50 are byte-equal; at `+0x3490..+0x3491`, the original far-call
segment word `0x0072` reads `0x0896` in RAM. This is an observed loaded-byte
difference, without assigning a relocation mechanism.

| Original PC | Byte-checked effect | Model result |
| --- | --- | --- |
| `+0x345B..+0x3466` | If `[442F] != 0`, call near `+0x31B4`, then jump to `+0x34F5` | `PR_SLOT_A_CALLER_31B4`, before call at `+0x3462` |
| `+0x3469..+0x3470` | If `[49FF] != 0`, jump to `+0x34F5` | `PR_SLOT_A_CALLER_TAIL` |
| `+0x3473..+0x347F` | Sign-extend party x `[6AAD]` and store word window 0 `+0x1E0` | First word committed |
| `+0x3480..+0x348C` | Sign-extend party y `[6AAE]` and store word window 0 `+0x1E2` | Second word committed |
| `+0x348D..+0x3491` | Far call unit `0x68` entry 1 | `PR_SLOT_A_CALLER_ENTRY1`, before call at `+0x348D` |

An invalid reached window access stops at its write PC. If the first word
fits and the second does not, the first write remains. Neither bypass branch
requires a window pointer because it never reaches one. The model does not
execute entry 1, the view call, the coordinate comparison, the resident
notification, or the search script.

The same run003 native AND-handler RAM at `3582:0DEF` is 655,360 bytes,
SHA-256 `9c82fae60d03939669c8f8a256a96ad0910376c80e07f31ade33eadc1ba04f95`.
Its DS segment is `0x14A0`: `[442F]=[49FF]=0`, party `(15,4,6)`, window 0
pointer `3996:0000`, comparison words `(+1E0,+1E2)=(1,4)`, time words
`(+18E,+190,+192)=(0,5,10)`, and `[84E0,84E1]=(0,0)`. Applying the
bounded caller prefix to a copy of those selected fields reaches `+0x348D`
and writes comparison words `(15,4)`.

The same-run later search `+0x3171` RAM is SHA-256
`cac6b2b97ea481dbb4bf4c165dec5035bc769beac80773e5fa661bab1600ee1d`.
It has `[442F]=[49FF]=0`, party `(14,4,6)`, comparison words `(15,4)`,
time `(1,5,10)`, and `[84E0,84E1]=(0,1)`. Its stack at `SS:3EF0` has
return `3582:34EA` and argument `0x9975`, identifying the later caller
search call at `+0x34E7`. The matching comparison words support the
decoded caller write but are **not** an isolated native delta: entry 1,
view, resident work, and time effects occur between these stops. The AND
stop is before the ECL EXIT, so its fields are only a projection into the
caller; there is no direct native `+0x345B` or `+0x348D` stop yet.

## Verification and next boundary

The quick synthetic cases are registered as
`poolrad_slot_a_caller_to_entry1_tests` in CTest. Run
`bash tests/replay/check_slot_a_caller_to_entry1.sh` for strict C11 synthetic
cases, or pass ignored `GAME.OVR`, AND RAM, and later search RAM paths in
that order for hash-locked private audits and the field projection.
The audit checks both loaded caller bodies, selected DS/window fields, and
the later search call frame. All original game media and RAM stay under
ignored `resources/`.

The next unresolved step is the live unit `0x68` entry-1 call at `+0x348D`,
then view/resident effects through the `+0x34E7` search call. A direct native
stop at `+0x345B` or `+0x348D` would isolate the post-EXIT input. Until
that or equivalent measured state exists, neither this projection nor PR
#341 establishes full C-native one-Up parity, a final coordinate result, or
a new game-routine count. `pr_slot_a_native_one_up` remains
`BLOCKED_KEY_DISPATCH`.

**Later direct evidence:** [the post-EXIT native return trace](SLOT_A_POST_EXIT_NATIVE_RETURN.md)
captures `+345B`, `+348D`, the entry-1 return `+3492`, and the caller through
`+34E7`. Its private audit verifies exact full-RAM C/native parity for this
caller's `+345B..+348C` prefix. The earlier limitation above describes the
state of evidence when this prefix model was first merged.
