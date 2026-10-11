# New-game ECL route: the first unsupported handler (issue #84)

**Scope.** A private trace of the new-game script block (file 3, block id 0 of the ECL set,
`ECL3.DAX` sha256 `fe3f2d46ca7f71caea26f114f229a59d6008eb1d116b09215bf0b06c3bee7135`, 24,955
bytes; `GAME.OVR` sha256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`)
through the merged C models (block load, initializer, run loop and dispatcher, PRs #72 and
#78) with the translated handlers bound (GOTO, GOSUB, PRINT RETURN), to find the first
handler body the model could not execute. Script bytes, text and the trace itself stay
under ignored `resources/` and the scratch directory; only counts, addresses and identities
appear here. `deja` recall returned only this lane's own sessions (no prior route trace).

## 1. Where the route stops before this issue

| entry (ECL_CONTRACTS.md section 1) | address | result with GOTO, GOSUB and PRINT RETURN bound |
| --- | --- | --- |
| 0, run | `0x9914` | first opcode `0x09` at `0x9914`: unsupported |
| 1, search | `0x99EB` | first opcode `0x12` at `0x99EB`: unsupported |
| 2, pre-camp check | `0x9A5E` | one GOTO, then opcode `0x03` at `0x9A63`: unsupported |
| 3, camp interrupted | `0x9A93` | one GOTO, then opcode `0x09` at `0xAF49`: unsupported |
| 4, initial entry | `0x9AF2` | first opcode `0x09` at `0x9AF2`: unsupported |

The initializer's first entry (`0x9914`, the run entry; the gameplay loop runs the fifth,
`0x9AF2`, first) and the initial entry both begin with opcode `0x09`, so the first
unsupported handler is unique: the dispatcher case at unit 0x1c `+0x2F86` calling
`+0x0292` (GAME.OVR `0x00FD8`), SAVE in the reference naming.

## 2. The handler and its helper, recovered as models

FUNCTION_LEDGER.md section 1.17: `pr_rec_ecl_op_save` (unit 0x1c + 0x0292, 78 bytes, 30
instructions, two paths) and `pr_rec_ecl_slot_value` (unit 0x25 + 0x0136, public entry 1,
128 bytes, 57 instructions, four paths), both path-complete models over the loader core,
the load-word and store-word cores and the store-string translation. The one finding on the
helper: for tag `0x01` (and `0x03`, `0x80`) the value is a word read from the assembled
address through the load-word core, while tags `0x02` and `0x81` yield the assembled word
itself; any other tag returns an uninitialized local in the engine.

## 3. Where the route stops after this issue

### 3.1 Exact stops of a single `pr_rec_ecl_run` call with the C SAVE handler bound

| entry | address | steps | real stop | run status | handler status | latch | handler |
| --- | --- | ---: | --- | --- | --- | --- | --- |
| 0 | `0x9914` | 1 | `0x991A` | HANDLER | HOOK | `0x09` | `+0x0292` |
| 1 | `0x99EB` | 1 | `0x99EB` | UNMODELLED | OK | `0x12` | `+0x0A09` |
| 2 | `0x9A5E` | 2 | `0x9A63` | UNMODELLED | OK | `0x03` | `+0x0126` |
| 3 | `0x9A93` | 1 | `0x9A99` | HANDLER | HOOK | `0x09` | `+0x0292` |
| 4 | `0x9AF2` | 5 | `0x9B0A` | UNMODELLED | OK | `0x21` | `+0x0C74` |

A HANDLER stop means the SAVE made its store and reported HOOK (a class-1 destination in
the `0x6B00` window, whose engine-side store hook `+0x0A74` is not modelled); the
dispatcher maps HOOK to `RUN_HANDLER` and the run loop returns at once with the PC already
past the instruction. So the first real C-run stop on the run entry is the **first** SAVE
at `0x9914` (PC `0x991A` on return), and on the camp-interrupted entry the first SAVE after
its GOTO. Entries 1, 2 and 4 stop on an unsupported opcode with no hook involved.

### 3.2 Continuation past hooks (conditional, extrapolated)

The private tracer wraps the handler in an adapter that maps HOOK (and UNRESOLVED) to OK
and lets the run loop continue from the returned PC, i.e. it assumes the un-modelled hook
would not change the control flow that follows. The rows below are therefore
**conditional on that assumption** for the entries with hooks (0 and 3); unresolved hook
effects may change later state, including window words the following instructions read.
For entries 1, 2 and 4 the rows equal 3.1.


| entry | address | steps | SAVEs (tag 0) | dest classes 0..4 | hooks | unresolved | stops at | opcode | handler |
| --- | --- | ---: | ---: | --- | ---: | ---: | --- | --- | --- |
| 0 | `0x9914` | 3 | 2 (2) | 0 / 2 / 0 / 0 / 0 | 2 | 0 | `0x9920` (unsupported) | `0x03` | `+0x0126` |
| 1 | `0x99EB` | 1 | 0 (0) | 0 / 0 / 0 / 0 / 0 | 0 | 0 | `0x99EB` (unsupported) | `0x12` | `+0x0A09` |
| 2 | `0x9A5E` | 2 | 0 (0) | 0 / 0 / 0 / 0 / 0 | 0 | 0 | `0x9A63` (unsupported) | `0x03` | `+0x0126` |
| 3 | `0x9A93` | 4 | 2 (2) | 0 / 2 / 0 / 0 / 0 | 2 | 0 | `0xAF49` (unsupported) | `0x0C` | `+0x03B4` |
| 4 | `0x9AF2` | 5 | 4 (4) | 4 / 0 / 0 / 0 / 0 | 0 | 0 | `0x9B0A` (unsupported) | `0x21` | `+0x0C74` |

Status "unsupported" is the model's UNMODELLED result: the next opcode has a dispatcher case but no C body yet. Entry 0 (two class-1 hooks) and entry 3 (two class-1 hooks) are the extrapolated rows.

Dest classes count the destination addresses by window class (0: `0x4900` window, 1:
`0x6B00` window with the un-modelled store hook, 2: `0x9700` window, 3: script, 4:
pseudo-address). Every SAVE on the route carries a tag-`0x00` byte value. The next
unsupported handlers are `+0x0126` (opcode `0x03`, COMPARE), `+0x0A09` (opcodes `0x11`/`0x12`,
PRINT/PRINTCLEAR), `+0x03B4` (opcode `0x0C`) and `+0x0C74` (opcodes `0x21`/`0x37`); the
initial-entry route reaches `+0x0C74` after four SAVEs.

## 3.3 After issue #92: the initial entry past opcodes 0x21 and 0x37

With `pr_rec_ecl_op_load_files` bound for opcodes 0x21 and 0x37 (both dispatch to unit 0x1c
`+0x0C74`; FUNCTION_LEDGER.md section 1.18), one `pr_rec_ecl_run` call on the initial entry
`0x9AF2` is exact (no hook on this route):

| entry | address | steps | real stop | run status | handler status | latch | handler | callback counts and flags [0x4431..0x4434] |
| --- | --- | ---: | --- | --- | --- | --- | --- | --- |
| 4 | `0x9AF2` | 7 | `0x9B18` | UNMODELLED | OK | `0x03` | `+0x0126` | wallset 1, pieces 1, text 0, resident 1 1, resident 2 1; flags 1 0 1 1 |

The 0x21 instruction at `0x9B0A` carries three tag-`0x00` operands; its first value is
neither `0xFF` nor `0x7F` and window 0 word `0xE6` is 1 after the initializer, so the
wallset branch runs (window 0 word `0xC5` set, unit 0x3bd entry 7 called, window 1 word
`0x2C9` cleared) and the text branch does not. The following 0x37 at `0x9B11` has a first
value of `0x7F`, so unit 0x3bd entry 6 is called once with (0, 1). With `[0x4432]` set by
the gameplay loop, that second call also runs the two resident routines `0xBA:0x27D9` and
`0xBA:0x0767` and clears `[0x4432]`. The next boundary is opcode `0x03` (`+0x0126`,
COMPARE in the reference naming) at `0x9B18`, which also stops the pre-camp entry.

## 3.4 After issue #102: COMPARE and the IF family on both routes

With `pr_rec_ecl_op_compare` (opcode 0x03) and `pr_rec_ecl_op_if` (opcodes 0x16..0x1B)
bound as well (FUNCTION_LEDGER.md section 1.19), one `pr_rec_ecl_run` call per entry gives:

| mode | entry | address | steps | stop | run status | handler status | latch | handler | hooks mapped |
| --- | --- | --- | ---: | --- | --- | --- | --- | --- | ---: |
| exact | 0 | `0x9914` | 1 | `0x991A` | HANDLER | HOOK | `0x09` | `+0x0292` | 0 |
| exact | 1 | `0x99EB` | 1 | `0x99EB` | UNMODELLED | OK | `0x12` | `+0x0A09` | 0 |
| exact | 2 | `0x9A5E` | 6 | `0x9A7F` | HANDLER | HOOK | `0x09` | `+0x0292` | 0 |
| exact | 3 | `0x9A93` | 1 | `0x9A99` | HANDLER | HOOK | `0x09` | `+0x0292` | 0 |
| exact | 4 | `0x9AF2` | 7 | `0x9B1E` | HANDLER | HOOK | `0x03` | `+0x0126` | 0 |
| continued (HOOK to OK) | 0 | `0x9914` | 10 | `0x9965` | UNMODELLED | OK | `0x2F` | `+0x0DEF` | 4 |
| continued (HOOK to OK) | 1 | `0x99EB` | 1 | `0x99EB` | UNMODELLED | OK | `0x12` | `+0x0A09` | 0 |
| continued (HOOK to OK) | 2 | `0x9A5E` | 8 | `0x9A85` | UNMODELLED | OK | `0x13` | `+0x0A9F` | 2 |
| continued (HOOK to OK) | 3 | `0x9A93` | 4 | `0xAF49` | UNMODELLED | OK | `0x0C` | `+0x03B4` | 2 |
| continued (HOOK to OK) | 4 | `0x9AF2` | 12 | `0x9A92` | UNMODELLED | OK | `0x13` | `+0x0A9F` | 3 |

Rows marked "continued" come from the private tracer's adapter (HOOK mapped to OK, counted in the last column) and are conditional on the un-modelled hooks not changing later state; "exact" rows are single unmodified `pr_rec_ecl_run` calls.

The pre-camp entry's COMPARE at `0x9A63` reads a window-0 variable (class 0) against a byte
immediate, so its path is exact. The initial entry's COMPARE at `0x9B18` reads a `0x6B00`
window variable (class 1): the load-word hook `+0x07BC` is not modelled, so the handler
stops at that lookup with the six flags untouched and the real run ends there with
`RUN_HANDLER`/HOOK, PC `0x9B1E`. The continuation rows (adapter mapping HOOK to OK) are conditional on the
hook not overriding that word.

## 4. Unresolved boundaries

The class-1 store hook (`+0x0A74` in unit 0x25, mirrors window words into the active
character record) runs on the run entry's two SAVEs and on the camp-interrupted entry's two
SAVEs and is reported as HOOK, not executed, which stops a plain `pr_rec_ecl_run` there
(section 3.1); the continuation in 3.2 is extrapolated;
the load-word class-1 hook likewise; class-4 pseudo-addresses stay unresolved. The
reference names of the opcodes are not asserted by this build. For opcodes 0x21/0x37 the
wallset and piece loaders (unit 0x3bd entries 7 and 6: file-name construction, the resident
DAX loader `0x802:0x0716`, resident `0x464:0x00FF`), the text routine (unit 0x3ce entry 1)
and the resident routines `RES_00BA_27D9` and `RES_00BA_0767` are callbacks, not translated.

## 5. Reproduce (private)

Open `ECL3.DAX` with `pr_dax_open`, unpack block id 0 with `pr_dax_read`, serve it through
`pr_rec_ecl_load_block`'s callback, run `pr_rec_ecl_script_init`, bind `pr_rec_ecl_op_goto`,
`pr_rec_ecl_op_gosub`, `pr_rec_ecl_op_print_return` and `pr_rec_ecl_op_save` in a
`pr_rec_ecl_dispatch_table`, and call `pr_rec_ecl_run` on each `entry_addr[k]` with a step
bound; the scratch program prints counts only.
