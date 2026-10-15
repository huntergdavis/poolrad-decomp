# U6A +0824: bounded native reachability boundary

No `+0824` leaf entry/return was reached. A 420-second attempt watched
the entry-1 code address without a hit; a separate corrected capture reached
the unit's startup stub and an interrupt-loader stop. Neither supplies
saved-game leaf parity or strict credit. This follows
[PR #445](https://github.com/huntergdavis/poolrad-decomp/pull/445).

## Pinned source and measured addresses

Base main: `104ac11fdc897bb94c219801734212dcaacaad1a`.
Required recalls for the native route (20 seconds) and newly identified
startup prompt (12 seconds) both expired 124 without a reusable finding.
Reused repository evidence is the [native delay replay](SLOT_A_NATIVE_DELAY_TIMER_REACH.md),
[cold-start input timing](NATIVE_COLD_START_CODEWORD_KEY_TIMING.md),
[saved-load procedure](SLOT_A_NATIVE_0425_RETURN_OUTPUT.md),
and original layout rules in `tools/overlay/overlay_inventory.py`.

| Input | SHA-256 |
| --- | --- |
| `START.EXE` | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| `GAME.OVR` | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| `SAVGAMA.DAT` | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The initial PSP was `0814`, image load base `0824`, and unit-6A runtime
stub segment `0899` (original memory segment `0075`). Slot addressing is
`+20 + 5*slot`. These **three** stubs, each five bytes, match original
descriptor bytes in both complete native RAM captures:

| Runtime address | Original entry offset | Stub SHA-256 |
| --- | --- | --- |
| `0899:0020` | `+1FD9` (startup initializer) | `b04a3f40f7e19dfea2fc12b3b2bfa947d1795c0cdc4d4f0e90f7e5ac4fba59ab` |
| `0899:0025` | `+1E6F` | `4d66544299d9ca7832408d92c1e2aef1847f1bb7a10ebfb118bdfe4d3f5aef16` |
| `0899:002A` | `+04A1` | `a5edf4ff03be5a621b26845a73a5dcdaef8bceb01276b6734f61440a5abf0e37` |

The earlier `+002F` watch is outside this unit's three-entry table. It is
not evidence of another u6A public entry. The relevant entry-1 watch `+0025`
remains correct. Its static route leads to `+090A`, caller `+0A4E`, then
leaf `+0824`, with caller return `+0A51`. No runtime segment for that
155-byte leaf was identified in these captures.

## First bounded pass: exact code-BP nonhit

Private run: `resources/oracle-runs/u6a-0824-direct-boundary-002`.
The controlling-PTY driver armed `BP 0899:0025` before explicit `RUN`;
debugger output confirms the address. It also armed `+002A`, the unvalidated
extra `+002F`, and a candidate saved-prompt control at `3582:3387`.

The driver queued delayed startup input, then menu-key candidates. Private
screens at elapsed `236` and `308` seconds show the startup code-wheel input
prompt; the menu keys went into that field. A queued startup sequence alone
therefore did not establish saved-game loading or the intended menu route.

The bounded code-address nonhit window is from `RUN` at **9.356 seconds**
to the first manual pause at **370.710 seconds**. The transcript has only
two distinct observed stop addresses: initial `1796:0012` and
`2116:0539`. It has no observed stop at the physical address of `0899:0025`
or at the saved-prompt control. This is a startup-route nonhit, not a
saved-game/menu nonhit or a leaf breakpoint measurement. Runtime stub
identity was checked in the separate capture below, not inferred as a
same-run RAM proof for this pass.

The outer bound was 440 seconds; the driver's planned bound was 420 and its
recorded elapsed time, including cleanup, was **420.155 seconds**. These
are host bounds, not guest timing or continuous CPU-execution measurements.
The original private EV parser mishandled ANSI terminal formatting; the
tracked audit reparses the locked raw transcript and does not treat curses
redraws as extra breakpoint hits. No CPU/RAM pair was obtained in this pass.

## Corrected pass: startup and loader boundary only

Private run: `resources/oracle-runs/u6a-0824-direct-boundary-003`.
This fresh, hash-verified input copy had a 120-second planned bound and
140-second outer timeout. It ended after **106.036 seconds**, following a
four-second host screenshot timeout. That exception is not a target hit or
a completed saved-game replay.

| Observed stop | Measured frame address | Meaning |
| --- | --- | --- |
| `0899:0020` | `SS:SP=1D16:3FFC`, far return `0824:00AB` | u6A slot-0 startup stub; not `+0824` |
| `12C6:02E6` | `SS:SP=1D16:3FF6`, interrupt return `0899:0022` | stop after the stub's interrupt; not a leaf entry |

Full fourteen-word CPU records agree with the hashed native EV transcript.
Their canonical little-endian CPU hashes, in AX/BX/CX/DX/SI/DI/BP/SP/CS/IP/
DS/ES/SS/FLAGS order, are respectively
`897ab17f7243d52cd8458a275e9c90127ceeb52729f80e75dad3ca9c550d60e3` and
`5aa4c97faef0e4977ec62d1ec581e37940b4aab48f6493f5a40f3838ec5feeee`.

| RAM artifact | Measured physical range | SHA-256 |
| --- | --- | --- |
| startup-stub stop | `[00000,A0000)` | `12954e898df4ab54730e7397e9ee160df8013131814d7fa4b5f66da4367042a5` |
| completed interrupt-stop dump | `[00000,A0000)` | `9f1b205450f4becac725d7d29df0f7d97bb5b76a5d41c0e796d05452a681f4f4` |
| early copy of that dump | `[00000,71000)` | `9d974e81ae3c7032ea8ae3ed905dffda21fa48912db66bedd80b64ed6f58bd66` |

The early copy raced dump completion and is explicitly partial. Its entire
462848-byte range matches the completed dump's prefix. The final
`MEMDUMP.BIN` was preserved separately after process exit; the last dump
command was issued at the interrupt stop and no subsequent dump command
overwrote it. Only the measured conventional-RAM range is asserted; there is
no complete 1 MiB leaf-frame capture or C/native full-RAM comparison.

The memory-watch diagnostic reported `00 -> CD`, but the watched stub
already contains its original opcode in the earlier dump. It does not prove
a code write, overlay load or patch. The complete `+0824` body is absent
from both measured RAM images. Its original SHA-256 remains
`c77f6aa238e6d9f11bf01decb9084b02ac0a6cd4a5742611adae6848b0d927d7`.
The entry-1 code BP was not observed; a dynamically identified leaf BP was
never armed. Guest instructions, save inputs and registers were not edited.

## Next concrete trigger and disjoint queue

1. In a fresh copy, stop at slot 0 and use its **measured far return
   `0824:00AB`** as the next code breakpoint. Remove the diagnostic memory
   watch, resume the loader, and capture at that return. Search only the
   captured RAM range for the complete pinned leaf and aligned `+0A4D`
   caller, then derive and verify the loaded segment before arming
   `CS:0824` / `CS:0A51`. That segment is valid only for the captured load;
   re-check the body at the entry-1 stub after the saved-game trigger before
   trusting a later leaf watch. A startup stub address alone is insufficient.
2. Resume to the visible code-wheel input prompt and send **Return only
   after it appears**, as in the linked timing procedure. Wait for each
   following menu before `l` and slot `a`. Stop at and hash-check the actual
   saved prompt before pursuing the entry-1 menu path. Timed input sent
   during credits cannot substitute for those observed transitions.
3. Queue [U77:+27DF](U77_ACTIVE_SELECT_27DF.md) native active-selection caller
   reach (975 original/C and 12 synthetic native cases, no natural frame),
   then [U33:+08A4](U33_BYTE_FLAGS_08A4.md) native classifier caller reach
   (2644 original/C and 512 synthetic native cases, no natural frame).
   These are two distinct advisory leads for PM ownership confirmation,
   not new coding claims or credit. Neither touches pG's U6A:+1601 or
   pD's resident arithmetic.

## Reproduction

```sh
python3 tools/overlay/test_u6a_0824_reach_boundary.py
python3 tools/overlay/u6a_0824_reach_boundary_audit.py \
  resources/oracle-runs/u6a-0824-direct-boundary-002 \
  resources/oracle-runs/u6a-0824-direct-boundary-003
```

The audit locks original inputs, emulator, event timelines, raw transcripts,
capture metadata, images and RAM hashes. It validates stub/source identity,
both observed frames, CPU/transcript agreement, physical-address aliases,
the partial-dump boundary and absence of the pinned leaf in measured RAM.
Its success validates this bounded evidence; it does not promote a routine.
Capture origin and the private prompt images remain independent-review
inputs. No original bytes, decoded text, images, saves or RAM are tracked.
