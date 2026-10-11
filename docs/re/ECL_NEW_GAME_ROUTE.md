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

## 4. Unresolved boundaries

The class-1 store hook (`+0x0A74` in unit 0x25, mirrors window words into the active
character record) runs on the run entry's two SAVEs and on the camp-interrupted entry's two
SAVEs and is reported as HOOK, not executed, which stops a plain `pr_rec_ecl_run` there
(section 3.1); the continuation in 3.2 is extrapolated;
the load-word class-1 hook likewise; class-4 pseudo-addresses stay unresolved. The
reference names of the opcodes are not asserted by this build.

## 5. Reproduce (private)

Open `ECL3.DAX` with `pr_dax_open`, unpack block id 0 with `pr_dax_read`, serve it through
`pr_rec_ecl_load_block`'s callback, run `pr_rec_ecl_script_init`, bind `pr_rec_ecl_op_goto`,
`pr_rec_ecl_op_gosub`, `pr_rec_ecl_op_print_return` and `pr_rec_ecl_op_save` in a
`pr_rec_ecl_dispatch_table`, and call `pr_rec_ecl_run` on each `entry_addr[k]` with a step
bound; the scratch program prints counts only.
