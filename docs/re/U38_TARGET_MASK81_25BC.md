# Unit 0x38 public entry 78: target mask at +0x25BC

## Provenance and extent

The bounded `deja "poolrad U38 +25BC entry78 tail +2605 +101D strict C parity"`
query timed out without a prior-session result. This work reuses the checked
[+0x101D target selector](U38_TARGET_SELECT_101D.md), its
[resident selector](RESIDENT_CHARACTER_TARGET_SELECT.md), and the
[effect-table installer](U38_EFFECTS_INSTALL_31A2.md). The source bytes and
public entry boundaries were independently checked against ignored local
inputs:

| Input | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `START.EXE` | 65,231 | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |

Public entry 78 starts at unit `0x38:+0x25BC`; entry 79 starts at
`+0x2610`. Their 84-byte public span is `GAME.OVR` file
`[0x11217,0x1126B)`, SHA-256
`285366ab2370d76ffbb648eb1f45cd6a2a143557effb9cc3ebb873d4d568ed12`.
There are no wrapper fixups. The reachable prefix is 73 bytes and 29
contiguous instructions, `+0x25BC..+0x2605`, SHA-256
`4921365d3d512d870ac24085a63d408b38ab60e6f98e4136f583a7c5c78fe753`.
It ends with `RETF 10` at `+0x2602`; no branch reaches the remaining 11
bytes. The tail `+0x2605..+0x2610`, SHA-256
`095a90f713555df4530f7b65abc167913bbd989a9ac89489bc059ee02adf516d`,
is an ASCII message fragment referenced as data by entry 79 at `+0x2637`
(`MOV DI,2605`). This implementation models the reachable prefix only.

## Behavior and callback boundary

The prefix passes the active character pointer at `DS:5D92/5D94` to
`+0x101D` through its sole near call at `+0x25CB`. For a null returned
target it leaves `DS:6818` unchanged. Otherwise it reads target byte
`+0x2E`, scales it by 16, reads `DS:[0x5589 + 16*index]`, and tests bits
`0x81`. If either bit is set, it halves the unsigned byte at `DS:6818`;
otherwise it preserves that byte. It returns through `RETF 10`.

`pr_u38_target_mask81_25bc` models the prefix with 1 MiB RAM and the
14-field CPU state. It composes the merged +0x101D model and its actual
near-call frame. The callback at the resident far call `:2E8B` supplies
resident register, flag, and memory effects. Callback refusal leaves that
call frame in place and returns `PR_U38_25BC_CALL_ABORT`.

## Measured parity and limit

The original-byte Unicorn checker loads the full 84-byte public span,
executes the 73-byte prefix plus +0x101D, and compares full 1 MiB RAM,
all 14 CPU fields, and every resident callback frame across 720 cases.
Cases cover all eight helper selection modes, indexes 0, 7, and 255,
table bytes 0, 1, 2, 0x80, and 0x81, two loaded resident segments,
and three DS/SS/stack layouts.

Five DOSBox-X normal-core synthetic far-entry frames execute the pinned
prefix and helper with a deterministic resident stub. They compare all
14 CPU words, 52 stack bytes, and `DS:6818`. AF after `SHR AX,1` is
undefined, so only that bit is masked on halving frames. The native
capture stays under ignored `resources/u38-25bc-parity/native/`, SHA-256
`485344acbc62cc6824deddee645c174a7b47ba67b2ce19ad7b6fc902c798aba3`.
Focused C11 `-Wall -Wextra -Wpedantic -Werror` ASan/UBSan CTest covers
null link, preserve, both mask bits, and callback refusal.

These are bounded prefix and synthetic callback checks. A natural game
caller frame has not been reached, so game identity and strict function
count remain provisional.
