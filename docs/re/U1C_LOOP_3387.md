# Unit 0x1c gameplay loop +0x3387..+0x3507: bounded slice model

`src/recovered/overlay_u1c_loop_3387.c` models the part of DOS v1.3 `GAME.OVR`
unit `0x1c`'s gameplay loop ([GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) §3) that runs
after the loop's initialisation: the unit `0x68` entry 2 input call, the key
dispatch, and the movement bookkeeping through `+0x34F2`, up to the loop's
`retf`. It is the route a saved-state Up key takes after
[U68_ENTRY2_UP_SLICE.md](U68_ENTRY2_UP_SLICE.md). Classification: **bounded
model, not strict full C**: every callee is an explicit hook (several are
translated elsewhere and are wired in by the host). **No count or credit is
claimed.** No saved party tuple is assumed and no live branch is inferred; the
tests set the party bytes and hook behaviour themselves.

`deja "Poolrad unit 0x1C gameplay loop +0x3387 input unit 0x68 entry2 movement
bookkeeping +0x34F2"` found no earlier trace. Reused and re-checked:
GAMEPLAY_PATH.md §3 (the loop table, rows `+0x3387..+0x34F5`),
U68_ENTRY2_UP_SLICE.md (p5: entry 2 returns its saved key byte by plain `RETF`;
an extended key with scan code `0x48` takes the Up preflight), U68_ENTRY1.md
(entry 1 is the callee at `+0x348D`), U68_COORDINATE_WRITER_073B.md,
SLOT_A_UP_ECL_GATE.md (the saved selector). p5 owns resident `0x8AE:0x00DC` and
the entry-2 key route; p9 owns the native ABI.

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

| Item | Value |
| --- | --- |
| routine | unit `0x1c` (file `3,398`) `+0x32F8..+0x3507` = file `0x403E..0x424D`, 528 bytes, 164 instructions, one `retf` at `+0x3507` (next routine: the 5-byte leaf `+0x3508`); SHA-256 `a6f44cd9d3397ea0d96b03f45dfb82fa10c44071a9023a57d8e4979e2260ea79` |
| modelled slice | `+0x3387..+0x3507` = file `0x40CD..0x424D`, **385 bytes, 123 instructions**; SHA-256 `399e420f32c88e8f3b3b4c2ead70f9a71bb229bb8ceb47457fc4b87fb5b3bad5`. Not modelled: `+0x32F8..+0x3386` (the initialisation, steps 1-9 of GAMEPLAY_PATH.md) |
| control flow | every jump target is inside the routine; two loops: the outer `jmp +0x3395` at `+0x34FC` and the key loop `jmp +0x33B9` at `+0x3449` |
| frame | `sub sp, 2`: `[bp-1]` the key byte |
| runtime routine | `0xAF8:0x182A` (called at `+0x33C9`, `+0x33DD`), read from `START.EXE` (the `0xAF8` mapping checked on `0x0B0B`): `al := [bx+4]; if 'a' <= al <= 'z' then al -= 0x20`, `retf 2` |

## Route (byte-verified)

| Offsets | Effect |
| --- | --- |
| `+0x3387..+0x3390` | window-0 word `0x1E4 := [0x84DC]` (zero-extended) |
| `+0x3395..+0x33B4` | `key := lcall 0x72:0x2A` (unit `0x68` entry 2, stub paragraph `0x72`); `[0x4436]:[0x4438] := [0x5D92]:[0x5D94]`; word `0x1E4 := [0x84DC]` |
| `+0x33B9..+0x33D0` | window-1 word `0x594 > 1` (unsigned) goes to `+0x33D2`; otherwise `upcase(key) <> 0x45` goes to `+0x344C` |
| `+0x33D2..+0x33EA` | `[0x49FF] <> 0` goes to `+0x344C`; `upcase(key) = 0x45` runs near `+0x2DD3` and goes to `+0x342C` |
| `+0x33EC..+0x3427` | `[0x84DD] := word 0x594 and 1`; word `0x594 := 1`; `lcall 0x3CA:0x25`; near `+0x3171` with `[0x49E5]`; `[0x442F] <> 0` runs near `+0x31B4`; word `0x594 := [0x84DD]` |
| `+0x342C..+0x3449` | `[0x49FF] = 0`: prompt again (entry 2, `[0x4436]:[0x4438]` refreshed, word `0x1E4` **not** rewritten); then `jmp +0x33B9` |
| `+0x344C..+0x3470` | `[0x49FF] = 0`: near `+0x3171` with `[0x49E3]`; `[0x442F] <> 0` runs near `+0x31B4` and goes to `+0x34F5`; `[0x49FF] <> 0` goes to `+0x34F5` |
| `+0x3473..+0x348D` | words `0x1E0`, `0x1E2` := sign-extended `[0x6AAD]`, `[0x6AAE]`; `lcall 0x72:0x25` (unit `0x68` entry 1, no arguments, no result) |
| `+0x3492..+0x34CD` | `lcall 0x3CA:0x25`; the stored word zero-extended to 32 bits against the byte sign-extended to 32 bits: x, and only if equal y. A coordinate byte of `0x80..0xFF` can never match |
| `+0x34CF..+0x34D3` | not equal: resident `0x8AE:0x00DC` with word `[0x2628]` |
| `+0x34D8..+0x34F2` | `[0x84E0] := 0`; `[0x84E1] := 1`; near `+0x3171` with `[0x49E5]`; `[0x442F] <> 0` runs near `+0x31B4` |
| `+0x34F5..+0x3507` | `[0x49FF] = 0` goes to `+0x3395`; else `[0x49FF] := 0` and `retf` |

