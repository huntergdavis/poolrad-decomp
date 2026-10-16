# U6F +22D5 as a whole function: the area-transition prompt, not the ECL loader (Claude C1, PM GO i8541)

**Scope.** The complete executable span of GAME.OVR unit 0x6F + 0x22D5 translated into
portable C at the level of its Pascal semantics, with every external call kept as an
explicit boundary, and the correction of three documents that called this entry the ECL
block loader. Inputs: `GAME.OVR` 206,105 bytes, sha256
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`; `START.EXE` 65,231
bytes, sha256 `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`. Unit 0x6F
starts at GAME.OVR offset 0x1DFE6 (9,189 code bytes); the function is public entry 11
(run-time stub `0x7C:0x57`). Ownership: pN released U6F +01B8/+02AE/+1446 in i8328; the PM
assigned +22D5 as this lane's disjoint follow-on in i8483 and i8541. No C of this span
exists in the tree (searched `docs`, `include`, `src`, `tests`, `tools` for `22D5`,
`0x7C:0x57` and `entry 11` of unit 0x6f). The `deja` recall hung (index damaged per
i8257); the merged notes cited below are the prior evidence. C:
`src/recovered/overlay_u6f_22d5_transition_prompt.c`,
`include/poolrad/recovered/overlay_u6f_22d5_transition_prompt.h`,
`tests/recovered/test_overlay_u6f_22d5_transition_prompt.c`, built by
`tests/recovered/build_u6f_22d5.sh` (no shared CMake edit without PM coordination).

## 1. Byte identity, independently derived

| item | value |
| --- | --- |
| executable span | `[22D5,23E0)`, GAME.OVR `[202BB,203C6)`, 267 bytes, 107 instruction starts, sha256 `73493121c65dd753668b5009b3ba4bd6033e945cb6ad27bdfb0d6ff47cb1ca3b` |
| outer return | `retf 4` at +23DD (one far argument, a Pascal string, at `[bp+6..8]`) |
| preceding bytes | `[22B7,22D5)`, 30 bytes: the private 28-character constant with its length byte (CS:0x22B7) and the empty string (CS:0x22D4, length byte 0); data (sha256 `0361c7d1b3f4ae482d87afa04d8479f06ccf26e98cb838ea5f4b9077a45c353a`) |
| following bytes | `[23E0,23E5)`, 5 bytes: public entry 0, the unit's empty initialization stub (`push bp; mov bp,sp; pop bp; retf`), the end of the unit |
| frame | `sub sp, 0x156`: name `string[40]` at `SS:BP-0x29`, menu `string[40]` at `SS:BP-0x52`, digit `string[1]` at `SS:BP-0x54`, key result byte `SS:BP-0x55`, extended-key byte `SS:BP-0x56`, title `string[0]` at `SS:BP-0x57`, message `string[255]` at `SS:BP-0x156` |
| calls | 11: six SYSTEM far (`0xAF8:0x0B25` x2, `0x1366`, `0x0B0B` x2, `0x0B98`), three resident far (`0x802:0x02EB`, `0x802:0x095D`, `0x899:0x0000`), one overlay far (`0x3C5:0x2A`, unit 0x3B0 entry 2), one near (+23D2 to +1D58, entry 9) |
| branches | 8: +22FB `je +2300` / +22FD `jmp +23DA`; +2376 `jne +23DA`; +237C `jne +23DA`; +2392 `jge +2399`; +239E `jle +23A5`; +23B9 `jge +23C0`; +23C5 `jle +23CC`; no loops |
| caller | unit 0x1C + 0x0BD1 only (the NEWECL handler, opcode 0x20), far site +0C47, fixup-confirmed; it passes the file name it composed at +0C1F..+0C42 (`DS:0x547C` + a constant + its two-character operand string + a constant) and, after the return, calls unit 0x25 entry 4 (`0x2B:0x34`, the block loader of [ECL_BLOCK_LOAD.md](ECL_BLOCK_LOAD.md)) with the byte at `[bp-1]` and then entry 3 (`0x2B:0x2F`) |

The PM's figures (267 bytes, 107 starts) were reproduced from the bytes before use.

Source and ownership map of what the span touches:

| item | where it is established | owner |
| --- | --- | --- |
| resident `0x802:0x02EB`, the file probe returning a Boolean in AL (copies at most 0x28 characters of the name; AL = 1 only on a zero IOResult and a non-empty name) | [RESIDENT_DAX_SELECTOR_02EB.md](RESIDENT_DAX_SELECTOR_02EB.md), which lists this call site (+22F4, direct `SS:DI` push) | bounded model `pr_rec_res_0802_02eb` (Codex); boundary here |
| unit 0x3B0 entry 2 (`+0x03BD`, `retf 0x16`), the menu key input: (title, key-list, row, two bytes, byte, byte, var extended-key byte) returning the selected letter in AL | [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) section 2.4 | decoded read-only there; boundary here |
| `DS:0x5376`, the current archive (disk) number; `0x802:0x095D` = `DS:[0x5376] := DS:[0x5377]` (7 bytes, decoded read-only) | [RESIDENT_DAX_SELECTOR_PUSH.md](RESIDENT_DAX_SELECTOR_PUSH.md), [SLOT_A_U40C_FIRST_TILE_PLANE.md](SLOT_A_U40C_FIRST_TILE_PLANE.md) | resident lane (Codex); boundary here |
| `DS:0x6AAD`, `0x6AAE`, `0x6AAF` (party x, y, facing) and the delta tables `DS:0x2736 + facing`, `DS:0x273F + facing` | [U68_MOVEMENT_FLAG_04DF.md](U68_MOVEMENT_FLAG_04DF.md), [U68_COORDINATE_WRITER_073B.md](U68_COORDINATE_WRITER_073B.md) | caller-supplied state here |
| unit 0x6F + 0x1D58 (entry 9), the save routine | [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md) (the main menu's key 0x53 path) | not translated; boundary here |
| resident `0x899:0x0000`, the exit routine | [GAMEPLAY_PATH.md](GAMEPLAY_PATH.md), [SLOT_A_RESIDENT_DISPATCH_GATE.md](SLOT_A_RESIDENT_DISPATCH_GATE.md) | resident lane (Codex); boundary here |
| `0xAF8:0x0B25`, `0x0B0B`, `0x0B98` | the PM decode in the ledger, section 1.19 | translated inline |
| `0xAF8:0x1366` (Str): image 0x0C2E6, `retf 0xC`; the digits are produced by the helper at image 0x0C1D4, then the length is `max(min(width, capacity), min(digits, capacity))`, spaces pad on the left, and the first `min(digits, capacity)` digits are copied; here width 0 and capacity 1 | decoded read-only for this note | translated inline as the first decimal digit |

## 2. Semantics

```
procedure TransitionPrompt(name: string);               { entry 11, retf 4 }
  local := Copy(name, 1, 40);                           { +22DC: 0x0B25 }
  if Probe(local) then exit;                            { +22EF: 0x802:0x02EB; AL <> 0 -> +23DA }
  Str(DS:[0x5376], digit);                              { +2300: 0x1366 into string[1]: the first digit }
  menu := Copy(CS:0x22B7 + digit, 1, 40);               { +231A..+233D: 28 + 1 characters }
  title := CS:0x22D4;                                   { +2342: the empty string }
  key := KeyInput(title, menu, $0D, $0A, $0F, 0, 0, extended);   { +2351..+236F: 0x3C5:0x2A }
  if (extended <> 0) or (key <> 'S') then exit;         { +2372..+237C }
  x := x - dx[facing]; if x < 0 then x := 15; if x > 15 then x := 0;   { +237E..+23A0, signed byte tests }
  y := y - dy[facing]; if y < 0 then y := 15; if y > 15 then y := 0;   { +23A5..+23C7 }
  RestoreArchiveNumber;                                 { +23CC: 0x802:0x095D }
  Save;                                                 { +23D2: near +1D58 }
  ExitGame;                                             { +23D5: 0x899:0x0000 }
