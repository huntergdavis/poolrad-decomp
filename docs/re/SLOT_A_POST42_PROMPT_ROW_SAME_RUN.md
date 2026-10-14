# Saved slot-A UI: call-42 to prompt-row page checkpoint

This bounded same-run trace follows the second-cycle printer call 42 to the
first persistent EGA page change before saved input. The reached unit
`0x3B0:+01DA` drawer, called at loaded `2116:048B`, changes 518 plane-byte
values. The portable [prompt-row page model](SLOT_A_PREINPUT_PROMPT_ROW_PAGE.md)
matches its immediate return over **32,000/32,000 bytes**. This is page-effect
parity for a measured caller frame, not a complete UI frame, keyboard path,
CPU translation, or game-function count.

Before debugging, `deja "Poolrad saved slot A first view after printer call 42
return before input prompt first unmodeled EGA page metadata writer"` returned
no usable result within 30 seconds. The merged
[second-cycle page](SLOT_A_SECOND_CYCLE_PERSISTENT_PAGE.md) and preserved
bounded prompt-row model supplied leads; this new run checks its own loaded
code, frame, RAM, and four-plane pages. The private run is
`resources/oracle-runs/slot-a-ui-post42-first-p9-083`.

| Ignored input | SHA-256 |
| --- | --- |
| DOS v1.3 START.EXE | `d825df2b174675c9088ba1489488bdeebe66ad2a22943f17d3a198e60b6a07bd` |
| GAME.OVR | `872964a32cef0fc32bbf83bab68b15e04ad36d59076f37b00a179dfee7928614` |
| Slot-A save | `abca248371287c43c99e1b34e03394819fca3cf406bed184e7e4fbfe4f0c08f1` |
| Heavy DOSBox-X | `8c5e15028a7e276d277295e4adaf01d2e3f32341758b77920b21b22c753d6f92` |

The first `08DE:07BF` breakpoint alone did not identify call 24. The later
`+07BF` stop was verified by its 32,000-byte page hash below and party
`(15,4,6)`. From that anchor, the sixth successive `08DE:09C3` stop was the
second-cycle call-42 immediate return. `3582:3387` was the next first-view
return. This count and hash anchor prevent a first-cycle `+09C3` return from
being mistaken for call 42.

| Reached stop | 640 KiB RAM SHA-256 | 32,000-byte page SHA-256 |
| --- | --- | --- |
| call 24 `08DE:07BF` | `b4307486e3cdcc47f1a4987dcfbe256126615a44ea6effa471e9a6f5332752b7` | `3eebca28aa66b05e4bfb4a1e199f01e2f8c849dd6a7dd82ed9df61af64a79738` |
| call 42 `08DE:09C3` | `90263e56f3b3856e3764e581cf662d65de8909652525de5443bde04c8c5d6dc2` | same |
| first-view `3582:3387` | `64e776c9de01fc3635fe2d864c8ea10fcc0696c67b158e03e5accda8915d2d44` | same |
| drawer entry `2116:048B` | `68524f64e2e8d9726d0129c2b6e8d3ab3c5a640b7503b598210f2fd748df3062` | same |
| drawer return `2116:048E` | `34721afe87430b2fc3bfc3fb6de597b0f5dbd621fbac858b808d8be190f0aafe` | `09926b914ef24ca2c77eca35c49fd526b54fc27cafbc2d985eefecbcdc61500a` |

The reached near call's original and loaded three-byte hashes are both
`a47ae6e9d1136bdf24bdde8a222cffe3c2c5df0d98fec86529b0650bd2bdf4f3`;
its relative target is `+01DA`. The 419-byte original helper
`+01DA..+037C`, ending `RETF 4`, hashes
`8676c97ae6dcaa88c9f7d68d75c75f81e4936572b517bd84a7e19876c58d96ce`;
the loaded body hashes
`a6a772f3cf170d69c3e73cac96056ab2689f25811ae148c5e4026c78ac0f02b8`.
The fresh drawer frame has length 33, first column 0, selected positions
1..4, selected/ordinary colors 15/10, and selection 1. The copied input,
32-byte set, and 1,416-byte font hashes match the linked model note; no
content bytes are tracked.

The drawer changes **518 page values**, by plane `(63,196,63,196)`, within
scanlines 192..198 and byte columns 0..32. The C probe starts with this
run's measured drawer-entry page; it reports 39 cells, 1,248 modeled plane
stores, and 518 changed values, then matches all 32,000 native return bytes.
The fresh drawer entry/return RAM differs at 32 stack bytes only. Party
`(15,4,6)`, window-0 time `(0,5,10)`, map byte 20, script PC `0x9AA5`,
and selection 1 remain unchanged.

Between call 42 and the first-view return, the EGA page is unchanged while
RAM contains ECL interpreter state changes at `DS:4432`, `49ED`,
`5E24..5E25`, `6F78..6F7F`, `7006`, `7046`, and `7086..7087`, plus stack
and timer changes. Those belong to the gameplay/ECL path. These snapshots
establish the first **persistent page difference** at the drawer; they do
not exclude transient or same-value hardware stores. The C model does not
claim CPU or ECL metadata parity.

The strict focused synthetic test passed **1/1**. Reproduce the private
same-run comparison with the ignored hash-pinned input and strict build:

```sh
cmake -S . -B resources/build-slot-a-post42-prompt \
  -DCMAKE_C_FLAGS='-std=c11 -Wall -Wextra -Wpedantic -Werror'
cmake --build resources/build-slot-a-post42-prompt --target \
  poolrad_slot_a_u3b0_prompt_row_tests poolrad_slot_a_u3b0_prompt_row_probe
ctest --test-dir resources/build-slot-a-post42-prompt \
  -R '^poolrad_slot_a_u3b0_prompt_row_tests$' --output-on-failure
python3 tools/replay/slot_a_post42_prompt_row_audit.py \
  --exe resources/DOS/POOLRAD/START.EXE \
  --game resources/DOS/POOLRAD/GAME.OVR \
  --save resources/DOS/POOLRAD/SAVGAMA.DAT \
  --emulator resources/dosbox-x-heavy-src/src/dosbox-x \
  --run resources/oracle-runs/slot-a-ui-post42-first-p9-083 \
  --probe resources/build-slot-a-post42-prompt/poolrad_slot_a_u3b0_prompt_row_probe
```

The private replay was prepared with `tools/oracle/prepare_run.sh`. At the
visible slot prompt, arm `BP 08DE:07BF`; identify call 24 by the page hash,
then arm `BP 08DE:09C3` and count six returns. Break next at
`3582:3387`, `2116:048B`, and `2116:048E`. At each stop, dump 640 KiB
with `MEMDUMPBIN 0000 0000 A0000`; for each EGA plane select read map
with `OUTP 3CE 04` and `OUTP 3CF 00` through `03`, then dump
`MEMDUMPBIN A000 0000 1F40`. Restore read map 0 and Graphics Controller
selector 5 before resuming. Original media, save, screenshots, RAM,
page captures, and probe input/output remain under ignored `resources/`.