**The Up key route.** By the cited entry-2 note, an Up key makes entry 2 return
the key byte `0x48` (not an `E`). With window-1 word `0x594` at most 1 and
`[0x49FF] = 0`, that goes `+0x33B9 -> +0x344C`: the script entry `[0x49E3]` is
run, and, if `[0x442F]` and `[0x49FF]` are still 0, the movement bookkeeping
runs: entry 1 (merged as a bounded model), the view call, the position compare,
`0x8AE:0x00DC`, then the script entry `[0x49E5]`. Whether the live run takes
this route depends on word `0x594`, `[0x49FF]` and `[0x442F]` at that moment
and on what the script runner leaves in them; none of that is asserted here.

## Callees (all hooks) and the first unresolved dependency

| Call | What is known | Size / status |
| --- | --- | --- |
| `0x72:0x2A` | unit `0x68` entry 2, returns the saved key byte (p5's note) | 996 bytes; p5 |
| `0x72:0x25` | unit `0x68` entry 1 (`pr_u68_entry1`, bounded) | 826 bytes; merged model, host wires it |
| near `+0x3171` | one word argument (a script entry), `retf 2` | 67 bytes (66 decoded), 22 instructions. **First unresolved dependency**: it runs the script entry and decides `[0x442F]`/`[0x49FF]`, which steer everything after it. Not read here |
| near `+0x31B4` | no argument | 324 bytes |
| near `+0x2DD3` | no argument | 61 bytes |
| `0x3CA:0x25` | unit `0x3b4` entry 1 (the view refresh of GAMEPLAY_PATH.md) | |
| `0x8AE:0x00DC` | resident, one word | p5 |

**Hooks are fallible.** Every hook returns an `int` (0 = success). `entry2_key`
writes its key through an output pointer only on success; a nonzero return from
any reached hook stops the run with `PR_U1C_CALLBACK` (the failing call and
value are in the report), keeping that callback's own effects and everything
done before it, so a failed input, script, view or resident call cannot look
successful. A reached `NULL` hook returns `PR_U1C_HOOK` the same way; a loop that
never ends returns `PR_U1C_LIMIT` after 100,000 iterations (a host guard; the
original has none).

**Staged window guards.** Window 0 is validated once at entry (the first
instruction, `+0x3387`, writes it). Window 1 is **not** checked at entry: the
original first reads it at `+0x33BD`, after the `+0x3387` write, the first
entry-2 call and the `+0x339D..+0x33B4` copies, so those effects are retained
when window 1 is missing or short (`PR_U1C_ARGUMENT`). Because the hooks may
change the state, **every** window access re-checks its pointer and size at
that access: window 0 at the `+0x33B0` rewrite (after entry 2), at the
`+0x3477`/`+0x3484` writes (after the script), and at the `+0x3497`/`+0x34B3`
reads (after entry 1 and the view call); window 1 at the `+0x33BD` read, the
`+0x33EC` read, the `+0x33FB` write and the `+0x3423` restore (after the view,
script and `+0x31B4`). A refused access returns `PR_U1C_ARGUMENT` with nothing
dereferenced and all earlier effects retained.

## Verification

`tests/recovered/test_overlay_u1c_loop_3387.c`: 5,000 generated runs (random
initial `[0x49FF]`, `[0x442F]`, window-1 word `0x594` including values above 1
and `0x0100`, party bytes including `>= 0x80`, key sequences with `0x48`, `E`,
`e`, `h`, `x`, and hooks that randomly change the party, `[0x442F]`,
`[0x49FF]`) compare the model with a program-counter restatement of the jump
structure on the call order, final DS state, both windows and the `0x8AE` word;
a third of the generated runs inject a callback failure at a random call, and
model and restatement must agree on the call log, the status and every byte of
state at the stop; hand-traced scenes with deterministic hooks: the Up route (exact call sequence
including the second prompt, the pre-move words `0x1E0/0x1E2`, the `0x8AE`
word), an unchanged position (no `0x8AE`), `[0x442F]` set by the first script
(`+0x31B4`, no movement pass), a coordinate byte `0x90` always changed, the
`E`/`e` search branch, word `0x594` of 3 and 2 (saved bit, forced to 1 during
the view and script, restored); the staged guard (window 1 missing or short after
the early effects, window 0 refused at entry); an entry-2 callback that supplies
no key (first prompt and re-prompt); late callback failures (`0x8AE` after the
movement words, the closing script after `[0x84E0]`/`[0x84E1]`); hooks that
invalidate a window (entry 2 drops window 0 before the `+0x33B0` rewrite, the
search-branch view drops window 1 before the `+0x3423` restore, the movement view
drops window 0 before the `+0x3497` read, the first script drops window 1 before
the next `+0x33BD` read); a reached `NULL` hook; the iteration cap; the upcase
edges. Synthetic data only; clean under
AddressSanitizer and UndefinedBehaviorSanitizer; strict
`-Wall -Wextra -Wpedantic -Werror` full build. The restatement shares my reading
of the decode; the correspondence is the table above, not a DOS replay.
