# Saved slot-A DELAY residual stack writer

The first measured writer of the residual stack region in the bounded
`A40:+029C` C/native comparison is a hardware IRQ0 frame, entered while the
game is executing the A40 delay loop. The first debugger watch report is for
`SS:3ED3` at `F000:FEA5`; the saved interrupted game address is
`1264:02C1`. The interrupt then reaches BIOS code and `INT 1C`. All 17
previously unexplained lower-stack bytes differ at the A40 return. This is
native stack-effect evidence, not a new game routine, a C implementation of
interrupt delivery, or complete C/native route parity.

The prior [A40 C frame comparison](SLOT_A_A40_C_FRAME_COMPARE.md) established
the 17-byte residual. Before this replay, `deja "Poolrad slot A DELAY A40
029C lower stack 21022 2102C native writer watchpoint interrupt frame"`
timed out after 20 seconds without a recalled match. The earlier
[native DELAY reach](SLOT_A_NATIVE_DELAY_TIMER_REACH.md) supplied the
candidate call chain; this separate replay checked its own caller frame and
loaded hashes. The resident printer `0709:+0626/+0768` and RGB capture paths
are outside this work.

## Locked replay and capture bounds

The ignored input hashes are START.EXE
`d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd`,
GAME.OVR
`872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614`,
and SAVGAMA.DAT
`abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1`.
The private heavy DOSBox-X executable hashes
`ab0f800660996bda1d60978cbae3f307f0842a24c7a4f0cb0c2d6137e623f1b5`.
Its sole debugger change seeds a new `BPM` watchpoint from the current guest
byte. The stock heavy debugger otherwise starts it at zero, which produces
an immediate false change for a nonzero stack byte. No guest code, save, RAM,
or timer was patched. The original unmodified heavy executable hashes
`8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92`.

The private 640 KiB RAM snapshots have these SHA-256 hashes:

| Boundary | Hash |
| --- | --- |
| `1264:029C` A40 entry | `ace0a0ff3aac14152a331b505baa1c924ecde4a9755bb18e293675e648176a1b` |
| first IRQ0 watch stop, `F000:FEA5` | `36415a7a4f584183a11703b44bb5a8410ea460ba879ff7f69caded5e239cd58a` |
| `0F2D:0DD1` A40 return | `ea096da21f27bceddf4f35fe5d9721b81eace9bf0aa853338e04297a7b35c92c` |

The capture is physical `[0x00000,0xA0000)`. BIOS code at `F000:FEA5`
lies outside it; the BIOS PC and instruction names below come from the live
debugger display, while the saved CPU frame and RAM differences come from
the hash-locked snapshots. Loaded spans in the entry RAM independently match
the prior hashes: `3582:+271F` (14 bytes)
`ecb6e7a981de47539c4809d0338832f0c107bf96a9e83d65e2847bf5a2f14ec2`,
`0F2D:+0DBE` (21 bytes)
`7830cf292711ac320d18f57cde8ddafdc977b6dd12d8438f1f449a34ca5635be`,
and `1264:+029C..+02C9` (45 bytes)
`d493452e23ca3a0e81d53c6c6e31171d56f543409f3881247d729c5e394e29cb`.

## Reached call and first changed byte

After the saved `(15,4,N) 10:50` frame settled, one Search returned to the
command prompt. One Up then reached `3582:271F` with DS:`49ED=9C28`, followed
by `1264:029C`. At A40 entry, `SS:SP=1D16:3ED6`, the far return is
`0F2D:0DD1`, and the word argument is `900`. These values distinguish this
call from the separate A40 use observed in another attempt.

The 17 watched bytes are `SS:[3EC2,3ECB)` (nine) and `SS:[3ECC,3ED4)`
(eight), physical `[0x21022,0x2102B)` and `[0x2102C,0x21034)`. All `BPM`
baselines matched the entry RAM. The first reported change was
`SS:3ED3`, `0x04 → 0x72`, on entry to IRQ0 at `F000:FEA5` (`sti`), before
that BIOS instruction executed. At this stop, the only entry-to-stop RAM
change was physical `[0x2102E,0x21036)` (eight bytes). The frame at
`SS:3ECE` holds saved `IP=02C1`, `CS=1264`, and `FLAGS=7206`; the word at
`SS:3ED4` is the A40 near-call return `02B7`. Thus six watched bytes belong
to the CPU interrupt frame, while the other two changed bytes are the near
call outside the watched region. There is no original game instruction at
`F000:FEA5` to credit as this writer.

The live debugger next displayed the IRQ0 callback, `push ds`, `push ax`,
`push dx`, `int 1C`, the `F000:CC00` callback, and return to `1264:02C1`.
The stack effects descend from the IRQ frame at `3ECE..3ED3` through the
BIOS pushes at `3EC8..3ECD` and the `INT 1C` frame at `3EC2..3EC7`. This
accounts for every watched address. At `0F2D:0DD1`, all 17 watched bytes
still differ from A40 entry; the full entry-to-return RAM differences are
`[0x46C,0x46D)`, `[0x21022,0x2102B)`, and `[0x2102C,0x21036)` (20 bytes).
The BIOS tick DWORD advanced by **16** in this run. The earlier native
replay advanced by 17; elapsed interrupt timing is replay dependent.

The debugger's `BPM` detects a changed value after CPU work. It reports one
address per stop from its breakpoint list, so the sequence of 17 watch
reports is **not** the per-byte store order. The first-stop RAM and saved
frame prove the IRQ0 CPU push for the first changed watched byte; the
displayed BIOS/`INT 1C` instructions and reached stack locations bound the
rest. A value restored before polling would be invisible to this watch.
Whether game code reads any residual byte after DELAY returns and before an
overwrite remains unmeasured. Full-RAM C parity therefore remains open.

## Reproduce the numeric audit

Keep executable, save, RAM, debugger output, and screens in ignored
`resources/oracle-runs/slot-a-delay-stack-writer-p9-102/`. With a heavy
DOSBox-X debugger, change `CBreakpoint::AddMemBreakpoint` in the private
`src/debug/debug.cpp` build to seed the new breakpoint before it is added:

```c
uint8_t initial = 0;
if (!mem_readb_checked(bp->GetLocation(), &initial))
    bp->SetValue(initial);
```

Rebuild with
`make -C resources/dosbox-x-heavy-src/src/debug libdebug.a` and
`make -C resources/dosbox-x-heavy-src/src dosbox-x`. Keep that build ignored.
Load the
hash-pinned slot A. Arm `BP 3582:271F`, run Search then Up, and at that stop
arm `BP 1264:029C`. Verify the A40 far return and argument before arming
`BPM 1D16:3EC2` through `3ECA` and `BPM 1D16:3ECC` through `3ED3`.
Capture `MEMDUMPBIN 0000 0000 A0000` at entry, first watched stop, and
`BP 0F2D:0DD1` return; preserve each private dump before the next one.
Because the stock breakpoint parser has no range syntax, arm each byte
separately. The three captured RAM images reproduce the address/hash/numeric
checks with:

```sh
python3 tools/replay/slot_a_delay_irq_stack_audit.py \
  resources/oracle-runs/slot-a-delay-stack-writer-p9-102
python3 tests/replay/test_slot_a_delay_irq_stack_audit.py
```

This audit does not reconstruct BIOS code above `0xA0000`, infer unobserved
guest reads, or add a game-function count.