```

What it is: when the NEWECL handler is about to load the next area's script file, this
entry probes that file first. If the file opens, nothing is shown and the handler goes on
to the block loader. If it does not (the next area is on another disk of the floppy set),
the menu line shows the 28-character constant followed by the first digit of the current
archive number, and the player may press the key `S` to step the party one square back
against its facing (the coordinates wrap in 0..15), restore the previous archive number,
save and leave the game; any other key, or an extended key, returns to the handler. It
loads nothing. The block loader is unit 0x25 entry 4 (`+0x0341`, ledger section 1.15).

Facts the C preserves: the probe sees at most 40 characters of the name; the digit is the
first decimal digit of the byte (so archive numbers above 9 show their first digit only);
the title passed to the key input is the empty string and the menu is the 29-character
text (the 40-character cap never truncates it); the extended-key test precedes the letter
test; the letter is compared with 0x53 exactly (the key input upcases before returning,
the caller does not); the coordinate step subtracts the table byte with 8-bit wrap and
then applies the two signed tests in order (a negative byte becomes 15, a byte above 15
becomes 0); nothing is written on any early exit; there is no loop and no IOResult read.

## 3. Boundaries, kept explicit

| source call | identity | C boundary |
| --- | --- | --- |
| `0x802:0x02EB` | temporary-file probe, Boolean in AL, `retf 4` | `select_file(name, &al)` |
| `0x3C5:0x2A` | unit 0x3B0 entry 2, the menu key input, `retf 0x16` | `key_input(title, menu, row, a, b, c, d, &extended, &al)` with the exact byte arguments 0x0D, 0x0A, 0x0F, 0, 0 |
| `0x802:0x095D` | `DS:[0x5376] := DS:[0x5377]` | `restore_archive_number()` |
| near +1D58 | entry 9, the save routine (1,375 bytes, not translated) | `save()` |
| `0x899:0x0000` | the exit routine | `exit_game()`; if it returns, the model returns OK at +23E0 as the engine would |
| `0x0B25`, `0x0B0B`, `0x0B98`, `0x1366` | bounded store, load, concatenate, Str | translated inline |

Host spans and contracts: the name span must hold the whole string (length + 1 bytes);
the two delta tables are supplied with their spans and must hold the byte at the facing,
checked at +237E after the prompt and before any write (`ARGUMENT`, the prompt's effects
kept); the message literal must have its original length (28). A missing callback at a
reached boundary reports `NEED_HOOK` with the call instruction's offset in `next_offset`;
a callback failure reports `HOST_FAILURE` with every earlier effect kept (the coordinate
step is applied before the restore, save and exit calls, as in the engine). The private
CS literal is caller-supplied; its bytes are not in the public tree. No host guard is
needed: the engine has no loop.

## 4. Documentation corrections made with this note

| document | before | after |
| --- | --- | --- |
| [ECL_CONTRACTS.md](ECL_CONTRACTS.md), NEWECL bullet | "unit 0x6f entry 11 (… unit 0x6f is a resource-loading unit)" | the area-transition prompt; the block load is unit 0x25 entry 4, called after it |
| [ECL_CONTRACTS.md](ECL_CONTRACTS.md), open item 3 | "The exact loader (unit 0x6f entry 11 …) contract: file name, block selection …" | entry 11 is not a loader; the file name is composed by the handler and the loader is unit 0x25 entry 4 |
| [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md), section 4 | "The NEWECL handler's loader call (0x7C:0x57) resolves to unit 0x6f entry 11" | the call resolves to entry 11, which is the prompt (section 1.24), not the loader (section 1.15) |
| [DECOMP_FEASIBILITY.md](DECOMP_FEASIBILITY.md), opcode 0x20 bullet | "unit 0x6f entry 11 (a resource-loading unit)" | the area-transition prompt |

## 5. Classification

Whole-function **path-complete model**: all 107 instruction starts are covered at the
semantics level and all eight branches are represented; the 11 calls are boundaries or
inline string operations. It is not a strict full translation (the probe, the key input,
the restore, the save and the exit are callbacks) and no native run was made (host
heavy-native hold). No strict credit is claimed.

## 6. Tests

Synthetic callbacks record the call order. File present: order `P`, no prompt, no change;
file absent with key 0x53 and facing 1: order `P K R S X`, the key input's title empty and
menu equal to the 28-character literal plus the digit, the exact byte arguments, x and y
stepped; the digit for 7, 123, 0, 10 and 255; another letter, an extended key with AL
still 0x53, and a lower-case letter all returning after the prompt without a step; the
wrap cases (0 minus 1 to 15, 15 plus 1 to 0, 0x7F and 0x80 through the signed tests, zero
deltas keeping 15); each of the five boundaries missing, in reach order, with the stopped
offset and the earlier effects; key-input, save, exit and probe failures keeping the
earlier effects; a 60-character name probed as 40; argument checks (name span, literal
length, null state, hooks, output; a facing past the tables refused at +237E after the
prompt and accepted on the paths that never read the tables). Builds: strict
(`-std=c11 -Wall -Wextra -Wpedantic -Werror`), the same under
`-fsanitize=address,undefined`, and `gcc -fanalyzer` on the model and the test (clean).
`cppcheck` and `clang-tidy` are not installed on this host.

## 7. Proposed registration (for PM, not applied)

```cmake
# poolrad_recovered sources: + src/recovered/overlay_u6f_22d5_transition_prompt.c
add_executable(poolrad_overlay_u6f_22d5_transition_prompt_tests tests/recovered/test_overlay_u6f_22d5_transition_prompt.c)
target_link_libraries(poolrad_overlay_u6f_22d5_transition_prompt_tests PRIVATE poolrad_recovered)
add_test(NAME poolrad_overlay_u6f_22d5_transition_prompt_tests COMMAND poolrad_overlay_u6f_22d5_transition_prompt_tests)
```

## 8. Queue

+1446 was screened and withdrawn as a duplicate of the merged `pr_u6f_party_insert_1446`
and `pr_u6f_1446_step` models (i8575). Next, as read-only ownership screens before any
edit: unit 0x6F + 0x0482 (entry 3, 498 bytes, nine far callers) and + 0x1262 (entry 7,
452 bytes). Neither is started.
