# Actual 025D caller0292 continuation

This disjoint CPU adapter executes the actual resident 0292 continuation in
three independent fragments: selector return 0292 to Assign 2C9; actual native
after-Assign 2CE to Reset 2D7; actual native after-Reset 2DC through RETF0E to
loader C2B. Assign, Reset, message 193 and retry 289 remain explicit gates.
No runtime callback, supplied register result, DOS journal or in-flight state
replacement connects the fragments. The adapter stops before loader C2B.

Base is main 217e809448be096fcc48b2413714856f5b165284. Scope is six new
replay files. Earlier 025D prefix, sidecar occurrences, allocation, printer,
shared integration and all count/identity/playable claims remain untouched.

## Provenance and native capture

Bounded prior-session query "Poolrad 025D 0292 02C9 02D7 caller continuation"
expired without reusable output. Reused leads, rechecked against originals
and fresh native state, are [the nested-open map](RESIDENT_DAX_NESTED_OPEN.md),
[the probe ABI](SLOT_A_FILE_PROBE_02EB_ABI.md),
[approved actual selector return evidence](https://github.com/huntergdavis/poolrad-decomp/blob/52aee0899389528ca9bf6f80d5d6d5da96f534f0/docs/re/SLOT_A_CLOSE_IORESULT_ABI.md), and
[the corrected seven-word frame note](https://github.com/huntergdavis/poolrad-decomp/blob/304e672180d35111330266d108a5f11c88d4044d/docs/re/SLOT_A_LOADER_0AEC_PREFIX.md).
Those worker branches are unmerged; this adapter has no C dependency on them.
The audit reuses existing CPU/media/comparison/Unicorn helpers from the
0356 and delete audit modules. No existing C implementation changes.

A fresh untouched original saved-A run reached C26, executed the nested
selector and stopped at 0292. All 29 CPU/full-RAM states were captured and
frozen before C implementation. They partition into three independent inputs,
24 post-input C checkpoints and two native entry states after unsupported
far CALLs. The two original-byte CALL-only emulator comparisons establish
frames; they are excluded from C counts. All original inputs, captures,
screenshots and builds remain ignored in
resources/oracle-runs/slot-a-open-0292-continuation-20261014.

## Actual frames and return

At 0292, CS=1026, SS=ES=1D16, DS=14A0, SP=3384, BP=33D8, AX=0001,
BX=0006, CX=0000, DX=3300, SI=3368, DI=32D0, FLAGS=7206. The retained outer
far return and seven argument words at SS:33DA are:

~~~text
[C2B,2A8E, 3626,1D16, 0000, 35EE,1D16, 34ED,1D16]
~~~

The caller's first file record is 1D16:3626; low mode byte is 0. Candidate and
extension/error strings are 1D16:34ED (length 23) and1D16:35EE (length 19).
Their already copied locals are SS:33AE and SS:3385. Filename bytes remain
private. C saves selector AL=1 at BP-54 and follows the nonzero path.

Before Assign 2C9, SP=337C holds [33AE,1D16,3626,1D16]. Original far CALL
enters 131C:13E2 at SP=3378 with [2CE,1026] preceding those arguments.
C blocks before that frame push. After actual native Assign, the separate
2CE fragment loads the same caller record pointer, pushes it and size 0001,
and stops before Reset 2D7 at SP=337E with [0001,3626,1D16]. The original
Reset call enters 131C:1410 at SP=337A with return [2DC,1026].

Native Assign left the record header [0000,D7B0,0000]. Native Reset changed
it to [0006,D7B3,0001], retaining the remaining 122 record bytes. This size 1
caller record is distinct from the selector's temporary size 0080 record.
These runtime effects are observations, excluded from C parity.

The independently seeded 2DC fragment reads saved AL, stores/reloads the
result byte, restores BP, and executes real RETF0E. It consumes 18 bytes from
SP=33DA: four return-frame bytes plus 14 argument bytes. Actual loader C2B has
SP=33EC, BP=37A6, AX=0001, BX=3378, CX=0000, DX=3656, SI=33C6, DI=3626,
DS=14A0, ES=SS=1D16, FLAGS=7246. No loader instruction executes in this API.

## Pins and verification

| Original artifact | Bytes | SHA256 |
| --- | ---: | --- |
| START.EXE | 65231 | d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd |
| GAME.OVR | 206105 | 872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614 |
| SAVGAMA.DAT | 13137 | abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1 |
| Original 0292..02EA | 89 | ddfe7addfa0e224bf10c01d330559c947c5e8644ffe2a28affbe121ae82b9737 |
| Native transcript | local | dd5f4aa13963e9d4031f9e1cc5428cb0acda2b262b84937e7d78c132fbd2399a |

Every native snapshot verifies the entire 142-byte original/loaded 025D body,
including its four AF8-to-131C far-call segment relocations; the original
C26 call with its 802-to-1026 relocation; and original/loaded Assign (46) and
Reset (101) bytes. Hash/identity checks do not grant C credit for those callees.

~~~sh
bash tests/replay/check_slot_a_open_0292_continuation.sh
POOLRAD_UNICORN_PYTHON="$PWD/resources/venv-string0356/bin/python" \
 bash tests/replay/check_slot_a_open_0292_continuation.sh \
 resources/oracle-runs/slot-a-open-0292-continuation-20261014
~~~

The audit passes 386 complete CPU/RAM C instruction comparisons, including
24 natural comparisons (13 + 5 + 6) and all 24 post-input native checkpoints.
Two additional original-CALL-only CPU/RAM frame comparisons pass separately;
these supply no C effects or runtime result. Flags are never masked. The
complete strict and differential suites also pass ASan/UBSan. All C/test
objects compile under C11 Wall/Wextra/Wpedantic/Wconversion/Werror.

Controlled cases independently copy native inputs: raw AL 0/1/80/FF with
nonzero AH variants; mode 0000/0001/0100/0101; an independently seeded retry
tail; saved-return byte 0/1/80/FF preserving AH; real RETF0E stack adjustment
wrapping from SP=FFFC; and an alternate far file-record pointer. Tests follow
the original write/read order and compare every defined CPU field plus all
655360 conventional RAM bytes after each instruction.

If selector AL is 0 and low mode byte is 0, C prepares code 000E and the
extension local, then stops before near CALL 2AA->0193, return 2AD. A synthetic
independent retry-tail input reaches 289 and stops before another selector
attempt. Mode 0100 is treated like 0000 because only its low byte is read.
For zero AL and nonzero low mode, C skips Assign/Reset and returns saved AL=0.
Those negative branches are controlled CPU checks, not measured native I/O
failures or results of a translated message helper.

Cases are bounded at 100 instructions / 10s; the process at 120s plus 5s kill
allowance. Strict tests check unchanged typed gates and caller/retry stops,
null/bad spans, actual 18-byte RETF cleanup, complete CMP flags and AH.
Original media, RAM and string payloads remain private; public evidence uses
fingerprints, addresses and ABI word constants.

- target_function: reached resident 0292 continuation, independent after-Assign
  and after-Reset CPU fragments, and real RETF0E return to loader C2B.
- source_candidates: slot_a_open_0292_continuation.c only.
- signals: pinned original/loaded code, fresh full-state native checkpoints,
  distinct candidate/extension/file-record roles, argument sizes, real return
  cleanup, full flags/AH, and controlled mode, pointer and stack boundary cases.
- score: 4/5 ordinal evidence strength for these fragments; not probability.
- verdict: match, bounded to the stated fragments; complete 025D/I/O unclear.
- review_notes: native Assign/Reset effects, message/input 193, retry selector
  execution, loader C2B work, later sidecar/heap/printer behavior, full-C
  ownership and continuous saved-A C parity remain unsupported.
