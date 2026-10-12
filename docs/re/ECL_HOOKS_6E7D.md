# ECL class-1 hooks for address 0x6E7D (store and load)

`pr_rec_ecl_store_word` and `pr_rec_ecl_load_word` report HOOK after every
class-1 (window 1) access except the addresses proven to leave the engine's hook
quiet: `0x6E82` (store, [ECL_STORE_HOOK_6E82.md](ECL_STORE_HOOK_6E82.md)) and
`0x6DD5` (load, [ECL_LOAD_HOOK_6DD5.md](ECL_LOAD_HOOK_6DD5.md)). This note adds
**`0x6E7D`** (window-1 word index `0x37D`, byte offset `0x6FA`) to **both**
lists, using the same method. `0x6E7D` is the destination of the saved run
entry's first SAVE (PC `0x9914`) and the source of its SAVE at PC `0x9927`. Lane-A
files were edited with the PM's approval; neighbouring addresses still report
HOOK. **No count or credit is claimed.** Executing the saved run entry after this
change remains a conditional model result, not a DOS trace.

`deja "Poolrad ECL class 1 load store hook 0x6E7D"` found no earlier trace beyond
the saved-route notes. Reused: the 6E82 and 6DD5 notes (spans, caller order,
method), SLOT_A_UP_ECL_GATE.md (the run-entry path).

## Provenance (ignored local inputs; hashes only)

| File | Size | SHA-256 |
| --- | ---: | --- |
| `GAME.OVR` | 206,105 | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` (slot A) | 13,137 | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| saved script, file `0x1401..0x3200` | 7,680 | `bba1d911557ded2417fde047b32c135c4b955c59c4cae45b8e868f9926459ae8` |

| Routine | Extent | SHA-256 |
| --- | --- | --- |
| store hook, unit `0x25` `+0x0A74` | `+0x0A74..+0x0C76`, 515 bytes, 163 instructions, `retf 4` | `258b587e214213205991b5eb810131948000e266d244bf086a7b427bb8bf0938` |
| load hook, unit `0x25` `+0x07BC` | `+0x07BC..+0x0A73`, 696 bytes, 233 instructions, `retf 6` | `6ec8c66f021fed88e9a816e2cf45a8b927889f036600c9b0d6c0d61ca96a5451` |

## The two paths for 0x6E7D (rel = 0x37D after the subtraction of 0x6B00)

* **Store hook** (after the caller's window-1 write; arguments address and value,
  `+0x0CCE..+0x0CD5`): every exact and range compare falls through (the largest
  case is `rel = 0x326`; `0x37D` is above it), so the final `cmp ax, 0x326; jne`
  lands on the epilogue `+0x0C71`. **42 instructions before the epilogue, no
  DS/ES game-state write and no call**; its only stores are to its own stack
  argument `[bp+8]`.
* **Load hook** (before the caller's window read; `+0x0E41..+0x0E69`): `*flag := 1` at
  `+0x07C5`; the compare chain falls through every case (the largest case is
  `rel = 0x33E` at `+0x0A51`; `0x37D` differs), reaching `+0x0A64`: `*flag := 0` at
  `+0x0A67`, then `+0x0A6B` returns. **59 instructions to `+0x0A6B`, no read of
  the character record or DS, no ES/DS write, no call.** The caller sees flag 0 and
  reads the window-1 word, so the value is whatever the host holds.

Both traces come from the same private flow follower as the 6E82/6DD5 notes
(static, tool-assisted; both sides of data-dependent branches explored; all 65,536
sixteen-bit addresses). That 0x6E7D is quiet agrees with the whole-window results
already recorded there: it is not among the 96 store-hook addresses that can write or
call, nor among the 29 load-hook addresses that read, call or keep the flag at 1.

## Change

* `PR_REC_ECL_STORE_HOOK_NOOP_ADDRESS_B` and `PR_REC_ECL_LOAD_HOOK_PLAIN_ADDRESS_B`
  (`0x6E7D`) in `ecl_helpers.h`; `pr_rec_ecl_store_word` returns OK after the
  window write at `0x6E82` or `0x6E7D`, and `pr_rec_ecl_load_word` returns OK at
  `0x6DD5` or `0x6E7D`; every other class-1 address is HOOK (including `0x6E7C`,
  `0x6E7E`, `0x6E7F`, `0x6E80`, `0x6E81`, `0x6E83`, `0x6DD4`, `0x6DD6`, `0x6DE1`).
  `0x6DE1` and the EXIT handler are later gates and are not part of this change.
* Tests in `test_ecl_helpers.c`: the store and load sweeps over all 1,024 class-1
  addresses (exactly the two exempt addresses OK each), neighbour checks, the
  standalone hook model at `0x6E7D` (case NONE, record and flags untouched), and
  `test_save_6e7d` (SAVE `0x0B` to `0x6E7D` OK with the word written; SAVE
  `0x6E7D` to `0x49FD` OK and sets `[0x84E8]`; the same SAVEs through `0x6E7E`
  still report HOOK).

## Private smoke on the restored slot-A state (documented, not committed)

A private C program (scratchpad only) maps the hash-checked save as the audit
tool does (window 0 at file `0x0001`, window 1 `0x0801`, window 2 `0x1001`, the
script `0x1401`, party bytes `0x3201..0x3205`) and calls `pr_rec_ecl_run` at PC
`0x9914` with the merged SAVE, COMPARE, IF (`0x16..0x1B`), GOTO and AND/OR handlers
bound and every other opcode unmodelled. No HOOK or UNRESOLVED status is mapped
to OK.

| State | Result |
| --- | --- |
| before this change | stop after **1** dispatch: run status HANDLER, handler status HOOK at PC `0x991A` (SAVE `0x0B` to `0x6E7D`; the word was written, the store reported HOOK) |
| after this change | **9** dispatches complete in order: PCs `0x9914`, `0x991A`, `0x9920`, `0x9927`, `0x992E`, `0x9934`, `0x993A`, `0x993B`, `0x996B`; the 10th dispatch stops with run status UNMODELLED at PC `0x9974`: opcode `0x00` (EXIT, handler `+0x0001`), latch `0x00`, previous opcode `0x2F` |

Measured state at the stop (numbers from the model, not a DOS trace): `[0x84E8] = 1`
(set by the SAVEs to `0x49FD` and `0x49FE`), `[0x49FF] = 0`, `[0x442E] = 0`, the
six compare flags from COMPARE `0x6DD5` against 0 are equal, less-or-equal and
greater-or-equal (the saved gate word is 0), and the IF at `0x993A` leads through
the GOTO at `0x993B` to `0x996B`, matching the static path of
SLOT_A_UP_ECL_GATE.md (the SAVE at `0x9921` and the CALL at `0x9945` are not
reached). The AND at `0x996B` completed with OK and stored a byte to `0x6E82`;
its value depends on the saved `[0x6AB1]` as the host supplied it and is not
asserted here. The first unmodelled piece is the EXIT handler.

## Verification

Strict `-Wall -Wextra -Wpedantic -Werror` full build; the ECL suite clean under
AddressSanitizer and UndefinedBehaviorSanitizer. The restatement shares my reading
of the decode; the correspondence is the quoted offsets and hashes, not a DOS
replay.
