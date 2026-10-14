# Unit 0x25 `+0x0756`: party-list index

This is the call-free far Pascal routine `GAME.OVR` unit `0x25`
`+0x0756..+0x07BB`. Its complete terminating control flow is translated in
`pr_u25_party_index_0756`. It walks the party-list far links and returns the
zero-based position of the supplied far pointer in `AL`, or the list length if
the pointer is absent. The byte count wraps modulo 256. `AH` is not cleared:
it retains the high byte of the last `OR` of the current pointer words. A
cyclic list keeps looping, as it does in the DOS routine.

## Why this leaf was selected

The assigned unit `0x25` entry 22 at `+0x1EA6..+0x1FB0` is 267 bytes and 133
aligned instructions (original file `[0x9379,0x9484)`, SHA-256
`a18580355e2e8766ca756e3e562e3fdde0ffe89dc65e9312dbe72f7074b0e2df`).
It reads the script-buffer pointer at `DS:[49DE]` and PC at `DS:[49ED]`, writes
opcode latch `DS:[6F7F]`, and sends eight opcode groups to the near operand
loader at unit `0x25:+0x0000`; its fallback increments the PC. The existing
`pr_rec_ecl_operand_count` is only its extracted table core. The loader still
has unresolved string-by-address `0x81` side effects and a runtime string
capacity, as recorded in [FUNCTION_LEDGER.md](FUNCTION_LEDGER.md). Therefore
wrapping that core cannot establish a strict full-C translation of entry 22.
No full-C credit is claimed for it here.

At selection, `+0x0756` was the smallest unclaimed, game-evidenced, call-free leaf found in
the unit `0x25` classification: 102 bytes, 35 instructions, party evidence,
provisional ownership in the then-current classification. The approved
accounting below promotes it in [overlay_classification.csv](overlay_classification.csv).
The load hook at `+0x07BC` calls it at `+0x0981` for class-1 address `0x6DB1`
and at `+0x099A` for `0x6DB4`. The caller map in
[ECL_LOAD_HOOK_6DD5.md](ECL_LOAD_HOOK_6DD5.md) was reused and checked against
the original bytes. The same party head and links appear in the native party
evidence at [SLOT_A_NATIVE_SECOND_SLOT_INSERTION.md](SLOT_A_NATIVE_SECOND_SLOT_INSERTION.md).
The required `deja "unit 0x25 0756 party leaf"` query was attempted before
implementation; it waited without output and was interrupted. No session
result was reused.

## Pinned source and ABI

| Item | Value |
| --- | --- |
| Original overlay | 206,105 bytes, SHA-256 `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Function span | file `[0x7C29,0x7C8F)`, 102 bytes, SHA-256 `b307e1dd310b369eeb068ef1e0fd7b84667e59382144bec02e5295b974c3fcb9` |
| Decode | 35 aligned instructions, no calls, `RETF 4` at `+0x07B9` |
| Argument | far pointer at `SS:[BP+6]` offset, `SS:[BP+8]` segment |
| List head | far pointer at `DS:[0x5D96]` offset, `[0x5D98]` segment |
| Next link | current node far pointer at `ES:[DI+0x104]` offset, `+0x106` segment |
| Return | `AL` is byte count; `AH` is high byte of final pointer-word `OR`; caller consumes four argument bytes |

The frame writes the saved BP, copies the head into `[BP-6..BP-3]`, clears
found at `[BP-7]` and count at `[BP-2]`, then walks until both current pointer
words are zero or found is nonzero. Every comparison and far link read is made
from live RAM in source order. `LES` changes `ES:DI` only after a nonmatch.
On a found return, the last `CMP byte [BP-7],0` supplies flags; on a null return,
the last `OR AX,[BP-4]` supplies flags. The `OR` leaves AF undefined, so the
original-byte comparison excludes that bit only on null returns.

## Verification

`poolrad_overlay_u25_party_index_0756_tests` passes with
`-Wall -Wextra -Wpedantic -Werror`. It checks empty, head, middle, absent and
257-node count-wrap paths, plus returned registers and stack locals.

`tests/recovered/check_overlay_u25_party_index_0756.py` hash-locks the original
span, checks all branch targets remain inside it, then executes sixteen cases
against both the original bytes in Unicorn and the C translation. These include
empty, head, middle, tail, absent, count wrap, four stack-offset boundaries,
an argument-frame link alias, a same-segment offset miss, both one-word-zero far pointers, and two cases using a native captured party
list. All 1 MiB RAM bytes and all returned registers match, comparing flags
with only undefined AF on null returns masked.

The two captured-state emulation cases use the ignored 640 KiB native RAM image with
SHA-256 `0b50157ab03676d796b60f6ccda0b73fa17556a5775f49c91154f81faf0b8705`;
it has head `449A:0008` and a null next link. The audit supplies its own call
frame and loads the pinned original routine bytes into unused RAM. This is
captured-state emulation, separate from the reached native frame below.

## Reached native entry and return

A private DOSBox-X 2026.01.02 heavy-debugger run with the pinned executable,
overlay and slot-A save (SHA-256
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`)
loaded the saved party at `(15,4,N)`. One Up moved it to `(14,4,N)`. On the
second Up, the script dispatcher reached `3582:2F2D` with PC `0x9914` and
buffer `3AD6:0000`. At that stop, the already documented class-1 COMPARE at
PC `0x9B40` was changed from address `0x6DCA` to `0x6DB1` by changing its
single address byte at `3AD6:0242` (`CA → B1`). An earlier exploratory edit at
PC `0x9B68` was restored to its original six bytes before continuing. The
controlled script then entered the original load hook's `+0x0981` call and
stopped at `30CF:0756`. This establishes a real native call frame under a
controlled script input; natural unedited-script reachability is unmeasured.

| Native stop | Entry `30CF:0756` | Return `30CF:0984` |
| --- | --- | --- |
| Full 640 KiB RAM SHA-256 | `db63f4b0a0f45faa2aac4148ace9b7597b55dc9e436569436fc3bc7e833d818f` | `a53e797d7d052a7a07e32025557c5aa28d1f958a1a3ec2dce1660a8259130bac` |
| `AX`, `DX` | `02B1`, `00B1` | `4400`, `449A` |
| `BP`, `SP` | `3CB2`, `3CA6` | `3CB2`, `3CAE` |
| `CS:IP`, `DS`, `ES`, `SS` | `30CF:0756`, `14A0`, `1D16`, `1D16` | `30CF:0984`, `14A0`, `1D16`, `1D16` |
| `FLAGS` | `7246` | `7202` |

The other entry and return registers were identical: `BX=0082`, `CX=0008`,
`SI=3E47`, `DI=3CC0`. Live code at `30CF:0756..07BB` had the pinned original
102-byte SHA-256. The entry stack's far argument and DS party head were both
`449A:0008`, with far return `30CF:0984`. Native execution changed only nine
stack-local RAM bytes. Running the C routine from this exact entry CPU and RAM
produced **zero mismatches across all captured 640 KiB bytes and all fourteen
returned CPU fields**, including exact FLAGS. The audit's `--native-run`
option hash-locks both ignored captures and repeats that comparison.

After independent parity review and PR #380 merge
`bb77539ed4444ad2bd430ce3ed88d8322902a328`, the PM approved exactly one new
verified identity and one strict full-C original for `+0x0756` (inbox i2886).
The [accounting refresh](FUNCTION_ACCOUNTING_REFRESH.md) records that decision
and its regenerated checkpoint. Natural unedited-script reachability remains
unmeasured. The blocked `+0x1EA6` entry still receives no full-C credit.
